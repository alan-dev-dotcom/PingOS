
# PingOS

PingOS is a freestanding, bare-metal 32-bit x86 operating system built from scratch. Developed entirely as a hobbyist learning project, it runs in protected mode and showcases direct hardware communication, cooperative multitasking, custom memory management, an in-memory VFS, and an extensive suite of low-level hardware drivers.

## Why No Makefile? Introducing Spartan

PingOS replaces traditional complex Makefiles with **Spartan** (`spartan.sh`), a unified command-line build manager written entirely in Bash.

Spartan automates low-level assembly, handles source file organization, manages individual object linking, verifies multiboot compliance, creates the GRUB file hierarchy, and builds bootable ISO images.

### Fallback Mechanism
If any graphics, wireless, or network driver file is missing or skipped during development, Spartan dynamically generates a clean, safe C stub on the fly. This guarantees that your kernel links successfully and boots into an ISO without ever throwing an "undefined reference" compilation error.

---

## Building & Running PingOS

### Prerequisites
Ensure you have the following tools installed on your host system:
* `gcc` (configured for i386 target or multilib)
* `nasm`
* `qemu-system-i386`
* `xorriso` (for grub-mkrescue)

### 1. Launching the Spartan CLI
Navigate to your project directory on your host computer and run:
```bash
bash spartan.sh
```

### 2. Available Spartan Commands
Once inside the Spartan prompt, you can use the following pipeline commands:

* **Build the bootable ISO:**
  ```bash
  spartan> spartan build
  ```
  Compiles the kernel assembly, compiles all nested active drivers (or generates safe fallback stubs), links everything into an ELF `kernel.bin`, and packages it into `pingos.iso`.

* **Run the OS in QEMU:**
  ```bash
  spartan> spartan run
  ```
  Bootstraps the built operating system inside the QEMU simulator (`qemu-system-i386`). It will automatically build the ISO first if it cannot be found in your folder.

* **Clean the workspace:**
  ```bash
  spartan> spartan clean
  ```
  Purges compiled objects, binary files, assembled stubs, and temporal staging directories to keep your repository pristine.

---

## OS Features & Driver Architecture

### Core Kernel Engine
* **Segment Allocation:** Custom Global Descriptor Table (GDT) defining flat memory segments.
* **Interrupt Handling:** Custom Interrupt Descriptor Table (IDT) routing hardware interrupts (IRQs) and handling processor faults via a clean Kernel Panic visual block.
* **Multitasking:** A cooperative round-robin scheduler managing individual Process Control Blocks (PCBs) and yielding time-slices safely.
* **Dynamic Allocations:** A doubly-linked list first-fit heap allocator managing a dedicated 4MB memory region.
* **RAM-Based Disk File System:** `PingFS`, a hierarchical in-memory VFS supporting multi-block files, directories, path navigation, and dynamic size re-calculations.

### Hardware Drivers
* **VGA Text Console:** Direct-to-video memory implementation mapping character glyphs and color styling to address `0xB8000`. Handles screen scrolling and programs VGA registers `0x3D4`/`0x3D5` to update the blinking hardware text cursor.
* **Keyboard & Mouse:** Classic Intel 8042 controller driver reading from raw CPU I/O ports `0x60`/`0x64`. Supports shifts, caps lock, backspaces, and mouse button/coordinate tracking.
* **Direct Rendering Manager (DRM/KMS):** A software-defined graphics mode setting system for Bochs Graphic Adapters (BGA). Simulates high-resolution modes (e.g., 800x600 32-bit ARGB) by mapping the physical Linear Framebuffer (LFB).
* **Hardware PCI Probing:** Performs low-level I/O scanning of physical PCIe buses (`0xCF8`/`0xCFC`) to identify network, audio, wireless, and companion host controllers.

---

## Contributing Guidelines

We welcome contributions to PingOS! To propose features, optimize drivers, or improve the multitasking scheduler, please follow this workflow:

1. Fork the repository on GitHub.
2. Create a clean, descriptive feature branch:
   ```bash
   git checkout -b feature/AmazingFeature
   ```
3. Commit your modifications with clear, concise messages:
   ```bash
   git commit -m 'Implement hypervisor-optimized network buffer queues'
   ```
4. Push the branch to your GitHub fork:
   ```bash
   git push origin feature/AmazingFeature
   ```
5. Open a Pull Request for review and community testing.

---

## License

This project is licensed under the **GPL-3.0 License** - see the LICENSE file for details.

## Acknowledgments

I hope you will have as much fun using PingOS as I had creating it. Enjoy!
