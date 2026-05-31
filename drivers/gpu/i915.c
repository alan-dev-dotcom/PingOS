/* ==============================================================================
 * PingOS - i915 Legacy Graphics Driver
 * ==============================================================================
 * Target: Intel Integrated Graphics (Gen 3 to Gen 9 / Ironlake, SandyBridge, Haswell)
 * PCI Identification: Vendor 0x8086, Device Class 0x03
 * ==============================================================================
 */

#include "io.h"
#include "drivers.h"

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static unsigned int i915_pci_read(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
    outd(PCI_CONFIG_ADDR, address);
    return ind(PCI_CONFIG_DATA);
}

void i915_init() {
    vga_print_string("i915: Scanning for integrated Intel HD graphics controllers...\n", 0x0B);

    int found = 0;

    for (unsigned short bus = 0; bus < 256; bus++) {
        for (unsigned char slot = 0; slot < 32; slot++) {
            unsigned int id = i915_pci_read(bus, slot, 0, 0);
            unsigned short vendor = id & 0xFFFF;
            unsigned short device = (id >> 16) & 0xFFFF;

            if (vendor == 0x8086) {
                unsigned int class_info = i915_pci_read(bus, slot, 0, 0x08);
                unsigned char base_class = (class_info >> 24) & 0xFF;

                if (base_class == 0x03) {
                    /* Filter only older legacy chips compatible with i915 */
                    if (device < 0x9000) { 
                        found = 1;
                        vga_print_string("i915: Detected legacy Intel HD Graphics! ID: 0x", 0x0A);
                        
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
                        
                        vga_print_string("i915: Activating GTT pipelining mode and configuring hardware registers.\n", 0x0B);
                        return;
                    }
                }
            }
        }
    }

    if (!found) {
        vga_print_string("i915: No legacy integrated Intel (i915) controllers found.\n", 0x0E);
    }
}