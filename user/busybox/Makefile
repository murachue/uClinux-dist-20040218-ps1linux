# Makefile for busybox
#
# Copyright (C) 1999-2002 Erik Andersen <andersee@debian.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

PROG      := busybox
VERSION   := 0.60.5
BUILDTIME := $(shell TZ=UTC date -u "+%Y.%m.%d-%H:%M%z")
export VERSION

# With a modern GNU make(1) (highly recommended, that's what all the
# developers use), all of the following configuration values can be
# overridden at the command line.  For example:
#   make CROSS=powerpc-linux- BB_SRC_DIR=$HOME/busybox PREFIX=/mnt/app

# If you want to add some simple compiler switches (like -march=i686),
# especially from the command line, use this instead of CFLAGS directly.
# For optimization overrides, it's better still to set OPTIMIZATION.
CFLAGS_EXTRA =
 
# If you want a static binary, turn this on.
DOSTATIC = false

# Set the following to `true' to make a debuggable build.
# Leave this set to `false' for production use.
DODEBUG = false

# Setting this to `true' will cause busybox to directly use the system's
# password and group functions.  Assuming you use GNU libc, when this is
# `true', you will need to install the /etc/nsswitch.conf configuration file
# and the required libnss_* libraries. This generally makes your embedded
# system quite a bit larger... If you leave this off, busybox will directly use
# the /etc/password, /etc/group files (and your system will be smaller, and I
# will get fewer emails asking about how glibc NSS works).  Enabling this adds
# just 1.4k to the binary size (which is a _lot_ less then glibc NSS costs).
# Note that if you want hostname resolution to work with glibc, you still need
# the libnss_* libraries.  
USE_SYSTEM_PWD_GRP = true

# This enables compiling with dmalloc ( http://dmalloc.com/ )
# which is an excellent public domain mem leak and malloc problem
# detector.  To enable dmalloc, before running busybox you will
# want to first set up your environment.
# eg: `export DMALLOC_OPTIONS=debug=0x34f47d83,inter=100,log=logfile`
# The debug= value is generated using the following command
# dmalloc -p log-stats -p log-non-free -p log-bad-space -p log-elapsed-time \
#      -p check-fence -p check-heap -p check-lists -p check-blank \
#      -p check-funcs -p realloc-copy -p allow-free-null
# Do not enable this for production builds...
DODMALLOC = false

# Electric-fence is another very useful malloc debugging library.
# Do not enable this for production builds...
DOEFENCE  = false

# If you want large file summit support, turn this on.
# This has no effect if you don't have a kernel with lfs
# support, and a system with libc-2.1.3 or later.
# Some of the programs that can benefit from lfs support
# are dd, gzip, mount, tar, and mkfs_minix.
# LFS allows you to use the above programs for files
# larger than 2GB!
DOLFS = false

# If you have a "pristine" source directory, point BB_SRC_DIR to it.
# Experimental and incomplete; tell the mailing list
# <busybox@busybox.net> if you do or don't like it so far.
BB_SRC_DIR =

# If you are running a cross compiler, you may want to set this
# to something more interesting, like "powerpc-linux-".
ifdef NOT_IN_UCLINUX_DIST
CROSS =
CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip
endif

FLTFLAGS += -s 20000

# Becuase Config.h is always imported as part of Busybox, we set it
# up so that we can use a copy of it that gets modified
CONFIG_H=config.h

#
# To compile vs uClibc, just use the compiler wrapper built by uClibc...
# Everything should compile and work as expected these days...
#CC=/usr/i386-linux-uclibc/bin/i386-uclibc-gcc

# To compile vs some other alternative libc, you may need to use/adjust
# the following lines to meet your needs...
#
# If you are using Red Hat 6.x with the compatible RPMs (for developing under
# Red Hat 5.x and glibc 2.0) uncomment the following.  Be sure to read about
# using the compatible RPMs (compat-*) at http://www.redhat.com !
#LIBCDIR=/usr/i386-glibc20-linux
#
# The following is used for libc5 (if you install altgcc and libc5-altdev
# on a Debian system).  
#LIBCDIR=/usr/i486-linuxlibc1
#
# For other libraries, you are on your own...
#LDFLAGS+=-nostdlib
#LIBRARIES = $(LIBCDIR)/lib/libc.a -lgcc
#CROSS_CFLAGS+=-nostdinc -I$(LIBCDIR)/include -I$(GCCINCDIR)
#GCCINCDIR = $(shell gcc -print-search-dirs | sed -ne "s/install: \(.*\)/\1include/gp")

WARNINGS = -Wall -Wshadow

ARFLAGS = -r

LIBRARIES = 
INCLUDES = 

#

TARGET_ARCH:=${shell $(CC) -dumpmachine | sed -e s'/-.*//' \
		-e 's/i.86/i386/' \
		-e 's/sparc.*/sparc/' \
		-e 's/arm.*/arm/g' \
		-e 's/m68k.*/m68k/' \
		-e 's/ppc/powerpc/g' \
		-e 's/v850.*/v850/g' \
		-e 's/sh[234]/sh/' \
		-e 's/mips.*/mips/' \
		}

#--------------------------------------------------------
# Arch specific compiler optimization stuff should go here.
# Unless you want to override the defaults, do not set anything
# for OPTIMIZATION...

# use '-Os' optimization if available, else use -O2
OPTIMIZATION := ${shell if $(CC) -Os -S -o /dev/null -xc /dev/null >/dev/null 2>&1; \
	then echo "-Os"; else echo "-O2" ; fi}

# Some nice architecture specific optimizations
ifeq ($(strip $(TARGET_ARCH)),arm)
	OPTIMIZATION+=-fstrict-aliasing
endif
ifeq ($(strip $(TARGET_ARCH)),i386)
	OPTIMIZATION+=-march=i386
	OPTIMIZATION+=${shell if $(CC) -mpreferred-stack-boundary=2 -S -o /dev/null -xc \
		/dev/null >/dev/null 2>&1; then echo "-mpreferred-stack-boundary=2"; fi}
	OPTIMIZATION+=${shell if $(CC) -falign-functions=1 -falign-jumps=0 -falign-loops=0 \
		-S -o /dev/null -xc /dev/null >/dev/null 2>&1; then echo \
		"-falign-functions=1 -falign-jumps=0 -falign-loops=0"; else \
		if $(CC) -malign-functions=0 -malign-jumps=0 -S -o /dev/null -xc \
		/dev/null >/dev/null 2>&1; then echo "-malign-functions=0 -malign-jumps=0"; fi; fi}
endif
OPTIMIZATIONS:=$(OPTIMIZATION) -fomit-frame-pointer


#--------------------------------------------------------
# If you're going to do a lot of builds with a non-vanilla configuration,
# it makes sense to adjust parameters above, so you can type "make"
# by itself, instead of following it by the same half-dozen overrides
# every time.  The stuff below, on the other hand, is probably less
# prone to casual user adjustment.
# 
ifeq ($(strip $(DOLFS)),true)
    # For large file support
    CFLAGS+=-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
endif
ifeq ($(strip $(DODMALLOC)),true)
    # For testing mem leaks with dmalloc
    CFLAGS+=-DDMALLOC
    LIBRARIES += -ldmalloc
    # Force debug=true, since this is useless when not debugging...
    DODEBUG = true
else
    ifeq ($(strip $(DOEFENCE)),true)
	LIBRARIES += -lefence
	# Force debug=true, since this is useless when not debugging...
	DODEBUG = true
    endif
endif
ifeq ($(strip $(DODEBUG)),true)
    CFLAGS  += $(WARNINGS) -g -D_GNU_SOURCE
    LDFLAGS += -Wl,-warn-common
    #STRIPCMD    = /bin/true -Since_we_are_debugging
    STRIPCMD = : dont_strip
else
    CFLAGS  += $(WARNINGS) $(OPTIMIZATIONS) -D_GNU_SOURCE
    LDFLAGS += -Wl,-warn-common
    #STRIPCMD    = $(STRIP) -s --remove-section=.note --remove-section=.comment
    STRIPCMD = : dont_strip
endif
ifeq ($(strip $(DOSTATIC)),true)
    LDFLAGS += --static
    #
    #use '-ffunction-sections -fdata-sections' and '--gc-sections' (if they 
    # work) to try and strip out any unused junk.  Doesn't do much for me, 
    # but you may want to give it a shot...
    #
    #ifeq ($(shell $(CC) -ffunction-sections -fdata-sections -S \
    #	-o /dev/null -xc /dev/null 2>/dev/null && $(LD) \
    #			--gc-sections -v >/dev/null && echo 1),1)
    #	CFLAGS += -ffunction-sections -fdata-sections
    #	LDFLAGS += --gc-sections
    #endif
endif

ifeq ($(strip $(PREFIX)),)
    PREFIX = `pwd`/_install
endif

# Additional complications due to support for pristine source dir.
# Include files in the build directory should take precedence over
# the copy in BB_SRC_DIR, both during the compilation phase and the
# shell script that finds the list of object files.
# Work in progress by <ldoolitt@recycle.lbl.gov>.
#
ifneq ($(strip $(BB_SRC_DIR)),)
    VPATH = $(BB_SRC_DIR)
endif
#ifneq ($(strip $(VPATH)),)
#    CFLAGS += -I- -I. $(patsubst %,-I%,$(subst :, ,$(VPATH)))
#endif

# We need to set APPLET_SOURCES to something like
#   $(shell busybox.sh $(CONFIG_H))
# but in a manner that works with VPATH and BB_SRC_DIR.
# Possible ways to approach this:
#
#   1. Explicitly search through .:$(VPATH) for busybox.sh and config.h,
#      then $(shell $(BUSYBOX_SH) $(CONFIG_H) $(BB_SRC_DIR))
#
#   2. Explicity search through .:$(VPATH) for slist.mk,
#      then $(shell $(MAKE) -f $(SLIST_MK) VPATH=$(VPATH) BB_SRC_DIR=$(BB_SRC_DIR))
#
#   3. Create slist.mk in this directory, with commands embedded in
#      a $(shell ...) command, and $(MAKE) it immediately.
#
#   4. Use a real rule within this makefile to create a file that sets 
#      APPLET_SOURCE_LIST, then include that file.  Has complications
#      with the first trip through the makefile (before processing the
#      include) trying to do too much, and a spurious warning the first
#      time make is run.
#
# This is option 3:
#
#APPLET_SOURCES = $(shell \
#   echo -e 'all: busybox.sh $(CONFIG_H)\n\t@ $$(SHELL) $$^ $$(BB_SRC_DIR)' >slist.mk; \
#   make -f slist.mk VPATH=$(VPATH) BB_SRC_DIR=$(BB_SRC_DIR) \
#)
# And option 4:
-include applet_source_list

OBJECTS   = $(APPLET_SOURCES:.c=.o) busybox.o usage.o applets.o
CFLAGS    += $(CROSS_CFLAGS)
CFLAGS    += -DBB_VER='"$(VERSION)"'
CFLAGS    += -DBB_BT='"$(BUILDTIME)"'
CFLAGS    += -DCONFIG_H='"$(CONFIG_H)"'
ifdef BB_INIT_SCRIPT
    CFLAGS += -DINIT_SCRIPT='"$(BB_INIT_SCRIPT)"'
endif

ifneq ($(strip $(USE_SYSTEM_PWD_GRP)),true)
    PWD_GRP	= pwd_grp
    PWD_GRP_DIR = $(BB_SRC_DIR:=/)$(PWD_GRP)
    PWD_LIB     = libpwd.a
    PWD_CSRC=__getpwent.c pwent.c getpwnam.c getpwuid.c putpwent.c getpw.c \
	    fgetpwent.c __getgrent.c grent.c getgrnam.c getgrgid.c fgetgrent.c \
	    initgroups.c setgroups.c
    PWD_OBJS=$(patsubst %.c,$(PWD_GRP)/%.o, $(PWD_CSRC))
ifneq ($(strip $(BB_SRC_DIR)),)
    PWD_CFLAGS = -I- -I.
endif
    PWD_CFLAGS += -I$(PWD_GRP_DIR)
else
    CFLAGS    += -DUSE_SYSTEM_PWD_GRP
endif
    
LIBBB	  = libbb
LIBBB_LIB = libbb.a
LIBBB_CSRC= ask_confirmation.c chomp.c concat_path_file.c copy_file.c \
copy_file_chunk.c libc5.c device_open.c error_msg.c inode_hash.c \
error_msg_and_die.c fgets_str.c find_mount_point.c find_pid_by_name.c \
find_root_device.c full_read.c full_write.c get_console.c \
get_last_path_component.c get_line_from_file.c gz_open.c human_readable.c \
isdirectory.c kernel_version.c loop.c mode_string.c module_syscalls.c mtab.c \
mtab_file.c my_getgrnam.c my_getgrgid.c my_getpwnam.c my_getpwnamegid.c \
my_getpwuid.c parse_mode.c parse_number.c perror_msg.c perror_msg_and_die.c \
print_file.c process_escape_sequence.c read_package_field.c recursive_action.c \
safe_read.c safe_strncpy.c syscalls.c syslog_msg_with_name.c time_string.c \
trim.c unzip.c vdprintf.c verror_msg.c vperror_msg.c wfopen.c xfuncs.c \
xgetcwd.c xreadlink.c xregcomp.c interface.c remove_file.c last_char_is.c \
copyfd.c vherror_msg.c herror_msg.c herror_msg_and_die.c xgethostbyname.c \
dirname.c make_directory.c create_icmp_socket.c u_signal_names.c arith.c \
simplify_path.c
LIBBB_OBJS=$(patsubst %.c,$(LIBBB)/%.o, $(LIBBB_CSRC))
ifeq ($(strip $(BB_SRC_DIR)),)
    LIBBB_CFLAGS += -I$(LIBBB)
else
    LIBBB_CFLAGS = -I- -I. -I./$(LIBBB) -I$(BB_SRC_DIR)/$(LIBBB) -I$(BB_SRC_DIR)
endif


LIBBB_MSRC=libbb/messages.c
LIBBB_MESSAGES= full_version name_too_long omitting_directory not_a_directory \
memory_exhausted invalid_date invalid_option io_error dash_dash_help \
write_error too_few_args name_longer_than_foo unknown can_not_create_raw_socket
LIBBB_MOBJ=$(patsubst %,$(LIBBB)/%.o, $(LIBBB_MESSAGES))

LIBBB_ARCSRC=libbb/unarchive.c
LIBBB_ARCOBJ= archive_offset seek_sub_file extract_archive unarchive \
get_header_ar get_header_cpio get_header_tar deb_extract
LIBBB_AROBJS=$(patsubst %,$(LIBBB)/%.o, $(LIBBB_ARCOBJ))


# Put user-supplied flags at the end, where they
# have a chance of winning.
CFLAGS += $(CFLAGS_EXTRA)

.EXPORT_ALL_VARIABLES:

all:
	[ "$(CONFIG_USER_BUSYBOX_BUSYBOX)" != y ] || $(MAKE) build-config
	[ "$(CONFIG_USER_BUSYBOX_BUSYBOX)" != y ] || $(MAKE) applet_source_list
	[ "$(CONFIG_USER_BUSYBOX_BUSYBOX)" != y ] || $(MAKE) busybox
	[ "$(CONFIG_USER_BUSYBOX_BUSYBOX)" != y ] || $(MAKE) busybox.links
	[ "$(CONFIG_USER_BUSYBOX_ENV_STAND)" != y ] || $(MAKE) env
	[ "$(CONFIG_USER_BUSYBOX_BUSYBOX)" != y ] || $(MAKE) doc

applet_source_list: busybox.sh $(CONFIG_H)
ifdef NOT_IN_UCLINUX_DIST
	(echo -n "APPLET_SOURCES := "; CC="$(CC)" BB_SRC_DIR="$(BB_SRC_DIR)" $(SHELL) $^) > $@
else
	(echo -n "APPLET_SOURCES := "; BB_SRC_DIR=$(BB_SRC_DIR) $(SHELL) $^) > $@
endif

doc: olddoc

# Old Docs...
olddoc: docs/busybox.pod docs/BusyBox.txt docs/BusyBox.1 docs/BusyBox.html

docs/busybox.pod : docs/busybox_header.pod usage.h docs/busybox_footer.pod
	- ( cat docs/busybox_header.pod; \
	    docs/autodocifier.pl usage.h; \
	    cat docs/busybox_footer.pod ) > docs/busybox.pod

docs/BusyBox.txt: docs/busybox.pod
	@echo
	@echo BusyBox Documentation
	@echo
	-mkdir -p docs
	-pod2text $< > $@

docs/BusyBox.1: docs/busybox.pod
	- mkdir -p docs
	- pod2man --center=BusyBox --release="version $(VERSION)" \
		$< > $@

docs/BusyBox.html: docs/busybox.pod
	-@ mkdir -p docs
	-  pod2html --noindex $< > docs/BusyBox.html
	-@ rm -f pod2htm*


# New docs based on DOCBOOK SGML
newdoc: docs/busybox.txt docs/busybox.pdf docs/busybox/busyboxdocumentation.html

docs/busybox.txt: docs/busybox.sgml
	@echo
	@echo BusyBox Documentation
	@echo
	- mkdir -p docs
	(cd docs; sgmltools -b txt busybox.sgml)

docs/busybox.dvi: docs/busybox.sgml
	- mkdir -p docs
	(cd docs; sgmltools -b dvi busybox.sgml)

docs/busybox.ps: docs/busybox.sgml
	- mkdir -p docs
	(cd docs; sgmltools -b ps busybox.sgml)

docs/busybox.pdf: docs/busybox.ps
	- mkdir -p docs
	(cd docs; ps2pdf busybox.ps)

docs/busybox/busyboxdocumentation.html: docs/busybox.sgml
	- mkdir -p docs
	(cd docs; sgmltools -b html ../busybox.sgml)


busybox: $(PWD_LIB) $(LIBBB_LIB) $(OBJECTS) 
ifdef NOT_IN_UCLINUX_DIST
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBBB_LIB) $(PWD_LIB) $(LIBRARIES)
else
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(PWD_LIB) $(LIBBB_LIB) $(LIBRARIES) $(LDLIBS) $(LIBM) $(LDLIBS)
endif
	-$(STRIPCMD) $(PROG)

# Without VPATH, rule expands to "/bin/sh busybox.mkll $(CONFIG_H) applets.h"
# but with VPATH, some or all of those file names are resolved to the
# directories in which they live.
busybox.links: busybox.mkll $(CONFIG_H) applets.h
	- $(SHELL) $^ >$@

env: env_stand.o
	$(CC) $(LDFLAGS) -o $@ $< $(LIBRARIES) $(LDLIBS)

env_stand.o: env_stand.c $(CONFIG_H) busybox.h applets.h Makefile
ifeq ($(strip $(BB_SRC_DIR)),)
	$(CC) $(CFLAGS) -I. $(INCLUDES) $(patsubst %,-I%,$(subst :, ,$(BB_SRC_DIR))) -c $< -o $*.o
else
	$(CC) $(CFLAGS) -I- -I. $(patsubst %,-I%,$(subst :, ,$(BB_SRC_DIR))) -c $< -o $*.o
endif

nfsmount.o cmdedit.o: %.o: %.h
ash.o hush.o lash.o msh.o: cmdedit.h
$(OBJECTS): %.o: %.c $(CONFIG_H) busybox.h applets.h Makefile
ifeq ($(strip $(BB_SRC_DIR)),)
	$(CC) $(CFLAGS) -I. $(INCLUDES) $(patsubst %,-I%,$(subst :, ,$(BB_SRC_DIR))) -c $< -o $*.o
else
	$(CC) $(CFLAGS) -I- -I. $(patsubst %,-I%,$(subst :, ,$(BB_SRC_DIR))) -c $< -o $*.o
endif

$(PWD_OBJS): %.o: %.c $(CONFIG_H) busybox.h applets.h Makefile
ifneq ($(strip $(BB_SRC_DIR)),)
	-mkdir -p $(PWD_GRP)
endif
	$(CC) $(CFLAGS) $(PWD_CFLAGS) -c $< -o $*.o

$(LIBBB_OBJS): %.o: %.c $(CONFIG_H) busybox.h applets.h Makefile libbb/libbb.h
ifneq ($(strip $(BB_SRC_DIR)),)
	-mkdir -p $(LIBBB)
endif
	$(CC) $(CFLAGS) $(LIBBB_CFLAGS) -c $< -o $*.o

$(LIBBB_MOBJ): $(LIBBB_MSRC)
ifneq ($(strip $(BB_SRC_DIR)),)
	-mkdir -p $(LIBBB)
endif
	$(CC) $(CFLAGS) $(LIBBB_CFLAGS) -DL_$(patsubst libbb/%,%,$*) -c $< -o $*.o

$(LIBBB_AROBJS): $(LIBBB_ARCSRC)
ifneq ($(strip $(BB_SRC_DIR)),)
	-mkdir -p $(LIBBB)
endif
	$(CC) $(CFLAGS) $(LIBBB_CFLAGS) -DL_$(patsubst libbb/%,%,$*) -c $< -o $*.o

libpwd.a: $(PWD_OBJS)
	$(AR) $(ARFLAGS) $@ $^

libbb.a:  $(LIBBB_MOBJ) $(LIBBB_AROBJS) $(LIBBB_OBJS)
	$(AR) $(ARFLAGS) $@ $^

usage.o: usage.h

libbb/loop.o: libbb/loop.h

libbb/loop.h: mk_loop_h.sh
	@ $(SHELL) $< > $@

test tests:
	# old way of doing it
	#cd tests && $(MAKE) all
	# new way of doing it
	cd tests && ./tester.sh

clean:
	- cd tests && $(MAKE) clean
	- rm -f docs/BusyBox.txt docs/BusyBox.1 docs/BusyBox.html docs/BusyBox.html
	- rm -f docs/busybox.txt docs/busybox.dvi docs/busybox.ps \
	    docs/busybox.pdf docs/busybox.html docs/busybox.pod
	- rm -f multibuild.log $(CONFIG_H).orig *.gdb *.elf
	- rm -rf docs/busybox _install libpwd.a libbb.a pod2htm*
	- rm -f busybox busybox.links libbb/loop.h *~ slist.mk core applet_source_list
	- find . -name \*.o -exec rm -f {} \;
	- rm -f *.gdb *.elf
	- rm -f $(CONFIG_H)-
ifneq ($(CONFIG_H),Config.h)
	- rm -f $(CONFIG_H)
endif

distclean: clean
	- cd tests && $(MAKE) distclean
	- rm -f busybox applet_source_list

install: install.sh busybox busybox.links
	$(SHELL) $< $(PREFIX)

uninstall: busybox busybox.links
	for i in `cat busybox.links` ; do rm -f $$PREFIX$$i; done

install-hardlinks: install.sh busybox busybox.links
	$(SHELL) $< $(PREFIX) --hardlinks

debug_pristine:
	@ echo VPATH=\"$(VPATH)\"
	@ echo OBJECTS=\"$(OBJECTS)\"

dist release: distclean doc
	cd ..;					\
	rm -rf busybox-$(VERSION);		\
	cp -a busybox busybox-$(VERSION);	\
						\
	find busybox-$(VERSION)/ -type d	\
				 -name CVS	\
				 -print		\
		-exec rm -rf {} \; ;            \
						\
	find busybox-$(VERSION)/ -type f	\
				 -name .\#*	\
				 -print		\
		-exec rm -f {} \;  ;            \
						\
	tar -cvzf busybox-$(VERSION).tar.gz busybox-$(VERSION)/;

.PHONY: tags
tags:
	ctags -R .

.PHONY: syslog-install
syslog-install:
	$(ROMFSINST) -e CONFIG_USER_BUSYBOX_SYSLOGD -s /var/tmp/log /dev/log

.PHONY: env-stand-install
env-stand-install:
	$(ROMFSINST) -e CONFIG_USER_BUSYBOX_ENV_STAND /bin/env

ifdef CONFIG_UCLINUX
INSTALLER=install-romfs.sh
TARGETDIR=$(ROMFSDIR)/bin/
COPYPROG=cp
else
INSTALLER=install.sh
TARGETDIR=$(ROMFSDIR)
COPYPROG=:
endif

# Before copying, remove all existing busybox links
busybox-install: $(INSTALLER) busybox.links syslog-install
	if [ -f "$$ROMFSDIR/bin/busybox" ]; then \
		inode=`ls -i $$ROMFSDIR/bin/busybox | awk '{print $$1}'`; \
		ls -iL "$$ROMFSDIR/bin" | grep "^ *$$inode" | awk '{print $$2}' | \
			sed "s:^:$$ROMFSDIR/bin/:" | env -i xargs rm -f; \
	fi
	$(COPYPROG) $(PROG) $(ROMFSDIR)/bin/$(PROG)
	$(SHELL) $< $(TARGETDIR)

romfs:
	[ "$(CONFIG_USER_BUSYBOX_BUSYBOX)" != y ] || make busybox-install
	[ "$(CONFIG_USER_BUSYBOX_ENV_STAND)" != y ] || make env-stand-install

# Neat rule to build a new config.h and overwrite if it is different
# This avoids bulk busybox builds.
build-config $(CONFIG_H): build-config.awk
	awk -f build-config.awk Config.h >Config.temp
	if cmp -s Config.temp $(CONFIG_H)	 	;	\
	then						\
		rm -f Config.temp		;	\
	else						\
		mv -f $(CONFIG_H) $(CONFIG_H)-	;	\
		mv -f Config.temp $(CONFIG_H)	;	\
		rm -f applet_source_list	;	\
		make applet_source_list 	;	\
	fi

ifeq ($(strip $(CONFIG_COLDFIRE)),y)
insmod.o: insmod.c
	$(CC) $(CFLAGS:-Os=-O2) -c insmod.c
endif
