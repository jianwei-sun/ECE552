#include <stdio.h>
#include <stdlib.h>

#define ITERATIONS 100

//Microbenchmark program to test for reliability of next line prefetching

int main(void){

/*
	initialize the registers to store addresses
	this way, by incrementing them, we can index consequtive memory blocks
*/
		
	asm("move	$3,$sp			");	//$3 used in case 1
	asm("move	$7,$sp			");	
	asm("addu	$7, $7, 12800000	");	//$7 and $ 10 used in case 2
	asm("move	$10, $7			");	
	asm("addu	$11, $7, 25600000	");	//$11 is used in Case 3

/*
	Set the loop interations
*/

	asm("addu	$5, $sp, 12800000	");
	asm("addu	$9, $5,  25600000	");
	asm("addu	$13,$9,  12800000	");

/*
	Case 1: 112112
*/

	asm("move	$6, $0			");
	asm("$L101:	sltu	$2,$3,$5	");
	asm("beq	$2, $0, $L201		");
	//asm("lw		$4, 0($3)		");

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


/*
	Case 2: random
*/

	asm("$L201:	sltu	$2,$7,$9	");
	asm("beq	$2,$0,$L301		");
	//asm("lw		$8,0($7)		");
	//asm("lw		$8,0($10)		");
	asm("addu	$7,$7,128		");
	asm("addu	$10,$10,64		");
	asm("j	$L201");

/*
	Case 3: 121212
*/

	asm("$L301:	sltu	$2,$11,$13");
	asm("beq	$2,$0,$L1");
	asm("lw		$12,0($11)");
	asm("addu	$11,$11,64");
	asm("lw		$12,0($11)");
	asm("addu	$11,$11,128");
	asm("j	$L301");


}
