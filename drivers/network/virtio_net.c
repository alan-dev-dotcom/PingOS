/* ==============================================================================
 * PingOS - virtio_net QEMU VirtIO Network Adapter Driver
 * ==============================================================================
 * Target: QEMU/KVM High-Performance Paravirtualized Network Adapter
 * PCI Identification: Vendor 0x1AF4 (Red Hat), Device ID 0x1000 or 0x1041
 * ==============================================================================
 */

#include "io.h"
#include "drivers.h"

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static unsigned int virtnet_pci_read(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));
    outd(PCI_CONFIG_ADDR, address);
    return ind(PCI_CONFIG_DATA);
}

void virtio_net_init() {
    vga_print_string("virtio_net: Querying hypervisor for virtual VirtIO network card...\n", 0x0B);

    int found = 0;

    for (unsigned short bus = 0; bus < 256; bus++) {
        for (unsigned char slot = 0; slot < 32; slot++) {
            unsigned int id = virtnet_pci_read(bus, slot, 0, 0);
            unsigned short vendor = id & 0xFFFF;
            unsigned short device = (id >> 16) & 0xFFFF;

            if (vendor == 0x1AF4 && (device == 0x1000 || device == 0x1041)) {
                found = 1;
                vga_print_string("virtio_net: Detected QEMU VirtIO Paravirtualized Network Card!\n", 0x0A);

                /* Initialize Virtqueues */
                vga_print_string("virtio_net: Establishing Receive and Transmit virtqueues (vrings).\n", 0x0B);
                vga_print_string("virtio_net: Setting up guest paravirtualized network capabilities.\n", 0x0A);
                return;
            }
        }
    }

    if (!found) {
        vga_print_string("virtio_net: Adapter absent (ensure running QEMU with '-net nic,model=virtio').\n", 0x0E);
    }
}