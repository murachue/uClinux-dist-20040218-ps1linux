# Makefile for uClibc
#
# Copyright (C) 2000-2003 Erik Andersen <andersen@uclibc.org>
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU Library General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU Library General Public License for more
# details.
#
# You should have received a copy of the GNU Library General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

TOPDIR=../../../../
include $(TOPDIR)Rules.mak
ASFLAGS=$(CFLAGS)

CRT0_SRC = crt0.S
CRT0_OBJ = crt0.o crt1.o
CTOR_TARGETS=$(TOPDIR)lib/crti.o $(TOPDIR)lib/crtn.o

SSRC=bsd-_setjmp.S bsd-setjmp.S setjmp.S clone.S fork.S syscall.S pipe.S \
	vfork.S
SOBJS=$(patsubst %.S,%.o, $(SSRC))

CSRC=__longjmp.c  brk.c setjmp_aux.c _mmap.c __syscall_error.c cacheflush.c \
	pread_write.c sysmips.c _test_and_set.c
COBJS=$(patsubst %.c,%.o, $(CSRC))

OBJS=$(SOBJS) $(MOBJ) $(COBJS)

all: $(OBJS) $(LIBC)

$(LIBC): ar-target 

ar-target: $(OBJS) $(CRT0_OBJ) $(CTOR_TARGETS)
	$(AR) $(ARFLAGS) $(LIBC) $(OBJS)
	cp $(CRT0_OBJ) $(TOPDIR)lib/

$(CRT0_OBJ): $(CRT0_SRC)
	$(CC) $(CFLAGS) -DL_$* $< -c -o $*.o
	$(STRIPTOOL) -x -R .note -R .comment $*.o

$(SOBJS): %.o : %.S
	$(CC) $(CFLAGS) -c $< -o $@
	$(STRIPTOOL) -x -R .note -R .comment $*.o

$(COBJS): %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
	$(STRIPTOOL) -x -R .note -R .comment $*.o


ifeq ($(strip $(UCLIBC_CTOR_DTOR)),y)
crti.o: crti.S
	$(CC) $(CFLAGS) -c crti.S -o crti.o

$(TOPDIR)lib/crti.o: crti.o
	$(INSTALL) -d $(TOPDIR)lib/
	cp crti.o $(TOPDIR)lib/

crtn.o: crtn.S
	$(CC) $(CFLAGS) -c crtn.S -o crtn.o

$(TOPDIR)lib/crtn.o: crtn.o
	$(INSTALL) -d $(TOPDIR)lib/
	cp crtn.o $(TOPDIR)lib/
else
$(TOPDIR)lib/crti.o:
	$(INSTALL) -d $(TOPDIR)lib/
	$(AR) $(ARFLAGS) $(TOPDIR)lib/crti.o
$(TOPDIR)lib/crtn.o:
	$(INSTALL) -d $(TOPDIR)lib/
	$(AR) $(ARFLAGS) $(TOPDIR)lib/crtn.o
endif


headers:
	$(LN) -fs ../libc/sysdeps/linux/mips/sgidefs.h $(TOPDIR)/include/
#	$(LN) -fs ../libc/sysdeps/linux/mips/regdef.h $(TOPDIR)/include/
	$(LN) -fs ../libc/sysdeps/linux/mips/fpu_control.h $(TOPDIR)/include/

clean:
	$(RM) *.[oa] *~ core
	$(RM) bits/sysnum.h
	$(RM) $(TOPDIR)/include/sgidefs.h
