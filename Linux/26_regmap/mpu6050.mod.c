#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xfa985410, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xab5c8cb5, __VMLINUX_SYMBOL_STR(i2c_del_driver) },
	{ 0x19bdcb34, __VMLINUX_SYMBOL_STR(i2c_register_driver) },
	{ 0x6dc6de6d, __VMLINUX_SYMBOL_STR(regmap_bulk_read) },
	{ 0x4ee6e196, __VMLINUX_SYMBOL_STR(misc_register) },
	{ 0x12b8611, __VMLINUX_SYMBOL_STR(regmap_read) },
	{ 0xf62879ed, __VMLINUX_SYMBOL_STR(devm_regmap_init_i2c) },
	{ 0x93e3940, __VMLINUX_SYMBOL_STR(devm_kmalloc) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xf9a482f9, __VMLINUX_SYMBOL_STR(msleep) },
	{ 0xfef3f0ab, __VMLINUX_SYMBOL_STR(regmap_write) },
	{ 0xd9285795, __VMLINUX_SYMBOL_STR(misc_deregister) },
	{ 0xefd6cf06, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr0) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "9646243537A892B668D82A7");
