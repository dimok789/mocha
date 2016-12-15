# SDCafiine
## Usage Instructions
Place in libwiiu root (root/sdcafiine), and either run build.py sdcafiine/installer (includes will work if you place it like this)
or cd to installer and run make, and then copy the codeXXX.bin, which I've provided in this repo. This is just the payload, you
need to use the appropriate userspace exploit to run it, eg @yellows8 libstagefright_wiiu, or the HTML exploit that libwiiu compiles

Once you have the installer, run the normal kernel exploit (maps to 0x31000000, not the 0x10000000 needed for loadiine/HBL), then
run the installer for SDCafiine which will open with an OSScreen showing IP and says to press X or A. It'll exit you out, then boot
the game. SD card only has to be inserted before you open the game.

X installs with server logging turned on, edit installer/sdcafiine.c SERVER_IP to use your own IP or edit the binary at 0x352/0x356
0xC0A8 -> 192.168, 0x000E -> .0.14 (Laptop Local IPv4). A just installs normally, but you have no way of knowing if your edits crashed

Only works with titles that allow access to the SD card !!! Only mainstream one is Smash Bros. which is why it's a mode in loadiine
To replace files eg Smash USA you put it in SD:/0005000010144F00/ which maps to /content, so in TitleID would be /movies, /patch, etc

## Credits
Cafiine creation - chadderz (and MrBean35000vr ?)

SDCafiine creation - golden45 (see https://gbatemp.net/goto/post?id=5680630)

5.5.0/5.5.1 port - NWPlayer123
