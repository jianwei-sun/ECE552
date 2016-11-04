/* Jianwei Sun 1000009821, Yi Fan Zhang 1000029284
   Microbench mark program for Assignment 1, ECE552H1F

*/

#include <stdio.h>

#define LOOP_ITERATIONS 1000000

int main (int argc, char *argv[]){
	int register random_number = 0;
	int register loop_index = 0;
	int register a = 0;

	/* Begin the main loop */
	for(loop_index=0; loop_index<LOOP_ITERATIONS; loop_index++){
	/* 
		cause misprediction every 8th instruction (Which is outside the BHR 6th length)
	*/
	/*
		400238:	05 00 00 00 	beq $2,$0,400248 <main+0x58>	//taken if(loop_index%8) is false
  		400240:	43 00 00 00 	addiu $5,$0,1			// randum_number=1;
	*/
		if((loop_index%8)) 
			random_number = 1; 

	/*
		will change direction every cycle, should always cause misprediction
	*/
	/*
		400248:	05 00 00 00 	beq $4,$0,400260 <main+0x70>  	//taken if(a) is false
  		400250:	42 00 00 00 	addu $4,$0,$0			//if not taken, changes a so next time is taken
	*/
		if(a) 
			a = 0;
		else
	/*
		400260:	43 00 00 00 	addiu $4,$0,1 			//if(a) branch is taken, change a so next time it's not taken
	*/
			a = 1;

	/*
		400278:	05 00 00 00 	beq $2,$0,400230 <main+0x40> 	//branches back to the beginning of the for loop. This should always be taken, except on the last iteration
	*/

	}
	  
	printf("%d",random_number);
	
	return 0;
}





