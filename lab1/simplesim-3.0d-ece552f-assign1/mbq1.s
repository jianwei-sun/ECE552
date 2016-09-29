	.file	1 "mbq1.c"

 # GNU C 2.7.2.3 [AL 1.1, MM 40, tma 0.1] SimpleScalar running sstrix compiled by GNU C

 # Cc1 defaults:
 # -mgas -mgpOPT

 # Cc1 arguments (-G value = 8, Cpu = default, ISA = 1):
 # -quiet -dumpbase -O0 -o

gcc2_compiled.:
__gnu_compiled_c:
	.text
	.align	2
	.globl	main

	.text

	.loc	1 11
	.ent	main
main:
	.frame	$fp,56,$31		# vars= 32, regs= 2/0, args= 16, extra= 0
	.mask	0xc0000000,-4
	.fmask	0x00000000,0
	subu	$sp,$sp,56
	sw	$31,52($sp)
	sw	$fp,48($sp)
	move	$fp,$sp
	sw	$4,56($fp)
	sw	$5,60($fp)
	jal	__main
	li	$2,0x000f4240		# 1000000
	sw	$2,16($fp)
	sw	$0,20($fp)
	sw	$0,24($fp)
	sw	$0,28($fp)
	sw	$0,32($fp)
	sw	$0,36($fp)
	sw	$0,40($fp)
	sw	$0,44($fp)
	sw	$0,20($fp)
$L2:
	lw	$2,20($fp)
	lw	$3,16($fp)
	slt	$2,$2,$3
	bne	$2,$0,$L5
	j	$L3
$L5:
	lw	$3,24($fp)
	addu	$2,$3,1
	move	$3,$2
	sw	$3,24($fp)
	lw	$2,24($fp)
	move	$3,$2
	sll	$2,$3,1
	sw	$2,28($fp)
	sw	$0,36($fp)
	sw	$0,40($fp)
	lw	$2,28($fp)
	lw	$3,40($fp)
	addu	$2,$2,$3
	sw	$2,28($fp)
$L4:
	lw	$3,20($fp)
	addu	$2,$3,1
	move	$3,$2
	sw	$3,20($fp)
	j	$L2
$L3:
	move	$2,$0
	j	$L1
$L1:
	move	$sp,$fp			# sp not trusted here
	lw	$31,52($sp)
	lw	$fp,48($sp)
	addu	$sp,$sp,56
	j	$31
	.end	main
