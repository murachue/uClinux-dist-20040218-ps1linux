/* vi: set sw=4 ts=4: */
/*
 * mkfs.c - make a linux (minix) file-system.
 *
 * (C) 1991 Linus Torvalds. This file may be redistributed as per
 * the Linux copyright.
 */

/*
 * DD.MM.YY
 *
 * 24.11.91  -	Time began. Used the fsck sources to get started.
 *
 * 25.11.91  -	Corrected some bugs. Added support for ".badblocks"
 *		The algorithm for ".badblocks" is a bit weird, but
 *		it should work. Oh, well.
 *
 * 25.01.92  -	Added the -l option for getting the list of bad blocks
 *		out of a named file. (Dave Rivers, rivers@ponds.uucp)
 *
 * 28.02.92  -	Added %-information when using -c.
 *
 * 28.02.93  -	Added support for other namelengths than the original
 *		14 characters so that I can test the new kernel routines..
 *
 * 09.10.93  -	Make exit status conform to that required by fsutil
 *		(Rik Faith, faith@cs.unc.edu)
 *
 * 31.10.93  -	Added inode request feature, for backup floppies: use
 *		32 inodes, for a news partition use more.
 *		(Scott Heavner, sdh@po.cwru.edu)
 *
 * 03.01.94  -	Added support for file system valid flag.
 *		(Dr. Wettstein, greg%wind.uucp@plains.nodak.edu)
 *
 * 30.10.94 - added support for v2 filesystem
 *	      (Andreas Schwab, schwab@issan.informatik.uni-dortmund.de)
 * 
 * 09.11.94  -	Added test to prevent overwrite of mounted fs adapted
 *		from Theodore Ts'o's (tytso@athena.mit.edu) mke2fs
 *		program.  (Daniel Quinlan, quinlan@yggdrasil.com)
 *
 * 03.20.95  -	Clear first 512 bytes of filesystem to make certain that
 *		the filesystem is not misidentified as a MS-DOS FAT filesystem.
 *		(Daniel Quinlan, quinlan@yggdrasil.com)
 *
 * 02.07.96  -  Added small patch from Russell King to make the program a
 *		good deal more portable (janl@math.uio.no)
 *
 * 03.14.25  -	add make-from-directory function.
 *
 * Usage:  mkfs [-c | -l filename ] [-v] [-nXX] [-iXX] [-d dirname] device [size-in-blocks]
 *
 *	-c for readablility checking (SLOW!)
 *      -l for getting a list of bad blocks from a file.
 *	-n for namelength (currently the kernel only uses 14 or 30)
 *	-i for number of inodes
 *	-v for v2 filesystem
 *	-d for make filesystem from that directory
 *
 * The device may be a block device or a image of one, but this isn't
 * enforced (but it's not much fun on a character device :-). 
 *
 * Modified for BusyBox by Erik Andersen <andersen@debian.org> --
 *	removed getopt based parser and added a hand rolled one.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <mntent.h>
#include <sys/types.h>
#include <dirent.h>
//#include "busybox.h"
#define xmalloc malloc
#define xfopen fopen
#include <stdarg.h>
void error_msg_and_die(const char *fmt, ...){va_list p;va_start(p,fmt);vfprintf(stderr,fmt,p);va_end(p);fputs("\n",stderr);exit(1);}
void perror_msg_and_die(const char *fmt, ...){va_list p;va_start(p,fmt);vfprintf(stderr,fmt,p);perror("");va_end(p);exit(1);}
void error_msg(const char *fmt, ...){va_list p;va_start(p,fmt);vfprintf(stderr,fmt,p);va_end(p);}
#include <sys/stat.h>
void show_usage(void){fprintf(stderr, /*"%s\n\n"*/"Usage: %s %s\n\n", /*"",*/ "mkfs.minix", "[-c | -l filename] [-nXX] [-iXX] [-d dirname] /dev/name [blocks]" "\n\n"
	"Make a MINIX filesystem.\n\n"
	"Options:\n"
	"\t-c\t\tCheck the device for bad blocks\n"
	"\t-n [14|30]\tSpecify the maximum length of filenames\n"
	"\t-i INODES\tSpecify the number of inodes for the filesystem\n"
	"\t-l FILENAME\tRead the bad blocks list from FILENAME\n"
	"\t-v\t\tMake a Minix version 2 filesystem\n"
	"\t-d DIRNAME\tMake a filesystem from DIRNAME"
);exit(1);}
#define FALSE   ((int) 0)
#define TRUE    ((int) 1)
#define BB_FEATURE_MINIX2 1

#define MINIX_ROOT_INO 1
#define MINIX_LINK_MAX	250
#define MINIX2_LINK_MAX	65530

#define MINIX_I_MAP_SLOTS	8
#define MINIX_Z_MAP_SLOTS	64
#define MINIX_SUPER_MAGIC	0x137F		/* original minix fs */
#define MINIX_SUPER_MAGIC2	0x138F		/* minix fs, 30 char names */
#define MINIX2_SUPER_MAGIC	0x2468		/* minix V2 fs */
#define MINIX2_SUPER_MAGIC2	0x2478		/* minix V2 fs, 30 char names */
#define MINIX_VALID_FS		0x0001		/* Clean fs. */
#define MINIX_ERROR_FS		0x0002		/* fs has errors. */

#define MINIX_INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct minix_inode)))
#define MINIX2_INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct minix2_inode)))

#define MINIX_V1		0x0001		/* original minix fs */
#define MINIX_V2		0x0002		/* minix V2 fs */

#define INODE_VERSION(inode)	inode->i_sb->u.minix_sb.s_version

/*
 * This is the original minix inode layout on disk.
 * Note the 8-bit gid and atime and ctime.
 */
struct minix_inode {
	u_int16_t i_mode;
	u_int16_t i_uid;
	u_int32_t i_size;
	u_int32_t i_time;
	u_int8_t  i_gid;
	u_int8_t  i_nlinks;
	u_int16_t i_zone[9];
};

/*
 * The new minix inode has all the time entries, as well as
 * long block numbers and a third indirect block (7+1+1+1
 * instead of 7+1+1). Also, some previously 8-bit values are
 * now 16-bit. The inode is now 64 bytes instead of 32.
 */
struct minix2_inode {
	u_int16_t i_mode;
	u_int16_t i_nlinks;
	u_int16_t i_uid;
	u_int16_t i_gid;
	u_int32_t i_size;
	u_int32_t i_atime;
	u_int32_t i_mtime;
	u_int32_t i_ctime;
	u_int32_t i_zone[10];
};

/*
 * minix super-block data on disk
 */
struct minix_super_block {
	u_int16_t s_ninodes;
	u_int16_t s_nzones;
	u_int16_t s_imap_blocks;
	u_int16_t s_zmap_blocks;
	u_int16_t s_firstdatazone;
	u_int16_t s_log_zone_size;
	u_int32_t s_max_size;
	u_int16_t s_magic;
	u_int16_t s_state;
	u_int32_t s_zones;
};

struct minix_dir_entry {
	u_int16_t inode;
	char name[0];
};

#define BLOCK_SIZE_BITS 10
#define BLOCK_SIZE (1<<BLOCK_SIZE_BITS)

#define NAME_MAX         255   /* # chars in a file name */

#define MINIX_INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct minix_inode)))

#define MINIX_VALID_FS               0x0001          /* Clean fs. */
#define MINIX_ERROR_FS               0x0002          /* fs has errors. */

#define MINIX_SUPER_MAGIC    0x137F          /* original minix fs */
#define MINIX_SUPER_MAGIC2   0x138F          /* minix fs, 30 char names */

#ifndef BLKGETSIZE
#define BLKGETSIZE _IO(0x12,96)    /* return device size */
#endif


#ifndef __linux__
#define volatile
#endif

#define MINIX_ROOT_INO 1
#define MINIX_BAD_INO 2

#define TEST_BUFFER_BLOCKS 16
#define MAX_GOOD_BLOCKS 512

#define UPPER(size,n) (((size)+((n)-1))/(n))
#define INODE_SIZE (sizeof(struct minix_inode))
#ifdef BB_FEATURE_MINIX2
#define INODE_SIZE2 (sizeof(struct minix2_inode))
#define INODE_BLOCKS UPPER(INODES, (version2 ? MINIX2_INODES_PER_BLOCK \
				    : MINIX_INODES_PER_BLOCK))
#else
#define INODE_BLOCKS UPPER(INODES, (MINIX_INODES_PER_BLOCK))
#endif
#define INODE_BUFFER_SIZE (INODE_BLOCKS * BLOCK_SIZE)

#define BITS_PER_BLOCK (BLOCK_SIZE<<3)

static char *device_name = NULL;
static int DEV = -1;
static long BLOCKS = 0;
static int check = 0;
static int badblocks = 0;
static int namelen = 30;		/* default (changed to 30, per Linus's

								   suggestion, Sun Nov 21 08:05:07 1993) */
static int dirsize = 32;
static int magic = MINIX_SUPER_MAGIC2;
static int version2 = 0;

static char root_block[BLOCK_SIZE] = "\0";

static char *inode_buffer = NULL;

#define Inode (((struct minix_inode *) inode_buffer)-1)
#ifdef BB_FEATURE_MINIX2
#define Inode2 (((struct minix2_inode *) inode_buffer)-1)
#endif
static char super_block_buffer[BLOCK_SIZE];
static char boot_block_buffer[512];

#define Super (*(struct minix_super_block *)super_block_buffer)
#define INODES (/*(unsigned long)*/Super.s_ninodes)
#ifdef BB_FEATURE_MINIX2
#define ZONES (/*(unsigned long)*/(version2 ? Super.s_zones : Super.s_nzones))
#else
#define ZONES (/*(unsigned long)*/(Super.s_nzones))
#endif
#define IMAPS (/*(unsigned long)*/Super.s_imap_blocks)
#define ZMAPS (/*(unsigned long)*/Super.s_zmap_blocks)
#define FIRSTZONE (/*(unsigned long)*/Super.s_firstdatazone)
#define ZONESIZE (/*(unsigned long)*/Super.s_log_zone_size)
#define MAXSIZE (/*(unsigned long)*/Super.s_max_size)
#define MAGIC (Super.s_magic)
#define NORM_FIRSTZONE (2+IMAPS+ZMAPS+INODE_BLOCKS)

static char *inode_map;
static char *zone_map;

static unsigned short good_blocks_table[MAX_GOOD_BLOCKS];
static int used_good_blocks = 0;
static unsigned long req_nr_inodes = 0;
static int last_alloc_inode = 0;
static int last_alloc_zone = 0;

static inline int bit(char * a,unsigned int i)
{
	  return (a[i >> 3] & (1<<(i & 7))) != 0;
}
#define inode_in_use(x) (bit(inode_map,(x)))
#define zone_in_use(x) (bit(zone_map,(x)-FIRSTZONE+1))

#define mark_inode(x) (setbit(inode_map,(x)))
#define unmark_inode(x) (clrbit(inode_map,(x)))

#define mark_zone(x) (setbit(zone_map,(x)-FIRSTZONE+1))
#define unmark_zone(x) (clrbit(zone_map,(x)-FIRSTZONE+1))

/*
 * Check to make certain that our new filesystem won't be created on
 * an already mounted partition.  Code adapted from mke2fs, Copyright
 * (C) 1994 Theodore Ts'o.  Also licensed under GPL.
 */
static void check_mount(void)
{
	FILE *f;
	struct mntent *mnt;

	if ((f = setmntent(MOUNTED, "r")) == NULL)
		return;
	while ((mnt = getmntent(f)) != NULL)
		if (strcmp(device_name, mnt->mnt_fsname) == 0)
			break;
	endmntent(f);
	if (!mnt)
		return;

	error_msg_and_die("%s is mounted; will not make a filesystem here!", device_name);
}

static long valid_offset(int fd, int offset)
{
	char ch;

	if (lseek(fd, offset, 0) < 0)
		return 0;
	if (read(fd, &ch, 1) < 1)
		return 0;
	return 1;
}

static int count_blocks(int fd)
{
	int high, low;

	low = 0;
	for (high = 1; valid_offset(fd, high); high *= 2)
		low = high;
	while (low < high - 1) {
		const int mid = (low + high) / 2;

		if (valid_offset(fd, mid))
			low = mid;
		else
			high = mid;
	}
	valid_offset(fd, 0);
	return (low + 1);
}

static int get_size(const char *file)
{
	int fd;
	long size;

	if ((fd = open(file, O_RDWR)) < 0)
		perror_msg_and_die("%s", file);
	if (ioctl(fd, BLKGETSIZE, &size) >= 0) {
		close(fd);
		return (size * 512);
	}

	size = count_blocks(fd);
	close(fd);
	return size;
}

static void write_tables(void)
{
	/* Mark the super block valid. */
	Super.s_state |= MINIX_VALID_FS;
	Super.s_state &= ~MINIX_ERROR_FS;

	if (lseek(DEV, 0, SEEK_SET))
		error_msg_and_die("seek to boot block failed in write_tables");
	if (512 != write(DEV, boot_block_buffer, 512))
		error_msg_and_die("unable to clear boot sector");
	if (BLOCK_SIZE != lseek(DEV, BLOCK_SIZE, SEEK_SET))
		error_msg_and_die("seek failed in write_tables");
	if (BLOCK_SIZE != write(DEV, super_block_buffer, BLOCK_SIZE))
		error_msg_and_die("unable to write super-block");
	if (IMAPS * BLOCK_SIZE != write(DEV, inode_map, IMAPS * BLOCK_SIZE))
		error_msg_and_die("unable to write inode map");
	if (ZMAPS * BLOCK_SIZE != write(DEV, zone_map, ZMAPS * BLOCK_SIZE))
		error_msg_and_die("unable to write zone map");
	if (INODE_BUFFER_SIZE != write(DEV, inode_buffer, INODE_BUFFER_SIZE))
		error_msg_and_die("unable to write inodes");

}

static void write_block(int blk, char *buffer)
{
	if (blk * BLOCK_SIZE != lseek(DEV, blk * BLOCK_SIZE, SEEK_SET))
		error_msg_and_die("seek failed in write_block");
	if (BLOCK_SIZE != write(DEV, buffer, BLOCK_SIZE))
		error_msg_and_die("write failed in write_block");
}

static int get_free_block(void)
{
	int blk;

	if (used_good_blocks + 1 >= MAX_GOOD_BLOCKS)
		error_msg_and_die("too many bad blocks");
	if (used_good_blocks)
		blk = good_blocks_table[used_good_blocks - 1] + 1;
	else
		blk = FIRSTZONE;
	while (blk < ZONES && zone_in_use(blk))
		blk++;
	if (blk >= ZONES)
		error_msg_and_die("not enough good blocks");
	good_blocks_table[used_good_blocks] = blk;
	used_good_blocks++;
	return blk;
}

static void mark_good_blocks(void)
{
	int blk;

	for (blk = 0; blk < used_good_blocks; blk++)
		mark_zone(good_blocks_table[blk]);
}

static int next(int zone)
{
	if (!zone)
		zone = FIRSTZONE - 1;
	while (++zone < ZONES)
		if (zone_in_use(zone))
			return zone;
	return 0;
}

static void make_bad_inode(void)
{
	struct minix_inode *inode = &Inode[MINIX_BAD_INO];
	int i, j, zone;
	int ind = 0, dind = 0;
	unsigned short ind_block[BLOCK_SIZE >> 1];
	unsigned short dind_block[BLOCK_SIZE >> 1];

#define NEXT_BAD (zone = next(zone))

	if (!badblocks)
		return;
	mark_inode(MINIX_BAD_INO);
	inode->i_nlinks = 1;
	inode->i_time = time(NULL);
	inode->i_mode = S_IFREG + 0000;
	inode->i_size = badblocks * BLOCK_SIZE;
	zone = next(0);
	for (i = 0; i < 7; i++) {
		inode->i_zone[i] = zone;
		if (!NEXT_BAD)
			goto end_bad;
	}
	inode->i_zone[7] = ind = get_free_block();
	memset(ind_block, 0, BLOCK_SIZE);
	for (i = 0; i < 512; i++) {
		ind_block[i] = zone;
		if (!NEXT_BAD)
			goto end_bad;
	}
	inode->i_zone[8] = dind = get_free_block();
	memset(dind_block, 0, BLOCK_SIZE);
	for (i = 0; i < 512; i++) {
		write_block(ind, (char *) ind_block);
		dind_block[i] = ind = get_free_block();
		memset(ind_block, 0, BLOCK_SIZE);
		for (j = 0; j < 512; j++) {
			ind_block[j] = zone;
			if (!NEXT_BAD)
				goto end_bad;
		}
	}
	error_msg_and_die("too many bad blocks");
  end_bad:
	if (ind)
		write_block(ind, (char *) ind_block);
	if (dind)
		write_block(dind, (char *) dind_block);
}

#ifdef BB_FEATURE_MINIX2
static void make_bad_inode2(void)
{
	struct minix2_inode *inode = &Inode2[MINIX_BAD_INO];
	int i, j, zone;
	int ind = 0, dind = 0;
	unsigned long ind_block[BLOCK_SIZE >> 2];
	unsigned long dind_block[BLOCK_SIZE >> 2];

	if (!badblocks)
		return;
	mark_inode(MINIX_BAD_INO);
	inode->i_nlinks = 1;
	inode->i_atime = inode->i_mtime = inode->i_ctime = time(NULL);
	inode->i_mode = S_IFREG + 0000;
	inode->i_size = badblocks * BLOCK_SIZE;
	zone = next(0);
	for (i = 0; i < 7; i++) {
		inode->i_zone[i] = zone;
		if (!NEXT_BAD)
			goto end_bad;
	}
	inode->i_zone[7] = ind = get_free_block();
	memset(ind_block, 0, BLOCK_SIZE);
	for (i = 0; i < 256; i++) {
		ind_block[i] = zone;
		if (!NEXT_BAD)
			goto end_bad;
	}
	inode->i_zone[8] = dind = get_free_block();
	memset(dind_block, 0, BLOCK_SIZE);
	for (i = 0; i < 256; i++) {
		write_block(ind, (char *) ind_block);
		dind_block[i] = ind = get_free_block();
		memset(ind_block, 0, BLOCK_SIZE);
		for (j = 0; j < 256; j++) {
			ind_block[j] = zone;
			if (!NEXT_BAD)
				goto end_bad;
		}
	}
	/* Could make triple indirect block here */
	error_msg_and_die("too many bad blocks");
  end_bad:
	if (ind)
		write_block(ind, (char *) ind_block);
	if (dind)
		write_block(dind, (char *) dind_block);
}
#endif

static void make_root_inode(void)
{
	struct minix_inode *inode = &Inode[MINIX_ROOT_INO];

	mark_inode(MINIX_ROOT_INO);
	inode->i_zone[0] = get_free_block();
	inode->i_nlinks = 2;
	inode->i_time = time(NULL);
	if (badblocks)
		inode->i_size = 3 * dirsize;
	else {
		// no badblocks but we already wrote ".badblocks" entry; ream it
		root_block[2 * dirsize] = '\0';
		root_block[2 * dirsize + 1] = '\0';
		inode->i_size = 2 * dirsize;
	}
	inode->i_mode = S_IFDIR + 0755;
	inode->i_uid = getuid();
	if (inode->i_uid)
		inode->i_gid = getgid();
	write_block(inode->i_zone[0], root_block);
}

#ifdef BB_FEATURE_MINIX2
static void make_root_inode2(void)
{
	struct minix2_inode *inode = &Inode2[MINIX_ROOT_INO];

	mark_inode(MINIX_ROOT_INO);
	inode->i_zone[0] = get_free_block();
	inode->i_nlinks = 2;
	inode->i_atime = inode->i_mtime = inode->i_ctime = time(NULL);
	if (badblocks)
		inode->i_size = 3 * dirsize;
	else {
		// no badblocks but we already wrote ".badblocks" entry; ream it
		root_block[2 * dirsize] = '\0';
		root_block[2 * dirsize + 1] = '\0';
		inode->i_size = 2 * dirsize;
	}
	inode->i_mode = S_IFDIR + 0755;
	inode->i_uid = getuid();
	if (inode->i_uid)
		inode->i_gid = getgid();
	write_block(inode->i_zone[0], root_block);
}
#endif

static void setup_tables(void)
{
	int i;
	unsigned long inodes;

	memset(super_block_buffer, 0, BLOCK_SIZE);
	memset(boot_block_buffer, 0, 512);
	MAGIC = magic;
	ZONESIZE = 0;
	MAXSIZE = version2 ? 0x7fffffff : (7 + 512 + 512 * 512) * 1024;
#ifdef BB_FEATURE_MINIX2
#if 0
	ZONES = BLOCKS;
#else
	if (version2)
		Super.s_zones = BLOCKS;
	else
		Super.s_nzones = BLOCKS;
#endif
#else
	ZONES = BLOCKS;
#endif
/* some magic nrs: 1 inode / 3 blocks */
	if (req_nr_inodes == 0)
		inodes = BLOCKS / 3;
	else
		inodes = req_nr_inodes;
	/* Round up inode count to fill block size */
#ifdef BB_FEATURE_MINIX2
	if (version2)
		inodes = ((inodes + MINIX2_INODES_PER_BLOCK - 1) &
				  ~(MINIX2_INODES_PER_BLOCK - 1));
	else
#endif
		inodes = ((inodes + MINIX_INODES_PER_BLOCK - 1) &
				  ~(MINIX_INODES_PER_BLOCK - 1));
	if (inodes > 65535)
		inodes = 65535;
	INODES = inodes;
	IMAPS = UPPER(INODES + 1, BITS_PER_BLOCK);
	ZMAPS = 0;
	i = 0;
	while (ZMAPS !=
		   UPPER(BLOCKS - (2 + IMAPS + ZMAPS + INODE_BLOCKS) + 1,
				 BITS_PER_BLOCK) && i < 1000) {
		ZMAPS =
			UPPER(BLOCKS - (2 + IMAPS + ZMAPS + INODE_BLOCKS) + 1,
				  BITS_PER_BLOCK);
		i++;
	}
	/* Real bad hack but overwise mkfs.minix can be thrown
	 * in infinite loop...
	 * try:
	 * dd if=/dev/zero of=test.fs count=10 bs=1024
	 * /sbin/mkfs.minix -i 200 test.fs
	 * */
	if (i >= 999) {
		error_msg_and_die("unable to allocate buffers for maps");
	}
	FIRSTZONE = NORM_FIRSTZONE;
	inode_map = xmalloc(IMAPS * BLOCK_SIZE);
	zone_map = xmalloc(ZMAPS * BLOCK_SIZE);
	memset(inode_map, 0xff, IMAPS * BLOCK_SIZE);
	memset(zone_map, 0xff, ZMAPS * BLOCK_SIZE);
	for (i = FIRSTZONE; i < ZONES; i++)
		unmark_zone(i);
	for (i = MINIX_ROOT_INO; i <= INODES; i++)
		unmark_inode(i);
	inode_buffer = xmalloc(INODE_BUFFER_SIZE);
	memset(inode_buffer, 0, INODE_BUFFER_SIZE);
	printf("%ld inodes\n", INODES);
	printf("%ld blocks\n", ZONES);
	printf("Firstdatazone=%ld (%ld)\n", FIRSTZONE, NORM_FIRSTZONE);
	printf("Zonesize=%d\n", BLOCK_SIZE << ZONESIZE);
	printf("Maxsize=%ld\n\n", MAXSIZE);
}

/*
 * Perform a test of a block; return the number of
 * blocks readable/writeable.
 */
static long do_check(char *buffer, int try, unsigned int current_block)
{
	long got;

	/* Seek to the correct loc. */
	if (lseek(DEV, current_block * BLOCK_SIZE, SEEK_SET) !=
		current_block * BLOCK_SIZE) {
		error_msg_and_die("seek failed during testing of blocks");
	}


	/* Try the read */
	got = read(DEV, buffer, try * BLOCK_SIZE);
	if (got < 0)
		got = 0;
	if (got & (BLOCK_SIZE - 1)) {
		printf("Weird values in do_check: probably bugs\n");
	}
	got /= BLOCK_SIZE;
	return got;
}

static unsigned int currently_testing = 0;

static void alarm_intr(int alnum)
{
	if (currently_testing >= ZONES)
		return;
	signal(SIGALRM, alarm_intr);
	alarm(5);
	if (!currently_testing)
		return;
	printf("%d ...", currently_testing);
	fflush(stdout);
}

static void check_blocks(void)
{
	int try, got;
	static char buffer[BLOCK_SIZE * TEST_BUFFER_BLOCKS];

	currently_testing = 0;
	signal(SIGALRM, alarm_intr);
	alarm(5);
	while (currently_testing < ZONES) {
		if (lseek(DEV, currently_testing * BLOCK_SIZE, SEEK_SET) !=
			currently_testing * BLOCK_SIZE)
			error_msg_and_die("seek failed in check_blocks");
		try = TEST_BUFFER_BLOCKS;
		if (currently_testing + try > ZONES)
			try = ZONES - currently_testing;
		got = do_check(buffer, try, currently_testing);
		currently_testing += got;
		if (got == try)
			continue;
		if (currently_testing < FIRSTZONE)
			error_msg_and_die("bad blocks before data-area: cannot make fs");
		mark_zone(currently_testing);
		badblocks++;
		currently_testing++;
	}
	if (badblocks > 1)
		printf("%d bad blocks\n", badblocks);
	else if (badblocks == 1)
		printf("one bad block\n");
}

static void get_list_blocks(filename)
char *filename;

{
	FILE *listfile;
	unsigned long blockno;

	listfile = xfopen(filename, "r");
	while (!feof(listfile)) {
		fscanf(listfile, "%ld\n", &blockno);
		mark_zone(blockno);
		badblocks++;
	}
	if (badblocks > 1)
		printf("%d bad blocks\n", badblocks);
	else if (badblocks == 1)
		printf("one bad block\n");
}

static int alloc_inode(const struct stat *stat)
{
	while (++last_alloc_inode < INODES)
		if (!inode_in_use(last_alloc_inode)) {
			struct minix_inode *inode;

			mark_inode(last_alloc_inode);
			inode = &Inode[last_alloc_inode];
			inode->i_mode = stat->st_mode;
			inode->i_uid = stat->st_uid;
			inode->i_size = 0;
			inode->i_time = stat->st_mtim.tv_sec;
			inode->i_gid = stat->st_gid;
			inode->i_nlinks = 1;

			if (S_ISCHR(stat->st_mode) || S_ISBLK(stat->st_mode)) {
				inode->i_zone[0] = stat->st_rdev;
			}

			return last_alloc_inode;
		}
	error_msg_and_die("inode full");
	return -1; // NOTREACHED
}

#ifdef BB_FEATURE_MINIX2
static int alloc_inode2(const struct stat *stat)
{
	while (++last_alloc_inode < INODES)
		if (!inode_in_use(last_alloc_inode)) {
			struct minix2_inode *inode;

			mark_inode(last_alloc_inode);
			inode = &Inode2[last_alloc_inode];
			inode->i_mode = stat->st_mode;
			inode->i_nlinks = 1;
			inode->i_uid = stat->st_uid;
			inode->i_gid = stat->st_gid;
			inode->i_size = 0;
			inode->i_atime = stat->st_atim.tv_sec;
			inode->i_mtime = stat->st_mtim.tv_sec;
			inode->i_ctime = stat->st_ctim.tv_sec;

			if (S_ISCHR(stat->st_mode) || S_ISBLK(stat->st_mode)) {
				inode->i_zone[0] = stat->st_rdev;
			}

			return last_alloc_inode;
		}
	error_msg_and_die("inode full");
	return -1; // NOTREACHED
}
#endif

static int alloc_zone(void)
{
	if (last_alloc_zone < FIRSTZONE)
		last_alloc_zone = FIRSTZONE - 1;
	while (++last_alloc_zone < ZONES)
		if (!zone_in_use(last_alloc_zone)) {
			mark_zone(last_alloc_zone);
			return last_alloc_zone;
		}
	error_msg_and_die("zone full");
	return -1; // NOTREACHED
}

static void read_block(int blk, void *buffer)
{
	if (blk * BLOCK_SIZE != lseek(DEV, blk * BLOCK_SIZE, SEEK_SET))
		error_msg_and_die("seek failed in read_block");
	if (BLOCK_SIZE != read(DEV, buffer, BLOCK_SIZE))
		error_msg_and_die("read failed in read_block");
}

static int extend_inode(int ino)
{
	struct minix_inode *inode = &Inode[ino];
	unsigned short ind_block[BLOCK_SIZE >> 1];
	int next_block = (inode->i_size + BLOCK_SIZE - 1) >> BLOCK_SIZE_BITS;
	int alloced;

	if (next_block < 7) {
		inode->i_zone[next_block] = alloced = alloc_zone();
	} else if (next_block < 7 + 512) {
		if (next_block == 7) {
			inode->i_zone[7] = alloc_zone();
			bzero(ind_block, BLOCK_SIZE);
		} else
			read_block(inode->i_zone[7], ind_block);
		ind_block[next_block - 7] = alloced = alloc_zone();
		write_block(inode->i_zone[7], (char *) ind_block);
	} else if (next_block < 7 + 512 + 512 * 512) {
		int i, ind;
		i = next_block - 7 - 512;
		if (i == 0) {
			inode->i_zone[8] = alloc_zone();
			bzero(ind_block, BLOCK_SIZE);
		} else
			read_block(inode->i_zone[8], ind_block);
		if (i % 512 == 0) {
			ind = ind_block[i / 512] = alloc_zone();
			write_block(inode->i_zone[8], (char *) ind_block);
			bzero(ind_block, BLOCK_SIZE);
		} else {
			ind = ind_block[i / 512];
			read_block(ind, ind_block);
		}
		ind_block[i % 512] = alloced = alloc_zone();
		write_block(ind, (char *) ind_block);
	} else
		error_msg_and_die("too many blocks");

	inode->i_size += BLOCK_SIZE;

	return alloced;
}

#ifdef BB_FEATURE_MINIX2
static int extend_inode2(int ino)
{
	struct minix2_inode *inode = &Inode2[MINIX_BAD_INO];
	unsigned long ind_block[BLOCK_SIZE >> 2];
	int next_block = (inode->i_size + BLOCK_SIZE - 1) >> BLOCK_SIZE_BITS;
	int alloced;

	if (next_block < 7) {
		inode->i_zone[next_block] = alloced = alloc_zone();
	} else if (next_block < 7 + 256) {
		if (next_block == 7)
			bzero(ind_block, BLOCK_SIZE);
		else
			read_block(inode->i_zone[7], ind_block);
		ind_block[next_block - 7] = alloced = alloc_zone();
		write_block(inode->i_zone[7], (char *) ind_block);
	} else if (next_block < 7 + 256 + 256 * 256) {
		int i, ind;
		i = next_block - 7 - 256;
		if (i == 0) {
			inode->i_zone[8] = alloc_zone();
			bzero(ind_block, BLOCK_SIZE);
		} else
			read_block(inode->i_zone[8], ind_block);
		if (i % 256 == 0) {
			ind = ind_block[i / 256] = alloc_zone();
			write_block(inode->i_zone[8], (char *) ind_block);
			bzero(ind_block, BLOCK_SIZE);
		} else {
			ind = ind_block[i / 256];
			read_block(ind, ind_block);
		}
		ind_block[i % 256] = alloced = alloc_zone();
		write_block(ind, (char *) ind_block);
	} else
		/* Could make triple indirect block here */
		error_msg_and_die("too many blocks");

	inode->i_size += BLOCK_SIZE;

	return alloced;
}
#endif

static unsigned int inode_zone0(int ino)
{
	return Inode[ino].i_zone[0];
}

#ifdef BB_FEATURE_MINIX2
static unsigned int inode2_zone0(int ino)
{
	return Inode2[ino].i_zone[0];
}
#endif

static unsigned int inode_fsize(int ino)
{
	return Inode[ino].i_size;
}

#ifdef BB_FEATURE_MINIX2
static unsigned int inode2_fsize(int ino)
{
	return Inode2[ino].i_size;
}
#endif

static void inode_setfsize(int ino, int size)
{
	Inode[ino].i_size = size;
}

#ifdef BB_FEATURE_MINIX2
static void inode2_setfsize(int ino, int size)
{
	Inode2[ino].i_size = size;
}
#endif

static void inode_inclinks(int ino)
{
	Inode[ino].i_nlinks++;
}

#ifdef BB_FEATURE_MINIX2
static void inode2_inclinks(int ino)
{
	Inode2[ino].i_nlinks++;
}
#endif

#ifdef BB_FEATURE_MINIX2
#define ALLOC_INODE(stat) (version2 ? alloc_inode2(stat) : alloc_inode(stat))
#define EXTEND_INODE(ino) (version2 ? extend_inode2(ino) : extend_inode(ino))
#define INODE_ZONE0(ino) (version2 ? inode2_zone0(ino) : inode_zone0(ino))
#define INODE_FSIZE(ino) (version2 ? inode2_fsize(ino) : inode_fsize(ino))
#define INODE_SETFSIZE(ino,size) (version2 ? inode2_setfsize(ino,size) : inode_setfsize(ino,size))
#define INODE_INCLINKS(ino) (version2 ? inode2_inclinks(ino) : inode_inclinks(ino))
#else
#define ALLOC_INODE(stat) alloc_inode(stat)
#define EXTEND_INODE(ino) extend_inode(ino)
#define INODE_ZONE0(ino) inode_zone0(ino)
#define INODE_FSIZE(ino) inode_fsize(ino)
#define INODE_SETFSIZE(ino,size) inode_setfsize(ino,size)
#define INODE_INCLINKS(ino) inode_inclinks(ino)
#endif

/// ino.size must be 0 (freshly created inode)
static int init_dir(int ino, int pino)
{
	int bno;
	char block[BLOCK_SIZE] = "\0";
	char *tmp = block;
	*(short *) tmp = ino;
	strcpy(tmp + 2, ".");
	tmp += dirsize;
	*(short *) tmp = pino;
	strcpy(tmp + 2, "..");
	bno = EXTEND_INODE(ino);
	write_block(bno, block);
	INODE_SETFSIZE(ino, dirsize * 2);
	return bno;
}

static void copy_file(int ino, const char *fn, unsigned int size)
{
	char block[BLOCK_SIZE];
	unsigned int rem = size;
	FILE *sf = fopen(fn, "rb");
	if (!sf)
		perror_msg_and_die("could not open file %s/%s", get_current_dir_name(), fn);
	while(0 < rem) {
		unsigned int bs = rem < BLOCK_SIZE ? rem : BLOCK_SIZE;
		int bno = EXTEND_INODE(ino);
		if (fread(block, 1, bs, sf) < bs)
			perror_msg_and_die("short read file %s/%s", get_current_dir_name(), fn);
		write_block(bno, block);
		rem -= bs;
	}
	fclose(sf);
	INODE_SETFSIZE(ino, size);
}

// TODO: hardlink detection
static void seed(int ino)
{
	DIR *src;
	struct dirent *ent;
	char block[BLOCK_SIZE] = "\0";
	int i = 0, bno, size;

	// lazy
	if (BLOCK_SIZE - dirsize < INODE_FSIZE(ino)) {
		error_msg_and_die("too big existing dirsize %d", INODE_FSIZE(ino));
	}
	bno = INODE_ZONE0(ino);
	read_block(bno, block);
	while(*(short *)&block[dirsize * i] != 0) i++;
	size = dirsize * i;

	src = opendir(".");
	if (!src)
		error_msg_and_die("could not opendir to %s", get_current_dir_name());

	while (!!(ent = readdir(src))) {
		int cino;
		struct stat st;

		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
			continue;

		if (BLOCK_SIZE / dirsize <= i) {
			i = 0; //i -= BLOCK_SIZE / dirsize;
			write_block(bno, block);
			bno = EXTEND_INODE(ino);
			bzero(block, BLOCK_SIZE);
		}

		if (lstat(ent->d_name, &st))
			error_msg_and_die("could not stat %s/%s", get_current_dir_name(), ent->d_name);

		printf("%6d %s  %d\n", st.st_size, ent->d_name, ent->d_ino);

		cino = ALLOC_INODE(&st);
		if (S_ISDIR(st.st_mode)) {
			if (chdir(ent->d_name))
				error_msg_and_die("could not chdir to %s/%s", get_current_dir_name(), ent->d_name);
			init_dir(cino, ino);
			INODE_INCLINKS(ino);
			seed(cino);
			if (chdir(".."))
				error_msg_and_die("could not chdir to %s/%s/..", get_current_dir_name(), ent->d_name);
		} else if (S_ISREG(st.st_mode)) {
			copy_file(cino, ent->d_name, st.st_size);
		} else if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)) {
			// nothing to do
		} else if (S_ISLNK(st.st_mode)) {
			char symblock[BLOCK_SIZE] = "\0";
			int sbno = EXTEND_INODE(cino);
			int sz = readlink(ent->d_name, symblock, BLOCK_SIZE);
			if (sz < 0)
				perror_msg_and_die("could not readlink %s/%s", get_current_dir_name(), ent->d_name);
			write_block(sbno, symblock);
			INODE_SETFSIZE(cino, sz);
		} else {
			fprintf(stderr, "warning: unknown file mode %o; %s\n", st.st_mode, ent->d_name);
		}

		*(short *)&block[dirsize * i] = cino;
		strncpy(&block[dirsize * i + 2], ent->d_name, dirsize - 2);
		if (dirsize - 2 < strlen(ent->d_name))
			fprintf(stderr, "warning: truncated name: %s/%s\n", get_current_dir_name(), ent->d_name);
		i++;
		size += dirsize;
	}

	write_block(bno, block);
	INODE_SETFSIZE(ino, size);

	if(closedir(src))
		error_msg_and_die("could not closedir of %s", get_current_dir_name());
}

extern int /*mkfs_minix_*/main(int argc, char **argv)
{
	int i=1;
	char *tmp;
	struct stat statbuf;
	char *listfile = NULL;
	char *seeddir = NULL;
	int stopIt=FALSE;

	if (INODE_SIZE * MINIX_INODES_PER_BLOCK != BLOCK_SIZE)
		error_msg_and_die("bad inode size");
#ifdef BB_FEATURE_MINIX2
	if (INODE_SIZE2 * MINIX2_INODES_PER_BLOCK != BLOCK_SIZE)
		error_msg_and_die("bad inode size");
#endif
	
	/* Parse options */
	argv++;
	while (--argc >= 0 && *argv && **argv) {
		if (**argv == '-') {
			stopIt=FALSE;
			while (i > 0 && *++(*argv) && stopIt==FALSE) {
				switch (**argv) {
					case 'c':
						check = 1;
						break;
					case 'i':
						{
							char *cp=NULL;
							if (*(*argv+1) != 0) {
								cp = ++(*argv);
							} else {
								if (--argc == 0) {
									goto goodbye;
								}
								cp = *(++argv);
							}
							req_nr_inodes = strtoul(cp, &tmp, 0);
							if (*tmp)
								show_usage();
							stopIt=TRUE;
							break;
						}
					case 'l':
						if (--argc == 0) {
							goto goodbye;
						}
						listfile = *(++argv);
						stopIt=TRUE;
						break;
					case 'n':
						{
							char *cp=NULL;

							if (*(*argv+1) != 0) {
								cp = ++(*argv);
							} else {
								if (--argc == 0) {
									goto goodbye;
								}
								cp = *(++argv);
							}
							i = strtoul(cp, &tmp, 0);
							if (*tmp)
								show_usage();
							if (i == 14)
								magic = MINIX_SUPER_MAGIC;
							else if (i == 30)
								magic = MINIX_SUPER_MAGIC2;
							else 
								show_usage();
							namelen = i;
							dirsize = i + 2;
							stopIt=TRUE;
							break;
						}
					case 'v':
#ifdef BB_FEATURE_MINIX2
						version2 = 1;
#else
						error_msg("%s: not compiled with minix v2 support",
								device_name);
						exit(-1);
#endif
						break;
					case 'd':
						if (--argc == 0) {
							goto goodbye;
						}
						seeddir = *(++argv);
						stopIt=TRUE;
						break;
					case '-':
					case 'h':
					default:
goodbye:
						show_usage();
				}
			}
		} else {
			if (device_name == NULL)
				device_name = *argv;
			else if (BLOCKS == 0)
				BLOCKS = strtol(*argv, &tmp, 0);
			else {
				goto goodbye;
			}
		}
		argv++;
	}

	if (device_name && !BLOCKS)
		BLOCKS = get_size(device_name) / 1024;
	if (!device_name || BLOCKS < 10) {
		show_usage();
	}
#ifdef BB_FEATURE_MINIX2
	if (version2) {
		if (namelen == 14)
			magic = MINIX2_SUPER_MAGIC;
		else
			magic = MINIX2_SUPER_MAGIC2;
	} else
#endif
	if (BLOCKS > 65535)
		BLOCKS = 65535;
	check_mount();				/* is it already mounted? */
	tmp = root_block;
	*(short *) tmp = 1;
	strcpy(tmp + 2, ".");
	tmp += dirsize;
	*(short *) tmp = 1;
	strcpy(tmp + 2, "..");
	tmp += dirsize;
	*(short *) tmp = 2;
	strcpy(tmp + 2, ".badblocks");
	DEV = open(device_name, O_RDWR);
	if (DEV < 0)
		error_msg_and_die("unable to open %s", device_name);
	if (fstat(DEV, &statbuf) < 0)
		error_msg_and_die("unable to stat %s", device_name);
	if (!S_ISBLK(statbuf.st_mode))
		check = 0;
	else if (statbuf.st_rdev == 0x0300 || statbuf.st_rdev == 0x0340)
		error_msg_and_die("will not try to make filesystem on '%s'", device_name);
	setup_tables();
	if (check)
		check_blocks();
	else if (listfile)
		get_list_blocks(listfile);
	// here, only badblock(zone)s are marked,
	// effectively zone bitmap is the badblock bitmap.
	// next, we'll allocate some zone, but don't mark it yet
	// until make_bad_inode*() is done. (that uses badblock bitmap.)
#ifdef BB_FEATURE_MINIX2
	if (version2) {
		make_root_inode2();
		make_bad_inode2();
	} else
#endif
	{
		make_root_inode();
		make_bad_inode();
	}
	mark_good_blocks(); // make_bad_inode*() is done, mark alloced good ones too here.
	if (seeddir) {
		if (chdir(seeddir))
			error_msg_and_die("could not chdir to %s/%s", get_current_dir_name(), seeddir);

		seed(MINIX_ROOT_INO);
	}
	write_tables();
	return( 0);

}
