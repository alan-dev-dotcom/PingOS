/* ==============================================================================
 * Linker script for PingKernel
 * Instructs dynamic output organization of the machine code sections
 * ==============================================================================
 */

ENTRY(main)

SECTIONS
{
    /* Since our bootloader loads the kernel payload into physical address 0x10000 (1000:0000) */
    . = 0x10000;

    .text : ALIGN(4096)
    {
        *(.text)
    }

    .rodata : ALIGN(4096)
    {
        *(.rodata)
    }

    .data : ALIGN(4096)
    {
        *(.data)
    }

    .bss : ALIGN(4096)
    {
        *(.bss)
        *(COMMON)
    }
}
