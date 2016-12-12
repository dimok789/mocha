OUTPUT_ARCH(arm)

SECTIONS
{
	.text 0xE6010A80 : {
	    _text_start = .;
		*(.text*);
		*(.rodata*);
	}
    _text_end = .;

	.bss 0xE60481F0 : {
		_bss_start = .;
		*(.bss*);
		*(COMMON);
	}
	.seeprom_buffer : {
	    _seeprom_buffer_start = .;
		*(.seeprom_buffer*);
	}
	_bss_end = .;

	/DISCARD/ : {
		*(*);
	}
}

