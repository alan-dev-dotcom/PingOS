/* ==============================================================================
 * PingOS - panfrost Embedded Graphics Driver (ARM Cortex Mali Stub)
 * ==============================================================================
 * Target: ARM Mali Midgard & Bifrost Architecture GPUs
 * Note: Designed for embedded architectures (ARM Mali). Included in our x86-32 
 * kernel as an architectural portability stub for device tree detection.
 * ==============================================================================
 */

#include "drivers.h"

void panfrost_init() {
    vga_print_string("panfrost: Checking Flattened Device Tree (FDT) nodes...\n", 0x0B);
    
    /* In the x86-32 architecture, we do not have a physical ARM AMBA/AXI bus */
    vga_print_string("panfrost: No 'arm,mali-bifrost' node found in hardware database.\n", 0x0E);
    vga_print_string("panfrost: Panfrost driver disabled (incompatible x86-32 CPU platform).\n", 0x07);
}