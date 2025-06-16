#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

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



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x102fe6de, "hrtimer_cancel" },
	{ 0xca192dd7, "device_remove_file" },
	{ 0xe095e43a, "device_destroy" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x2f2c95c4, "flush_work" },
	{ 0x3c12dfe, "cancel_work_sync" },
	{ 0x122c3a7e, "_printk" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0xbcf2e638, "i2c_smbus_read_i2c_block_data" },
	{ 0x570b288a, "i2c_smbus_read_byte_data" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xf311fc60, "class_create" },
	{ 0x60b678d1, "i2c_register_driver" },
	{ 0x464601cd, "i2c_del_driver" },
	{ 0x4a41ecb3, "class_destroy" },
	{ 0x29434d21, "i2c_smbus_write_byte_data" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0xfa61d21, "devm_kmalloc" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x93ab9e33, "device_create" },
	{ 0x89c41bdf, "device_create_file" },
	{ 0xea82d349, "hrtimer_init" },
	{ 0xc0b7c197, "hrtimer_start_range_ns" },
	{ 0x8c8569cb, "kstrtoint" },
	{ 0x2d3385d3, "system_wq" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0x135bb7ec, "hrtimer_forward" },
	{ 0x39ff040a, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cbosch,_bmp280");
MODULE_ALIAS("of:N*T*Cbosch,_bmp280C*");
MODULE_ALIAS("i2c:bmp280");

MODULE_INFO(srcversion, "4C3D3D374EA846890CE983D");
