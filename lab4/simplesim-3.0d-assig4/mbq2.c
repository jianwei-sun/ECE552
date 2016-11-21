#include <stdio.h>
#include <stdlib.h>

#define ITERATIONS 100

//Microbenchmark program to test for reliability of next line prefetching

int main(void){
		
/*
	initialize the registers to store addresses
	this way, by incrementing them, we can index consequtive memory blocks
*/

	asm("move	$3,$sp			");
	asm("move	$7,$sp			");
	asm("addu	$7, $7, 128000000	"); //$7 and $10 are used for case 2
	asm("move	$10, $7			");	

/*
	Set the loop interations
*/

	asm("addu	$5, $sp, 128000000	");
	asm("addu	$9, $sp, 384000000	");

/*
	Case 1: We will fetch unequal strides of 1,2,1,2... block sizes. 
		Stride will not be able to predict the correct block size, 
		causing it to fail every time. This will run 100k times,
		resulting in approximately 133k mispredictions from the unequal strides.
*/
	asm("move	$6, $0			"); //set a flag to toggle between 1 and 0
	asm("$L101:	sltu	$2,$3,$5	"); 
	asm("beq	$2, $0, $L201		");
	asm("lw		$4, 0($3)		"); //memory access
	asm("beq	$6, $0, $L102		"); //check flag so every 2nd time, index increments by 2 blocks
	asm("addu	$3, $3, 64		"); //if beq not taken every other iteration; here we increment index by 1
	asm("addu 	$6, $0, 0		");
	asm("j		$L101			");
	asm("$L102:	addu	$3,$3,128	");
	asm("addu 	$6, $0, 1		");
	asm("j		$L101			");
/*
	Case 2: We will fetch unequal strides of 1,2,1,2... block sizes, 
		but with different instructions. Each instruction will be stored in different
		RPTs, which would not confuse Stride, thus it will accurately prefetch
		every time. This will run 200k times, resulting in approximately 400k 			mispredictions from the two lw.
*/

	asm("$L201:	sltu	$2,$7,$9	");
	asm("beq	$2,$0,$L1		");
	asm("lw		$8,0($7)		"); //this lw indexes memory at every 2 block sizes
	asm("lw		$8,0($10)		"); //this lw indexes memory at every block size
	asm("addu	$7,$7,128		");
	asm("addu	$10,$10,64		");
	asm("j	$L201");


}
