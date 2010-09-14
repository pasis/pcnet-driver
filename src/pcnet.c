/* pcnet.c: PCNet-PCI II/III Ethernet controller driver */
/*
 * Authors:
 *		Dmitry Podgorny <pasis.ua@gmail.com>
 *		Denis Kirjanov <kirjanov@gmail.com>
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/pci.h>

#define DRV_NAME	"pcnet_dummy"
#define DRV_VERSION	"dev"
#define DRV_DESCRIPTION	"PCNet-PCI II/III Ethernet controller driver"

MODULE_AUTHOR("Dmitry Podgorny <pasis.ua@gmail.com>");
MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_VERSION(DRV_VERSION);
MODULE_LICENSE("GPL");

static DEFINE_PCI_DEVICE_TABLE(pcnet_dummy_pci_tbl) = {
	{ PCI_DEVICE(PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_AMD_LANCE) },
	{ }
};

static int __devinit pcnet_dummy_init_one(struct pci_dev *pdev,
		const struct pci_device_id *ent)
{
	/* basically pci-skeleton stuff */
	/* reset the chip at the end */
	return 0;
}

static void __devexit pcnet_dummy_remove_one(struct pci_dev *pdev)
{
	/* noop */
}

static struct pci_driver pcnet_dummy_driver = {
	.name		= DRV_NAME,
	.id_table	= pcnet_dummy_pci_tbl,
	.probe		= pcnet_dummy_init_one,
	.remove		= __devexit_p(pcnet_dummy_remove_one),
#if 0
	/* FIXME: add PM hooks */
#endif
};

static int __init pcnet_init(void)
{
	printk(KERN_INFO DRV_NAME ": " DRV_DESCRIPTION);

	return pci_register_driver(&pcnet_dummy_driver);
}

static void __exit pcnet_exit(void)
{
	printk(KERN_INFO DRV_NAME ": unloading...");
	pci_unregister_driver(&pcnet_dummy_driver);
}

module_init(pcnet_init);
module_exit(pcnet_exit);
