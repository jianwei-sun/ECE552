#include <stdio.h>
#include <stdlib.h>

#define ITERATIONS 100

//Microbenchmark program to test for reliability of next line prefetching

int main(void){
		
	asm("move	$3,$sp");
	asm("move	$6,$sp");
	asm("addu	$6, $6, 12800000");	
	asm("addu	$5, $sp, 12800000");
	asm("addu	$8, $sp, 25600000");

//Jumping two blocks at a time, so that next-line always miss; should be 100k misses

	asm("$L101:	sltu	$2,$3,$5");
	asm("beq	$2,$0,$L201");
	asm("lw		$4,0($3)");
	asm("addu	$3,$3,128");
	asm("j	$L101");

//Taking the next block size, so that next-line always hits; should be 200k hits

	asm("$L201:	sltu	$2,$6,$8");
	asm("beq	$2,$0,$L1");
	asm("lw		$7,0($6)");
	asm("addu	$6,$6,64");
	asm("j	$L201");
/*
	register int i, b;
	for(i = 0; i < ITERATIONS; i++){
		b += i;
		asm("addu	$4,$4,$3");
	} 
	return 0; */
}
