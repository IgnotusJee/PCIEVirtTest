#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xdc658e53, "module_layout" },
	{ 0xc1f15dee, "kmalloc_caches" },
	{ 0x27864d57, "memparse" },
	{ 0x91b9a4ba, "e820__mapped_any" },
	{ 0x77358855, "iomem_resource" },
	{ 0xa6d60d1a, "pci_remove_root_bus" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xda67d27a, "pci_scan_bus" },
	{ 0x9e9fdd9d, "memunmap" },
	{ 0x9d0ccb82, "pci_bus_add_devices" },
	{ 0xeafc3090, "irq_get_irq_data" },
	{ 0x17d99d8, "pci_stop_root_bus" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0x92997ed8, "_printk" },
	{ 0x65487097, "__x86_indirect_thunk_rax" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0xbc8a1041, "param_get_ulong" },
	{ 0x5f8ff9eb, "kmem_cache_alloc_trace" },
	{ 0x4d924f20, "memremap" },
	{ 0x37a0cba, "kfree" },
	{ 0x69acdf38, "memcpy" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "141833BFC7327EA350E3683");
