# "min userspace" Makefile
# 	only dep on libc-min, no libc, etc

# Apr 2024 mandates using gcc-9 (not gcc 11 which is default in Ubuntu 22.04)

include $(abspath ../config.mk)

# our collection of support C functions, enough for "nes-min"
ULIB = ulibc.o printf.o usys_s.o umalloc.o 

COPS = -MMD -Wall -Werror -nostdlib -g -fno-omit-frame-pointer
COPS += -nostartfiles -ffreestanding -mgeneral-regs-only
COPS += -O0

# Temporarily allow unused variables to avoid compiler complaints.
# This should be commented out after completing all quests.
#!STUDENT_WILL_SEE(COPS += -Wno-unused-variable -Wno-unused-but-set-variable)

ASMOPS = -g

all: build/nes-min

%.o: %.c
	$(VB) echo  USER CC $< "->" $@
	$(VB) $(CC) $(COPS) -MMD -c $< -o $@

%_s.o: %.S
	$(VB) echo  USER AS $< "->" $@
	$(VB) $(CC) $(ASMOPS) -MMD -c $< -o $@

build/nes-min: $(ULIB)
	cd LiteNES && $(MAKE)
	cp LiteNES/nes-min $@
	$(ARMGNU)-strip $@

# to prevent makefile from its implicit rule which is shorter & preferred
# https://www.gnu.org/software/make/manual/html_node/Canceling-Rules.html
% : %.c

clean: 
	cd LiteNES && $(MAKE) clean
	rm -f *.o $(UPROGS) build-lib/*.a build-lib/*.o mkfs *.d *.sym *.asm sd.bin
