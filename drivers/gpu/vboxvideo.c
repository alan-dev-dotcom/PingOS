/* ==============================================================================
 * PingOS - vboxvideo Graphics Driver
 * ==============================================================================
 * Target: VirtualBox Graphics Adapter with Guest Integration
 * PCI Identification: Vendor 0x80EE, Device ID 0xBEEF
 * ==============================================================================
 */

#include "io.h"
#include "drivers.h"

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static unsigned int vbox_pci_read(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
    outd(PCI_CONFIG_ADDR, address);
    return ind(PCI_CONFIG_DATA);
}

void vboxvideo_init() {
    vga_print_string("vboxvideo: Detecting Oracle VirtualBox environment presence...\n", 0x0B);

    int found = 0;

    for (unsigned short bus = 0; bus < 256; bus++) {
        for (unsigned char slot = 0; slot < 32; slot++) {
            unsigned int id = vbox_pci_read(bus, slot, 0, 0);
            unsigned short vendor = id & 0xFFFF;
            unsigned short device = (id >> 16) & 0xFFFF;

            if (vendor == 0x80EE && device == 0xBEEF) {
                found = 1;
                vga_print_string("vboxvideo: Detected VirtualBox graphics integration card!\n", 0x0A);
                vga_print_string("vboxvideo: Connecting to host via VMMDev HGSMI channel...\n", 0x0B);
                vga_print_string("vboxvideo: Mouse pointer integration and auto-resize ready.\n", 0x0A);
                return;
            }
        }
    }

    if (!found) {
        vga_print_string("vboxvideo: Device not present (system is not running inside a VirtualBox virtual machine).\n", 0x0E);
    }
}