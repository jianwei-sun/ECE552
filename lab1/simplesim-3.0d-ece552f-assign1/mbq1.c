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
	/* Since the macro SUFFICIENT_NOP sets gamma to 50 and gamma is declared as a register int, its dissassembly shows that the particular instruction appears as a nop since it does not have any hazard dependencies with any other register. */
int main (int argc, char *argv[]){
	/* Initialize all variables */ 
	int loop_iterations = LOOP_ITERATIONS;
	register int loop_index = 0;
	
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
		lw	$2,16($30)
		slt	$9,$3,$2		Q1 2 cycle stall, Q2 2 cycle stall
		bne	$9,$0,4002a0 <main+0xb0>
		j	400410 <main+0x220>
	*/
		a = *c;
		b = a + 1;
 		a++;
		/*
			lw	$3,0($5)
			addiu	$4,$3,1		Q1 2 cycle stall, Q2 2 cycle stall
			addiu	$3,$3,1		No stall because previous instruction stalled for 2 cycles 
		*/
		SUFFICIENT_NOP

		a = *c;
		b = b + 1;
		a++;
		/*
			lw	$3,0($5)	
			addiu	$4,$4,1		Instruction without dependency
			addiu	$3,$3,1		Q1 1 cycle stalls, Q2 1 cycle stall
		*/
		SUFFICIENT_NOP

		d = 5;
		b = 2*d;
		a = d + 1;
		a ++;
		d = a - 1;
		/*
			addiu	$6,$0,5		
			addu	$2,$0,$6	Q1 2 cycle stall, Q2 1 cycle stall
			sll	$8,$2,1		Q1 2 cycle stall, Q2 1 cycle stall
			addu	$4,$0,$8	Q1 2 cycle stall, Q2 1 cycle stall
			addiu	$3,$6,1
			addiu	$3,$3,1		Q1 2 cycle stall, Q2 1 cycle stall
			addiu	$6,$3,-1	Q1 2 cycle stall, Q2 1 cycle stall
		*/
		
		SUFFICIENT_NOP

		a = *c;
		b = a + 1;
		/*
			lw	$3,0($5)	
			addiu	$4,$3,1		Q1 2 cycle stall, Q2 2 cycle stall
		*/
		SUFFICIENT_NOP		

		a ++;
		b ++;
		a ++;
		/*
			addiu	$3,$3,1		
			addiu	$4,$4,1		Instruction without dependency
			addiu	$3,$3,1		Q1 1 cycle stall
		*/
		SUFFICIENT_NOP

		alpha ++;
		beta ++;
		/*
			lw	$8,24($30)	
			addiu	$2,$8,1		Q1 2 cycle stall, Q2 2 cycle stall
			addu	$8,$0,$2	Q1 2 cycle stall, Q2 1 cycle stall
			sw	$8,24($30)	Q1 2 cycle stall
			lw	$8,28($30)
			addiu	$2,$8,1		Q1 2 cycle stall, Q2 2 cycle stall
			addu	$8,$0,$2	Q1 2 cycle stall, Q2 1 cycle stall
			sw	$8,28($30)	Q1 2 cycle stall
		*/
		SUFFICIENT_NOP
	/*
		This code increments loop_index and jumps to the beginning of the loop
		addiu	$3,$3,1
		j	400280 <main+0x90>
	*/
	}

	/*	ANALYSIS
		From a manual inspection of the above source code along with the generated assembly code from objdump, we have determined 
		how many stalls would result from each of the instructions for both questions. All possible stall cases are covered for 
		both questions. This source code is compiled with the -O0 compiler flag and ran with our sim-safe. We expect a total of 2 
		one-cycle Q1 hazards, 14 two-cycle Q1 hazards, 8 one-cycle Q2 hazards, and 5 two-cycle Q2 hazards. The below simulation 
		(main loop run at 1000000 times) confirms our prediction.

		./sim-safe -redir:sim mbq1.simout -redir:prog mbq1.progout mbq1

		sim: ** simulation statistics **
		sim_num_insn               49006324 # total number of instructions executed
		sim_num_refs                8003726 # total number of loads and stores executed
		sim_elapsed_time                  5 # total simulation time in seconds
		sim_inst_rate          9801264.8000 # simulation speed (in insts/sec)
		sim_num_RAW_hazard_q1      16000386 # total number of RAW hazards (q1)
		num_1cycle_q1               2000086 # total number of one cycle stalls (q1)
		num_2cycle_q1              14000300 # total number of two cycle stalls (q1)
		sim_num_RAW_hazard_q2      13000313 # total number of RAW hazards (q2)
		num_1cycle_q2               8000271 # total number of one cycle stalls (q2)
		num_2cycle_q2               5000042 # total number of two cycle stalls (q2)
		CPI_from_RAW_hazard_q1       1.6122 # CPI from RAW hazard (q1)
		CPI_from_RAW_hazard_q2       1.3673 # CPI from RAW hazard (q2)
		ld_text_base             0x00400000 # program text (code) segment base
		ld_text_size                  23616 # program text (code) size in bytes
		ld_data_base             0x10000000 # program initialized data segment base
		ld_data_size                   4096 # program init'ed `.data' and uninit'ed `.bss' size in bytes
		ld_stack_base            0x7fffc000 # program stack segment base (highest address in stack)
		ld_stack_size                 16384 # program initial stack size
		ld_prog_entry            0x00400140 # program entry point (initial PC)
		ld_environ_base          0x7fff8000 # program environment base address address
		ld_target_big_endian              0 # target executable endian-ness, non-zero if big endian
		mem.page_count                   13 # total number of pages allocated
		mem.page_mem                    52k # total size of memory pages allocated
		mem.ptab_misses                  13 # total first level page table misses
		mem.ptab_accesses         212179038 # total page table accesses
		mem.ptab_miss_rate           0.0000 # first level page table miss rate

		The simulation snippit above shows our expected prediction, but multiplied by ~1000000 (main loop iterations).
		We have a total of 49 instructions in our main for loop (obtained from objdump) that are executed 1000000 times.
		Using a manual calculation, we expect the CPI's to be:
		Q1
			CPI = (49 + 2 + 2*14) / 49 = 1.6122 
		Q2
			CPI = (49 + 8 + 2*5) / 49 = 1.3673

		These values agree with the simulation data shown above, as expected.
	*/



	/* Program is now exiting */
	return 0;
}

