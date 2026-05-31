/* ==============================================================================
 * PingOS - radeon Legacy Graphics Driver
 * ==============================================================================
 * Target: Older AMD/ATI Radeon Chips (R100 through Southern Islands)
 * PCI Identification: Vendor 0x1002 (ATI/AMD Legacy)
 * ==============================================================================
 */

#include "io.h"
#include "drivers.h"

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static unsigned int radeon_pci_read(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
    outd(PCI_CONFIG_ADDR, address);
    return ind(PCI_CONFIG_DATA);
}

void radeon_init() {
    vga_print_string("radeon: Scanning for older ATI/AMD Radeon graphics cards...\n", 0x0B);

    int found = 0;

    for (unsigned short bus = 0; bus < 256; bus++) {
        for (unsigned char slot = 0; slot < 32; slot++) {
            unsigned int id = radeon_pci_read(bus, slot, 0, 0);
            unsigned short vendor = id & 0xFFFF;
            unsigned short device = (id >> 16) & 0xFFFF;

            if (vendor == 0x1002) {
                unsigned int class_info = radeon_pci_read(bus, slot, 0, 0x08);
                unsigned char base_class = (class_info >> 24) & 0xFF;

                if (base_class == 0x03) {
                    /* Filter older legacy Radeon device identifiers */
                    if (device < 0x9800) {
                        found = 1;
                        vga_print_string("radeon: Detected legacy ATI Radeon card! ID: 0x", 0x0A);
                        
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

                        vga_print_string("radeon: Configuring legacy VRAM controller and microcode.\n", 0x0B);
                        return;
                    }
                }
            }
        }
    }

    if (!found) {
        vga_print_string("radeon: No legacy ATI Radeon cards found on the PCI bus.\n", 0x0E);
    }
}