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
	Case 1: Will do a repeating pattern of 1 1 2 block size strides. This will fail normal stride,
		but because our history buffer tracks "irregular" changes and patterns, it will accurately
		prefetch. This code runs 100k times, and should accurately predict 150k times from the alternating instructions.
*/

	asm("move	$6, $0			"); // $6 is a flag that takes on 0, 1, 2 to create the pattern
	asm("$L101:	sltu	$2,$3,$5	"); 
	asm("beq	$2, $0, $L201		");
	asm("lw		$4, 0($3)		"); // memory access

	asm("beq	$6, $0, $L103		"); // if $6 == 0 , go to $L103
	asm("sltiu	$2, $6, 2		");
	asm("bne	$2, $0, $L102		"); // if $6 == 1 , go to $L102
						    // if $6 == 2 (else)
	asm("addu	$3, $3, 64		"); // increment by 1 block size
	asm("addu 	$6, $0, 0		"); // change flag to 0
	asm("j		$L101			");

	asm("$L102:	addu	$3,$3,64	"); // increment by 1 block size
	asm("addu 	$6, $0, 2		"); // change flag to 2
	asm("j		$L101			");


	asm("$L103:	addu	$3,$3,128	"); // increment by 2 block size
	asm("addu 	$6, $0, 1		"); // change flag to 1
	asm("j		$L101			");


/*
	Case 2: two instructions will increment at different speeds starting from the same memory location.
		Because of this, the difference between the two memory accesses on the GHB will always change
		each time they're called. Thus, no clear pattern on the GHB exists, and our prefetcher cannot predict accurately
		This code runs 200k times, resulting in 400k mispredictions from the two loads
*/

	asm("$L201:	sltu	$2,$7,$9	");
	asm("beq	$2,$0,$L301		");
	asm("lw		$8,0($7)		"); // memory access that jumps every 2 block sizes
	asm("lw		$8,0($10)		"); // memory access that jumpes every 1 block size
	asm("addu	$7,$7,128		"); // increment by 2 block sizes
	asm("addu	$10,$10,64		"); // increment by 1 block size
	asm("j	$L201");

/*
	Case 3: This case jumps in an alternating pattern of 1 and 2 block sizes, called by two different instructions.
		Unlike case 1, the two instructions means different entries on the index table. In our implementation,
		each index table stores the global history. Thus, the prefetcher should still find the corresponding pattern
		on the GHB and accurately prefetch. This loop runs 100 times, and should produce 133k hits from the two instructions
*/

	asm("$L301:	sltu	$2,$11,$13	");
	asm("beq	$2,$0,$L1		");
	asm("lw		$12,0($11)		"); //memory access
	asm("addu	$11,$11,64		"); //increment by 1 block size
	asm("lw		$12,0($11)		"); //memory access again
	asm("addu	$11,$11,128		"); //increment by 2 block sizes
	asm("j	$L301");


}
