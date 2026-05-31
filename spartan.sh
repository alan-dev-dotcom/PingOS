#!/bin/bash

# ==============================================================================
# Spartan Commandline Interface (CLI) for PingOS
# This script manages compilation, linking, and ISO generation directly in Bash,
# completely removing the need for a Makefile.
# ==============================================================================

# Toolchain definitions
CC="gcc"
LD="ld"
ASM="nasm"

# Compiler and Linker flags matching original Makefile
CFLAGS="-m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -Iinclude -c"
LDFLAGS="-m elf_i386"

# Output images and paths
KERNEL_ELF="kernel.bin"
OS_ISO="pingos.iso"
ISO_DIR="iso_staging"

# Upgraded driver list matching the new nested directory structure
DRIVER_SOURCES=(
    "drivers/gpu/vga.c"
    "drivers/gpu/gpu_drm.c"
    "drivers/gpu/intel_gpu.c"
    "drivers/gpu/amdgpu.c"
    "drivers/gpu/i915.c"
    "drivers/gpu/xe.c"
    "drivers/gpu/nouveau.c"
    "drivers/gpu/radeon.c"
    "drivers/gpu/virtio-gpu.c"
    "drivers/gpu/mgag200.c"
    "drivers/gpu/vboxvideo.c"
    "drivers/gpu/panfrost.c"
    "drivers/gpu/lima.c"
    "drivers/peripherals/keyboard.c"
    "drivers/peripherals/mouse.c"
    "drivers/peripherals/usb.c"
    "drivers/wireless/iwlwifi.c"
    "drivers/wireless/ath9k.c"
    "drivers/wireless/ath10k.c"
    "drivers/wireless/ath11k.c"
    "drivers/wireless/rtw88.c"
    "drivers/wireless/rtl8187.c"
    "drivers/wireless/mt76.c"
    "drivers/wireless/brcmfmac.c"
    "drivers/wireless/b43.c"
    "drivers/wireless/btusb.c"
    "drivers/network/e1000e.c"
    "drivers/network/igb.c"
    "drivers/network/igxbe.c"
    "drivers/network/i40e.c"
    "drivers/network/r8169.c"
    "drivers/network/tg3.c"
    "drivers/network/forcedeth.c"
    "drivers/network/sky2.c"
    "drivers/network/alx.c"
    "drivers/network/virtio_net.c"
)

# Helper function to check if a command exists
check_tool() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Error: Required tool '$1' is not installed."
        return 1
    fi
    return 0
}

# Function to compile and link the kernel without a Makefile
compile_kernel() {
    echo "===================================================="
    echo " Starting compilation without Makefile..."
    echo "===================================================="

    # 1. Verify required toolchain utilities
    check_tool "$ASM" || return 1
    check_tool "$CC" || return 1
    check_tool "$LD" || return 1

    # Verify linker script exists
    if [ ! -f "linker.ld" ]; then
        echo "Error: 'linker.ld' script not found in current directory."
        return 1
    fi

    # 2. Assemble entry stub
    echo "Assembling kernel/entry.asm..."
    if [ ! -f "kernel/entry.asm" ]; then
        echo "Error: kernel/entry.asm not found!"
        return 1
    fi
    if ! $ASM -f elf32 kernel/entry.asm -o entry.o; then
        echo "Error: Assembly of entry.asm failed."
        return 1
    fi

    # 3. Compile main kernel
    echo "Compiling kernel/kernel.c..."
    if [ ! -f "kernel/kernel.c" ]; then
        echo "Error: kernel/kernel.c not found!"
        return 1
    fi
    if ! $CC $CFLAGS kernel/kernel.c -o kernel.o; then
        echo "Error: Compilation of kernel.c failed."
        return 1
    fi

    # 4. Compile drivers
    local compiled_objs=()
    for src in "${DRIVER_SOURCES[@]}"; do
        # Self-healing: If a driver file is missing, automatically generate a safe fallback stub
        if [ ! -f "$src" ]; then
            echo "Warning: Driver file $src not found! Generating a safe fallback stub..."
            # Automatically create directory structure if missing
            mkdir -p "$(dirname "$src")"
            local filename=$(basename "$src" .c)
            # Map hyphenated names (like virtio-gpu or virtio_net) to clean C identifiers
            local funcname=$(echo "$filename" | tr '-' '_')
            
            if [ "$filename" = "gpu_drm" ]; then
                cat << 'EOF' > "$src"
/* Safe fallback stub generated automatically by Spartan */
int drm_core_init() { return -1; }
void drm_print_diagnostics() {}
int drm_set_mode(unsigned int w, unsigned int h, unsigned char b) { return -1; }
void drm_restore_vga_text_mode() {}
void drm_clear_screen(unsigned int c) {}
void drm_put_pixel(unsigned int x, unsigned int y, unsigned int c) {}
void drm_draw_rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {}
void drm_draw_line(int x0, int y0, int x1, int y1, unsigned int c) {}
void drm_page_flip() {}
EOF
            else
                cat << EOF > "$src"
/* Safe fallback stub generated automatically by Spartan */
void ${funcname}_init() {}
EOF
            fi
        fi

        # Extract object file path (e.g. drivers/gpu/vga.o)
        local obj="${src%.c}.o"
        echo "Compiling driver: $src -> $obj"
        if ! $CC $CFLAGS "$src" -o "$obj"; then
            echo "Error: Failed to compile $src"
            return 1
        fi
        compiled_objs+=("$obj")
    done

    # 5. Link everything together using linker.ld
    echo "Linking kernel object files into standard ELF binary: $KERNEL_ELF..."
    if ! $LD $LDFLAGS -T linker.ld -o "$KERNEL_ELF" entry.o kernel.o "${compiled_objs[@]}"; then
        echo "Error: Linking failed."
        return 1
    fi

    echo "Kernel binary ($KERNEL_ELF) successfully built!"
    return 0
}

# Function to execute the complete ISO generation build pipeline
build_iso() {
    # Run the self-contained compilation process
    if ! compile_kernel; then
        echo "Error: Kernel build step failed. Aborting ISO generation."
        return 1
    fi

    echo "Spartan creates GRUB structure..."
    mkdir -p "$ISO_DIR/boot/grub"
    
    echo "Spartan copies kernel ELF binary to staging boot directory..."
    cp "$KERNEL_ELF" "$ISO_DIR/boot/"
    
    echo "Spartan generates grub.cfg dynamic configuration file..."
    echo 'set default=0' > "$ISO_DIR/boot/grub/grub.cfg"
    echo 'set timeout=0' >> "$ISO_DIR/boot/grub/grub.cfg"
    echo '' >> "$ISO_DIR/boot/grub/grub.cfg"
    echo 'menuentry "PingOS v0.2.0" {' >> "$ISO_DIR/boot/grub/grub.cfg"
    echo "    multiboot /boot/$KERNEL_ELF" >> "$ISO_DIR/boot/grub/grub.cfg"
    echo '    boot' >> "$ISO_DIR/boot/grub/grub.cfg"
    echo '}' >> "$ISO_DIR/boot/grub/grub.cfg"
    
    echo "Spartan verifies multiboot compliance..."
    if command -v grub-file >/dev/null 2>&1; then
        if grub-file --is-x86-multiboot "$ISO_DIR/boot/$KERNEL_ELF"; then 
            echo "Success: Kernel is Multiboot compliant."
        else 
            echo "Warning: Kernel is NOT Multiboot compliant! Verify your entry.asm multiboot headers."
        fi
    else
        echo "Notice: 'grub-file' is not installed. Skipping multiboot validation check..."
    fi
    
    echo "Spartan builds a bootable GRUB CD-ROM ISO image with grub-mkrescue..."
    if command -v grub-mkrescue >/dev/null 2>&1; then
        if ! grub-mkrescue -o "$OS_ISO" "$ISO_DIR"; then
            echo "Error: grub-mkrescue failed to generate the ISO."
            return 1
        fi
        echo "ISO successfully built: $OS_ISO"
    else
        echo "Error: 'grub-mkrescue' not found. Please install grub-common and xorriso to generate the ISO."
        return 1
    fi
}

# Function to boot QEMU (replacing 'make run')
run_qemu() {
    if [ ! -f "$OS_ISO" ]; then
        echo "Notice: ISO not found. Building it first..."
        if ! build_iso; then
            return 1
        fi
    fi

    echo "Booting QEMU with $OS_ISO..."
    if command -v qemu-system-i386 >/dev/null 2>&1; then
        qemu-system-i386 -cdrom "$OS_ISO"
    else
        echo "Error: 'qemu-system-i386' is not installed or not in PATH."
        return 1
    fi
}

# Function to clean up builds
clean_build() {
    echo "Cleaning up generated assembly, driver object files, binaries, and staging directories..."
    rm -rf *.o drivers/gpu/*.o drivers/peripherals/*.o drivers/wireless/*.o drivers/network/*.o "$KERNEL_ELF" "$OS_ISO" "$ISO_DIR"
    echo "Cleanup complete."
}

# Main interactive terminal loop
echo "===================================================="
echo "                 Spartan Project                    "
echo "  Type 'help' for options or 'exit' to close.       "
echo "===================================================="

while true; do
    # Display the Spartan prompt
    read -p "spartan> " userInput
    
    # Trim leading/trailing whitespace
    userInput=$(echo "$userInput" | xargs)

    case "$userInput" in
        "spartan build")
            echo "Spartan executes ISO build pipeline..."
            build_iso
            ;;
        "spartan run")
            echo "Spartan boots virtual environment..."
            run_qemu
            ;;
        "spartan clean")
            echo "Spartan executes cleanup pipeline..."
            clean_build
            ;;
        "help")
            echo "Available commands:"
            echo "  spartan build  - Compiles, links, and builds the bootable GRUB ISO"
            echo "  spartan run    - Launches the system in QEMU (builds ISO if missing)"
            echo "  spartan clean  - Cleans up compiled kernel files, objects, and ISOs"
            echo "  help           - Displays this command reference list"
            echo "  exit / quit    - Exits the Spartan commandline"
            ;;
        "exit" | "quit")
            echo "Exiting Spartan..."
            break
            ;;
        "")
            # If the user pressed enter without a command, do nothing
            ;;
        *)
            echo "Unknown command: '$userInput'. Type 'help' to see valid commands."
            ;;
    esac
done