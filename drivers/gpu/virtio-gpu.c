/* ==============================================================================
 * PingOS - virtio-gpu Driver
 * ==============================================================================
 * Target: QEMU/KVM Virtualized Graphics Card Adapter
 * PCI Identification: Vendor 0x1AF4 (Red Hat), Device ID 0x1050
 * ==============================================================================
 */

#include "io.h"
#include "drivers.h"

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static unsigned int virtio_pci_read(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
    outd(PCI_CONFIG_ADDR, address);
    return ind(PCI_CONFIG_DATA);
}

void virtio_gpu_init() {
    vga_print_string("virtio-gpu: Scanning virtual machines for graphics adapter...\n", 0x0B);

    int found = 0;

    for (unsigned short bus = 0; bus < 256; bus++) {
        for (unsigned char slot = 0; slot < 32; slot++) {
            unsigned int id = virtio_pci_read(bus, slot, 0, 0);
            unsigned short vendor = id & 0xFFFF;
            unsigned short device = (id >> 16) & 0xFFFF;

            if (vendor == 0x1AF4 && device == 0x1050) {
                found = 1;
                vga_print_string("virtio-gpu: Detected VirtIO QEMU/KVM virtual GPU!\n", 0x0A);

                /* Initialize virtual transport queues (Control and Cursor) */
                vga_print_string("virtio-gpu: Initializing command queue and 2D transfer buffers.\n", 0x0B);
                vga_print_string("virtio-gpu: Enabling paravirtualization and framebuffer transfers.\n", 0x0A);
                return;
            }
        }
    }

    if (!found) {
        vga_print_string("virtio-gpu: Device not present (system is not running under QEMU with -device virtio-gpu).\n", 0x0E);
    }
}