
SH = sh
SHOBJS = sash.o cmds.o cmd_uclinux.o ls.o ps.o hexdump.o df.o free.o \
	hostname.o date.o

SHUTDOWN = shutdown
SHUTDOWNOBJS = shutdown.o

REBOOT = reboot
REBOOTOBJS = reboot.o

MOUNT = mount
MOUNTOBJS = mount.o

UMOUNT = umount
UMOUNTOBJS = umount.o

LIBSASH = libsash/libsash.a

ifdef CONFIG_USER_SASH_HISTORY
CFLAGS += -DCOMMAND_HISTORY
endif

all: $(SH) $(SHUTDOWN) $(REBOOT) $(MOUNT) $(UMOUNT)

$(SH): $(SHOBJS) $(LIBSASH)
	FLTFLAGS="$(FLTFLAGS) -s 8192" $(CC) $(LDFLAGS) -o $@ $(SHOBJS) $(LIBSASH) $(LDLIBS$(LDLIBS_$@))

$(SHUTDOWN): $(SHUTDOWNOBJS) $(LIBSASH)
	$(CC) $(LDFLAGS) -o $@ $(SHUTDOWNOBJS) $(LIBSASH) $(LDLIBS)

$(REBOOT): $(REBOOTOBJS) $(LIBSASH)
	$(CC) $(LDFLAGS) -o $@ $(REBOOTOBJS) $(LIBSASH) $(LDLIBS$(LDLIBS_$@))

mount.o: mount.c cmds.c sash.h
umount.o: mount.c cmds.c sash.h

$(MOUNT): $(MOUNTOBJS)
	$(CC) $(LDFLAGS) -o $@ $(MOUNTOBJS) $(LDLIBS$(LDLIBS_$@))

$(UMOUNT): $(UMOUNTOBJS)
	$(CC) $(LDFLAGS) -o $@ $(UMOUNTOBJS) $(LDLIBS$(LDLIBS_$@))

romfs:
	$(ROMFSINST) -e CONFIG_USER_SASH_SH       /bin/sh
	$(ROMFSINST) -e CONFIG_USER_SASH_REBOOT /bin/reboot
	$(ROMFSINST) -e CONFIG_USER_SASH_REBOOT_SNAPGEAR reboot-snapgear.sh /bin/reboot
	$(ROMFSINST) -e CONFIG_USER_SASH_SHUTDOWN /bin/shutdown
	$(ROMFSINST) -e CONFIG_USER_SASH_MOUNT /bin/mount
	$(ROMFSINST) -e CONFIG_USER_SASH_UMOUNT /bin/umount

dummy_target:

$(LIBSASH): dummy_target
	$(MAKE) -C libsash

clean:
	-rm -f $(SH) $(SHUTDOWN) $(REBOOT) $(MOUNT) $(UMOUNT) *.elf *.gdb *.o
	$(MAKE) -C libsash clean

