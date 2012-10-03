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
	PCNET_RDP = 0x10,
	PCNET_RAP = 0x14,
	PCNET_RESET = 0x18,
	PCNET_BDP = 0x1C,
};
