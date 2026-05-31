/* drivers/usb.c - USB Host Controller PCI driver for PingOS */
#include "io.h"
#include "drivers.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

/* Helper function to scan PCI configurations */
static unsigned int usb_pci_read(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
    outd(PCI_CONFIG_ADDRESS, address);
    return ind(PCI_CONFIG_DATA);
}

/* USB Driver - Scans PCI for USB Host Controllers (UHCI, EHCI, xHCI) */
void usb_init() {
    vga_print_string("USB Bus: Scanning PCI bus for active USB Host Controllers...\n", 0x0B);

    int controllers_found = 0;

    for (unsigned short bus = 0; bus < 256; bus++) {
        for (unsigned char slot = 0; slot < 32; slot++) {
            for (unsigned char func = 0; func < 8; func++) {
                unsigned int id = usb_pci_read(bus, slot, func, 0);
                if ((id & 0xFFFF) == 0xFFFF || (id & 0xFFFF) == 0x0000) continue;

                // Read Class info register
                unsigned int class_info = usb_pci_read(bus, slot, func, 0x08);
                unsigned char base_class = (class_info >> 24) & 0xFF;
                unsigned char sub_class = (class_info >> 16) & 0xFF;
                unsigned char interface  = (class_info >> 8) & 0xFF;

                // Base Class 0x0C: Serial Bus Controller, Sub Class 0x03: USB Controller
                if (base_class == 0x0C && sub_class == 0x03) {
                    controllers_found++;
                    vga_print_string("USB Bus: Found Host Controller. Interface Type: ", 0x0B);

                    if (interface == 0x00) {
                        vga_print_string("UHCI (USB 1.1 Standard-speed)\n", 0x0F);
                    } else if (interface == 0x10) {
                        vga_print_string("OHCI (USB 1.1 Full-speed)\n", 0x0F);
                    } else if (interface == 0x20) {
                        vga_print_string("EHCI (USB 2.0 High-speed)\n", 0x0F);
                    } else if (interface == 0x30) {
                        vga_print_string("xHCI (USB 3.0 Super-speed)\n", 0x0F);
                    } else {
                        vga_print_string("Unknown interface class\n", 0x0E);
                    }
                }
            }
        }
    }

    if (controllers_found == 0) {
        vga_print_string("USB Bus: No USB Controllers detected on motherboard interfaces.\n", 0x0E);
    } else {
        vga_print_string("USB Bus: Initialized USB host routing interface channels.\n", 0x0A);
    }
}