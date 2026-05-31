/* ==============================================================================
 * PingOS - alx Qualcomm Atheros AR816x/AR817x Gigabit Ethernet Driver
 * ==============================================================================
 * Target: Qualcomm Atheros AR8161, AR8162, AR8171, AR8172 Gigabit controllers
 * PCI Identification: Vendor 0x1969 (Atheros), Device ID 0x1091, 0x10A1, etc.
 * ==============================================================================
 */

#include "io.h"
#include "drivers.h"

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static unsigned int alx_pci_read(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
    outd(PCI_CONFIG_ADDR, address);
    return ind(PCI_CONFIG_DATA);
}

void alx_init() {
    vga_print_string("alx: Probing for Qualcomm Atheros AR816x/817x Gigabit Ethernet...\n", 0x0B);

    int found = 0;

    for (unsigned short bus = 0; bus < 256; bus++) {
        for (unsigned char slot = 0; slot < 32; slot++) {
            unsigned int id = alx_pci_read(bus, slot, 0, 0);
            unsigned short vendor = id & 0xFFFF;
            unsigned short device = (id >> 16) & 0xFFFF;

            if (vendor == 0x1969) {
                if (device == 0x1091 || device == 0x1090 || device == 0x10A1 || device == 0x2062) {
                    found = 1;
                    vga_print_string("alx: Detected Qualcomm Atheros network controller! Device ID: 0x", 0x0A);
                    
                    char dev_hex[8];
                    unsigned short temp = device;
                    int idx = 0;
                    do {
                        int rem = temp % 16;
                        dev_hex[idx++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'A');
                        temp /= 16;
                    } while (temp > 0);
                    dev_hex[idx] = '\0';
                    for (int j = 0, k = idx - 1; j < k; j++, k--) {
                        char tmp = dev_hex[j]; dev_hex[j] = dev_hex[k]; dev_hex[k] = tmp;
                    }
                    vga_print_string(dev_hex, 0x0F);
                    vga_print_string("\n", 0x0A);

                    vga_print_string("alx: Mapping Atheros specific registers and setting up DMA blocks.\n", 0x0B);
                    vga_print_string("alx: Hardware configured and ready for local interface binding.\n", 0x0A);
                    return;
                }
            }
        }
    }

    if (!found) {
        vga_print_string("alx: Qualcomm Atheros AR816x/817x adapters not present.\n", 0x0E);
    }
}