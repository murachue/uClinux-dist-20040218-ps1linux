#include <sys/regdef.h>
#include <sys/syscall.h>

	.globl	vfork
	.ent	vfork
	.type	vfork, @function
vfork:
	/* prepare for calling __set_errno on failure */
	/* this must be on top because $t9 set to &vfork */
	/* $gp must be kept over syscall... */
	.set noreorder
	.cpload	t9
	.set reorder
#ifndef __NR_vfork
	/* just use fork */
	j fork
#else
	/* we cannot impl by C function as it will save fp and ra which modifies
	 * stack memory, which breaks parent process on succeed of vfork. */
	li	v0, __NR_vfork
	syscall
	bnez	a3, 1f
	/* v0 already set */
	jr	ra
1:
	/* failure; ok to modify memory */
	addiu	sp, -16 /* -12 is enough, 8 align */
	sw	ra, 8(sp)

	/* __errno_location is weak, allowed to be nullptr!? */
	la	t9, __errno_location
	.cprestore	4 /* self reorder for MIPS I load-delay */
	sw	v0, 0(sp) /* self reorder for branch-delay */
	beqz	t9, 1f

	jalr	t9

	lw	t0, 0(sp)
	sw	t0, 0(v0)

1:
	li	v0, -1
	lw	ra, 4(sp)
	addiu	sp, 16
	jr	ra

#endif
	.end	vfork
	.size	vfork,.-vfork
