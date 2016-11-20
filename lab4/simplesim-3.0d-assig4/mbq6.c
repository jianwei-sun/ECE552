#include <stdio.h>
#include <stdlib.h>

#define ITERATIONS 100

//Microbenchmark program to test for reliability of next line prefetching

int main(void){
		
	asm("move	$3,$sp			");

	asm("move	$7,$sp			");
	asm("addu	$7, $7, 128000000	");
	asm("move	$10, $7			");	

	asm("addu	$5, $sp, 128000000	");
	asm("addu	$9, $sp, 384000000	");

	asm("move	$6, $0			");
	asm("$L101:	sltu	$2,$3,$5	");
	asm("beq	$2, $0, $L201		");
	asm("lw		$4, 0($3)		");

	asm("beq	$6, $0, $L103		");
	asm("sltiu	$2, $6, 2		");
	asm("bne	$2, $0, $L102		");

	asm("addu	$3, $3, 64		");
	asm("addu 	$6, $0, 0		");
	asm("j		$L101			");

	asm("$L102:	addu	$3,$3,64	");
	asm("addu 	$6, $0, 2		");
	asm("j		$L101			");


	asm("$L103:	addu	$3,$3,128	");
	asm("addu 	$6, $0, 1		");
	asm("j		$L101			");


	asm("$L201:	sltu	$2,$7,$9	");
	asm("beq	$2,$0,$L1		");
	asm("lw		$8,0($7)		");
	asm("lw		$8,0($10)		");
	asm("addu	$7,$7,128		");
	asm("addu	$10,$10,64		");
	asm("j	$L201");


}
