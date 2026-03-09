/* linker script for rpi3 hardware */

SECTIONS
{
	. = 0xffff000000080000;
	.text.boot : { *(.text.boot) }

	/* include user text/data compiled as part of kernel 
		e.g. user.c user_sys.S user_donut.c */			
	. = ALIGN(0x00001000);
	user_begin = .;
	.text.user : { build-rpi3/kuser* (.text) }
	.rodata.user : { build-rpi3/kuser* (.rodata) }
	.data.user : { build-rpi3/kuser* (.data) }
	.bss.user : { build-rpi3/kuser* (.bss) }
	user_end = .;

	. = ALIGN(512);		/*sector aligned*/
	ramdisk_start = .;
	.ramdisk : {build-rpi3/sd* (.data)}		/* must come before generic *(.data) */
	ramdisk_end = .;		
	
	. = ALIGN(512);		/*sector aligned*/
	nes_start = .; 
	.nes : {build-rpi3/nes* (.data)}		/* must come before generic *(.data) */
	/* we don't need nes_end */
		
	. = ALIGN(512);		/*sector aligned*/
	.text :  { *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	
	. = ALIGN(0x8);
	bss_begin = .;
	.bss : { *(.bss*) } 
	bss_end = .;

	. = ALIGN(0x00001000);
	idmap_dir = .;
	.data.idmapd : {. += (3* (1 << 12));}
	. = ALIGN(0x00001000);
	pg_dir = .;
	.data.pgd : { . += (4 * (1 << 12)); }	/* PG_DIR_SIZE */
	kernel_end = .; 
}

