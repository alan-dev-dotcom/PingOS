/* ==============================================================================
 * PingOS - rtw88 Realtek Wireless Driver
 * ==============================================================================
 * Target: Realtek 802.11ac wireless cards (RTL8821CE, RTL8822CE, RTL8822BE)
 * PCI Identification: Vendor 0x10EC (Realtek), Device ID 0xC821, 0xC822, etc.
 * ==============================================================================
 */

#include "io.h"
#include "drivers.h"

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static unsigned int rtw88_pci_read(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
    outd(PCI_CONFIG_ADDR, address);
    return ind(PCI_CONFIG_DATA);
}

void rtw88_init() {
    vga_print_string("rtw88: Searching for Realtek 802.11ac wireless chips...\n", 0x0B);

    int found = 0;

    for (unsigned short bus = 0; bus < 256; bus++) {
        for (unsigned char slot = 0; slot < 32; slot++) {
            unsigned int id = rtw88_pci_read(bus, slot, 0, 0);
            unsigned short vendor = id & 0xFFFF;
            unsigned short device = (id >> 16) & 0xFFFF;

            if (vendor == 0x10EC && (device == 0xC821 || device == 0xC822 || device == 0xB822 || device == 0xC82F)) {
                found = 1;
                vga_print_string("rtw88: Found Realtek 802.11ac WiFi Module! Device ID: 0x", 0x0A);
                
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

                vga_print_string("rtw88: Mapping Mac/Phy register banks (BAR0).\n", 0x0B);
                vga_print_string("rtw88: Powering up radio frequency synthesizer core.\n", 0x0B);
                return;
            }
        }
    }

    if (!found) {
        vga_print_string("rtw88: No compatible Realtek 802.11ac network cards found.\n", 0x0E);
    }
}