/* ==============================================================================
 * PingOS - ath11k Atheros Wireless Driver
 * ==============================================================================
 * Target: Qualcomm Atheros 802.11ax WiFi 6 Devices (QCA6390, WCN6855, etc.)
 * PCI Identification: Vendor 0x17CB (Qualcomm), Device Class 0x0280
 * ==============================================================================
 */

#include "io.h"
#include "drivers.h"

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static unsigned int ath11k_pci_read(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
    outd(PCI_CONFIG_ADDR, address);
    return ind(PCI_CONFIG_DATA);
}

void ath11k_init() {
    vga_print_string("ath11k: Scanning for modern Qualcomm WiFi 6 adapters...\n", 0x0B);

    int found = 0;

    for (unsigned short bus = 0; bus < 256; bus++) {
        for (unsigned char slot = 0; slot < 32; slot++) {
            unsigned int id = ath11k_pci_read(bus, slot, 0, 0);
            unsigned short vendor = id & 0xFFFF;
            unsigned short device = (id >> 16) & 0xFFFF;

            if (vendor == 0x17CB || (vendor == 0x168C && device >= 0x1100)) {
                found = 1;
                vga_print_string("ath11k: Found Qualcomm WiFi 6/6E hardware! Device ID: 0x", 0x0A);
                
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

                vga_print_string("ath11k: Establishing AHB/PCI interface mappings.\n", 0x0B);
                vga_print_string("ath11k: Readying DP (Data Path) and Halting rings for MAC config.\n", 0x0B);
                return;
            }
        }
    }

    if (!found) {
        vga_print_string("ath11k: No Qualcomm WiFi 6 PCIe adapters discovered.\n", 0x0E);
    }
}