#ifndef _LIB_PCIEV_H
#define _LIB_PCIEV_H

#include <linux/pci.h>
#include <linux/msi.h>
#include <asm/apic.h>
#include <linux/types.h>

#define PCIEV_DRV_NAME "PCIEVirt"
#define PCIEV_VERSION 0x0110
#define PCIEV_DEVICE_ID PCIEV_VERSION
#define PCIEV_VENDOR_ID 0x0c51
#define PCIEV_SUBSYSTEM_ID 0x370d
#define PCIEV_SUBSYSTEM_VENDOR_ID PCIEV_VENDOR_ID

#ifdef CONFIG_PCIEV_DEBUG
#define PCIEV_DEBUG(string, args...) printk(KERN_INFO "%s: " string, PCIEV_DRV_NAME, ##args)
#ifdef CONFIG_PCIEV_DEBUG_VERBOSE
#define PCIEV_DEBUG_VERBOSE(string, args...) printk(KERN_INFO "%s: " string, PCIEV_DRV_NAME, ##args)
#else
#define PCIEV_DEBUG_VERBOSE(string, args...)
#endif
#else
#define PCIEV_DEBUG(string, args...)
#define PCIEV_DEBUG_VERBOSE(string, args...)
#endif

#define PCIEV_INFO(string, args...) printk(KERN_INFO "%s: " string, PCIEV_DRV_NAME, ##args)
#define PCIEV_ERROR(string, args...) printk(KERN_ERR "%s: " string, PCIEV_DRV_NAME, ##args)
#define PCIEV_ASSERT(x) BUG_ON((!(x)))

#define NR_MAX_IO_QUEUE 72
#define NR_MAX_PARALLEL_IO 16384

#define PCIEV_INTX_IRQ 15

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

	// struct __pcie_bar *old_bar;
	// struct pcie_ctrl_regs __iomem *bar;

	// u32 *old_dbs;
	// u32 __iomem *dbs;
};

extern struct pciev_dev *pciev_vdev;
struct pciev_dev *VDEV_INIT(void);
void VDEV_FINALIZE(struct pciev_dev *pciev_vdev);
void pciev_proc_bars(void);
bool PCIEV_PCI_INIT(struct pciev_dev *dev);

#endif /* _LIB_PCIEV_H */