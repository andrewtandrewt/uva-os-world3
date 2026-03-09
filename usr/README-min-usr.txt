The "min" userspace contains a minimalist "libc".

DESIGNS

- No file abstractions, filesystems, etc. 

- program(s) will be built & linked as *.elf binaries, and packed into
kernel8.img, then executed via the exec() syscall


LIBRARY
    *.c *.S a micro "libc" that provides basic C functions and syscall stubs.

PROGRAMS

    LiteNES/
        The NES emulator that can execute a built-in ROM (mario).
