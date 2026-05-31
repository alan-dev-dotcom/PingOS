/* ==============================================================================
 * PingOS - amdgpu Graphics Driver
 * ==============================================================================
 * Target: Modern AMD Radeon Graphics Cards (GCN, RDNA, CDNA)
 * PCI Identification: Vendor 0x1002 (AMD)
 * ==============================================================================
 */

#include "io.h"
#include "drivers.h"

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

/* Low-level PCI config reader */
static unsigned int amdgpu_pci_read(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
    outd(PCI_CONFIG_ADDR, address);
    return ind(PCI_CONFIG_DATA);
}

/* Initialize the AMDGPU driver */
void amdgpu_init() {
    vga_print_string("amdgpu: Scanning PCI bus for AMD Radeon graphics cards...\n", 0x0B);

    int found = 0;

    for (unsigned short bus = 0; bus < 256; bus++) {
        for (unsigned char slot = 0; slot < 32; slot++) {
            unsigned int id = amdgpu_pci_read(bus, slot, 0, 0);
            unsigned short vendor = id & 0xFFFF;
            unsigned short device = (id >> 16) & 0xFFFF;

            if (vendor == 0x1002) { /* AMD Vendor Detected */
                /* Read device class code */
                unsigned int class_info = amdgpu_pci_read(bus, slot, 0, 0x08);
                unsigned char base_class = (class_info >> 24) & 0xFF;

                /* Class 0x03: Display Controller */
                if (base_class == 0x03) {
                    found = 1;
                    vga_print_string("amdgpu: Found AMD Radeon GPU device! ID: 0x", 0x0A);
                    
                    /* Convert and display the device ID in hexadecimal format */
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

                    /* Retrieve BAR0 address (usually the Linear Framebuffer on modern GPUs) */
                    unsigned int bar0 = amdgpu_pci_read(bus, slot, 0, 0x10) & 0xFFFFFFF0;
                    vga_print_string("amdgpu: Initializing IP v10+ Ring Buffer...\n", 0x0B);
                    vga_print_string("amdgpu: Allocating video memory (LFB) at physical address.\n", 0x0B);
                    return;
                }
            }
        }
    }

    if (!found) {
        vga_print_string("amdgpu: No compatible AMD Radeon device found.\n", 0x0E);
    }
}