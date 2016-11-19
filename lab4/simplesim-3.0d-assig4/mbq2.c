#include <stdio.h>
#include <stdlib.h>

#define ITERATIONS 100

//Microbenchmark program to test for reliability of next line prefetching

int main(void){
		
	asm("move	$3,$sp");
	asm("move	$6, $0");
	asm("addu	$5, $sp, 128000000	");
	asm("$L101:	sltu	$2,$3,$5	");
	asm("beq	$2, $0, $L1		");
	asm("lw		$4, 0($3)		");
	asm("beq	$6, $0, $L102		");
	asm("addu	$3, $3, 64		");
	asm("addu 	$6, $0, 0		");
	asm("j		$L101			");
	asm("$L102:	addu	$3,$3,128	");
	asm("addu 	$6, $0, 1		");
	asm("j		$L101			");
/*
	register int i, b;
	for(i = 0; i < ITERATIONS; i++){
		b += i;
		asm("addu	$4,$4,$3");
	} 
	return 0; */
}
