OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

ADDR_IN_PROTECT = 0x10000;
ADDR_IN_REAL = 0;

SECTIONS
{
	. = ADDR_IN_REAL;
	.text.real : {
        *(.text.real)
    }

	. = ALIGN(32);
	. += ADDR_IN_PROTECT;
	text_base = . - ADDR_IN_PROTECT;
    .text : AT(text_base) {
        *(.text)
    }

	. = ALIGN(32);
	data_base = . - ADDR_IN_PROTECT;
    .data : AT(data_base) {
        *(.*)
    }

	/DISCARD/ : {
        *(.eh_frame .note.GNU-stack)
    }
}
