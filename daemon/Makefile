EXE=my_daemon
DTB = mydt.dtb
SRCS=$(wildcard *.c) 

ifeq ($(target),)
target=nano
endif

CFLAGS=-Wall -Wextra -g -c -O0 -MD -std=gnu11

ifeq ($(target),nano)
TOOLCHAIN_PATH=~/workspace/nano/buildroot/output/host/usr/bin/
TOOLCHAIN=$(TOOLCHAIN_PATH)aarch64-linux-gnu-
CFLAGS+=-mcpu=cortex-a53 -funwind-tables -lpthread
##CFLAGS+=-O2 -fno-omit-frame-pointer
OBJDIR=.obj/nano
EXEC=$(EXE)
endif



CC=$(TOOLCHAIN)gcc
LD=$(TOOLCHAIN)gcc
AR=$(TOOLCHAIN)ar
STRIP=$(TOOLCHAIN)strip

OBJDIR=.obj/$(target)
OBJS= $(addprefix $(OBJDIR)/, $(SRCS:.c=.o))

$(OBJDIR)/%o: %c
	$(CC) -lpthread $(CFLAGS) $< -o $@
	
all: $(OBJDIR)/ $(EXEC) dtb
	echo $(EXEC)
	echo $(SRCS)
	
$(EXEC): $(OBJS) $(LINKER_SCRIPT)
	$(LD) $(OBJS) $(LDFLAGS) -o $@ -lpthread

$(OBJDIR)/:
	mkdir -p $(OBJDIR)

clean:
	rm -Rf $(OBJDIR) $(EXEC) $(EXEC)_s *.dtb *~

clean_all: clean
	rm -Rf .obj $(EXE) $(EXE)_s $(EXE)_a $(EXE)_a_s $(EXE)_h $(EXE)_h_s


ROOTPATH :=/home/lmi/workspace/nano
CVER     := aarch64-linux-gnu-
KVER     := 4.18.5
CPU      := arm64

KDIR     := $(ROOTPATH)/buildroot/output/build/linux-$(KVER)/
TOOLS    := $(ROOTPATH)/buildroot/output/host/usr/bin/$(CVER)
PWD := $(shell pwd)

INCL+=-I. -I$(KDIR)/include -I$(KDIR)/arch/arm64/boot/dts/allwinner
DTS = $(DTB:.dtb=.dts)

dtb: $(DTB)
$(DTB) : $(DTS)
	ln -s $(KDIR)/arch/arm/boot/dts arm
	-cpp $(INCL) -E -P -x assembler-with-cpp $(DTS) | dtc -I dts -O dtb -o $(DTB) -
	rm arm

-include $(OBJS:.o=.d)

.PHONY: all clean clean_all
