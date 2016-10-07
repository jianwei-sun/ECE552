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
	.frame	$fp,40,$31		# vars= 16, regs= 2/0, args= 16, extra= 0
	.mask	0xc0000000,-4
	.fmask	0x00000000,0
	subu	$sp,$sp,40
	sw	$31,36($sp)
	sw	$fp,32($sp)
	move	$fp,$sp
	sw	$4,40($fp)
	sw	$5,44($fp)
	jal	__main
	li	$2,0x000f4240		# 1000000
	sw	$2,16($fp)
	sw	$0,20($fp)
	move	$3,$0
	move	$4,$0
	move	$6,$0
	sw	$0,24($fp)
	sw	$0,28($fp)
	move	$7,$0
	sw	$0,20($fp)
$L2:
	lw	$2,20($fp)
	lw	$8,16($fp)
	slt	$2,$2,$8
	bne	$2,$0,$L5
	j	$L3
$L5:
	lw	$3,0($5)
	addu	$4,$3,1
	addu	$3,$3,1
	li	$7,0x00000032		# 50
	li	$7,0x00000032		# 50
	li	$7,0x00000032		# 50
	lw	$3,0($5)
	addu	$4,$4,1
	addu	$3,$3,1
	li	$7,0x00000032		# 50
	li	$7,0x00000032		# 50
	li	$7,0x00000032		# 50
	li	$6,0x00000005		# 5
	move	$2,$6
	sll	$8,$2,1
	move	$4,$8
	addu	$3,$6,1
	addu	$3,$3,1
	subu	$6,$3,1
	li	$7,0x00000032		# 50
	li	$7,0x00000032		# 50
	li	$7,0x00000032		# 50
	lw	$3,0($5)
	addu	$4,$3,1
	li	$7,0x00000032		# 50
	li	$7,0x00000032		# 50
	li	$7,0x00000032		# 50
	addu	$3,$3,1
	addu	$4,$4,1
	addu	$3,$3,1
	li	$7,0x00000032		# 50
	li	$7,0x00000032		# 50
	li	$7,0x00000032		# 50
	lw	$8,24($fp)
	addu	$2,$8,1
	move	$8,$2
	sw	$8,24($fp)
	lw	$8,28($fp)
	addu	$2,$8,1
	move	$8,$2
	sw	$8,28($fp)
	li	$7,0x00000032		# 50
	li	$7,0x00000032		# 50
	li	$7,0x00000032		# 50
$L4:
	lw	$8,20($fp)
	addu	$2,$8,1
	move	$8,$2
	sw	$8,20($fp)
	j	$L2
$L3:
	move	$2,$0
	j	$L1
$L1:
	move	$sp,$fp			# sp not trusted here
	lw	$31,36($sp)
	lw	$fp,32($sp)
	addu	$sp,$sp,40
	j	$31
	.end	main
