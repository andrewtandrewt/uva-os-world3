include $(abspath ../config.mk)

ifeq ($(filter $(PLAT),virt rpi3qemu rpi3),)
$(error Invalid PLAT value. do "export PLAT=rpi3qemu" (for qemu build) or "export PLAT=rpi3" (for rpi3 build))
endif

# support for fat32 fs
# comment out == disable fatfs & secondary device
# CONFIG_FAT ?= 1

BUILD_DIR = build-$(PLAT)
SRC_DIR = .

ARMGNU ?= aarch64-linux-gnu
COPS = -Wall -Werror -nostdlib -nostartfiles -ffreestanding \
	-I$(SRC_DIR) \
	-Iaddon/usb/include\
	-mgeneral-regs-only \
	-g 

# Temporarily allow unused variables to avoid compiler complaints.
# This should be commented out after completing all quests.
COPS += -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function # TODO: replace this

# the default debug level, to be overridden by less verbose levels in individual files 
COPS += -DCONFIG_KAGE_GLOBAL_DEBUG_LEVEL=10   # verbose
# COPS += -DCONFIG_KAGE_GLOBAL_DEBUG_LEVEL=30   # info
# COPS += -DCONFIG_KAGE_GLOBAL_DEBUG_LEVEL=40   # warning

ASMOPS = -I$(SRC_DIR)  -g 

##### platform specific flags, targets ########
ifeq (${PLAT}, virt)
COPS += -DPLAT_VIRT
ASMOPS += -DPLAT_VIRT
LINKSCR = $(SRC_DIR)/linker-virt.ld
KERNEL = kernel8-virt.img
endif

ifeq (${PLAT}, rpi3qemu)
COPS += -DPLAT_RPI3QEMU
ASMOPS += -DPLAT_RPI3QEMU
LINKSCR = $(SRC_DIR)/linker-rpi3qemu.ld
KERNEL = kernel8-rpi3qemu.img
endif

# if SCTLR_EL1 specifies alginment check, we must specify -mstrict-align here,
# otherwise compiler will issue unaligned access -- memory exception. 
ifeq (${PLAT}, rpi3)
COPS += -DPLAT_RPI3 -mcpu=cortex-a53+fp+simd -mstrict-align
ASMOPS += -DPLAT_RPI3
LINKSCR = $(SRC_DIR)/linker-rpi3.ld
KERNEL = kernel8-rpi3.img
endif

##### feature specific flags ####
# ifeq (${CONFIG_FAT}, 1)
# COPS += -DCONFIG_FAT
# endif
#################################

##### the flags for certain .c files that must use -O0
COPS1 = $(COPS)
COPS1 += -O0 	
#################################

# default optimization level for C files
COPS += -O2
# COPS += -O0

# for stacktrace purpose
COPS += -fno-omit-frame-pointer -Wno-frame-address 

all : $(KERNEL)

clean :
	rm -rf $(BUILD_DIR) *.img *.asm *.sym

# use -O0 only for kuser.c (launcher), for which -O2 emits references to kernel va and crashes things
$(BUILD_DIR)/kuser_c.o: $(SRC_DIR)/kuser.c
	@echo  KERNEL CC $< "->" $@
	$(VB) mkdir -p $(@D)
	$(VB) $(ARMGNU)-gcc-9 $(COPS1) -MMD -c $< -o $@

# confirmed - needed; otherwise cannot boot (9/17/24)
$(BUILD_DIR)/kuser_donut_c.o: $(SRC_DIR)/kuser_donut.c
	@echo  KERNEL CC $< "->" $@
	$(VB) mkdir -p $(@D)
	$(VB) $(ARMGNU)-gcc-9 $(COPS1) -MMD -c $< -o $@	

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	@echo  KERNEL CC $< "->" $@
	$(VB) mkdir -p $(@D)
	$(VB) $(ARMGNU)-gcc-9 $(COPS) -MMD -c $< -o $@

# font build rules. 
# NB: the symbols in the binary will have prefix "_binary". 
$(BUILD_DIR)/%_psf.o: %.psf
	$(VB) mkdir -p $(@D)
	$(VB) $(ARMGNU)-ld -r -b binary -o $@ $<
	
# wrap nes binary as obj file, included into the kernel image
# be careful with the obj name, as the linker script depends on it 
$(BUILD_DIR)/nes-min_bin.o: $(USER_PATH)/build/nes-min
	$(VB) mkdir -p $(@D)
	$(VB) $(ARMGNU)-ld -r -b binary -o $@ $<	

# asm rule 
$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	@echo  KERNEL AS $< "->" $@
	$(VB) $(ARMGNU)-gcc-9 $(ASMOPS) -MMD -c $< -o $@

# addon: usb lib
# addon/usb/lib/libuspi.a:
# 	cd addon/usb/lib && $(MAKE)

# generic kernel
# C_OBJS = $(BUILD_DIR)/bio_c.o
# 	console dev: needed for user task stdout
C_OBJS += $(BUILD_DIR)/console_c.o
C_OBJS += $(BUILD_DIR)/exec0_c.o
C_OBJS += $(BUILD_DIR)/irq_c.o
C_OBJS += $(BUILD_DIR)/kernel_c.o
C_OBJS += $(BUILD_DIR)/vm_c.o
C_OBJS += $(BUILD_DIR)/alloc_c.o
C_OBJS += $(BUILD_DIR)/printf_c.o
C_OBJS += $(BUILD_DIR)/sched_c.o
# C_OBJS += $(BUILD_DIR)/sleeplock_c.o
C_OBJS += $(BUILD_DIR)/spinlock_c.o
C_OBJS += $(BUILD_DIR)/string_c.o
C_OBJS += $(BUILD_DIR)/sys_c.o
C_OBJS += $(BUILD_DIR)/timer_c.o
C_OBJS += $(BUILD_DIR)/mbox_c.o
C_OBJS += $(BUILD_DIR)/unittests_c.o
C_OBJS += $(BUILD_DIR)/kuser_c.o
C_OBJS += $(BUILD_DIR)/kuser_donut_c.o
# backtrace support
C_OBJS += $(BUILD_DIR)/stacktrace_c.o

ASM_OBJS = $(BUILD_DIR)/boot_s.o
ASM_OBJS += $(BUILD_DIR)/entry_s.o
ASM_OBJS += $(BUILD_DIR)/sched_s.o
ASM_OBJS += $(BUILD_DIR)/kuser_sys_s.o
ASM_OBJS += $(BUILD_DIR)/utils_s.o

# binary objs embedded in kernel8.img
# nes0 elf binary, build out of kernel tree as a separate project
BIN_OBJS += $(BUILD_DIR)/nes-min_bin.o# TODO: replace this

# font psf, for boot logo
BIN_OBJS += $(BUILD_DIR)/font_psf.o

# ------- Rpi3 QEMU specific source -------
ifeq (${PLAT}, rpi3qemu)
C_OBJS += $(BUILD_DIR)/mini_uart_c.o
endif

# ------- Rpi3 specific source -------
ifeq (${PLAT}, rpi3)
C_OBJS += $(BUILD_DIR)/mini_uart_c.o
endif

BOOTFS_DIR=/media/$(USER)/bootfs

$(KERNEL): $(LINKSCR) $(C_OBJS) $(ASM_OBJS) $(BIN_OBJS)
	$(VB) echo "KERNEL LD"
	$(VB) $(ARMGNU)-ld -T $(LINKSCR) -o $(BUILD_DIR)/kernel8.elf $(C_OBJS) $(ASM_OBJS) $(BIN_OBJS)
	# Create an elf where VA==PA, for debugging symbols only. cf "linker-rpi3qemu-bootdebug.ld"
	$(VB) $(ARMGNU)-ld -T $(SRC_DIR)/linker-rpi3qemu-bootdebug.ld -o $(BUILD_DIR)/kernel8-bootdebug.elf $(C_OBJS) $(ASM_OBJS) $(BIN_OBJS)
	$(VB) $(ARMGNU)-objcopy $(BUILD_DIR)/kernel8.elf -O binary $(KERNEL)	
	$(VB) $(ARMGNU)-objdump -dS $(BUILD_DIR)/kernel8.elf > kernel8.asm
	$(VB) nm --numeric-sort $(BUILD_DIR)/kernel8.elf > kernel8.sym
	$(VB) echo KERNEL INSTALL
	# WSL2: copy the kernel img to local dir, if exists
	$(VB) if [ -d /mnt/c/data/tmp ]; then \
		cp $(KERNEL) /mnt/c/data/tmp/$(KERNEL); \
	fi
	# VM/Linux: copy to dir like "/media/student/bootfs/"
	$(VB) if [ -d $(BOOTFS_DIR) ]; then \
		if [ ! -f $(BOOTFS_DIR)/$(KERNEL) ]; then \
			echo "Warning: Kernel $(KERNEL) does not exist in $(BOOTFS_DIR). Do you want to copy it there? (y/n)"; \
			read CONFIRMATION; \
			if [ "$$CONFIRMATION" = "y" ]; then \
				cp $(KERNEL) $(BOOTFS_DIR)/$(KERNEL); \
			else \
				echo "Skipping kernel copy to $(BOOTFS_DIR)."; \
			fi; \
		else \
			cp $(KERNEL) $(BOOTFS_DIR)/$(KERNEL); \
		fi; \
	fi