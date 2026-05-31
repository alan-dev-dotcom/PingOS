# PingOS

PingOS is a freestanding 32-bit x86 operating system developed as a hobby project. 

# Compile PingOS from source code

Clone the source code of PingOS.

Run "make spartan"

After running it, run "spartan build"

This will output the iso file.

If you want to remove the compiled files quickly, then run "spartan clean".

To exit the spartan script, run "exit".

# Project Overview

# Features

Video Driver: Direct-to-video memory implementation (address 0xB8000) supporting scroll, clear, and hardware cursor positioning.

Keyboard Driver: Support for ASCII translation, tracking of modifier states (Shift, Caps Lock), and backspace handling.

Mouse Driver: Basic controller initialization and packet decoding for PS/2 mouse coordinates and buttons.

PCI Bus Probe Drivers:

USB host controller scanning for UHCI, OHCI, EHCI, or xHCI controllers.

Intel Integrated GPU detector to locate display devices on the PCI bus and enable command registers.

Command Line Interface: Standard shell with support for basic commands including:

# Emulation

Run the bootable ISO using QEMU:

qemu-system-i386 -cdrom pingos.iso


# Contributing

Fork the project repository on GitHub.

Create a new feature branch (git checkout -b feature/AmazingFeature).

Commit your changes with clear messages (git commit -m 'Add support for feature').

Push the branch to your fork (git push origin feature/AmazingFeature).

Open a Pull Request for review.
