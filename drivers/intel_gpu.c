/* drivers/intel_gpu.c - Intel Integrated GPU PCI driver for PingOS */
#include "io.h"
#include "drivers.h"

// PCI Configuration Ports
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

/* PCI config space reading helper */
static unsigned int pci_read_config(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
    outd(PCI_CONFIG_ADDRESS, address);
    return ind(PCI_CONFIG_DATA);
}

/* PCI config space writing helper */
static void pci_write_config(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset, unsigned int data) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
    outd(PCI_CONFIG_ADDRESS, address);
    outd(PCI_CONFIG_DATA, data);
}

/* Intel Graphics Driver (Intel GPU) Initialization and PCI detector */
void intel_gpu_init() {
    vga_print_string("Intel GPU: Scanning motherboard PCI Bus...\n", 0x0B);

    // Scan PCI bus to find the Intel display controller (Vendor ID 0x8086)
    for (unsigned short bus = 0; bus < 256; bus++) {
        for (unsigned char slot = 0; slot < 32; slot++) {
            unsigned int id = pci_read_config(bus, slot, 0, 0);
            unsigned short vendor = id & 0xFFFF;
            unsigned short device = (id >> 16) & 0xFFFF;

            if (vendor == 0x8086) { // Found Intel Device!
                // Read Class Code at offset 0x08
                unsigned int class_info = pci_read_config(bus, slot, 0, 0x08);
                unsigned char base_class = (class_info >> 24) & 0xFF;
                unsigned char sub_class = (class_info >> 16) & 0xFF;

                // Class 0x03 is Display Controller (VGA, SVGA or modern high-performance GPU)
                if (base_class == 0x03) {
                    vga_print_string("Intel GPU: Found Intel Display device on PCI Bus!\n", 0x0B);
                    vga_print_string("Intel GPU: Setting up frame-buffer command registries...\n", 0x0B);

                    // Read Command register at offset 0x04
                    unsigned int command = pci_read_config(bus, slot, 0, 0x04);
                    
                    // Enable Bus Mastering (Bit 2), Memory Space (Bit 1), and I/O Space (Bit 0)
                    command |= (1 << 2) | (1 << 1) | (1 << 0);
                    pci_write_config(bus, slot, 0, 0x04, command);

                    vga_print_string("Intel GPU: Graphics driver successfully loaded.\n", 0x0A);
                    return;
                }
            }
        }
    }
    vga_print_string("Intel GPU: No Intel Integrated GPU found. Defaulting to legacy VESA VGA mode.\n", 0x0E);
}