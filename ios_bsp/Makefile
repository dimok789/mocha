ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

ifeq ($(filter $(DEVKITARM)/bin,$(PATH)),)
export PATH:=$(DEVKITARM)/bin:$(PATH)
endif

CC = arm-none-eabi-gcc
LINK = arm-none-eabi-gcc
AS = arm-none-eabi-as
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
CFLAGS += -Wall -mbig-endian -std=gnu11 -mcpu=arm926ej-s -msoft-float -mfloat-abi=soft -Os
LDFLAGS += -nostartfiles -nodefaultlibs -mbig-endian -Wl,-T,link.ld
LIBDIRS += -L$(CURDIR)/../libs
LIBS += -lgcc

CFILES = $(wildcard source/*.c)
BINFILES = $(wildcard data/*.bin)
OFILES = $(BINFILES:data/%.bin=build/%.bin.o)
OFILES += $(CFILES:source/%.c=build/%.o)
DFILES = $(CFILES:source/%.c=build/%.d)
SFILES = $(wildcard source/*.s)
OFILES += $(SFILES:source/%.s=build/%.o)
PROJECTNAME = ${shell basename "$(CURDIR)"}
CWD = "$(CURDIR)""

#---------------------------------------------------------------------------------
# canned command sequence for binary data, taken from devkitARM
#---------------------------------------------------------------------------------
define bin2o
	bin2s $< | $(AS) -EB -o $(@)
endef

.PHONY:=all dirs

all: dirs $(PROJECTNAME).bin $(PROJECTNAME)_syms.h $(PROJECTNAME).bin $(PROJECTNAME).bin.h

dirs:
	@mkdir -p build

$(PROJECTNAME).elf: $(OFILES)
	@echo "LD $@"
	@$(LINK) $(LDFLAGS) -o $(PROJECTNAME).elf $(sort $(filter-out build/crt0.o, $(OFILES))) $(LIBDIRS) $(LIBS)

$(PROJECTNAME).bin: $(PROJECTNAME).elf
	@echo "OBJCOPY $@\n"
	@$(OBJCOPY) -j .text -j .rodata -j .data -O binary $(PROJECTNAME).elf $@
	
$(PROJECTNAME).bin.h: $(PROJECTNAME).bin
	@xxd -i $< | sed "s/unsigned/static const unsigned/g;s/$(PROJECTNAME)$*/$(PROJECTNAME)/g" > $@
	
$(PROJECTNAME)_syms.h:
	@echo "#ifndef $(PROJECTNAME)_SYMS_H" > $@
	@echo "#define $(PROJECTNAME)_SYMS_H" >> $@
	@$(OBJDUMP) -EB -t -marm $(PROJECTNAME).elf | grep 'g     F .text' | grep -v '.hidden' | awk '{print "#define " $$6 " 0x" $$1}' >> $@
	@$(OBJDUMP) -EB -t -marm $(PROJECTNAME).elf | grep -e 'g       .text' -e '_bss_' -e "_seeprom_buffer_start" | awk '{print "#define " $$5 " 0x" $$1}' >> $@
	@echo "#endif" >> $@

clean:
	@rm -f build/*.o build/*.d
	@rm -f $(PROJECTNAME).elf $(PROJECTNAME).bin $(PROJECTNAME)_syms.h $(PROJECTNAME).bin $(PROJECTNAME).bin.h
	@echo "all cleaned up !"

-include $(DFILES)

build/%.o: source/%.c
	@echo "CC $(notdir $<)"
	@$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) -MM $< > build/$*.d

build/%.o: source/%.s
	@echo "CC $(notdir $<)"
	@$(CC) $(CFLAGS) -xassembler-with-cpp -c $< -o $@
	@$(CC) -MM $< > build/$*.d

build/%.bin.o: data/%.bin
	@echo "BIN $(notdir $<)"
	@$(bin2o)
