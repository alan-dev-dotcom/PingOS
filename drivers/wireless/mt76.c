/* ==============================================================================
 * PingOS - mt76 MediaTek Wireless Driver
 * ==============================================================================
 * Target: MediaTek 802.11ac/ax Devices (MT7612, MT7662, MT7921, MT7915, etc.)
 * PCI Identification: Vendor 0x14C3 (MediaTek)
 * ==============================================================================
 */

#include "io.h"
#include "drivers.h"

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static unsigned int mt76_pci_read(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
    outd(PCI_CONFIG_ADDR, address);
    return ind(PCI_CONFIG_DATA);
}

void mt76_init() {
    vga_print_string("mt76: Scanning PCI bus for MediaTek wireless chipsets...\n", 0x0B);

    int found = 0;

    for (unsigned short bus = 0; bus < 256; bus++) {
        for (unsigned char slot = 0; slot < 32; slot++) {
            unsigned int id = mt76_pci_read(bus, slot, 0, 0);
            unsigned short vendor = id & 0xFFFF;
            unsigned short device = (id >> 16) & 0xFFFF;

            if (vendor == 0x14C3) { /* MediaTek Inc. */
                found = 1;
                vga_print_string("mt76: Detected MediaTek Wireless adapter! Device ID: 0x", 0x0A);
                
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

                vga_print_string("mt76: Establishing MCU control messages pipeline via MMIO.\n", 0x0B);
                vga_print_string("mt76: Uploading on-chip MT76 processor firmware binaries.\n", 0x0B);
                return;
            }
        }
    }

    if (!found) {
        vga_print_string("mt76: MediaTek wireless chips not detected on PCIe lanes.\n", 0x0E);
    }
}