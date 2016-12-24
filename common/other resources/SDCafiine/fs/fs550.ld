OUTPUT(fs550.elf);

SECTIONS {
	.text 0x011dcc00 : {
		server_ip = .;
		. = . + 4;
		*(.text._start);
		*(.text*);
		*(.magicptr*);
	}
	.magic : {
		*(.magic*);
	}
	/DISCARD/ : {
		*(*);
	}
}

/* FSA methods */
PROVIDE(FSAInit = 0x010612F4);
PROVIDE(FSAShutdown = 0x010613BC);
PROVIDE(FSAAddClient = 0x01065EB4);
PROVIDE(FSADelClient = 0x010614EC);
PROVIDE(FSAOpenFile = 0x01062C40);

/* FS base methods */
PROVIDE(FSInit = 0x01068E10);
PROVIDE(FSShutdown = 0x01068F80);
PROVIDE(FSAddClientEx = 0x01069044);
PROVIDE(FSDelClient = 0x01069450);

/* FS methods for path replacement */
PROVIDE(FSOpenFile = 0x0106F9C4);
PROVIDE(FSOpenFileAsync = 0x0106AE7C);
PROVIDE(FSOpenDir = 0x010700D8);
PROVIDE(FSOpenDirAsync = 0x0106BA00);
PROVIDE(FSChangeDir = 0x0106F944);
PROVIDE(FSChangeDirAsync = 0x0106ABF0);
PROVIDE(FSGetStat = 0x01070810);
PROVIDE(FSGetStatAsync = 0x0106CA3C);

/* FS methods - log */
PROVIDE(FSCloseFile = 0x0106FAD0);
PROVIDE(FSCloseFileAsync = 0x0106AEB4);
PROVIDE(FSSetPosFile = 0x0106FF78);
PROVIDE(FSGetPosFile = 0x0106FF08);
PROVIDE(FSGetStatFile = 0x0106FFE8);
PROVIDE(FSIsEof = 0x01070058);
PROVIDE(FSReadFile = 0x0106FB50);
PROVIDE(FSReadFileWithPos = 0x0106FBDC);
PROVIDE(FSReadFileAsync = 0x0106B250);
PROVIDE(FSCloseDir = 0x01070148);
PROVIDE(FSGetCwd = 0x010702B8);
PROVIDE(FSReadDir = 0x010701C8);

/* FS methods for sd card */
PROVIDE(FSGetMountSource = 0x0106F66C);
PROVIDE(FSMount = 0x0106F75C);

/* GX2 methods */
PROVIDE(GX2WaitForVsync = 0x1151964);

/* Socket methods */
PROVIDE(socket_lib_init = 0x10C02F4);
PROVIDE(socket = 0x10C21C8);
PROVIDE(socketclose = 0x10C2314);
PROVIDE(connect = 0x10C0828);
PROVIDE(send = 0x10C16AC);
PROVIDE(recv = 0x10C0AEC);
PROVIDE(socketlasterr = 0x10C0490);

/* Standard library methods */
PROVIDE(memcpy = 0x1035FC8);
PROVIDE(memset = 0x1035FB4);
PROVIDE(MEMAllocFromDefaultHeapEx = 0x1004F870);
PROVIDE(MEMAllocFromExpHeapEx = 0x010497b0);

/* OS data */
PROVIDE(title_id = 0x10013C10);
