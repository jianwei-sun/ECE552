#include <stdio.h>
#include <stdlib.h>

#define ITERATIONS 100

//Microbenchmark program to test for reliability of next line prefetching

int main(void){
	register int i, b;
	for(i = 0; i < ITERATIONS; i++){
		b += i;
		asm("addu	$4,$4,$3");
	} 
	return 0;
} 
