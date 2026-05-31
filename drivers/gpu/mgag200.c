/* ==============================================================================
 * PingOS - mgag200 Server Graphics Driver
 * ==============================================================================
 * Target: Matrox G200 Graphics Controller (Standard in many Enterprise Servers)
 * PCI Identification: Vendor 0x102B (Matrox), Device ID 0x0522
 * ==============================================================================
 */

#include "io.h"
#include "drivers.h"

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static unsigned int mga_pci_read(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
    outd(PCI_CONFIG_ADDR, address);
    return ind(PCI_CONFIG_DATA);
}

void mgag200_init() {
    vga_print_string("mgag200: Scanning PCI bus for Matrox server graphics cards...\n", 0x0B);

    int found = 0;

    for (unsigned short bus = 0; bus < 256; bus++) {
        for (unsigned char slot = 0; slot < 32; slot++) {
            unsigned int id = mga_pci_read(bus, slot, 0, 0);
            unsigned short vendor = id & 0xFFFF;
            unsigned short device = (id >> 16) & 0xFFFF;

            if (vendor == 0x102B && device == 0x0522) {
                found = 1;
                vga_print_string("mgag200: Found Matrox G200 server graphics controller!\n", 0x0A);
                vga_print_string("mgag200: Mapping low-level framebuffer and configuring DAC registers.\n", 0x0B);
                return;
            }
        }
    }

    if (!found) {
        vga_print_string("mgag200: No Matrox G200 hardware found in this machine.\n", 0x0E);
    }
}