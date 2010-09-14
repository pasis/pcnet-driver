/* pcnet.c: PCNet-PCI II/III Ethernet controller driver */

#include <linux/module.h>
#include <linux/init.h>

MODULE_AUTHOR("Dmitry Podgorny <pasis.ua@gmail.com>");
MODULE_DESCRIPTION("PCNet-PCI II/III Ethernet controller driver");
MODULE_VERSION("dev");
MODULE_LICENSE("GPL");

static int __init pcnet_init(void)
{
	printk(KERN_INFO "pcnet: module has loaded");

	return 0;
}

static void __exit pcnet_exit(void)
{
	printk(KERN_INFO "pcnet: module was unloaded");
}

module_init(pcnet_init);
module_exit(pcnet_exit);
