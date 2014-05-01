#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
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
	{ 0x3b69997a, "module_layout" },
	{ 0x6b739471, "wake_up_process" },
	{ 0x6cadc7a6, "kthread_create_on_node" },
	{ 0x27e1a049, "printk" },
	{ 0x7d11c268, "jiffies" },
	{ 0xcea94adf, "set_cpus_allowed_ptr" },
	{ 0x119af014, "cpu_bit_bitmap" },
	{ 0x9db7a936, "current_task" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "D622263B684BA6A65525502");
