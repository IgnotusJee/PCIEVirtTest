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

static int PCIEV_init(void) {
    printk(KERN_INFO "Module load...\n");
}

static void PCIEV_exit(void) {
	printk(KERN_INFO "Module unload...\n");
}

MODULE_LICENSE("GPL v2");
module_init(PCIEV_init);
module_init(PCIEV_exit);