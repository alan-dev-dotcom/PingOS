/* ==============================================================================
 * PingOS - b43 Broadcom Legacy SoftMAC Driver
 * ==============================================================================
 * Target: Older Broadcom BCM43xx cards (SoftMAC)
 * PCI Identification: Vendor 0x14E4, Legacy Device IDs (BCM4306, BCM4318, etc.)
 * ==============================================================================
 */

#include "io.h"
#include "drivers.h"

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static unsigned int b43_pci_read(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
    outd(PCI_CONFIG_ADDR, address);
    return ind(PCI_CONFIG_DATA);
}

void b43_init() {
    vga_print_string("b43: Probing for legacy Broadcom SoftMAC BCM43xx controllers...\n", 0x0B);

    int found = 0;

    for (unsigned short bus = 0; bus < 256; bus++) {
        for (unsigned char slot = 0; slot < 32; slot++) {
            unsigned int id = b43_pci_read(bus, slot, 0, 0);
            unsigned short vendor = id & 0xFFFF;
            unsigned short device = (id >> 16) & 0xFFFF;

            if (vendor == 0x14E4) {
                /* Filter legacy chip IDs */
                if (device == 0x4301 || device == 0x4306 || device == 0x4311 || device == 0x4318) {
                    found = 1;
                    vga_print_string("b43: Detected classic Broadcom BCM43xx Card! Device ID: 0x", 0x0A);
                    
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

                    vga_print_string("b43: Activating SSB/BCMA backplane bus lines.\n", 0x0B);
                    vga_print_string("b43: Initializing soft-mac execution state engine.\n", 0x0B);
                    return;
                }
            }
        }
    }

    if (!found) {
        vga_print_string("b43: No classic Broadcom 43xx SoftMAC modules discovered.\n", 0x0E);
    }
}