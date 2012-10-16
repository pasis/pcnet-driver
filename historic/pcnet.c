/* pcnet.c: PCnet-PCI ethernet driver for x86
 *
 *	copyright (c) 2009 Dmitry Podgorny <pasis.ua@gmail.com>
 *
 *
 */

#ifndef MODULE
#define MODULE
#endif

// ------------------------------------------------------------
// include

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>

#include <asm/io.h>

// ------------------------------------------------------------
// Описание модуля

#define DRV_VERSION "0.1"
#define DRV_NAME "pcnet"
#define DRV_DESCRIPTION "PCnet-PCI II network card driver"

MODULE_AUTHOR("Dmitry Podgorny <pasis.ua@gmail.com>");
MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

// ------------------------------------------------------------
// Параметры драйвера (адреса и тп)

#define PCNET_IO_RANGE_SIZE	32
#define PCNET_INIT_TIMEOUT	1000
#define PCNET_MAX_PKT_SIZE	1528
#define PCNET_RLEN		0x7	/* 128 дескриптора */
#define PCNET_TLEN		0x7	/* 128 дескриптора */

enum {				/* 16bit IO offsets */
	PCNET_PIO_RDP = 0x10,
	PCNET_PIO_RAP = 0x12,
	PCNET_PIO_RESET = 0x14,
	PCNET_PIO_BDP = 0x16
};

enum {				/* 32bit IO offsets */
	PCNET_PIO32_RDP = 0x10,
	PCNET_PIO32_RAP = 0x14,
	PCNET_PIO32_RESET = 0x18,
	PCNET_PIO32_BDP = 0x1c
};

enum {				/* CSR */
	CSR0 = 0,
	CSR1 = 1,
	CSR2 = 2,
	CSR4 = 4,
	CSR15 = 15,
	CSR112 = 112		/* Missed Frame Count */
};

enum {				/* BCR */
	BCR9 = 9,
};

enum {				/* CSR0 */
	CSR0_INIT = 0x0001,
	CSR0_START= 0x0002,
	CSR0_STOP = 0x0004,
	CSR0_TDMD = 0x0008,	/* transmit demand */
	CSR0_TXON = 0x0010,	/* transmitter on */
	CSR0_RXON = 0x0020,	/* receiver on */
	CSR0_IENA = 0x0040,	/* interrupt enable */
	CSR0_INTR = 0x0080,
	CSR0_IDON = 0x0100,	/* initialisation done */
	CSR0_TINT = 0x0200,
	CSR0_RINT = 0x0400,
	CSR0_MERR = 0x0800,
	CSR0_MISS = 0x1000,
	CSR0_CERR = 0x2000,
	CSR0_BABL = 0x4000,
	CSR0_ERR  = 0x8000
};

enum {				/* CSR4 */
	CSR4_APAD_XMT = 0x0800,	/* automatic padding feature */
};

enum {				/* CSR15 */
	CSR15_DXMTFCS = 0x0008,	/* if 0, the transmitter will generate and append FCS  */
	CSR15_DRTY = 0x0020,	/* disable retry on collision */
	CSR15_PROM = 0x8000,	/* promiscuous mode */
};

enum {				/* BCR9 */
	BCR9_FDEN = 0x0001,	/* Full-duplex Enable */
	BCR9_AUIFD = 0x0002,	/* AUI full-duplex */
};

enum {				/* modes (16 or 32bit) */
	PCNET_MODE_16,
	PCNET_MODE_32,
	PCNET_MODE_ERROR = -1
};

enum {				/* md1 */
	MD1_ENP		= 0x01000000,	/* end of packet */
	MD1_STP		= 0x02000000,	/* start of packet */
	MD1_RXBUFF	= 0x04000000,	/* buffer error */
	MD1_DEF		= 0x04000000,	/* deferred */
	MD1_CRC		= 0x08000000,	/* CRC error */
	MD1_ONE		= 0x08000000,	/* one retry needed */
	MD1_OFLO	= 0x10000000,	/* overflow error */
	MD1_MORE	= 0x10000000,	/* more than one retry needed */
	MD1_FRAM	= 0x20000000,	/* framing error */
	MD1_ADDFSC	= 0x20000000,	/* add FSC */
	MD1_RXERR	= 0x40000000,	/* Fram|Oflo|Crc|RxBuff */
	MD1_TXERR	= 0x40000000,	/* Uflo|Lcol|Lcar|Rtry */
	MD1_OWN		= 0x80000000
};

enum {				/* md2 */
	MD2_RTRY	= 0x04000000,	/* failed after repeated retries */
	MD2_LCAR	= 0x08000000,	/* loss of carrier */
	MD2_LCOL	= 0x10000000,	/* late collision */
	MD2_UFLO	= 0x40000000,	/* underflow error */
	MD2_TXBUFF	= 0x80000000	/* buffer error */
};

struct pcnet_init_block {
	u16	mode;		/* CSR15 */
	u8	rlen;		/* старшие 4 бита */
	u8	tlen;		/* старшие 4 бита */
	u8	padr[6];	/* физический адрес */
	u8	reserved[2];
	u8	ladrf[8];
	u32	rdra;
	u32	tdra;
};

struct pcnet_ring_desc {
	u32	addr;
	u32	md1;
	u32	md2;
	u32	reserved;
};

#define READ_CSR(csr)		priv->read_csr(ioaddr, (csr))
#define WRITE_CSR(csr, data)	priv->write_csr(ioaddr, (csr), (data))
#define READ_BCR(bcr)		priv->read_bcr(ioaddr, (bcr))
#define WRITE_BCR(bcr, data)	priv->write_bcr(ioaddr, (bcr), (data))

// ------------------------------------------------------------
// Прототипы функций
int pcnet_probe(struct pci_dev *pdev, const struct pci_device_id *ent);
int pcnet_init_netdevice(struct pci_dev *pdev, unsigned int ioaddr);
void pcnet_remove(struct pci_dev *pdev);

struct pcnet_private *pcnet_get_priv(struct net_device *dev);
int pcnet_init_block_setup(struct net_device *dev, unsigned int rx_size, unsigned int tx_size);
void pcnet_init_block_free(struct net_device *dev);
struct pcnet_ring_desc *pcnet_ring_alloc(unsigned int count, struct pci_dev *pdev, dma_addr_t *dma);
void pcnet_ring_free(unsigned int count, struct pci_dev *pdev, struct pcnet_ring_desc *ring, dma_addr_t dma);

int pcnet_check_mode(unsigned int ioaddr);
void pcnet_reset(unsigned int ioaddr);
unsigned int pcnet_read_csr(unsigned int ioaddr, int csr_num);
void pcnet_write_csr(unsigned int ioaddr, int csr_num, u32 data);
unsigned int pcnet_read_bcr(unsigned int ioaddr, int bcr_num);
void pcnet_write_bcr(unsigned int ioaddr, int bcr_num, u32 data);
void pcnet_reset32(unsigned int ioaddr);
unsigned int pcnet_read32_csr(unsigned int ioaddr, int csr_num);
void pcnet_write32_csr(unsigned int ioaddr, int csr_num, u32 data);
unsigned int pcnet_read32_bcr(unsigned int ioaddr, int bcr_num);
void pcnet_write32_bcr(unsigned int ioaddr, int bcr_num, u32 data);

int pcnet_open(struct net_device *dev);
int pcnet_stop(struct net_device *dev);
int pcnet_start_xmit(struct sk_buff *skb, struct net_device *dev);
void pcnet_rx_tx(unsigned long data);
irqreturn_t pcnet_interrupt(int irq, void *device);

int pcnet_init(void);
void pcnet_exit(void);

// ------------------------------------------------------------
// Описание PCI

struct pci_device_id pcnet_pci_tbl[] = {
	{ PCI_DEVICE( PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_AMD_LANCE ), },
	{ }
};

MODULE_DEVICE_TABLE(pci, pcnet_pci_tbl);

struct pci_driver pcnet_pci_driver = {
	.name		= DRV_NAME,
	.id_table	= pcnet_pci_tbl,
	.probe		= pcnet_probe,
	.remove		= pcnet_remove
};

// ------------------------------------------------------------
// Описание сетевой подсистемы

struct pcnet_private {
	spinlock_t lock;
	struct pci_dev *pdev;
	struct tasklet_struct rx_tx_task;
	struct pcnet_init_block	*init_block;
	dma_addr_t init_block_dma;

	struct pcnet_ring_desc *rx_ring;
	struct pcnet_ring_desc *tx_ring;
	struct sk_buff **rx_skb;
	struct sk_buff **tx_skb;
	dma_addr_t rx_ring_dma;
	dma_addr_t tx_ring_dma;
	unsigned int rx_len;
	unsigned int tx_len;
	unsigned int rx_mask;		/* биты, которые могут использоваться в номере дескриптора */
	unsigned int tx_mask;		/*   вместо модуля по rx_len (tx_len)*/
	unsigned int rx_current;
	unsigned int tx_current;
	unsigned int tx_last;
	unsigned int tx_pks;		/* кол-во пакетов в очереди */

	void	(*reset) (unsigned int);
	u32	(*read_csr) (unsigned int, int);
	void	(*write_csr) (unsigned int, int, u32);
	u32	(*read_bcr) (unsigned int, int);
	void	(*write_bcr) (unsigned int, int, u32);
};

DECLARE_TASKLET(pcnet_rx_tx_task, pcnet_rx_tx, 0);

// в 2.6.27 структуры net_device_ops еще нету...
// в 2.6.31 наоброт - указатели на функции определены в ней
// см. <linux/netdevice.h>
#ifdef HAVE_NET_DEVICE_OPS
struct net_device_ops pcnet_netdev_ops = {
	.ndo_open = pcnet_open,
	.ndo_stop = pcnet_stop,
	.ndo_start_xmit = pcnet_start_xmit
};
#endif /* HAVE_NET_DEVICE_OPS */

// ============================================================
// А тут пошли сами функции... =^.^=
//

int __devinit pcnet_probe(struct pci_dev *pdev, const struct pci_device_id *ent) {

	unsigned int ioaddr;		// базовый адрес
	unsigned int iosize;		// размер области памяти
	int result;

	// 
	if ( pci_enable_device(pdev) ) {
		return -ENODEV;
	}

	// базовый адрес
	ioaddr = pci_resource_start(pdev, 0);
	iosize = pci_resource_len(pdev, 0);
	if ( ( !ioaddr ) || ( iosize < PCNET_IO_RANGE_SIZE ) ) {
		pci_disable_device(pdev);
		return -ENODEV;
	}

	// резервирование области портов ввода-вывода
	if ( pci_request_regions(pdev, DRV_NAME) ) {
		pci_disable_device(pdev);
		return -EBUSY;
	}

	// enables bus-mastering for device pdev
	pci_set_master(pdev);

	result = pcnet_init_netdevice(pdev, ioaddr);
	if ( result ) {
		pci_release_regions(pdev);
		pci_disable_device(pdev);
	}

	return result;

}

int __devinit pcnet_init_netdevice(struct pci_dev *pdev, unsigned int ioaddr) {

	struct net_device *dev;
	struct pcnet_private *priv;
	int i, mode;

	// выделение памяти для структуры net_device и заполнение значениями для Ethernet
	dev = alloc_etherdev( sizeof(struct pcnet_private) );
	if ( !dev ) {
		return -ENOMEM;
	}

	// (dev)->dev.parent = &pdev->dev
	SET_NETDEV_DEV(dev, &pdev->dev);

	// -----------------------------------------------------------
	// заполнение полей структуры pcnet_private
	
	priv = pcnet_get_priv(dev);

	mode = pcnet_check_mode(ioaddr);
	if ( mode == PCNET_MODE_16 ) {
		priv->reset = pcnet_reset;
		priv->read_csr = pcnet_read_csr;
		priv->write_csr = pcnet_write_csr;
		priv->read_bcr = pcnet_read_bcr;
		priv->write_bcr = pcnet_write_bcr;
	}
	else if ( mode == PCNET_MODE_32 ) {
		priv->reset = pcnet_reset32;
		priv->read_csr = pcnet_read32_csr;
		priv->write_csr = pcnet_write32_csr;
		priv->read_bcr = pcnet_read32_bcr;
		priv->write_bcr = pcnet_write32_bcr;
	}
	else {
		free_netdev(dev);
		return -ENODEV;
	}

	spin_lock_init(&priv->lock);
	priv->pdev = pdev;
	pcnet_init_block_setup(dev, PCNET_RLEN, PCNET_TLEN);
	memcpy(&priv->rx_tx_task, &pcnet_rx_tx_task, sizeof(struct tasklet_struct));
	priv->rx_tx_task.data = (unsigned long) dev;

	// -----------------------------------------------------------
	// заполнение полей структуры net_device

	#ifdef HAVE_NET_DEVICE_OPS
		dev->netdev_ops = &pcnet_netdev_ops;
	#else
		dev->open = pcnet_open;
		dev->stop = pcnet_stop;
		dev->hard_start_xmit = pcnet_start_xmit;
	#endif /* HAVE_NET_DEVICE_OPS */

	dev->base_addr = ioaddr;
	dev->irq = pdev->irq;
	
	// физический (MAC) адрес
	//dev->addr_len = 6; думаю(!) alloc_etherdev присвоил, как и широковещательный адрес
	for (i = 0; i < 6; i++) {
		dev->dev_addr[i] = inb(ioaddr + i);
	}

	//
	//------------------------------------------------------------

	//
	pci_set_drvdata(pdev, dev);

	// регистрация сетевого интерфейса
	// различие с register_netdevice в использовании семафор
	register_netdev(dev);

	// информируем об удачной регистрации =^.^=
	printk(KERN_INFO "%s: %s is registered, HWADDR=%02x:%02x:%02x:%02x:%02x:%02x\n",
			dev->name,
			DRV_NAME,
			dev->dev_addr[0],
			dev->dev_addr[1],
			dev->dev_addr[2],
			dev->dev_addr[3],
			dev->dev_addr[4],
			dev->dev_addr[5]
		);

	return 0;

}

void __devexit pcnet_remove(struct pci_dev *pdev) {

	struct net_device *dev = pci_get_drvdata(pdev);

	unregister_netdev(dev);
	pcnet_init_block_free(dev);
	pci_release_regions(pdev);
	pci_set_drvdata(pdev, NULL);
	free_netdev(dev);
	pci_disable_device(pdev);

}

struct pcnet_private *pcnet_get_priv(struct net_device *dev) {

	#ifdef HAVE_NETDEV_PRIV
		return (struct pcnet_private *) netdev_priv(dev);
	#else
		return (struct pcnet_private *) dev->priv;
	#endif /* HAVE_NETDEV_PRIV */
	
}

int pcnet_init_block_setup(struct net_device *dev, unsigned int rx_size, unsigned int tx_size) {

	struct pcnet_private *priv = pcnet_get_priv(dev);
	struct pcnet_init_block *init_block;
	int i;

	init_block = pci_alloc_consistent(priv->pdev, sizeof(struct pcnet_init_block), &priv->init_block_dma);

	init_block->rlen = rx_size << 4;
	init_block->tlen = tx_size << 4;
	priv->rx_len = 1 << rx_size;
	priv->tx_len = 1 << tx_size;
	priv->rx_mask = priv->rx_len - 1;
	priv->tx_mask = priv->tx_len - 1;

	// TODO: проверка на NULL!!!
	priv->rx_ring = pcnet_ring_alloc(priv->rx_len, priv->pdev, &priv->rx_ring_dma);
	priv->tx_ring = pcnet_ring_alloc(priv->tx_len, priv->pdev, &priv->tx_ring_dma);

	priv->rx_skb = kcalloc(priv->rx_len, sizeof(struct sk_buff *), GFP_ATOMIC);
	priv->tx_skb = kcalloc(priv->tx_len, sizeof(struct sk_buff *), GFP_ATOMIC);

	for(i = 0; i < priv->rx_len; i++) {
		priv->rx_skb[i] = dev_alloc_skb(PCNET_MAX_PKT_SIZE);
		priv->rx_ring[i].addr = pci_map_single(priv->pdev, priv->rx_skb[i]->data, PCNET_MAX_PKT_SIZE, PCI_DMA_FROMDEVICE);
		priv->rx_ring[i].md1 = MD1_OWN | ( (-PCNET_MAX_PKT_SIZE) & 0xffff );
	}

	init_block->rdra = (u32) priv->rx_ring_dma;
	init_block->tdra = (u32) priv->tx_ring_dma;

	priv->init_block = init_block;

	return 0;

}

void pcnet_init_block_free(struct net_device *dev) {

	struct pcnet_private *priv = pcnet_get_priv(dev);
	int i;

	for(i = 0; i < priv->rx_len; i++) {
		pci_unmap_single(priv->pdev, priv->rx_ring[i].addr, PCNET_MAX_PKT_SIZE, PCI_DMA_FROMDEVICE);
		dev_kfree_skb(priv->rx_skb[i]);
	}

	kfree(priv->rx_skb);
	kfree(priv->tx_skb);

	pcnet_ring_free(priv->rx_len, priv->pdev, priv->rx_ring, priv->rx_ring_dma);
	pcnet_ring_free(priv->tx_len, priv->pdev, priv->tx_ring, priv->tx_ring_dma);

	pci_free_consistent(priv->pdev, sizeof(struct pcnet_init_block), priv->init_block, priv->init_block_dma);

}

struct pcnet_ring_desc *pcnet_ring_alloc(unsigned int count, struct pci_dev *pdev, dma_addr_t *dma) {

	struct pcnet_ring_desc *ring;

	ring = pci_alloc_consistent(pdev, sizeof(struct pcnet_ring_desc) * count, dma);
	memset(ring, 0, sizeof(struct pcnet_ring_desc) * count);

	return ring;

}

void pcnet_ring_free(unsigned int count, struct pci_dev *pdev, struct pcnet_ring_desc *ring, dma_addr_t dma) {

	// TODO: освободить занятые буферы, нужно делать при установленом бите STOP
	// TODO: проверить на NULL

	pci_free_consistent(pdev, sizeof(struct pcnet_ring_desc) * count, ring, dma);

}

/* pcnet_check_mode:
 * возвращает:
 * 	PCNET_MODE_16
 * 	PCNET_MODE_32
 * 	PCNET_MODE_ERROR
 */
int pcnet_check_mode(unsigned int ioaddr) {

	pcnet_reset(ioaddr);
	if ( pcnet_read_csr(ioaddr, CSR0) == CSR0_STOP )
		return PCNET_MODE_16;
	
	pcnet_reset32(ioaddr);
	if ( pcnet_read32_csr(ioaddr, CSR0) == CSR0_STOP )
		return PCNET_MODE_32;

	return PCNET_MODE_ERROR;

}

void pcnet_reset(unsigned int ioaddr) {

	inw( ioaddr + PCNET_PIO_RESET );

}

unsigned int pcnet_read_csr(unsigned int ioaddr, int csr_num) {

	outw(csr_num, ioaddr + PCNET_PIO_RAP);
	return inw(ioaddr + PCNET_PIO_RDP);

}

void pcnet_write_csr(unsigned int ioaddr, int csr_num, u32 data) {

	outw(csr_num, ioaddr + PCNET_PIO_RAP);
	outw(data, ioaddr + PCNET_PIO_RDP);

}

unsigned int pcnet_read_bcr(unsigned int ioaddr, int bcr_num) {

	outw(bcr_num, ioaddr + PCNET_PIO_RAP);
	return inw(ioaddr + PCNET_PIO_BDP);

}

void pcnet_write_bcr(unsigned int ioaddr, int bcr_num, u32 data) {

	outw(bcr_num, ioaddr + PCNET_PIO_RAP);
	outw(data, ioaddr + PCNET_PIO_BDP);

}

void pcnet_reset32(unsigned int ioaddr) {

	inl( ioaddr + PCNET_PIO32_RESET );

}

unsigned int pcnet_read32_csr(unsigned int ioaddr, int csr_num) {

	outl(csr_num, ioaddr + PCNET_PIO32_RAP);
	return inl(ioaddr + PCNET_PIO32_RDP) & 0xffff;	// верхние 16 бит не определены

}

void pcnet_write32_csr(unsigned int ioaddr, int csr_num, u32 data) {

	outl(csr_num, ioaddr + PCNET_PIO32_RAP);
	outl(data, ioaddr + PCNET_PIO32_RDP);

}

unsigned int pcnet_read32_bcr(unsigned int ioaddr, int bcr_num) {

	outl(bcr_num, ioaddr + PCNET_PIO32_RAP);
	return inl(ioaddr + PCNET_PIO32_BDP) & 0xffff;

}

void pcnet_write32_bcr(unsigned int ioaddr, int bcr_num, u32 data) {

	outl(bcr_num, ioaddr + PCNET_PIO32_RAP);
	outl(data, ioaddr + PCNET_PIO32_BDP);

}

int pcnet_open(struct net_device *dev) {

	struct pcnet_private *priv = pcnet_get_priv(dev);
	unsigned int ioaddr = dev->base_addr;
	int result;
	int timeout;
	unsigned long flags;

	result = request_irq(dev->irq, pcnet_interrupt, IRQF_SHARED, DRV_NAME, dev);
	if ( result )
		return result;

	spin_lock_irqsave(&priv->lock, flags);

	priv->reset(ioaddr);
	//WRITE_CSR(CSR0, CSR0_STOP); так как мы сделали reset, то бит STOP и так установлен и контроллер сброшен
	
	// TODO: проверить это!!!!!
	   WRITE_BCR(20, 2);
	WRITE_BCR(BCR9, BCR9_FDEN | BCR9_AUIFD);	/* Full-Duplex */

	priv->init_block->mode = READ_CSR(CSR15);
	memcpy(priv->init_block->padr, dev->dev_addr, 6);
	memset(priv->init_block->ladrf, 0, 8);
	// остальное должно быть заполнено в pcnet_init_block_alloc()

	priv->rx_current = 0;
	priv->tx_current = 0;
	priv->tx_last = 0;
	priv->tx_pks = 0;

	WRITE_CSR(CSR1, (unsigned int) priv->init_block_dma & 0xffff);
	WRITE_CSR(CSR2, ( (unsigned int) priv->init_block_dma >> 16 ) & 0xffff );

	WRITE_CSR(CSR0, CSR0_INIT);
	timeout = PCNET_INIT_TIMEOUT;
	while ( ( ! (READ_CSR(CSR0) & CSR0_IDON) ) && --timeout )
		udelay(10);

	// TODO: обработка таймаута

	WRITE_CSR(CSR0, CSR0_START | CSR0_IENA);
	netif_start_queue(dev);

	spin_unlock_irqrestore(&priv->lock, flags);

	return 0;

}

int pcnet_stop(struct net_device *dev) {

	struct pcnet_private *priv = pcnet_get_priv(dev);
	unsigned int ioaddr = dev->base_addr;
	unsigned long flags;

	spin_lock_irqsave(&priv->lock, flags);

	WRITE_CSR(CSR0, CSR0_STOP);
	free_irq(dev->irq, dev);
	netif_stop_queue(dev);

	spin_unlock_irqrestore(&priv->lock, flags);

	return 0;

}


int pcnet_start_xmit(struct sk_buff *skb, struct net_device *dev) {

	// считаем, что хоятбы 1 дескриптор в кольце свободен

	struct pcnet_private *priv = pcnet_get_priv(dev);
	struct pcnet_ring_desc *ring = priv->tx_ring;
	unsigned int entry = priv->tx_current;
	unsigned int ioaddr = dev->base_addr;
	unsigned long flags;

	spin_lock_irqsave(&priv->lock, flags);

	priv->tx_skb[entry] = skb;
	ring[entry].addr = (u32) pci_map_single(priv->pdev, skb->data, skb->len, PCI_DMA_TODEVICE);
	ring[entry].md2 = 0x0;
	ring[entry].md1 = ( -skb->len ) & 0xffff;	// длина пакета, дополненная до 2-х и 4 бита единиц.
	ring[entry].md1 |= MD1_OWN | MD1_ENP | MD1_STP;	// TODO: wtf ENP & STP?

	priv->tx_current = ++entry & priv->tx_mask;
	if ( ++(priv->tx_pks) >= priv->tx_len )
		netif_stop_queue(dev);

	WRITE_CSR(CSR0, READ_CSR(CSR0) | CSR0_TDMD);	// установка бита TDMD
	dev->trans_start = jiffies;

	spin_unlock_irqrestore(&priv->lock, flags);

	return 0;

}

void pcnet_rx_tx(unsigned long data){

	struct net_device *dev = (struct net_device *) data;
	struct pcnet_private *priv = pcnet_get_priv(dev);
	struct pcnet_ring_desc *rx_ring = priv->rx_ring;
	struct pcnet_ring_desc *tx_ring = priv->tx_ring;
	struct sk_buff **rx_skb = priv->rx_skb;
	struct sk_buff **tx_skb = priv->tx_skb;
	struct sk_buff *skb;
	unsigned int rx_mask = priv->rx_mask;
	unsigned int tx_mask = priv->tx_mask;
	unsigned int ioaddr = dev->base_addr;
	unsigned int entry;
	unsigned int len;

	entry = priv->rx_current;
	while ( ( rx_ring[entry].md1 & MD1_OWN ) == 0 ) {
		len = ( rx_ring[entry].md2 & 0x0fff ) - 4;
		skb = dev_alloc_skb(len);
		if ( skb ) {
			skb_copy_from_linear_data(rx_skb[entry], skb_put(skb, len), len);
			skb->dev = dev;
			skb->protocol = eth_type_trans(skb, dev);
			netif_rx(skb);
			dev->stats.rx_bytes += len;
			dev->stats.rx_packets++;
		}
		else {
			/* нет памяти. выкидываем пакет */
			dev->stats.rx_dropped++;
		}

		rx_ring[entry].md1 |= MD1_OWN;
		entry = ( entry + 1 ) & rx_mask;
	}
	priv->rx_current = entry;

	spin_lock(&priv->lock);
	entry = priv->tx_last;
	while ( priv->tx_pks && ( (tx_ring[entry].md1 & MD1_OWN) == 0 ) ) {
		len = tx_skb[entry]->len;
		pci_unmap_single(priv->pdev, tx_ring[entry].addr, len, PCI_DMA_TODEVICE);
		dev_kfree_skb(tx_skb[entry]);
		tx_skb[entry] = NULL;

		entry = ( entry + 1 ) & tx_mask;
		priv->tx_pks--;
		dev->stats.tx_bytes += len;
		dev->stats.tx_packets++;
	}
	priv->tx_last = entry;
	spin_unlock(&priv->lock);

	if ( netif_queue_stopped(dev) && priv->tx_pks < priv->tx_len )
		netif_wake_queue(dev);

	WRITE_CSR(CSR0, CSR0_IENA);

}

irqreturn_t pcnet_interrupt(int irq, void *device) {

	struct net_device *dev = (struct net_device *) device;
	struct pcnet_private *priv = pcnet_get_priv(dev);
	unsigned int ioaddr = dev->base_addr;
	u16 csr0;
	bool set_csr0_iena = true;

	spin_lock(&priv->lock);

	csr0 = READ_CSR(CSR0);
	while ( csr0 & CSR0_INTR ) {
		/* подтвердить обработку прерывания и запретить прерывания от карты */
		WRITE_CSR(CSR0, csr0 & ~CSR0_IENA); // биты INIT и START уже установлены, потому они не повлияют

		if ( ( csr0 & CSR0_TINT ) || ( csr0 & CSR0_RINT ) ) {
			tasklet_schedule(&priv->rx_tx_task);
			set_csr0_iena = false;
		}

		// TODO: проверка на ошибки CSR0_BABL
		if ( csr0 & CSR0_CERR )
			dev->stats.collisions++;

		if ( csr0 & CSR0_MISS ) { /* потерян входящиц пакет (rx_ring заполнен?) */
			dev->stats.rx_errors++;
			dev->stats.rx_missed_errors++;
		}

		csr0 = READ_CSR(CSR0);
	}

	if ( set_csr0_iena ) {	/* если не запланировано тасклетов */
		WRITE_CSR(CSR0, CSR0_IENA);
	}

	spin_unlock(&priv->lock);

	return IRQ_RETVAL( IRQ_HANDLED );

}

int __init pcnet_init() {

	printk(KERN_INFO "%s\n", DRV_DESCRIPTION);

	return pci_register_driver(&pcnet_pci_driver);

}

void __exit pcnet_exit() {

	pci_unregister_driver(&pcnet_pci_driver);

}

module_init(pcnet_init);
module_exit(pcnet_exit);

