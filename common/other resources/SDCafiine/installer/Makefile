CC=powerpc-eabi-gcc
CFLAGS=-std=gnu99 -nostdinc -fno-builtin -c
LD=powerpc-eabi-ld
LDFLAGS=-Ttext 1800000 --oformat binary
project	:=	.
root:=$(CURDIR)
build	:=	 $(root)/bin
libs := $(root)/../../libwiiu/bin
www :=$(root)/../../www
framework:=$(root)/../../framework

all: clean setup fs main550 main540 main532

setup:
	mkdir -p $(root)/bin/

fs:
	cd ../fs/ && make

main550:
	$(CC) $(CFLAGS) -DVER=550 $(project)/sdcafiine.c
	#-Wa,-a,-ad
	cp -r $(root)/*.o $(build)
	rm $(root)/*.o
	$(LD) $(LDFLAGS) -o $(build)/code550.bin $(build)/sdcafiine.o `find $(build) -name "*.o" ! -name "sdcafiine.o"`

main540:
	$(CC) $(CFLAGS) -DVER=540 $(project)/sdcafiine.c
	#-Wa,-a,-ad
	cp -r $(root)/*.o $(build)
	rm $(root)/*.o
	$(LD) $(LDFLAGS) -o $(build)/code540.bin $(build)/sdcafiine.o `find $(build) -name "*.o" ! -name "sdcafiine.o"`

main532:
	$(CC) $(CFLAGS) -DVER=532 $(project)/sdcafiine.c
	#-Wa,-a,-ad
	cp -r $(root)/*.o $(build)
	rm $(root)/*.o
	$(LD) $(LDFLAGS) -o $(build)/code532.bin $(build)/sdcafiine.o `find $(build) -name "*.o" ! -name "sdcafiine.o"`

clean:
	rm -rf $(build)/*
