#ifndef _LIB_PCIEV_H
#define _LIB_PCIEV_H

#include <linux/pci.h>
#include <linux/msi.h>
#include <asm/apic.h>
#include <linux/types.h>

#define PCIEV_DRV_NAME "PCIEVirt"

#define PCIEV_INFO(string, args...) printk(KERN_INFO "%s: " string, PCIEV_DRV_NAME, ##args)
#define PCIEV_ERROR(string, args...) printk(KERN_ERR "%s: " string, PCIEV_DRV_NAME, ##args)
#define PCIEV_ASSERT(x) BUG_ON((!(x)))

#define KB(k) ((k) << 10)
#define MB(m) ((m) << 20)
#define GB(g) ((g) << 30)

struct pciev_config {
	unsigned long memmap_start; // byte
	unsigned long memmap_size; // byte

	unsigned long storage_start; //byte
	unsigned long storage_size; // byte
};

struct pciev_dev {
	struct pci_bus *virt_bus;
	void *virtDev;
	struct pci_header *pcihdr;
	struct pci_pm_cap *pmcap;
	struct pci_msix_cap *msixcap;
	struct pcie_cap *pciecap;
	struct pci_ext_cap *extcap;

	struct pci_dev *pdev;

	struct pciev_config config;

	void *storage_mapped;

	void __iomem *msix_table;

	bool intx_disabled;

	struct __nvme_bar *old_bar;
	struct nvme_ctrl_regs __iomem *bar;

	u32 *old_dbs;
	u32 __iomem *dbs;
};

extern struct pciev_dev *pciev_vdev;
struct pciev_dev *VDEV_INIT(void);
void VDEV_FINALIZE(struct pciev_dev *pciev_vdev);

#endif