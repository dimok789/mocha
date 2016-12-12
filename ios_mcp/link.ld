OUTPUT_ARCH(arm)

SECTIONS
{
	.text 0x05116000 : {
		_text_start = .;
		build/crt0.o(.init)
		*(.text*)
		*(.rodata*)
	}
    _text_end = .;

	.bss 0x050BE000 : {
		_bss_start = .;
		*(.bss*);
	}
	_bss_end = .;

	/DISCARD/ : {
		*(*);
	}
}

