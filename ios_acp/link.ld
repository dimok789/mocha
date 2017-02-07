OUTPUT_ARCH(arm)

SECTIONS
{
	.text 0xE00DB660 : {
		_text_start = .;
		*(.text*);
		*(.rodata*);
	}
    _text_end = .;

	.bss 0xE0261F10 : {
		_bss_start = .;
		*(.bss*);
		*(.data*);
	}
	_bss_end = .;

	/DISCARD/ : {
		*(*);
	}
}

