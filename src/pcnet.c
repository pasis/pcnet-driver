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
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/spinlock.h>

#define DRV_NAME	"pcnet_dummy"
#define DRV_VERSION	"dev"
#define DRV_DESCRIPTION	"PCNet-PCI II/III Ethernet controller driver"

MODULE_AUTHOR("Dmitry Podgorny <pasis.ua@gmail.com>");
MODULE_AUTHOR("Denis Kirjanov <kirjanov@gmail.com>");
MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_VERSION(DRV_VERSION);
MODULE_LICENSE("GPL");

static DEFINE_PCI_DEVICE_TABLE(pcnet_dummy_pci_tbl) = {
	{ PCI_DEVICE(PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_AMD_LANCE) },
	{ }
};

enum {
	PCNET_IOSIZE_LEN = 0x20,
};

struct pcnet_private {
	/* TODO:
	 * DMA buffer management operations
	 */
	spinlock_t lock;
	struct pci_dev *pci_dev;
	void __iomem *base;
};

static int pcnet_dummy_open(struct net_device *ndev)
{
	return 0;
}

static int pcnet_dummy_stop(struct net_device *ndev)
{
	return 0;
}

static int pcnet_dummy_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	return 0;
}

#ifdef HAVE_NET_DEVICE_OPS
/* net_device_ops structure is new for 2.6.31 */
static const struct net_device_ops pcnet_net_device_ops = {
	.ndo_open = pcnet_dummy_open,
	.ndo_stop = pcnet_dummy_stop,
	.ndo_start_xmit = pcnet_dummy_start_xmit
};
#endif /* HAVE_NET_DEVICE_OPS */

static int __devinit pcnet_dummy_init_netdev(struct pci_dev *pdev,
		unsigned long ioaddr)
{
	struct net_device *ndev = pci_get_drvdata(pdev);
	struct pcnet_private *pp;
	int irq;

	irq = pdev->irq;
	pp = netdev_priv(ndev);
	ndev->base_addr = ioaddr;
	ndev->irq = irq;
	pp->pci_dev = pdev;
	spin_lock_init(&pp->lock);

	/* init DMA rings */
	/* init net_dev_ops */
#ifdef HAVE_NET_DEVICE_OPS
	ndev->netdev_ops = &pcnet_net_device_ops;
#else
	ndev->open = pcnet_dummy_open;
	ndev->stop = pcnet_dummy_stop;
	ndev->hard_start_xmit = pcnet_dummy_start_xmit;
#endif /* HAVE_NET_DEVICE_OPS */

	/* registers net_device and returns err */
	/* TODO: reset the chip at the end */

	return 0;
}

static int __devinit pcnet_dummy_init_one(struct pci_dev *pdev,
		const struct pci_device_id *ent)
{
	struct pcnet_private *pp;
	struct net_device *ndev;
	void __iomem *ioaddr;
#ifdef USE_IO_OPS
	int bar = 0;
#else
	int bar = 1;
#endif

	if (pci_enable_device(pdev))
		return -ENODEV;
	/* enables bus-mastering for device pdev */
	pci_set_master(pdev);

	ndev = alloc_etherdev(sizeof(*pp));
	if (!ndev)
		goto out;
	/* register ourself under /sys/class/net/ */
	SET_NETDEV_DEV(ndev, &pdev->dev);

	if (pci_request_regions(pdev, DRV_NAME))
		goto out_netdev;

	ioaddr = pci_iomap(pdev, bar, PCNET_IOSIZE_LEN);
	if (!ioaddr)
		goto out_res;
	pci_set_drvdata(pdev, ndev);

	if (pcnet_dummy_init_netdev(pdev, (unsigned long)ioaddr))
		goto out_res_unmap;

	return 0;

out_res_unmap:
	pci_iounmap(pdev, ioaddr);
out_res:
	pci_release_regions(pdev);
out_netdev:
	free_netdev(ndev);
out:
	pci_disable_device(pdev);
	return -ENODEV;
}

static void __devexit pcnet_dummy_remove_one(struct pci_dev *pdev)
{
	struct net_device *ndev = pci_get_drvdata(pdev);
	struct pcnet_private *pp;

	pp = netdev_priv(ndev);
	pci_iounmap(pdev, pp->base);
	free_netdev(ndev);
	pci_disable_device(pdev);
	pci_release_regions(pdev);
	pci_set_drvdata(pdev, NULL);
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
