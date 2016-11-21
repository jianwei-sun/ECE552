#include <stdio.h>
#include <stdlib.h>

#define ITERATIONS 100

//Microbenchmark program to test for reliability of next line prefetching

int main(void){
		
/*
	initialize the registers to store addresses
	this way, by incrementing them, we can index consequtive memory blocks
*/

	asm("move	$3,$sp");
	asm("move	$6,$sp");
	asm("addu	$6, $6, 12800000"); //$6 is used to Case 2

/*
	Set the loop interations
*/
	asm("addu	$5, $sp, 12800000");
	asm("addu	$8, $sp, 25600000");

/*
	Case 1: Memory will jump two blocks at a time. This goes outside the next-line
	        "fetch range", causing it to fail every time. This will iterate 100k times
*/
	asm("$L101:	sltu	$2,$3,$5");
	asm("beq	$2,$0,$L201");
	asm("lw		$4,0($3)");
	asm("addu	$3,$3,128"); //increment by 2 block sizes
	asm("j	$L101");

/*
	Case 2: Memory will jump one blocks at a time. This is exactly the next block size
		the prefetcher fetches. This should succeed every time. We can also use any 			size below 64 (such as 32). As it's smaller than block size, it will be 		prefetched by next-line. This loop will run 200k times. 
*/

	asm("$L201:	sltu	$2,$6,$8");
	asm("beq	$2,$0,$L1");
	asm("lw		$7,0($6)");
	asm("addu	$6,$6,64"); //increment by 1 block size
	asm("j	$L201");

}
