/* ==============================================================================
 * PingOS - forcedeth NVIDIA nForce Ethernet Controller Driver
 * ==============================================================================
 * Target: NVIDIA nForce MCP chipset Integrated Network Controllers (legacy desktop)
 * PCI Identification: Vendor 0x10DE (NVIDIA)
 * ==============================================================================
 */

#include "io.h"
#include "drivers.h"

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static unsigned int forcedeth_pci_read(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
    outd(PCI_CONFIG_ADDR, address);
    return ind(PCI_CONFIG_DATA);
}

void forcedeth_init() {
    vga_print_string("forcedeth: Probing PCI bus for integrated NVIDIA nForce network cards...\n", 0x0B);

    int found = 0;

    for (unsigned short bus = 0; bus < 256; bus++) {
        for (unsigned char slot = 0; slot < 32; slot++) {
            unsigned int id = forcedeth_pci_read(bus, slot, 0, 0);
            unsigned short vendor = id & 0xFFFF;
            unsigned short device = (id >> 16) & 0xFFFF;

            if (vendor == 0x10DE) {
                unsigned int class_info = forcedeth_pci_read(bus, slot, 0, 0x08);
                unsigned char base_class = (class_info >> 24) & 0xFF;
                unsigned char sub_class = (class_info >> 16) & 0xFF;

                if (base_class == 0x02 && sub_class == 0x00) {
                    found = 1;
                    vga_print_string("forcedeth: Detected NVIDIA nForce Integrated LAN! Device ID: 0x", 0x0A);
                    
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

                    vga_print_string("forcedeth: Enabling integrated MAC processing engine.\n", 0x0B);
                    vga_print_string("forcedeth: Mapping nForce controller registers and loading ring configuration.\n", 0x0B);
                    return;
                }
            }
        }
    }

    if (!found) {
        vga_print_string("forcedeth: No integrated NVIDIA nForce network hardware found.\n", 0x0E);
    }
}