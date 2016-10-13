/* Jianwei Sun 1000009821, Yi Fan Zhang 1000029284
   Microbench mark program for Assignment 1, ECE552H1F

   Compile using:
   	/cad2/ece552f/compiler/bin/ssbig-na-sstrix-gcc mbq1.c -O0 -o mbq1  

   Get assembly from:
	/cad2/ece552f/compiler/bin/ssbig-na-sstrix-objdump -x -d -l mbq1 > mbq1.objdump
*/

#include <stdio.h>

#define LOOP_ITERATIONS 1000000
#define SUFFICIENT_NOP gamma=50;gamma=50;gamma=50;

int main (int argc, char *argv[]){
	int register random_number = 0;
	int register loop_index = 0;

	/* Begin the main loop */
	for(loop_index=0; loop_index<LOOP_ITERATIONS; loop_index++){
		if(loop_index %5)
			random_number = 5;
		if(loop_index%2)
			random_number = 2; 
	}

	
	return 0;
}

