#include <linux/pci.h>
#include <linux/irq.h>
#include <linux/version.h>

#include <linux/percpu-defs.h>
#include <linux/sched/clock.h>

#include "pciev.h"
#include "pci.h"

static int pciev_pci_read(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 *val)
{
	if (devfn != 0)
		return 1;

	memcpy(val, pciev_vdev->virtDev + where, size);

	PCIEV_DEBUG_VERBOSE("[R] 0x%x, size: %d, val: 0x%x\n", where, size, *val);

	return 0;
};

static int pciev_pci_write(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 _val)
{
	u32 mask = ~(0U);
	u32 val = 0x00;
	int target = where;

	WARN_ON(size > sizeof(_val));

	memcpy(&val, pciev_vdev->virtDev + where, size);

	if (where < OFFS_PCI_PM_CAP) {
		// PCI_HDR
		if (target == PCI_COMMAND) {
			mask = PCI_COMMAND_INTX_DISABLE; // Interrupt Disable
			if ((val ^ _val) & PCI_COMMAND_INTX_DISABLE) {
				pciev_vdev->intx_disabled = !!(_val & PCI_COMMAND_INTX_DISABLE);
				if (!pciev_vdev->intx_disabled) {
					pciev_vdev->pcihdr->sts.is = 0; // only disable no enable?
				}
			}
		} else if (target == PCI_STATUS) {
			mask = 0xF200; // ?
		} else if (target == PCI_BIST) {
			mask = PCI_BIST_START; // Start BIST
		} else if (target == PCI_BASE_ADDRESS_0) {
			mask = 0xFFFFC000; // bar, lower 14 bits read only
		} else if (target == PCI_INTERRUPT_LINE) {
			mask = 0xFF; // Max latency, Min Grant, Interrupt PIN, Interrupt Line
		} else {
			mask = 0x0; // otherwise read only
		}
	} else if (where < OFFS_PCI_MSIX_CAP) {
		// PCI_PM_CAP
	} else if (where < OFFS_PCIE_CAP) {
		// PCI_MSIX_CAP
		target -= OFFS_PCI_MSIX_CAP;
		if (target == PCI_MSIX_FLAGS) {
			mask = PCI_MSIX_FLAGS_MASKALL | /* 0x4000 */
			       PCI_MSIX_FLAGS_ENABLE; /* 0x8000 */

			if ((pciev_vdev->pdev) && ((val ^ _val) & PCI_MSIX_FLAGS_ENABLE)) {
				pciev_vdev->pdev->msix_enabled = !!(_val & PCI_MSIX_FLAGS_ENABLE);
			}
		} else {
			mask = 0x0;
		}
	} else if (where < OFFS_PCI_EXT_CAP) {
		// PCIE_CAP
	} else {
		// PCI_EXT_CAP
	}
	PCIEV_DEBUG_VERBOSE("[W] 0x%x, mask: 0x%x, val: 0x%x -> 0x%x, size: %d, new: 0x%x\n", where,
			    mask, val, _val, size, (val & (~mask)) | (_val & mask));

	val = (val & (~mask)) | (_val & mask);
	memcpy(pciev_vdev->virtDev + where, &val, size);

	return 0;
};

static struct pci_ops pciev_pci_ops = {
	.read = pciev_pci_read,
	.write = pciev_pci_write,
}; // specify how to read and write PCIE configuration space

static struct pci_sysdata pciev_pci_sysdata = {
	.domain = PCIEV_PCI_DOMAIN_NUM, // PCI domain, identify host bridge number(?)
	.node = 0, // NUMA node
};

static struct pci_bus *__create_pci_bus(void)
{
	struct pci_bus *bus = NULL;
	struct pci_dev *dev;

	bus = pci_scan_bus(PCIEV_PCI_BUS_NUM, &pciev_pci_ops, &pciev_pci_sysdata); // Scans the complete bus and update into the pci access structure(?)

	if (!bus) {
		PCIEV_ERROR("Unable to create PCI bus\n");
		return NULL;
	}

	/* XXX Only support a singe NVMeVirt instance in the system for now */
	list_for_each_entry(dev, &bus->devices, bus_list) {
		struct resource *res = &dev->resource[0];
		res->parent = &iomem_resource; // identify resource tree

		pciev_vdev->pdev = dev;
		dev->irq = pciev_vdev->pcihdr->intr.iline; // Interrupt Line

		pciev_vdev->msix_table =
			memremap(pci_resource_start(pciev_vdev->pdev, 0) + PAGE_SIZE * 2,
				 NR_MAX_IO_QUEUE * PCI_MSIX_ENTRY_SIZE, MEMREMAP_WT);
		memset(pciev_vdev->msix_table, 0x00, NR_MAX_IO_QUEUE * PCI_MSIX_ENTRY_SIZE);
	}

	PCIEV_INFO("Virtual PCI bus created (node %d)\n", pciev_pci_sysdata.node);

	return bus;
};

struct pciev_dev *VDEV_INIT(void)
{
	struct pciev_dev *pciev_vdev;
	pciev_vdev = kzalloc(sizeof(*pciev_vdev), GFP_KERNEL);

	pciev_vdev->virtDev = kzalloc(PAGE_SIZE, GFP_KERNEL);

	pciev_vdev->pcihdr = pciev_vdev->virtDev + OFFS_PCI_HDR;
	pciev_vdev->pmcap = pciev_vdev->virtDev + OFFS_PCI_PM_CAP;
	pciev_vdev->msixcap = pciev_vdev->virtDev + OFFS_PCI_MSIX_CAP;
	pciev_vdev->pciecap = pciev_vdev->virtDev + OFFS_PCIE_CAP;
	pciev_vdev->extcap = pciev_vdev->virtDev + OFFS_PCI_EXT_CAP;

	return pciev_vdev;
}

void VDEV_FINALIZE(struct pciev_dev *pciev_vdev)
{
	if (pciev_vdev->msix_table)
		memunmap(pciev_vdev->msix_table);

	if (pciev_vdev->virtDev)
		kfree(pciev_vdev->virtDev);

	if (pciev_vdev)
		kfree(pciev_vdev);
}

static void PCI_HEADER_SETTINGS(struct pci_header *pcihdr, unsigned long base_pa)
{
	pcihdr->id.did = PCIEV_DEVICE_ID;
	pcihdr->id.vid = PCIEV_VENDOR_ID;
	/*
	pcihdr->cmd.id = 1;
	pcihdr->cmd.bme = 1;
	*/
	pcihdr->cmd.mse = 1; // device can respond to Memory Space accesses
	pcihdr->sts.cl = 1;	 // the device implements the pointer for a New Capabilities Linked list at offset 0x34

	pcihdr->htype.mfd = 0; // header type, 0 refer to Endpoints in PCIe
	pcihdr->htype.hl = PCI_HEADER_TYPE_NORMAL;

	pcihdr->rid = 0x01; // revision ID, Specifies a revision identifier for a particular device. Where valid IDs are allocated by the vendor.

	// pcihdr->cc.bcc = PCI_BASE_CLASS_STORAGE; // Mass Storage Controller
	// pcihdr->cc.scc = 0x08;					 // Non-Volatile Memory Controller
	// pcihdr->cc.pi = 0x02; // NVM Express

	// pcihdr->cc.bcc = PCI_BASE_CLASS_STORAGE; // Mass Storage Controller
	// pcihdr->cc.scc = 0x06;					 // Serial ATA Controller
	// pcihdr->cc.pi = 0x01;					 // AHCI 1.0

	pcihdr->cc.bcc = 0x00;
	pcihdr->cc.scc = 0x00;
	pcihdr->cc.pi = 0x00;

	pcihdr->mlbar.tp = PCI_BASE_ADDRESS_MEM_TYPE_64 >> 1; // the base register is 64-bits wide and can be mapped anywhere in the 64-bit Memory Space
	pcihdr->mlbar.ba = (base_pa & 0xFFFFFFFF) >> 14; // minimum address space is 2^14 = 16KB

	pcihdr->mulbar = base_pa >> 32;

	pcihdr->ss.ssid = PCIEV_SUBSYSTEM_ID;
	pcihdr->ss.ssvid = PCIEV_SUBSYSTEM_VENDOR_ID;

	pcihdr->erom = 0x0; // disable expansion ROM

	pcihdr->cap = OFFS_PCI_PM_CAP; // power management capability

	pcihdr->intr.ipin = 0;
	pcihdr->intr.iline = PCIEV_INTX_IRQ;
}

static void PCI_PMCAP_SETTINGS(struct pci_pm_cap *pmcap)
{
	pmcap->pid.cid = PCI_CAP_ID_PM; // indicates that the data structure currently being pointed to is the PCI Power Management data structure
	pmcap->pid.next = OFFS_PCI_MSIX_CAP; // describes the location of the next item in the function’s capability list

	pmcap->pc.vs = 3; // 011b indicates that this function complies with revision 1.2 of the PCI Power Management	Interface Specification.pmcap->pmcs.nsfrst = 1;
	pmcap->pmcs.ps = PCI_PM_CAP_PME_D0 >> 16; // current power state is D0
}

static void PCI_MSIXCAP_SETTINGS(struct pci_msix_cap *msixcap)
{
	msixcap->mxid.cid = PCI_CAP_ID_MSIX;
	msixcap->mxid.next = OFFS_PCIE_CAP;

	msixcap->mxc.ts = 127; // 0x80, encoded as n-1
	msixcap->mxc.mxe = 1; // enable MSI-X

	msixcap->mtab.tbir = 0;
	msixcap->mtab.to = 0x400;

	msixcap->mpba.pbir = 0;
	msixcap->mpba.pbao = 0x1000;
}

static void PCI_PCIECAP_SETTINGS(struct pcie_cap *pciecap)
{
	pciecap->pxid.cid = PCI_CAP_ID_EXP;
	pciecap->pxid.next = 0x0;

	pciecap->pxcap.ver = PCI_EXP_FLAGS;
	pciecap->pxcap.imn = 0;
	pciecap->pxcap.dpt = PCI_EXP_TYPE_ENDPOINT; // endpoint function

	pciecap->pxdcap.mps = 1; // 256 bytes max payload size
	pciecap->pxdcap.pfs = 0; // No Function Number bits are used for Phantom Functions
	pciecap->pxdcap.etfs = 1; // 8-bit Tag field supported
	pciecap->pxdcap.l0sl = 6; // Maximum of 4 μs
	pciecap->pxdcap.l1l = 2;  // Maximum of 4 μs
	pciecap->pxdcap.rer = 1; // enable
	pciecap->pxdcap.csplv = 0;
	pciecap->pxdcap.cspls = 0; // 1.0x
	pciecap->pxdcap.flrc = 1;  // the Function supports the optional Function Level Reset mechanism
}

static void PCI_EXTCAP_SETTINGS(struct pci_ext_cap *ext_cap)
{
	off_t offset = 0;
	void *ext_cap_base = ext_cap;

	/* AER */
	ext_cap->cid = PCI_EXT_CAP_ID_ERR;
	ext_cap->cver = 1;
	ext_cap->next = PCI_CFG_SPACE_SIZE + 0x50;

	ext_cap = ext_cap_base + 0x50;
	ext_cap->cid = PCI_EXT_CAP_ID_VC;
	ext_cap->cver = 1;
	ext_cap->next = PCI_CFG_SPACE_SIZE + 0x80;

	ext_cap = ext_cap_base + 0x80;
	ext_cap->cid = PCI_EXT_CAP_ID_PWR;
	ext_cap->cver = 1;
	ext_cap->next = PCI_CFG_SPACE_SIZE + 0x90;

	ext_cap = ext_cap_base + 0x90;
	ext_cap->cid = PCI_EXT_CAP_ID_ARI;
	ext_cap->cver = 1;
	ext_cap->next = PCI_CFG_SPACE_SIZE + 0x170;

	ext_cap = ext_cap_base + 0x170;
	ext_cap->cid = PCI_EXT_CAP_ID_DSN;
	ext_cap->cver = 1;
	ext_cap->next = PCI_CFG_SPACE_SIZE + 0x1a0;

	ext_cap = ext_cap_base + 0x1a0;
	ext_cap->cid = PCI_EXT_CAP_ID_SECPCI;
	ext_cap->cver = 1;
	ext_cap->next = 0;

	/*
	*(ext_cap + 1) = (struct pci_ext_cap) {
		.id = {
			.cid = 0xdead,
			.cver = 0xc,
			.next = 0xafe,
		},
	};

	PCI_CFG_SPACE_SIZE + ...;

	ext_cap = ext_cap + ...;
	ext_cap->id.cid = PCI_EXT_CAP_ID_DVSEC;
	ext_cap->id.cver = 1;
	ext_cap->id.next = 0;
	*/
}

bool PCIEV_PCI_INIT(struct pciev_dev *pciev_vdev)
{
	PCI_HEADER_SETTINGS(pciev_vdev->pcihdr, pciev_vdev->config.memmap_start);
	PCI_PMCAP_SETTINGS(pciev_vdev->pmcap);
	PCI_MSIXCAP_SETTINGS(pciev_vdev->msixcap);
	PCI_PCIECAP_SETTINGS(pciev_vdev->pciecap);
	PCI_EXTCAP_SETTINGS(pciev_vdev->extcap);

#ifdef CONFIG_PCIEV_FAST_X86_IRQ_HANDLING
	__init_apicid_to_cpuid();
#endif
	pciev_vdev->intx_disabled = false;

	pciev_vdev->virt_bus = __create_pci_bus();
	if (!pciev_vdev->virt_bus)
		return false;

	return true;
}
