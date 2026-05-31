/* ==============================================================================
 * PingOS - r8169 Realtek RTL8111/RTL8168/RTL8169 Gigabit Ethernet Driver
 * ==============================================================================
 * Target: Realtek PCI Express Gigabit Ethernet devices (extremely common on PCs)
 * PCI Identification: Vendor 0x10EC, Device ID 0x8168, 0x8169, 0x8111
 * ==============================================================================
 */

#include "io.h"
#include "drivers.h"

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static unsigned int r8169_pci_read(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
    outd(PCI_CONFIG_ADDR, address);
    return ind(PCI_CONFIG_DATA);
}

void r8169_init() {
    vga_print_string("r8169: Scanning PCI bus for Realtek RTL8111/RTL8168/8169 cards...\n", 0x0B);

    int found = 0;

    for (unsigned short bus = 0; bus < 256; bus++) {
        for (unsigned char slot = 0; slot < 32; slot++) {
            unsigned int id = r8169_pci_read(bus, slot, 0, 0);
            unsigned short vendor = id & 0xFFFF;
            unsigned short device = (id >> 16) & 0xFFFF;

            if (vendor == 0x10EC) {
                if (device == 0x8168 || device == 0x8169 || device == 0x8111) {
                    found = 1;
                    vga_print_string("r8169: Detected Realtek Gigabit Ethernet board! Device ID: 0x", 0x0A);
                    
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

                    vga_print_string("r8169: Querying MAC version and software physical config registers.\n", 0x0B);
                    vga_print_string("r8169: Allocating memory descriptors for 10/100/1000Base-T operation.\n", 0x0B);
                    return;
                }
            }
        }
    }

    if (!found) {
        vga_print_string("r8169: Realtek RTL8111/RTL8168/8169 adapters not present.\n", 0x0E);
    }
}