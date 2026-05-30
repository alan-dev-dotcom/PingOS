# ==============================================================================
# Makefile to compile and assemble PingOS into a bootable GRUB ISO
# Requirements: nasm, gcc (i386-elf or multilib support with -m32 option),
#               xorriso, grub-pc-bin (on Ubuntu/Debian), and grub-common
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
# -fno-stack-protector disables stack protection checks (not present in freestanding)
# -Iinclude allows finding header files inside the include/ directory
CFLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -Iinclude -c

# Output images and paths
KERNEL_ELF = kernel.bin
OS_ISO = pingos.iso
ISO_DIR = iso_staging

# Driver object files
DRIVER_OBJS = drivers/vga.o drivers/keyboard.o drivers/mouse.o drivers/intel_gpu.o drivers/usb.o

all: $(OS_ISO)

entry.o: kernel/entry.asm
	$(ASM) -f elf32 kernel/entry.asm -o entry.o

kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) kernel/kernel.c -o kernel.o

# Pattern rule to automatically compile any C files in the drivers/ directory
drivers/%.o: drivers/%.c
	@mkdir -p drivers
	$(CC) $(CFLAGS) $< -o $@

# Link the entry stub, kernel, and all driver object files together as a standard ELF
# Note: GRUB natively supports loading ELF files with a Multiboot header, 
# so we remove --oformat binary to allow GRUB to parse the section headers correctly.
$(KERNEL_ELF): entry.o kernel.o $(DRIVER_OBJS) linker.ld
	$(LD) -m elf_i386 -T linker.ld -o $(KERNEL_ELF) entry.o kernel.o $(DRIVER_OBJS)

# Generate a bootable GRUB ISO using grub-mkrescue
$(OS_ISO): $(KERNEL_ELF)
	@echo "Creating GRUB ISO staging directory structure..."
	mkdir -p $(ISO_DIR)/boot/grub
	
	@echo "Copying kernel ELF binary to staging boot directory..."
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/
	
	@echo "Generating grub.cfg dynamic configuration file..."
	@echo 'set default=0' > $(ISO_DIR)/boot/grub/grub.cfg
	@echo 'set timeout=0' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo 'menuentry "PingOS v0.2.0" {' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '    multiboot /boot/$(KERNEL_ELF)' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '    boot' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg
	
	@echo "Verifying multiboot compliance..."
	@if grub-file --is-x86-multiboot $(ISO_DIR)/boot/$(KERNEL_ELF); then \
		echo "Success: Kernel is Multiboot compliant."; \
	else \
		echo "Warning: Kernel is NOT Multiboot compliant! Verify your entry.asm multiboot headers."; \
	fi
	
	@echo "Building bootable GRUB CD-ROM ISO image with grub-mkrescue..."
	grub-mkrescue -o $(OS_ISO) $(ISO_DIR)

# Boot QEMU using the newly built virtual GRUB CD-ROM ISO
run: $(OS_ISO)
	qemu-system-i386 -cdrom $(OS_ISO)

clean:
	rm -rf *.o drivers/*.o $(KERNEL_ELF) $(OS_ISO) $(ISO_DIR)