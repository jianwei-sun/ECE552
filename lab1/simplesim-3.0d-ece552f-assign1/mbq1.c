/* Jianwei Sun 1000009821, Yi Fan Zhang 1000029284
   Microbench mark program for Assignment 1, ECE552H1F

   /cad2/ece552f/compiler/bin/ssbig-na-sstrix-gcc mbq1.c -O0 -o mbq1  
*/

#include <stdio.h>

#define LOOP_ITERATIONS 1000000
#define SUFFICIENT_NOP gamma=50;gamma=50;gamma=50;
int main (int argc, char *argv[]){
	/* Initialize all variables */ 
	int loop_iterations = LOOP_ITERATIONS;
	int loop_index = 0;
	
	register int a = 0;
	register int b = 0;	
	register int *c;
	register int d = 0;
	int alpha = 0;
	int beta = 0;
	register int gamma = 0;

	/* Begin the main loop */
	for(loop_index=0; loop_index<loop_iterations; loop_index++){
	/* 		
		This code checks for the loop termination condition
		lw	$2,20($fp)
		lw	$8,16($fp)
		slt	$2,$2,$8		Q1 2 cycle stall, Q2 2 cycle stall
		bne	$2,$0,$L5		
		j	$L3
	*/

	/*	a = *c;
		b = a + 1;
 		a++;
		/*
			lw	$3,0($5)
			addu	$4,$3,1		Q1 2 cycle stall, Q2 2 cycle stall
			addu	$3,$3,1		Q1 1 cycle stall
		*/
		SUFFICIENT_NOP

		a = *c;
		b = b + 1;
		a++;
		/*
			lw	$3,0($5)	
			addu	$4,$4,1		
			addu	$3,$3,1		Q1 1 cycle stalls
		*/
		SUFFICIENT_NOP
		
		d = 5;
		b = 2*d;
		a = d + 1;
		a ++;
		d = a - 1;
		/*
			li	$6,0x00000005		
			move	$2,$6		Q1 2 cycle stall, Q2 1 cycle stall
			sll	$8,$2,1		Q1 2 cycle stall, Q2 1 cycle stall
			move	$4,$8		Q1 2 cycle stall, Q2 1 cycle stall
			addu	$3,$6,1
			addu	$3,$3,1		Q1 2 cycle stall, Q2 1 cycle stall
			subu	$6,$3,1		Q1 2 cycle stall, Q2 1 cycle stall
		*/
		
		SUFFICIENT_NOP

		a = *c;
		b = a + 1;
		/*
			lw	$3,0($5)	
			addu	$4,$3,1		Q1 2 cycle stall, Q2 2 cycle stall
		*/
		SUFFICIENT_NOP		

		a ++;
		b ++;
		a ++;
		/*
			addu	$3,$3,1		
			addu	$4,$4,1		
			addu	$3,$3,1		Q1 1 cycle stall
		*/
		SUFFICIENT_NOP

		alpha ++;
		beta ++;
		/*
			lw	$8,24($fp)	
			addu	$2,$8,1		Q1 2 cycle stall, Q2 2 cycle stall
			move	$8,$2		Q1 2 cycle stall, Q2 1 cycle stall
			sw	$8,24($fp)	Q1 2 cycle stall
			lw	$8,28($fp)
			addu	$2,$8,1		Q1 2 cycle stall, Q2 2 cycle stall
			move	$8,$2		Q1 2 cycle stall, Q2 1 cycle stall
			sw	$8,28($fp)	Q1 2 cycle stall
		*/*/
		SUFFICIENT_NOP
	/*
		This code increments loop_index and jumps to the beginning of the loop
		lw	$8,20($fp)		
		addu	$2,$8,1			Q1 2 cycle stall, Q2 2 cycle stall
		move	$8,$2			Q1 2 cycle stall, Q2 1 cycle stall
		sw	$8,20($fp)		Q1 2 cycle stall
		j	$L2
	*/
	}

	/* Program is now exiting */
	return 0;
}

