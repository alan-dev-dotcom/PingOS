# ==============================================================================
# Makefile to compile and assemble PingOS into a bootable CD-ROM ISO
# Requirements: nasm, gcc (i386-elf or multilib support with -m32 option), xorriso
# Supports directory structure: boot/, kernel/, drivers/, include/
# ==============================================================================

# Toolchain definitions
CC = gcc
LD = ld
ASM = nasm

# Compiler Flags
# -ffreestanding prevents the compiler from linking default standard C libraries
# -m32 compiles for 32-bit x86 target
# -fno-pie prevents Position Independent Executable generation
# -Iinclude allows finding header files inside the include/ directory
CFLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -Iinclude -c

# Output images and paths
BOOT_BIN = boot.bin
KERNEL_BIN = kernel.bin
OS_IMAGE = pingos.img
OS_ISO = pingos.iso
ISO_DIR = iso_staging

# Driver object files
DRIVER_OBJS = drivers/vga.o drivers/keyboard.o drivers/mouse.o drivers/intel_gpu.o drivers/usb.o

all: $(OS_ISO)

$(BOOT_BIN): boot/boot.asm
	$(ASM) -f bin boot/boot.asm -o $(BOOT_BIN)

entry.o: kernel/entry.asm
	$(ASM) -f elf32 kernel/entry.asm -o entry.o

kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) kernel/kernel.c -o kernel.o

# Pattern rule to automatically compile any C files in the drivers/ directory
drivers/%.o: drivers/%.c
	@mkdir -p drivers
	$(CC) $(CFLAGS) $< -o $@

# Link the entry stub, kernel, and all driver object files together
$(KERNEL_BIN): entry.o kernel.o $(DRIVER_OBJS) linker.ld
	$(LD) -m elf_i386 -T linker.ld -o $(KERNEL_BIN) --oformat binary entry.o kernel.o $(DRIVER_OBJS)

# Build the complete floppy image (1.44MB)
$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $(OS_IMAGE)
	truncate -s 1440k $(OS_IMAGE)

# Generate a bootable El Torito ISO using xorriso
$(OS_ISO): $(OS_IMAGE)
	@echo "Creating ISO staging directory..."
	mkdir -p $(ISO_DIR)
	cp $(OS_IMAGE) $(ISO_DIR)/
	@echo "Generating bootable CD-ROM ISO with xorriso..."
	xorriso -as mkisofs -V "PINGOS" -b $(OS_IMAGE) -o $(OS_ISO) $(ISO_DIR)/

# Boot QEMU using the newly built virtual CD-ROM drive (-cdrom)
run: $(OS_ISO)
	qemu-system-x86_64 -cdrom $(OS_ISO)

clean:
	rm -rf *.o drivers/*.o $(BOOT_BIN) $(KERNEL_BIN) $(OS_IMAGE) $(OS_ISO) $(ISO_DIR)