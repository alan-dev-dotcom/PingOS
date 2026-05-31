# PingOS

PingOS is a freestanding 32-bit x86 operating system developed as a hobby project. 

# Why no Makefile?

In PingOS, Makefile was replaced with my own script, Spartan.

# What is Spartan and how to use it?

When you clone or download the PingOS source code, you just need to run "bash spartan.sh" and you enter the Spartan commandline.

# How to build with Spartan?

Either use the commands from here or type "help" while being inside Spartan.

To build PingOS, type "spartan build"

To run PingOS in QEMU, type "spartan run"

To remove compiled files, type "spartan clean"

# Project Overview

# Features

Video Driver: Direct-to-video memory implementation (address 0xB8000) supporting scroll, clear, and hardware cursor positioning.

Keyboard Driver: Support for ASCII translation, tracking of modifier states (Shift, Caps Lock), and backspace handling.

Mouse Driver: Basic controller initialization and packet decoding for PS/2 mouse coordinates and buttons.

PCI Bus Probe Drivers:

USB host controller scanning for UHCI, OHCI, EHCI, or xHCI controllers.

Intel Integrated GPU detector to locate display devices on the PCI bus and enable command registers.

Command Line Interface: Standard shell with support for basic commands including:

# Contributing

Fork the project repository on GitHub.

Create a new feature branch (git checkout -b feature/AmazingFeature).

Commit your changes with clear messages (git commit -m 'Add support for feature').

Push the branch to your fork (git push origin feature/AmazingFeature).

Open a Pull Request for review.
