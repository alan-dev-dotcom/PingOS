ENTRY(_start)

SECTIONS
{
    /* GRUB convention: Load the kernel at the 1MB physical address */
    . = 1M;

    .text BLOCK(4K) : ALIGN(4K)
    {
        /* Ensure the multiboot header goes at the absolute beginning */
        *(.multiboot)
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}