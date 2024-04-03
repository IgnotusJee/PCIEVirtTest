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

	if (__validate_configs_arch()) {
		return -EPERM;
	}

	return 0;
}

static bool __load_configs(struct pciev_config *config)
{
	bool first = true;
	unsigned int cpu_nr;
	char *cpu;

	if (__validate_configs() < 0) {
		return false;
	}

	config->memmap_start = memmap_start;
	config->memmap_size = memmap_size;
	// storage space starts from 1M offset
	config->storage_start = memmap_start + MB(1);
	config->storage_size = memmap_size - MB(1);

	return true;
}

static int PCIEV_init(void) {
	int ret = 0;

	pciev_vdev = VDEV_INIT();
	if (!pciev_vdev)
		return -EINVAL;

	if (!__load_configs(&pciev_vdev->config)) {
		goto ret_err;
	}

	if (!PCIEV_PCI_INIT(pciev_vdev)) {
		goto ret_err;
	}

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

	VDEV_FINALIZE(pciev_vdev);

	PCIEV_INFO("Virtual PCIE device closed\n");
}

MODULE_LICENSE("GPL v2");
module_init(PCIEV_init);
module_exit(PCIEV_exit);