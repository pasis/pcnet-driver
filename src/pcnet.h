/* pcnet.h: PCNet-PCI II/III Ethernet controller driver */
/*
 * Authors:
 *		Dmitry Podgorny <pasis.ua@gmail.com>
 *		Denis Kirjanov <kirjanov@gmail.com>
 */

enum {
	PCNET_IOSIZE_LEN = 0x20,
};

enum {
	PCNET_APROM = 0x00,
	PCNET_RDP16 = 0x10,
	PCNET_RDP = 0x10,
	PCNET_RAP16 = 0x12,
	PCNET_RAP = 0x14,
	PCNET_RESET16 = 0x14,
	PCNET_RESET = 0x18,
	PCNET_BDP = 0x1C,
};

enum {
	CSR0 = 0,
	CSR0_STOP = 0x0004,
};

enum {
	BCR18 = 18,
	BCR18_DWIO = 0x0080,
};
