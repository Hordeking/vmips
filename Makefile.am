## Process this file with automake to produce Makefile.in.
# Makefile.in created from: $Id: Makefile.am,v 1.23 2001/04/06 08:56:20 brg Exp $

SUBDIRS = . test_code doc
EXTRA_DIST = VERSION include/dis-asm.h package/vmips.spec.in

# What are we building here?
bin_PROGRAMS = vmips

# Other things to install.
bin_SCRIPTS = vmipstool

# Defaults! Defaults for everyone!
CXXFLAGS = -DCONFIG_PREFIX=\"$(prefix)\" -I$(top_srcdir)/include \
  -I$(mipsinclude) @CXXFLAGS@
MOSTLYCLEANFILES = errs memdump.bin
CLEANFILES = romfile.rom vmips.out

# How to build the simulator:
vmips_SOURCES = cache.cc cacheline.cc cpu.cc cpzero.cc devicemap.cc \
  mapper.cc memorymodule.cc options.cc range.cc serialhost.cc intctrl.cc \
  spimconsole.cc stub-dis.cc testdev.cc tlbentry.cc vmips.cc zschip.cc \
  deviceint.cc debug.cc remotegdb.cc \
  cache.h cacheline.h cpu.h cpzero.h cpzeroreg.h deviceint.h \
  devicemap.h intctrl.h mapper.h memorymodule.h options.h optiontbl.h \
  periodic.h range.h regnames.h serialhost.h spimconsole.h spimconsreg.h \
  sysinclude.h testdev.h tlbentry.h vmips.h zschip.h debug.h remotegdb.h \
  accesstypes.h deviceexc.h
vmips_LDADD = $(mipslib)/libopcodes.la $(mipslib)/libbfd.la \
  $(mipslib)/libiberty.a
vmips_LDFLAGS = -static

# Miscellaneous targets.
tags: TAGS
	ctags *.cc

