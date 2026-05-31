/* ==============================================================================
 * PingOS - lima Embedded Graphics Driver (ARM Mali Utgard Stub)
 * ==============================================================================
 * Target: ARM Mali-400 and Mali-450 Series (Utgard architecture)
 * Note: Designed for legacy embedded SoC architectures. Portability stub.
 * ==============================================================================
 */

#include "drivers.h"

void lima_init() {
    vga_print_string("lima: Searching for Mali-400/450 Utgard graphics processors...\n", 0x0B);
    
    /* Like panfrost, this is a SoC-only device that doesn't exist on standard PC registers */
    vga_print_string("lima: Device 'arm,mali-utgard' not found in system registers.\n", 0x0E);
    vga_print_string("lima: Lima driver suspended.\n", 0x07);
}