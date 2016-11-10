#include <stdio.h>
#include <stdlib.h>

#define ITERATIONS 100

//Microbenchmark program to test for reliability of next line prefetching

int main(void){
	asm("move	$3,$sp");
	asm("$L101:	slt	$2,$3,40000");
	asm("bne	$2,$0,$L1");
	asm("lw		$4,0($3)");
	asm("addu	$3,$3,4");
	asm("j	$L101");

/*	register int i, b;
	for(i = 0; i < ITERATIONS; i++){
		b += i;
		asm("addu	$4,$4,$3");
	} 
	return 0;
*/
} 
