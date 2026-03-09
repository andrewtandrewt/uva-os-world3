// Format of an ELF executable file

/* fxl: Sept 2024

  an ELF has three types of headers.

  - elf header. a global one. 

  - Program headers. used by the runtime linker or loader. They describe how to
    create a process image in memory contain information about segments, i.e.
    where to load them into memory and with what permissions

    A loadable segment is contiguous in both the ELF file and in memory.
    ( ^^ our loader cares about this ^^ )

  - Section Headers. They describe the organization of the file itself. mainly
    for use during linking or static analysis, not during execution
    ( ^^ may even be stripped from executable
      Not relevant to our loader )
*/

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

// File header
// https://refspecs.linuxbase.org/elf/gabi4+/ch4.eheader.html
struct elfhdr {
  uint magic;  // must equal ELF_MAGIC
  uchar elf[12];
  ushort type;
  ushort machine;
  uint version;
  uint64 entry;
  uint64 phoff; // holds the program header table's file offset in bytes
  uint64 shoff;
  uint flags;
  ushort ehsize;
  ushort phentsize;
  ushort phnum;
  ushort shentsize;
  ushort shnum;
  ushort shstrndx;
};

// Program header
struct proghdr {
  uint32 type;
  uint32 flags;   // https://refspecs.linuxbase.org/elf/gabi4+/ch5.pheader.html#p_flags
  uint64 off; // "the offset from the beginning of the file at which the first byte of the segment resides"
  uint64 vaddr;
  uint64 paddr;
  uint64 filesz;
  uint64 memsz;
  uint64 align;
};

// Values for Proghdr type
#define ELF_PROG_LOAD           1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4
