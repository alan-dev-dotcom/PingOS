# PingOS

PingOS is a freestanding, bare-metal 32-bit x86 operating system built from scratch. Developed entirely as a hobbyist learning project, it runs in protected mode and showcases direct hardware communication, cooperative multitasking, custom memory management, an in-memory VFS, and an extensive suite of low-level hardware drivers.

# Why No Makefile? Introducing Spartan

PingOS replaces traditional complex Makefiles with Spartan (spartan.sh), a unified command-line build manager written entirely in Bash.

Spartan automates low-level assembly, handles source file organization, manages individual object linking, verifies multiboot compliance, creates the GRUB file hierarchy, and builds bootable ISO images.

# Fallback mechanism

Spartan features an intelligent self-healing driver compilation pipeline. If any graphics, wireless, or network driver file is missing or skipped during development, Spartan dynamically generates a clean, safe C stub on the fly. This guarantees that your kernel links successfully and boots into an ISO without ever throwing an "undefined reference" compilation error.

# Building & Running PingOS

Entering and operating the Spartan environment is straightforward.

# 1. Launching the Spartan CLI

Navigate to your project directory on your host computer and run:

bash spartan.sh


# 2. Available Spartan Commands

Once inside the spartan prompt, you can use the following pipeline commands:

Build the bootable ISO:

spartan> spartan build


Compiles the kernel assembly, compiles all nested active drivers (or generates safe fallback stubs), links everything into an ELF kernel.bin, and packages it into pingos.iso.

# Run the OS in QEMU:

spartan> spartan run


Bootstraps the built operating system inside the QEMU simulator (qemu-system-i386). It will automatically build the ISO first if it cannot be found in your folder.

# Clean the workspace:

spartan> spartan clean


Purges compiled objects, binary files, assembled stubs, and temporal staging directories to keep your repository pristine.

# OS Features & Driver Architecture

Core Kernel Engine

Segment Allocation: Custom Global Descriptor Table (GDT) defining flat memory segments.

Interrupt Handling: Custom Interrupt Descriptor Table (IDT) routing hardware interrupts (IRQs) and handling processor faults via a clean Kernel Panic visual block.

Multitasking: A cooperative round-robin scheduler managing individual Process Control Blocks (PCBs) and yielding time-slices safely.

Dynamic Allocations: A doubly-linked list first-fit heap allocator managing a dedicated 4MB memory region.

RAM-Based Disk File System: PingFS, a hierarchical in-memory VFS supporting multi-block files, directories, path navigation, and dynamic size re-calculations.

# Hardware Drivers

VGA Text Console: Direct-to-video memory implementation mapping character glyphs and color styling to address 0xB8000. Handles screen scrolling and programs VGA registers 0x3D4/0x3D5 to update the blinking hardware text cursor.

Keyboard & Mouse: Classic Intel 8042 controller driver reading from raw CPU I/O ports 0x60/0x64. Supports shifts, caps lock, backspaces, and mouse button/coordinate tracking.

Direct Rendering Manager (DRM/KMS): A software-defined graphics mode setting system for Bochs Graphic Adapters (BGA). Simulates high-resolution modes (e.g., 800x600 32-bit ARGB) by mapping the physical Linear Framebuffer (LFB).

Hardware PCI Probing: Performs low-level I/O scanning of physical PCIe buses (0xCF8/0xCFC) to identify network, audio, wireless, and companion host controllers.

# Contributing Guidelines

We welcome contributions to PingOS! To propose features, optimize drivers, or improve the multitasking scheduler, please follow this workflow:

Fork the repository on GitHub.

# Create a clean, descriptive feature branch:

git checkout -b feature/AmazingFeature


# Commit your modifications with clear, concise messages:

git commit -m 'Implement hypervisor-optimized network buffer queues'


# Push the branch to your GitHub fork:

git push origin feature/AmazingFeature


# Open a Pull Request for review and community testing.

# License

This project is made as a hobbyist project under a copyleft open-source license. (GPL-3.0)
