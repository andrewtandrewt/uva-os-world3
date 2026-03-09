# nes-min, a minimal NES emulator
# - standalone .elf; to be included in kernel8.img and loaded via
# 	exec0())
# - Minimum functions: only built-in Mario, only graphics.
# - No ROM file load, no input control, no sound.
# - Only depends on "min" libc: ulib, printf, usys_s, umalloc, etc.

include $(abspath ../../config.mk)

PROG 	:= nes-min

CFLAGS  := -MMD -Wall -Werror -nostdlib -g -fno-omit-frame-pointer
CFLAGS  += -nostartfiles -ffreestanding -mgeneral-regs-only
CFLAGS  += -O2  # Os to shrink binary size, e..g to fit xv6 fs limit

# Temporarily allow unused variables to avoid compiler complaints.
# This should be commented out after completing all quests.
#!STUDENT_WILL_SEE(CFLAGS += -Wno-unused-variable -Wno-unused-but-set-variable)

LDFLAGS := 

# game engine files
CFILES  := common.c  cpu-addressing.c  cpu.c  fce.c memory.c  mmc.c  ppu.c  psg.c
# hardware abstraction layer, need modification for different platforms
CFILES  += hal0.c main0.c 

OBJS    := $(CFILES:%.c=%.o)

# minimal "libc" for nes-min only
OBJS	+= ../ulibc.o
# for printf():
OBJS	+= ../printf.o 
OBJS	+= ../usys_s.o 
OBJS	+= ../umalloc.o

%.o: %.c
	@echo + CC $< "->" $@
	@$(CC) $(CFLAGS) -c -o $@ $<
 
$(PROG): $(OBJS)
	@echo + LD "->" $@
	@aarch64-linux-gnu-ld $(LDFLAGS) -T nes.ld -o $@ $^
	$(VB) $(ARMGNU)-objdump -S $@ > $@.asm
	$(VB) $(ARMGNU)-objdump -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $@.sym
	
-include $(patsubst %.o, %.d, $(OBJS))

.PHONY: clean

clean:
	rm -rf $(PROG) *.o *.d *.asm *.sym
