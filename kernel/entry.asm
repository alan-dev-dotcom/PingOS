; ==============================================================================
; kernel/entry.asm - Assembly entry point stub for PingOS
; ==============================================================================
[bits 32]
[extern main]       ; Let NASM know that 'main' is defined in our C file

global _start       ; Define the entry point symbol for the linker

_start:
    call main       ; Call our C kernel's main() function
    jmp $           ; If main somehow returns, loop infinitely to safely halt