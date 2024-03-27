#ifndef _LIB_PCIEV_H
#define _LIB_PCIEV_H

#include <linux/pci.h>
#include <linux/msi.h>
#include <asm/apic.h>

struct pciev_dev {

};

extern struct pciev_dev *pciev_vdev;
struct pciev_dev *VDEV_INIT(void);
void VDEV_FINALIZE(struct pciev_dev *pciev_vdev);

#endif