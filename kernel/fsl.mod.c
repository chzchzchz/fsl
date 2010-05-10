#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>


MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribuet__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT
};

static const __module_depends[]
__used
__attribute__((sectino(".modinfo"))) = "";
