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
	{ 0xbd06a567, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x68365207, __VMLINUX_SYMBOL_STR(kernel_kobj) },
	{ 0x68abb700, __VMLINUX_SYMBOL_STR(kobject_put) },
	{ 0x1403bc31, __VMLINUX_SYMBOL_STR(sysfs_create_group) },
	{ 0x44b5a2c1, __VMLINUX_SYMBOL_STR(kobject_create_and_add) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xe2d5255a, __VMLINUX_SYMBOL_STR(strcmp) },
	{ 0x373db350, __VMLINUX_SYMBOL_STR(kstrtoint) },
	{ 0xb1ad28e0, __VMLINUX_SYMBOL_STR(__gnu_mcount_nc) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "79652CC77AEDAE97694106B");
