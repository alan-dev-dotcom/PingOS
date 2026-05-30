PingOS

PingOS is a freestanding 32-bit x86 operating system developed as a hobby project. It is designed to demonstrate basic low-level system design concepts, including boot sector initialization, Protected Mode transitions, and simple hardware driver implementation.

Project Overview

The project initiates from a custom 16-bit real-mode bootloader that configures segments, switches the CPU to 32-bit protected mode, and executes the C kernel. The interface runs entirely in a monochrome text environment.

Features

Custom Bootloader: Written in NASM assembly, handles disk sector reading, sets up the Global Descriptor Table (GDT), and jumps into the 32-bit kernel entry point.

Monochrome Video Driver: Direct-to-video memory implementation (address 0xB8000) supporting scroll, clear, and hardware cursor positioning.

Keyboard Driver: Support for ASCII translation, tracking of modifier states (Shift, Caps Lock), and backspace handling.

Mouse Driver: Basic controller initialization and packet decoding for PS/2 mouse coordinates and buttons.

PCI Bus Probe Drivers:

USB host controller scanning for UHCI, OHCI, EHCI, or xHCI controllers.

Intel Integrated GPU detector to locate display devices on the PCI bus and enable command registers.

Command Line Interface: Standard shell with support for basic commands including:

help - Lists available commands.

info - Displays system parameters.

ping - Confirms kernel loop responsiveness.

clear - Clears the VGA display buffer.

uname - Prints the OS name.

uname -r - Prints the kernel release version.

devices - Scans and lists PCI controller nodes.

Directory Structure

├── boot
│   └── boot.asm        # 16-bit bootloader (loads kernel & enters 32-bit PM)
├── drivers
│   ├── intel_gpu.c     # Intel GPU scan helper via PCI configuration ports
│   ├── keyboard.c      # PS/2 keyboard translator & controller handler
│   ├── mouse.c         # PS/2 auxiliary mouse setup & polling packet decoder
│   ├── usb.c           # Serial Bus controller PCI interface detector
│   └── vga.c           # Direct screen memory buffer driver (address 0xB8000)
├── include
│   ├── drivers.h       # Prototypes and interface signatures for drivers
│   └── io.h            # Inline assembly port access wrappers (inb, outb, etc.)
├── kernel
│   ├── entry.asm       # 32-bit kernel startup bootstrap
│   └── kernel.c        # Principal main execution loop & command shell
├── linker.ld           # Linker instructions mapping the kernel entry point to 0x10000
└── Makefile            # Master build system sequence


Building and Running

Prerequisites

To compile and build this project on Debian-based or Ubuntu systems, you need gcc-multilib, nasm, and a system emulator like QEMU.

sudo apt update
sudo apt install build-essential nasm qemu-system-x86 xorriso


Compiling

Clean existing objects and compile the bootloader and kernel binaries:

make clean
make


Emulation

Run the bootable ISO using QEMU:

qemu-system-i386 -cdrom pingos.iso


Contributing

Fork the project repository on GitHub.

Create a new feature branch (git checkout -b feature/AmazingFeature).

Commit your changes with clear messages (git commit -m 'Add support for feature').

Push the branch to your fork (git push origin feature/AmazingFeature).

Open a Pull Request for review.
