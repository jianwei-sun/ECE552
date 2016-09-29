/* Jianwei Sun 1000009821, Yi Fan Zhang 1000029284
   Microbench mark program for Assignment 1, ECE552H1F

   
*/

#include <stdio.h>

#define LOOP_ITERATIONS 1000000

int main (int argc, char *argv[]){
	/* Initialize all variables */ 
	int loop_iterations = LOOP_ITERATIONS;
	int loop_index = 0;
	
	int a = 0;
	int b = 0;	
	int c = 0;
	int alpha = 0;
	int beta = 0;
	int gamma = 0;

	/* Begin the main loop */
	for(loop_index=0; loop_index<loop_iterations; loop_index++){
		/*
			lw      $2,20($fp)        loop_index
			lw      $3,16($fp)        loop_iterations
			slt     $2,$2,$3          $2 becomes 1 if loop_index < loop_iterations
			bne     $2,$0,<main+0xb0> jump to inside the for loop
			j       <main+0x148>      jumps to return 0 outside of for loop
		*/ 
		a++;
			/*
				lw      $3,24($fp)      a
			   	addu    $2,$3,1         a = a + 1
			   	addu    $3,$0,$2        move a
			   	sw      $3,24($fp)      store a
			*/
		b = 2*a;
			/*
				lw      $2,24($fp)      a
				addu    $3,$0,$2        move a
				sll     $2,$3,0x1       2*a
				sw      $2,28($fp)      store result as b
			*/
		alpha = 0;
		beta = 0;
		b += beta;
			/*
				lw	$2,28($fp)      b
				lw	$3,40($fp)      beta
				addu    $2,$2,$3        b = b + beta
				sw	$2,28($fp)      store b
			*/

		/*
			lw      $3,20($fp)       loop_index
			addiu   $2,$3,1          loop_index++
			addu    $3,$0,$2         move loop_index
			sw      $3,20($fp)	 store loop_index
		*/
	}
	/* Program is now exiting */
	return 0;
}

