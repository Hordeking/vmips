# This is not a proper Makefile, but it will do until one exists.

MGCC=/opt/mips/bin/mips-dec-ultrix4.5-gcc
MGCC_FLAGS=-g -I../.. -I../../test_code -mno-abicalls
MAS=/opt/mips/bin/mips-dec-ultrix4.5-as -EL
MLD=/opt/mips/bin/mips-dec-ultrix4.5-ld -EL
#CPP=cpp
CPP=$(shell $(MGCC) --print-prog-name=cpp) -I.. -I../../test_code
MOBJDUMP=/opt/mips/bin/mips-dec-ultrix4.5-objdump
MOBJCOPY=/opt/mips/bin/mips-dec-ultrix4.5-objcopy
HOSTGCC=gcc
DISASSEMBLE=$(MOBJDUMP) --disassemble-all --all-headers
RAWTEXTS=$(ROMS:.rom=.raw)
RELOCS=$(ROMS:.rom=.fix)
SETUP=setup.o
OBJS=$(COBJS) $(AOBJS)
HOST_PAGESIZE=$(shell ../../utils/getpagesize)
SHELL = /bin/sh
srcdir = .
top_srcdir = ../..

all: boot.rom pftest.rom boot.host coff.host loadtest

setup.S: $(top_srcdir)/test_code/setup.S
	ln -s $< .

ld.script: $(top_srcdir)/test_code/ld.script
	ln -s $< .

loadtest: loadtest_setup.o loadtest.o lib.o serial.o
	$(MLD) -T loadtest.script -o $@ $+

%.rom: %.raw
	dd if=$*.raw of=$@ bs=$(HOST_PAGESIZE) conv=sync

%.raw: %.fix
	$(MOBJCOPY) -O binary $< $@

pftest.fix: $(SETUP) pftest.o lib.o serial.o ld.script
	$(MLD) -T ld.script -o $@ $+

boot.fix: $(SETUP) boot.o coff.o lib.o xmrcv.o serial.o ld.script
	$(MLD) -T ld.script -o $@ $+

%.s: %.S
	$(CPP) $< > $@

%.s: %.c
	$(MGCC) $(MGCC_FLAGS) -S -c -o $@ $<

%.o: %.c
	$(MGCC) $(MGCC_FLAGS) -c -o $@ $<

%.o: %.s
	$(MAS) $(MAS_FLAGS) -o $@ $<

%.o: %.S
	@$(MAKE) $*.s
	@$(MAKE) $*.o

clean:
	rm -f *.rom *.raw *.fix *.o core setup.s libtest libtest.std ld.script setup.S boot.host coff.host loadtest loadtest_setup.s

HOSTCC=gcc
HOSTCFLAGS=-g

libtest.std: libtest.std.o
	${HOSTCC} ${HOSTCFLAGS} -o $@ $+

libtest: libtest.o lib.host.o
	${HOSTCC} ${HOSTCFLAGS} -o $@ $+

libtest.std.o: libtest.c
	${HOSTCC} ${HOSTCFLAGS} -DUSE_STANDARD_LIBRARY -c -o $@ $+

libtest.o: libtest.c
	${HOSTCC} ${HOSTCFLAGS} -c -o $@ $+

boot.host: boot.host.o coff.host.o lib.host.o xmrcv.host.o boot_main.host.o
	${HOSTCC} ${HOSTCFLAGS} -o $@ $+

lib.o: lib.c lib.h

lib.host.o: lib.c lib.h
	${HOSTCC} ${HOSTCFLAGS} -c -o $@ lib.c

boot.host.o: boot.c
	${HOSTCC} -nostdinc ${HOSTCFLAGS} -c -o $@ $+

coff.host: coff.host.o coff_main.host.o
	${HOSTCC} ${HOSTCFLAGS} -o $@ $+

coff.host.o: coff.c
	${HOSTCC} ${HOSTCFLAGS} -c -o $@ $+

coff_main.host.o: coff_main.c
	${HOSTCC} ${HOSTCFLAGS} -c -o $@ $+

boot_main.host.o: boot_main.c
	${HOSTCC} -nostdinc ${HOSTCFLAGS} -c -o $@ $+

xmrcv.host.o: xmrcv.c
	${HOSTCC} -nostdinc ${HOSTCFLAGS} -c -o $@ $+

libtest.output: libtest.std libtest.input
	./libtest.std < libtest.input > libtest.output

check: libtest libtest.input libtest.output
	./libtest < libtest.input | diff -u libtest.output -

