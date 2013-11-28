###
# Config
###

PREFIX		?= arm-none-eabi-
DEVICE		?= vision_0.85.0015

CFLAGS		+= -std=gnu11 -Wall -Wextra -MMD -Os -nostdlib \
		   -mcpu=generic-armv7-a -fno-toplevel-reorder \
		   -include common/hbootdbg.h -g
LDFLAGS		+=
OBJCOPYFLAGS	+= -O binary -j .text -j .bss --set-section-flags \
		   .bss=alloc,load,contents

CC		= $(PREFIX)gcc
LD		= $(PREFIX)ld
OBJCOPY		= $(PREFIX)objcopy

###
# Preloader
###

PRELD		= preloader
PRELDSRC	= $(PRELD)/entrypoint.c \
		  $(PRELD)/preloader.c
PRELDOBJ	= $(PRELDSRC:.c=.o)
PRELDDEP	= $(PRELDSRC:.c=.d)

###
# Hbootdbg
###

HBOOT		= hbootdbg
HBOOTSRC	= $(HBOOT)/hbootdbg.c \
		  $(HBOOT)/base64.c \
		  $(HBOOT)/cpu.c \
		  $(HBOOT)/dbg.c \
		  $(HBOOT)/int.c \
		  $(HBOOT)/mmu.c \
		  $(HBOOT)/hbootlib.c \
		  $(HBOOT)/reloc.c \
		  $(HBOOT)/darm/armv7.c \
		  $(HBOOT)/darm/armv7-tbl.c \
		  $(HBOOT)/darm/darm.c \
		  $(HBOOT)/darm/darm-tbl.c
HBOOTOBJ	= $(HBOOTSRC:.c=.o)
HBOOTDEP	= $(HBOOTSRC:.c=.d)

###
# Rules
###

.PHONY: all clean

all: $(PRELD).bin $(HBOOT).bin

-include $(PRELDDEP) $(HBOOTDEP)

%.bin: %.elf
	$(OBJCOPY) $(OBJCOPYFLAGS) $< $@

$(HBOOT).elf: $(HBOOTOBJ)
	$(LD) $(LDFLAGS) -T devices/$(DEVICE)/$@.ld $^ -o $@

$(PRELD).elf: $(PRELDOBJ)
	$(LD) $(LDFLAGS) -T devices/$(DEVICE)/$@.ld $^ -o $@

clean:
	rm -f $(PRELDDEP) $(PRELDOBJ) $(PRELD).elf
	rm -f $(HBOOTDEP) $(HBOOTOBJ) $(HBOOT).elf

distclean: clean
	rm -f $(PRELD).bin
	rm -f $(HBOOT).bin
