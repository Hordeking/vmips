## Process this file with automake to produce Makefile.in.

SUBDIRS = . sample_code test_code doc 
REDHAT_PKG_FILES = vmips.spec vmips.spec.in
DEBIAN_PKG_FILES = debian/changelog debian/conffiles debian/control \
 debian/copyright debian/dirs debian/docs debian/info debian/rules
EXTRA_DIST = VERSION include/dis-asm.h $(REDHAT_PKG_FILES) join.sh \
 $(sysconf_DATA) $(DEBIAN_PKG_FILES) buildstat.sh.in

# What are we building here?
bin_PROGRAMS = vmips vmipstool

# Defaults! Defaults for everyone!
CXXFLAGS = -DSYSCONFDIR=\"$(sysconfdir)\" \
  $(mipsincdirs) -I$(top_srcdir)/include @CXXFLAGS@
MOSTLYCLEANFILES = errs memdump.bin
CLEANFILES = romfile.rom vmips.out

# How to build the simulator:
vmips_SOURCES = cpu.cc cpzero.cc devicemap.cc \
  mapper.cc memorymodule.cc options.cc range.cc intctrl.cc \
  spimconsole.cc stub-dis.cc tlbentry.cc vmips.cc \
  deviceint.cc debug.cc remotegdb.cc clockdev.cc error.cc \
  clock.cc task.cc terminalcontroller.cc haltdev.cc \
  cpu.h cpzero.h cpzeroreg.h deviceint.h \
  devicemap.h intctrl.h mapper.h memorymodule.h options.h optiontbl.h \
  range.h regnames.h spimconsole.h spimconsreg.h \
  sysinclude.h tlbentry.h vmips.h debug.h remotegdb.h \
  accesstypes.h deviceexc.h clockdev.h excnames.h error.h \
  clock.h task.h terminalcontroller.h haltdev.h devreg.h clockreg.h \
  haltreg.h wipe.h
vmips_LDADD = $(mipslibdirs) -lopcodes -lbfd -liberty
vmips_LDFLAGS = -static

# Huh, huhuh, it says tool.
vmipstool_SOURCES = vmipstool.cc
vmipstool_CXXFLAGS = -DPKGDATADIR=\"$(pkgdatadir)\"
vmipstool.o: vmipstool.cc
	$(CXXCOMPILE) $(vmipstool_CXXFLAGS) -c $<

# Install a sample vmipsrc.
sysconf_DATA = vmipsrc

# Miscellaneous targets.
tags: TAGS
	ctags *.cc

