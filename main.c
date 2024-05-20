#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#ifdef CONFIG_X86
#include <asm/e820/types.h>
#include <asm/e820/api.h>
#endif

#include "pciev.h"

struct pciev_dev *pciev_vdev = NULL;

static unsigned long memmap_start = 0;
static unsigned long memmap_size = 0;
static unsigned int cpu = 0;

static int set_parse_mem_param(const char *val, const struct kernel_param *kp)
{
	unsigned long *arg = (unsigned long *)kp->arg;
	*arg = memparse(val, NULL);
	return 0;
}

static struct kernel_param_ops ops_parse_mem_param = {
	.set = set_parse_mem_param,
	.get = param_get_ulong,
};

module_param_cb(memmap_start, &ops_parse_mem_param, &memmap_start, 0444);
MODULE_PARM_DESC(memmap_start, "Reserved memory address");
module_param_cb(memmap_size, &ops_parse_mem_param, &memmap_size, 0444);
MODULE_PARM_DESC(memmap_size, "Reserved memory size");
module_param(cpu, uint, 0644);
MODULE_PARM_DESC(cpu, "CPU core to do dispatcher jobs");

static int pciev_dispatcher(void *data) {
	PCIEV_INFO("pciev_dispatcher started on cpu %d (node %d)\n",
			   pciev_vdev->config.cpu_nr_dispatcher,
			   cpu_to_node(pciev_vdev->config.cpu_nr_dispatcher));
	
	while (!kthread_should_stop()) {
		pciev_proc_bars();

		cond_resched();
	}

	return 0;
}

static void PCIEV_DISPATCHER_INIT(struct pciev_dev *pciev_vdev)
{
	pciev_vdev->pciev_dispatcher = kthread_create(pciev_dispatcher, NULL, "pciev_dispatcher");
	kthread_bind(pciev_vdev->pciev_dispatcher, pciev_vdev->config.cpu_nr_dispatcher);
	wake_up_process(pciev_vdev->pciev_dispatcher);
}

static void PCIEV_DISPATCHER_FINAL(struct pciev_dev *pciev_vdev)
{
	if (!IS_ERR_OR_NULL(pciev_vdev->pciev_dispatcher)) {
		kthread_stop(pciev_vdev->pciev_dispatcher);
		pciev_vdev->pciev_dispatcher = NULL;
	}
}

#ifdef CONFIG_X86
static int __validate_configs_arch(void)
{
	unsigned long resv_start_bytes;
	unsigned long resv_end_bytes;

	resv_start_bytes = memmap_start;
	resv_end_bytes = resv_start_bytes + memmap_size - 1;

	if (e820__mapped_any(resv_start_bytes, resv_end_bytes, E820_TYPE_RAM) ||
	    e820__mapped_any(resv_start_bytes, resv_end_bytes, E820_TYPE_RESERVED_KERN)) {
		PCIEV_ERROR("[mem %#010lx-%#010lx] is usable, not reseved region\n",
			    (unsigned long)resv_start_bytes, (unsigned long)resv_end_bytes);
		return -EPERM;
	}

	if (!e820__mapped_any(resv_start_bytes, resv_end_bytes, E820_TYPE_RESERVED)) {
		PCIEV_ERROR("[mem %#010lx-%#010lx] is not reseved region\n",
			    (unsigned long)resv_start_bytes, (unsigned long)resv_end_bytes);
		return -EPERM;
	}
	return 0;
}
#else
static int __validate_configs_arch(void)
{
	/* TODO: Validate architecture-specific configurations */
	return 0;
}
#endif

static int __validate_configs(void)
{
	if (!memmap_start) {
		PCIEV_ERROR("[memmap_start] should be specified\n");
		return -EINVAL;
	}

	if (!memmap_size) {
		PCIEV_ERROR("[memmap_size] should be specified\n");
		return -EINVAL;
	} else if (memmap_size <= MB(1)) {
		PCIEV_ERROR("[memmap_size] should be bigger than 1 MiB\n");
		return -EINVAL;
	}

	if (!cpu) {
		PCIEV_ERROR("[cpu] shoud be spcified\n");
		return -EINVAL;
	}

	if (__validate_configs_arch()) {
		return -EPERM;
	}

	return 0;
}

static bool __load_configs(struct pciev_config *config)
{
	bool first = true;

	if (__validate_configs() < 0) {
		return false;
	}

	config->memmap_start = memmap_start;
	config->memmap_size = memmap_size;
	// storage space starts from 1M offset
	config->storage_start = memmap_start + MB(1);
	config->storage_size = memmap_size - MB(1);

	config->cpu_nr_dispatcher = cpu;

	return true;
}

static void PCIEV_STORAGE_INIT(struct pciev_dev *pciev_vdev) {
	PCIEV_INFO("Storage: %#010lx-%#010lx (%lu MiB)\n",
			   pciev_vdev->config.storage_start,
			   pciev_vdev->config.storage_start + pciev_vdev->config.storage_size,
			   BYTE_TO_MB(pciev_vdev->config.storage_size));

	pciev_vdev->storage_mapped = memremap(pciev_vdev->config.storage_start,
										  pciev_vdev->config.storage_size, MEMREMAP_WB);

	if (pciev_vdev->storage_mapped == NULL)
		PCIEV_ERROR("Failed to map storage memory.\n");
}

static void PCIEV_STORAGE_FINAL(struct pciev_dev *pciev_vdev) {
	if (pciev_vdev->storage_mapped)
		memunmap(pciev_vdev->storage_mapped);
}

static int PCIEV_init(void) {
	int ret = 0;

	pciev_vdev = VDEV_INIT();
	if (!pciev_vdev)
		return -EINVAL;

	if (!__load_configs(&pciev_vdev->config)) {
		goto ret_err;
	}

	PCIEV_STORAGE_INIT(pciev_vdev);

	if (!PCIEV_PCI_INIT(pciev_vdev)) {
		goto ret_err;
	}

	PCIEV_DISPATCHER_INIT(pciev_vdev);

	pci_bus_add_devices(pciev_vdev->virt_bus);

	PCIEV_INFO("Virtual PCIE device created\n");

	return 0;

ret_err:
	VDEV_FINALIZE(pciev_vdev);
	return -EIO;
}

static void PCIEV_exit(void) {
	int i;

	if (pciev_vdev->virt_bus != NULL) {
		pci_stop_root_bus(pciev_vdev->virt_bus);
		pci_remove_root_bus(pciev_vdev->virt_bus);
	}

	PCIEV_DISPATCHER_FINAL(pciev_vdev);

	PCIEV_STORAGE_FINAL(pciev_vdev);
	VDEV_FINALIZE(pciev_vdev);

	PCIEV_INFO("Virtual PCIE device closed\n");
}

MODULE_LICENSE("GPL v2");
module_init(PCIEV_init);
module_exit(PCIEV_exit);