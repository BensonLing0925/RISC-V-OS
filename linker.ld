OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS {
	. = 0x80000000;

	.text : {
		*(.text*)
		*(.text.trap*)
	}

	.rodata : {
		*(.rodata*)
	}

	.data : {
		*(.data*)
	}

	.bss : {
		__bss_start = .;
		*(.bss*)
		*(COMMON)
		__bss_end = .;
	}

	.stack (NOLOAD) : {
		. = ALIGN(16);
		stack_bottom = .;
		. = . + 4096; /* 4KB stack space */
		stack_top = .;
	}

	.stack_s (NOLOAD) : {
		. = ALIGN(16);
		sstack_bottom = .;
		. = . + 4096;
		sstack_top = .;
	}

	end = .;
}
