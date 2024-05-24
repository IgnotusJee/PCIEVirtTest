#ifndef _LIB_PCIEV_HDR_H
#define _LIB_PCIEV_HDR_H

struct pciev_bar {
	uint64_t io_cnt;
	uint64_t storage_start;
	uint64_t storage_size;
};

// pci configuration uses little-endian
struct pci_header {
	struct {
		u16 vid; // vendor id, specify hardware manufacturer
		u16 did; // device id, given by hardware manufacturer
	} id;
	struct {
		u8 iose : 1;
		u8 mse : 1;
		u8 bme : 1;
		u8 sce : 1;
		u8 mwie : 1;
		u8 vga : 1;
		u8 pee : 1;
		u8 fixed : 1;
		u8 see : 1;
		u8 fbe : 1;
		u8 id : 1;
		u8 rsvd : 5;
	} cmd; // command register
	struct {
		u8 rsvd1 : 3;
		u8 is : 1;
		u8 cl : 1;
		u8 c66 : 1;
		u8 rsvd2 : 1;
		u8 fbc : 1;
		u8 dpd : 1;
		u8 devt : 2;
		u8 sta : 1;
		u8 rta : 1;
		u8 rma : 1;
		u8 sse : 1;
		u8 dpe : 1;
	} sts; // status register
	u8 rid; // revision id
	struct {
		u8 pi; // class code
		u8 scc; // subclass code
		u8 bcc; // Prog IF
	} cc;	// class code, A read-only register that specifies the type of function the device performs.
	u8 cls; // cache line size
	u8 mlt; // latency timer
	struct {
		u8 hl : 7;
		u8 mfd : 1; // PCI devices are inherently little-endian
	} htype; // header type
	struct {
		u8 cc : 4;
		u8 rsvd : 2;
		u8 sb : 1;
		u8 bc : 1;
	} bist; // BIST

	struct {
		u32 rte : 1; // 0 for Memory Space, 1 for IO Space
		u32 tp : 2; // type
		u32 pf : 1; // prefetchable
		u32 rsvd : 10; // lowbits, all zero, reserved(totally 14bits)
		u32 ba : 18; // valued bits
	} mlbar; // bar register

	u32 mulbar; // higher part of 64bit bar
	u32 idbar;

	u32 bar3;
	u32 bar4;
	u32 bar5;

	u32 ccptr; // Cardbus CIS Pointer

	struct {
		u16 ssvid; // Subsystem Vendor ID
		u16 ssid; // Subsystem ID
	} ss;

	u32 erom; // Expansion ROM base address
	u8 cap;	  // Capabilities Pointer, Points to a linked list of new capabilities implemented by the device
	u8 rsvd[7]; // Reserved Region
	struct {
		u8 iline; // Interrupt Line
		/*
			Specifies which input of the system interrupt controllers the device's interrupt pin is connected to
			and is implemented by any device that makes use of an interrupt pin. For the x86 architecture this
			register corresponds to the PIC IRQ numbers 0-15 (and not I/O APIC IRQ numbers) and
			a value of 0xFF defines no connection.
		*/
		u8 ipin;  // Interrupt PIN
		/*
			Specifies which interrupt pin the device uses. Where a value of 0x1 is INTA#, 0x2 is INTB#,
			0x3 is INTC#, 0x4 is INTD#, and 0x0 means the device does not use an interrupt pin.
		*/
	} intr;

	u8 mgnt; // Min Grant, A read-only register that specifies the burst period length, in 1/4 microsecond units, that the device needs (assuming a 33 MHz clock rate).
	u8 mlat; // Max latency, A read-only register that specifies how often the device needs access to the PCI bus (in 1/4 microsecond units)
};

struct pci_pm_cap {
	struct {
		u8 cid; // Capability Identifier
		u8 next; // Next Item Pointer
	} pid;
	struct {
		u16 vs : 3; // Version
		u16 pmec : 1; // PME Clock
		u16 resv : 1; // Reserved
		u16 dsi : 1;  // The Device Specific Initialization bit, indicates whether special initialization of this function is required
		u16 auxc : 3; // Aux_Current, This 3 bit field reports the 3.3Vaux auxiliary current requirements for the PCI function.
		u16 d1s : 1;  // D1 Power Management State support or not
		u16 d2s : 1;  // D2 Power Management State support or not
		u16 psup : 5; // PME Support
	} pc; // Power Management Capabilities, read only
	struct {
		u16 ps : 2; // PowerState
		u16 rsvd01 : 1; // Reserved for PCI Express
		u16 nsfrst : 1; // No_Soft_Reset
		u16 rsvd02 : 4; // Reserved
		u16 pmee : 1;	// PME_En
		u16 dse : 4;	// Data_Select
		u16 dsc : 2;	// Data_Scale
		u16 pmes : 1;	// PME_Status
	} pmcs; // Power Management Control/Status
	u8 ext[2]; // PMCSR_BSE and Data
};

struct pci_msi_cap {
	struct {
	} mid;
	struct {
	} mc;
	struct {
	} ma;
	struct {
	} mua;
	struct {
	} md;
	struct {
	} mmask;
	struct {
	} mpend;
};

struct pci_msix_cap {
	struct {
		u8 cid;
		u8 next;
	} mxid;
	struct {
		u16 ts : 11; // Table Size
		u16 rsvd : 3; // Reserved
		u16 fm : 1;	  // Function Mask
		u16 mxe : 1;  // MSI-X Enable
	} mxc; // Message Control
	struct {
		u32 tbir : 3; // BIR, which bar is used for message table
		u32 to : 29; // Table Offset, 8-byte aligned
	} mtab;
	struct {
		u32 pbir : 3; // Pending Bit BIR
		u32 pbao : 29; // Pending Bit Offset
	} mpba;
};

struct pcie_cap {
	struct {
		u8 cid;
		u8 next;
	} pxid;
	struct {
		u16 ver : 4; // Capability Version
		u16 dpt : 4; // Device/Port Type - Indicates the specific type of this PCI Express Function.
		u16 si : 1;	 // Slot Implemented
		u16 imn : 5; // Interrupt Message Number - For MSI-X, the value in this field indicates which MSI-X Table entry is used to generate the interrupt message
		u16 rsvd : 2;
	} pxcap; // PCI Express Capabilities Register
	struct {
		u32 mps : 3; // Max_Payload_Size Supported
		u32 pfs : 2; // Phantom Functions Supported
		u32 etfs : 1; // Extended Tag Field Supported
		u32 l0sl : 3; // Endpoint L0s Acceptable Latency
		u32 l1l : 3;  // Endpoint L1 Acceptable Latency
		u32 rsvd : 3;
		u32 rer : 1; // Role-Based Error Reporting
		u32 rsvd2 : 2;
		u32 csplv : 8; // Captured Slot Power Limit Value
		u32 cspls : 2; // Captured Slot Power Limit Scale
		u32 flrc : 1;  // Function Level Reset Capability
		u32 rsvd3 : 3;
	} pxdcap; // Device Capabilities Register
	struct {
		u16 cere : 1;
		u16 nfere : 1;
		u16 fere : 2;
		u16 urre : 1;
		u16 ero : 1;
		u16 mps : 3;
		u16 ete : 1;
		u16 pfe : 1;
		u16 appme : 1;
		u16 ens : 1;
		u16 mrrs : 3;
		u16 iflr : 1;
	} pxdc; // Device Control Register
	struct {
		u16 ced : 1;
		u16 nfed : 1;
		u16 fed : 1;
		u16 urd : 1;
		u16 apd : 1;
		u16 tp : 1;
		u16 rsvd : 10;
	} pxds; // Device Status Register
	struct {
		u32 sls : 4;
		u32 mlw : 6;
		u32 aspms : 2;
		u32 l0sel : 3;
		u32 l1el : 3;
		u32 cpm : 1;
		u32 sderc : 1;
		u32 dllla : 20;
		u32 lbnc : 1;
		u32 aoc : 1;
		u32 rsvd : 1;
		u32 pn : 8;
	} pxlcap; // Link Capabilities
	struct {
		u16 aspmc : 2;
		u16 rsvd : 1;
		u16 rcb : 1;
		u16 rsvd2 : 2;
		u16 ccc : 1;
		u16 es : 1;
		u16 ecpm : 1;
		u16 hawd : 1;
		u16 rsvd3 : 6;
	} pxlc; // Link Control
	struct {
		u16 clc : 4;
		u16 nlw : 6;
		u16 rsvd : 2;
		u16 scc : 1;
		u16 rsvd2 : 3;
	} pxls; // Link Status
	struct {
		u32 ctrs : 4;
		u32 ctds : 1;
		u32 arifs : 1;
		u32 aors : 1;
		u32 aocs32 : 1;
		u32 aocs : 1;
		u32 ccs128 : 1;
		u32 nprpr : 1;
		u32 ltrs : 1;
		u32 tphcs : 2;
		u32 rsvd : 4;
		u32 obffs : 2;
		u32 effs : 1;
		u32 eetps : 1;
		u32 meetp : 2;
		u32 rsvd2 : 8;
	} pxdcap2; // Device Capabilities 2
	struct {
		u32 ctv : 4;
		u32 ctd : 1;
		u32 rsvd : 5;
		u32 ltrme : 1;
		u32 rsvd2 : 2;
		u32 obffe : 2;
		u32 rsvd3 : 17;
	} pxdc2; // Device Control 2 and Device Status 2
}; // Device with Links

struct pci_ext_cap {
	u16 cid; // PCI Express Extended Capability ID
	u16 cver : 4; // Capability Version
	u16 next : 12; // Next Capability Offset
}; // PCI Express Extended Capability Header

struct pci_ext_cap_aer {
	struct pci_ext_cap id;
	struct {
		u32 rsvd : 4;
		u32 dlpes : 1;
		u32 rsvd2 : 7;
		u32 pts : 1;
		u32 fcpes : 1;
		u32 cts : 1;
		u32 cas : 1;
		u32 ucs : 1;
		u32 ros : 1;
		u32 mts : 1;
		u32 ecrces : 1;
		u32 ures : 1;
		u32 acsvs : 1;
		u32 uies : 1;
		u32 mcbts : 1;
		u32 aoebs : 1;
		u32 tpbes : 1;
		u32 rsvd3 : 6;
	} aeruces;
	struct {
		u32 rsvd : 4;
		u32 dlpem : 1;
		u32 rsvd2 : 7;
		u32 ptm : 1;
		u32 fcpem : 1;
		u32 ctm : 1;
		u32 cam : 1;
		u32 ucm : 1;
		u32 rom : 1;
		u32 mtm : 1;
		u32 ecrcem : 1;
		u32 urem : 1;
		u32 acsvm : 1;
		u32 uiem : 1;
		u32 mcbtm : 1;
		u32 aoebm : 1;
		u32 tpbem : 1;
		u32 rsvd3 : 6;
	} aerucem;
	struct {
		u32 rsvd : 4;
		u32 dlpesev : 1;
		u32 rsvd2 : 7;
		u32 ptsev : 1;
		u32 fcpesev : 1;
		u32 ctsev : 1;
		u32 casev : 1;
		u32 ucsev : 1;
		u32 rosev : 1;
		u32 mtsev : 1;
		u32 ecrcesev : 1;
		u32 uresev : 1;
		u32 acsvsev : 1;
		u32 uiesev : 1;
		u32 mcbtsev : 1;
		u32 aoebsev : 1;
		u32 tpbesev : 1;
		u32 rsvd3 : 6;
	} aerucesev;
	struct {
		u32 res : 1;
		u32 rsvd : 5;
		u32 bts : 1;
		u32 bds : 1;
		u32 rrs : 1;
		u32 rsvd2 : 3;
		u32 rts : 1;
		u32 anfes : 1;
		u32 cies : 1;
		u32 hlos : 1;
		u32 rsvd3 : 16;
	} aerces;
	struct {
		u32 rem : 1;
		u32 rsvd : 5;
		u32 btm : 1;
		u32 bdm : 1;
		u32 rrm : 1;
		u32 rsvd2 : 3;
		u32 rtm : 1;
		u32 anfem : 1;
		u32 ciem : 1;
		u32 hlom : 1;
		u32 rsvd3 : 16;
	} aercem;
	struct {
		u32 fep : 5;
		u32 egc : 1;
		u32 ege : 1;
		u32 ecc : 1;
		u32 ece : 1;
		u32 mhrc : 1;
		u32 mhre : 1;
		u32 tplp : 1;
		u32 rsvd : 20;
	} aercc;
	struct {
		u8 hb3;
		u8 hb2;
		u8 hb1;
		u8 hb0;
		u8 hb7;
		u8 hb6;
		u8 hb5;
		u8 hb4;
		u8 hb11;
		u8 hb10;
		u8 hb9;
		u8 hb8;
		u8 hb15;
		u8 hb14;
		u8 hb13;
		u8 hb12;
	} aerhl;
	struct {
		u8 tpl1b3;
		u8 tpl1b2;
		u8 tpl1b1;
		u8 tpl1b0;
		u8 tpl2b3;
		u8 tpl2b2;
		u8 tpl2b1;
		u8 tpl2b0;
		u8 tpl3b3;
		u8 tpl3b2;
		u8 tpl3b1;
		u8 tpl3b0;
		u8 tpl4b3;
		u8 tpl4b2;
		u8 tpl4b1;
		u8 tpl4b0;
	} aertlp;
};

struct pci_ext_cap_dsn {
	struct pci_ext_cap id;
	u64 serial;
};

#define PCIEV_PCI_DOMAIN_NUM 0x0001
#define PCIEV_PCI_BUS_NUM 0x10

//[PCI_HEADER][PM_CAP][MSIX_CAP][PCIE_CAP] | [AER_CAP][EXT_CAP]
#define SZ_PCI_HDR sizeof(struct pci_header) // 0
#define SZ_PCI_PM_CAP sizeof(struct pci_pm_cap) // 0x40
#define SZ_PCI_MSIX_CAP sizeof(struct pci_msix_cap) // 0x50
#define SZ_PCIE_CAP sizeof(struct pcie_cap) // 0x60

#define OFFS_PCI_HDR 0x0
#define OFFS_PCI_PM_CAP 0x40
#define OFFS_PCI_MSIX_CAP 0x50
#define OFFS_PCIE_CAP 0x60

#define SZ_HEADER (OFFS_PCIE_CAP + SZ_PCIE_CAP)

//#define PCI_CFG_SPACE_SIZE 0x100
#define PCI_EXT_CAP_START 0x50

#define OFFS_PCI_EXT_CAP (PCI_CFG_SPACE_SIZE)

enum {
	CAP_CSS_BIT_NVM = (1 << 0),
	CAP_CSS_BIT_SPECIFIC = (1 << 6),
	CAP_CSS_BIT_NOT_SUPPORTED = (1 << 7),
};

#endif /* _LIB_PCIEV_HDR_H */
