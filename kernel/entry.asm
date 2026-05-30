; ==============================================================================
; PingOS Kernel Entry Point Stub (with Multiboot Header)
; ==============================================================================

; Multiboot macros
MBOOT_PAGE_ALIGN    equ 1 << 0
MBOOT_MEM_INFO      equ 1 << 1
MBOOT_HEADER_MAGIC  equ 0x1BADB002
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

section .multiboot
align 4
    dd MBOOT_HEADER_MAGIC
    dd MBOOT_HEADER_FLAGS
    dd MBOOT_CHECKSUM

section .text
global _start
extern main

_start:
    cli                 ; Clear interrupts
    call main           ; Jump to our C main() function in kernel/kernel.c
    
.halt_loop:
    hlt                 ; Halt the CPU if main returns
    jmp .halt_loop