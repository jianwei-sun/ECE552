
#include <limits.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "sim.h"
#include "decode.def"

#include "instr.h"

/* PARAMETERS OF THE TOMASULO'S ALGORITHM */

#define INSTR_QUEUE_SIZE         10

#define RESERV_INT_SIZE    4
#define RESERV_FP_SIZE     2
#define FU_INT_SIZE        2
#define FU_FP_SIZE         1

#define FU_INT_LATENCY     4
#define FU_FP_LATENCY      9

/* IDENTIFYING INSTRUCTIONS */

//unconditional branch, jump or call
#define IS_UNCOND_CTRL(op) (MD_OP_FLAGS(op) & F_CALL || \
                         MD_OP_FLAGS(op) & F_UNCOND)

//conditional branch instruction
#define IS_COND_CTRL(op) (MD_OP_FLAGS(op) & F_COND)

//floating-point computation
#define IS_FCOMP(op) (MD_OP_FLAGS(op) & F_FCOMP)

//integer computation
#define IS_ICOMP(op) (MD_OP_FLAGS(op) & F_ICOMP)

//load instruction
#define IS_LOAD(op)  (MD_OP_FLAGS(op) & F_LOAD)

//store instruction
#define IS_STORE(op) (MD_OP_FLAGS(op) & F_STORE)

//trap instruction
#define IS_TRAP(op) (MD_OP_FLAGS(op) & F_TRAP) 

#define USES_INT_FU(op) (IS_ICOMP(op) || IS_LOAD(op) || IS_STORE(op))
#define USES_FP_FU(op) (IS_FCOMP(op))

#define WRITES_CDB(op) (IS_ICOMP(op) || IS_LOAD(op) || IS_FCOMP(op))

/* FOR DEBUGGING */

//prints info about an instruction
#define PRINT_INST(out,instr,str,cycle)	\
  myfprintf(out, "%d: %s", cycle, str);		\
  md_print_insn(instr->inst, instr->pc, out); \
  myfprintf(stdout, "(%d)\n",instr->index);

#define PRINT_REG(out,reg,str,instr) \
  myfprintf(out, "reg#%d %s ", reg, str);	\
  md_print_insn(instr->inst, instr->pc, out); \
  myfprintf(stdout, "(%d)\n",instr->index);

/* ECE552 Assignment 3 - BEGIN CODE */

//priority queue
typedef struct QUEUE_NODE{
	bool int_rs;
	int rs_index;
	struct QUEUE_NODE *next;
} NODE;

/* FUNCTIONAL DECLARATIONS */
bool contains(int*, int, int);
NODE* dequeue(void);
void dequeue_at(NODE*);
void enqueue(bool, int);

/* VARIABLES */
//instruction queue for tomasulo
//We implement a circular buffer for convenience. We need an extra space in the 
//implementation to determine if the buffer is full
static instruction_t* instr_queue[INSTR_QUEUE_SIZE + 1];
//push index == pop index means queue is empty
//push index 1 before pop index means queue is full
int ifq_push_index = 0;
int ifq_pop_index = 0;
bool last_fetched;

//number of instructions in the instruction queue
static int instr_queue_count = 0;

//the index of the last instruction fetched
static int fetch_index = 0;

//A linked list keeps track of which instructions entered the RS's first, to ensure
//that the oldest instructions are given priority
//Add at tail, pop from head
NODE* HEAD = NULL;

/* MAP TABLE */
int MT[MD_TOTAL_REGS];

/* FUNCTIONAL UNITS */
typedef struct FUNCITONAL_UNIT{
	instruction_t* inst;
	int cycles_to_completion;
	int rs;
} FU;

FU all_intFU[FU_INT_SIZE];
FU all_fpFU[FU_FP_SIZE];

/* RESERVATION STATIONS */
typedef struct RESERVATION_STATION{
	bool busy;
	bool executing;
	instruction_t* instruction;
	int R0;
	int R1;
	int T0;
	int T1;
	int T2;
	int instantiation_cycle;
} ResSta;

ResSta all_intRS[RESERV_INT_SIZE];
ResSta all_fpRS[RESERV_FP_SIZE];

/* CDB */
typedef struct COMMON_DATA_BUS{
	instruction_t* inst;
	int T;
} CDB;

CDB cdb;
/* ECE552 Assignment 3 - END CODE */

/* ECE552 Assignment 3 - BEGIN CODE */
/* 
 * Description: 
 * 	Checks if simulation is done by finishing the very last instruction
 *      Remember that simulation is done only if the entire pipeline is empty
 * Inputs:
 * 	sim_insn: the total number of instructions simulated
 * Returns:
 * 	True: if simulation is finished
 */
static bool is_simulation_done(counter_t sim_insn) {
	//Note, the printf's that are commented out were used for debugging purposes
	//The last instruction needs to have been fetched
	if(fetch_index < sim_insn){
		//printf("not last fetched %d, %d\n",fetch_index,sim_insn);
		return false;
	}
	//IFQ needs to be empty
	if(ifq_pop_index != ifq_push_index){
		//printf("ifq not empty\n");
		return false;
	}
	//Checks to make sure the RS and FU's are empty
  	int i;
	for(i = 0; i < RESERV_INT_SIZE; i++){
		if(all_intRS[i].busy == true || all_intRS[i].executing == true){
			if(all_intRS[i].executing){
				//printf("int rs executing\n");
			}			
			else if(all_intRS[i].busy){
				//printf("int rs busy\n");
			}

			return false;
		}
	}
	for(i = 0; i < RESERV_FP_SIZE; i++){
		if(all_fpRS[i].busy == true || all_fpRS[i].executing == true){
			//printf("fp rs busy or executing\n");
			return false;
		}
	}
	//Checks to make sure the CDB is also empty
	if(cdb.T != -1){
		//printf("cdb not empty\n");
		return false;
	}
  return true;
}
/* ECE552 Assignment 3 - END CODE */

/* ECE552 Assignment 3 - BEGIN CODE */
/* 
 * Description: 
 * 	Retires the instruction from writing to the Common Data Bus
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void CDB_To_retire(int current_cycle) {
	//Count how many cycles an instruction has been waiting inside the CDB
	if(cdb.inst != NULL){
		cdb.inst->tom_cdb_cycle ++;
	}
	int j;
	//Broadcast the result to the reservation stations
	for(j = 0; j < RESERV_INT_SIZE; j++){
		if(all_intRS[j].T0 == cdb.T){
			all_intRS[j].T0 = -1;
		}
		if(all_intRS[j].T1 == cdb.T){
			all_intRS[j].T1 = -1;
		}
		if(all_intRS[j].T2 == cdb.T){
			all_intRS[j].T2 = -1;
		} 
	}
	//Broadcast the result to the fp reservation stations
	for(j = 0; j < RESERV_FP_SIZE; j++){
		if(all_fpRS[j].T0 == cdb.T){
			all_fpRS[j].T0 = -1;
		}
		if(all_fpRS[j].T1 == cdb.T){
			all_fpRS[j].T1 = -1;
		}
		if(all_fpRS[j].T2 == cdb.T){
			all_fpRS[j].T2 = -1;
		} 
	}
	//Clear the map table to indicate writeback to the register file has occured
	for(j = 0; j < MD_TOTAL_REGS; j++){
		if(MT[j] == cdb.T){
			MT[j] = -1;
		}
	}
	//Clear the CDB
	cdb.T = -1;
	cdb.inst = NULL;
	return;
}
/* ECE552 Assignment 3 - END CODE */

/* ECE552 Assignment 3 - BEGIN CODE */
/* 
 * Description: 
 * 	Moves an instruction from the execution stage to common data bus (if possible)
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void execute_To_CDB(int current_cycle) {
	//Check which instructions are done executing
	NODE* node, *temp;
	int rs_number;
	int k;
	//Increment the counters to indicate that the instruction has been in the FU for another cycle
	for(k = 0; k < FU_INT_SIZE; k++){
		if(all_intFU[k].inst != NULL){
			all_intFU[k].inst->tom_execute_cycle ++;
		}
	}
	for(k = 0; k < FU_FP_SIZE; k++){
		if(all_fpFU[k].inst != NULL){
			all_fpFU[k].inst->tom_execute_cycle ++;
		}
	}
	//Find the next instruction that has finished executing based on age
	for(node = HEAD; (node != NULL); node = node->next){
		int j = node->rs_index, i;
		if(node->int_rs){	
			//Find the corresponding index in the functional unit
			if(all_intRS[j].executing){
				for(i = 0; i < FU_INT_SIZE; i++){
					if(all_intFU[i].rs == j){
						break;
					}
				}
			} else{
				continue;
			}
			//Check to see if it is done executing in the FU
			if((all_intFU[i].inst != NULL)&&(all_intFU[i].cycles_to_completion == 0)){
				rs_number = all_intFU[i].rs;
				//Check if the instruction writes to the CDB
				if(WRITES_CDB(all_intFU[i].inst->op)){
					//And if the CDB is free
					if(cdb.T == -1){
						cdb.T = rs_number;
						cdb.inst = all_intFU[i].inst;
						all_intFU[i].inst = NULL;
						all_intRS[rs_number].instruction = NULL;
						all_intRS[rs_number].busy = false;
						all_intRS[rs_number].executing = false;
						dequeue_at(node);
					} else {
						continue;
					}
				} else {
					//Stores do not write to the CDB, so no need to wait for it to be free
					all_intFU[i].inst = NULL;
					all_intRS[rs_number].instruction = NULL;
					all_intRS[rs_number].busy = false;
					all_intRS[rs_number].executing = false;
					dequeue_at(node);
				}
			}		
		} else {
			//Find the corresponding index in the functional unit
			if(all_fpRS[j].executing){
				for(i = 0; i < FU_FP_SIZE; i++){
					if(all_fpFU[i].rs == j){
						break;
					}
				}
			} else{
				continue;
			}
			//Check to see if it is done executing in the FU
			if((all_fpFU[i].inst != NULL)&&(all_fpFU[i].cycles_to_completion == 0)){
				rs_number = all_fpFU[i].rs;	
				//Check if the instruction writes to the CDB
				if(WRITES_CDB(all_fpFU[i].inst->op)){
					//And if the CDB is free
					if(cdb.T == -1){
						cdb.T = rs_number;
						cdb.inst = all_fpFU[i].inst;
						all_fpFU[i].inst = NULL;
						all_fpRS[rs_number].instruction = NULL;
						all_fpRS[rs_number].busy = false;
						all_fpRS[rs_number].executing = false;
						dequeue_at(node);
					} else {
						continue;
					}
				} else {
					//Stores do not write to the CDB, so no need to wait for it to be free
					all_fpFU[i].inst = NULL;
					all_fpRS[rs_number].instruction = NULL;
					all_fpRS[rs_number].busy = false;
					all_fpRS[rs_number].executing = false;
					dequeue_at(node);
				}
			}
		}		
	}

	//Allow all instructions inside functional units to progress by 1 cycle
	int new_cyc, i;
	for(i = 0; i < FU_INT_SIZE; i++){
		if(all_intFU[i].inst == NULL){
			continue;
		} else {
			//Ensure count does not become negative because we could be stalling for a free cdb
			new_cyc = all_intFU[i].cycles_to_completion - 1;
			all_intFU[i].cycles_to_completion = new_cyc < 0 ? 0 : new_cyc;
		}
	}
	for(i = 0; i < FU_FP_SIZE; i++){
		if(all_fpFU[i].inst == NULL){
			continue;
		} else {
			//Ensure count does not become negative because we could be stalling for a free cdb
			new_cyc = all_fpFU[i].cycles_to_completion - 1;
			all_fpFU[i].cycles_to_completion = new_cyc < 0 ? 0 : new_cyc;
		}
	}
}
/* ECE552 Assignment 3 - END CODE */

/* ECE552 Assignment 3 - BEGIN CODE */
/* 
 * Description: 
 * 	Moves instruction(s) from the issue to the execute stage (if possible). We prioritize old instructions
 *      (in program order) over new ones, if they both contend for the same functional unit.
 *      All RAW dependences need to have been resolved with stalls before an instruction enters execute.
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void issue_To_execute(int current_cycle) {
	NODE* node;
	bool int_rs;
	int rs_number;
	//Find the next instruction that has finished executing based on the age queue
	for(node = HEAD; (node != NULL); node = node->next){
		int_rs = node->int_rs;
		rs_number = node->rs_index;
		if(int_rs){
			//If the instruction is already in the FU, continue
			if(all_intRS[rs_number].executing){
				continue;
			}
			//Increment the count for how long the instruction has been inside the RS
			if(all_intRS[rs_number].instruction!=NULL){
				all_intRS[rs_number].instruction->tom_issue_cycle ++;
			}
			//Check if an instruction in the RS is ready to be moved to execution
			if((all_intRS[rs_number].instruction!=NULL)&&(all_intRS[rs_number].T0 == -1)&&(all_intRS[rs_number].T1 == -1)&&(all_intRS[rs_number].T2 == -1)){
				//Check if there is room in the functional unit
				int j;
				bool full = true;
				for(j = 0; j < FU_INT_SIZE; j++){
					if(all_intFU[j].inst == NULL){
						full = false;
						break;
					}
				}	
				//If there is room in the functional unit
				if(!full){
					//Move the instruction to execution
					all_intFU[j].inst = all_intRS[rs_number].instruction;
					all_intFU[j].cycles_to_completion = FU_INT_LATENCY - 1;
					all_intFU[j].rs = rs_number;
					all_intRS[rs_number].executing = true;
				
				}
			}
		} else {
			//If the instruction is already in the FU, continue
			if(all_fpRS[rs_number].executing){
				continue;
			}
			//Increment the count for how long the instruction has been inside the RS
			if(all_fpRS[rs_number].instruction!=NULL){
				all_fpRS[rs_number].instruction->tom_issue_cycle ++;
			}
			//Check if an instruction in the RS is ready to be moved to execution
			if((all_fpRS[rs_number].instruction!=NULL)&&(all_fpRS[rs_number].T0 == -1)&&(all_fpRS[rs_number].T1 == -1)&&(all_fpRS[rs_number].T2 == -1)){
				//Check if there is room in the functional unit
				int j;
				bool full = true;
				for(j = 0; j < FU_FP_SIZE; j++){
					if(all_fpFU[j].inst == NULL){
						full = false;
						break;
					}
				}	
				//If there is room in the functional unit
				if(!full){
					//Move the instruction to execution
					all_fpFU[j].inst = all_fpRS[rs_number].instruction;
					all_fpFU[j].cycles_to_completion = FU_FP_LATENCY - 1;
					all_fpFU[j].rs = rs_number;
					all_fpRS[rs_number].executing = true;
				
				}
			}
		}
	}

}
/* ECE552 Assignment 3 - END CODE */

/* ECE552 Assignment 3 - BEGIN CODE */
/* 
 * Description: 
 * 	Moves instruction(s) from the dispatch stage to the issue stage
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void dispatch_To_issue(int current_cycle) {
	instruction_t* dispatched_insn;
	//Check what type of instruction to figure out what RS is needed
	//Then see if that station is busy so we move it over
	bool RS_has_room = false;
	//If not empty, then proceed with pop
	if(ifq_pop_index != ifq_push_index){
		dispatched_insn = instr_queue[ifq_pop_index];
	} else {
		return;
	}
	dispatched_insn -> tom_dispatch_cycle ++;
	//If instruction is a branch, we just remove it from the IRQ
	//since we assume perfect branch prediction
	if(IS_UNCOND_CTRL(dispatched_insn->op)||IS_COND_CTRL(dispatched_insn->op)){
		ifq_pop_index = (ifq_pop_index + 1) % (INSTR_QUEUE_SIZE + 1);
		instr_queue_count--;
		return;
	}
	
	//Check if the instruction uses an integer functional unit
	if(USES_INT_FU(dispatched_insn->op)){
		//Check the 4 integer RS's for their busy bit
		int i;
		for(i = 0; i < RESERV_INT_SIZE; i++){
			if(all_intRS[i].busy == false){
				//If there is room in the RS, allocate
				enqueue(true, i);
				all_intRS[i].busy = true;
				all_intRS[i].instruction = dispatched_insn;
				all_intRS[i].instantiation_cycle = current_cycle;
				//For input registers, check if they are waiting on other RS's
				//If not, then get the value from the physical registers
				//First input register
				if((dispatched_insn->r_in)[0] != -1){
					if(MT[(dispatched_insn -> r_in)[0]] == -1){
						all_intRS[i].T0 = -1;			
					} else {
						all_intRS[i].T0 = MT[(dispatched_insn -> r_in)[0]];
					}
				} else {
					all_intRS[i].T0 = -1;
				}
				//Second input register
				if((dispatched_insn->r_in)[1] != -1){
					if(MT[(dispatched_insn -> r_in)[1]] == -1){
						all_intRS[i].T1 = -1;			
					} else {
						all_intRS[i].T1 = MT[(dispatched_insn -> r_in)[1]];
					}
				} else {
					all_intRS[i].T1 = -1;
				}
				//Third input register
				if((dispatched_insn->r_in)[2] != -1){
					if(MT[(dispatched_insn -> r_in)[2]] == -1){
						all_intRS[i].T2 = -1;			
					} else {
						all_intRS[i].T2 = MT[(dispatched_insn -> r_in)[2]];
					}
				} else {
					all_intRS[i].T2 = -1;
				}
						
				//Update the output registers in the map table
				if(!IS_STORE(dispatched_insn->op)){
					if((dispatched_insn->r_out)[0] != -1){
						all_intRS[i].R0 = (dispatched_insn -> r_out)[0];
						MT[(dispatched_insn -> r_out)[0]] = i;
					}
					if((dispatched_insn->r_out)[1] != -1){
						all_intRS[i].R1 = (dispatched_insn -> r_out)[1];
						MT[(dispatched_insn -> r_out)[1]] = i;
					}
				}
				RS_has_room = true;
				break;
			}
		}
	} else if(USES_FP_FU(dispatched_insn->op)){
		//Check the fp RS's for their busy bit
		int i;
		for(i = 0; i < RESERV_FP_SIZE; i++){
			if(all_fpRS[i].busy == false){
				//If there is room in the RS, allocate
				enqueue(false,i);
				all_fpRS[i].busy = true;
				all_fpRS[i].instruction = dispatched_insn;
				all_fpRS[i].instantiation_cycle = current_cycle;
				//For input registers, check if they are waiting on other RS's
				//If not, then get the value from the physical registers
				//First input register
				if((dispatched_insn->r_in)[0] != -1){
					if(MT[(dispatched_insn -> r_in)[0]] == -1){
						all_fpRS[i].T0 = -1;			
					} else {
						all_fpRS[i].T0 = MT[(dispatched_insn -> r_in)[0]];
					}
				} else {
					all_fpRS[i].T0 = -1;
				}
				//Second input register
				if((dispatched_insn->r_in)[1] != -1){
					if(MT[(dispatched_insn -> r_in)[1]] == -1){
						all_fpRS[i].T1 = -1;			
					} else {
						all_fpRS[i].T1 = MT[(dispatched_insn -> r_in)[1]];
					}
				} else {
					all_fpRS[i].T1 = -1;
				}
				//Third input register
				if((dispatched_insn->r_in)[2] != -1){
					if(MT[(dispatched_insn -> r_in)[2]] == -1){
						all_fpRS[i].T2 = -1;			
					} else {
						all_fpRS[i].T2 = MT[(dispatched_insn -> r_in)[2]];
					}
				} else {
					all_fpRS[i].T2 = -1;
				}

				//Update the output registers in the map table
				if(!IS_STORE(dispatched_insn->op)){
					if((dispatched_insn->r_out)[0] != -1){
						all_fpRS[i].R0 = (dispatched_insn -> r_out)[0];
						MT[(dispatched_insn -> r_out)[0]] = i;
					}
					if((dispatched_insn->r_out)[1] != -1){
						all_fpRS[i].R1 = (dispatched_insn -> r_out)[1];
						MT[(dispatched_insn -> r_out)[1]] = i;
					}
				}
				RS_has_room = true;
				break;
			}
		}
	} else {
		//Do nothing otherwise
	}

	//Remove the instruction from the IFQ if it was successfully moved into the RS
	if(RS_has_room){
		
		ifq_pop_index = (ifq_pop_index + 1) % (INSTR_QUEUE_SIZE + 1);
		instr_queue_count--;
	}
	return;
}
/* ECE552 Assignment 3 - END CODE */

/* ECE552 Assignment 3 - BEGIN CODE */
/* 
 * Description: 
 * 	Grabs an instruction from the instruction trace (if possible)
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * Returns:
 * 	None
 */
void fetch(instruction_trace_t* trace) {
	instruction_t* fetched_insn;
	//Make sure we fetch until we get a non-trap instruction, as required
	do{
		fetched_insn = get_instr(trace, ++fetch_index);
	} while(IS_TRAP(fetched_insn->op));
	//Pre-increment the fetch index because it is the index of the last 
	//fetched instruction, and add it to the IFQ
	//If we're at our limit of instructions, then don't add the instruction to the IFQ
	if(fetch_index <= sim_num_insn){
		instr_queue[ifq_push_index] = fetched_insn;
		ifq_push_index = (ifq_push_index + 1) % (INSTR_QUEUE_SIZE + 1);
		instr_queue_count++;
	}
  	return;
}
/* ECE552 Assignment 3 - END CODE */

/* ECE552 Assignment 3 - BEGIN CODE */
/* 
 * Description: 
 * 	Calls fetch and dispatches an instruction at the same cycle (if possible)
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void fetch_To_dispatch(instruction_trace_t* trace, int current_cycle) {
	//First check if there is room in the IFQ
	if(((ifq_push_index+1)%(INSTR_QUEUE_SIZE + 1))!=ifq_pop_index){
		fetch(trace);
		
	}
}
/* ECE552 Assignment 3 - END CODE */

/* ECE552 Assignment 3 - BEGIN CODE */
/* 
 * Description: 
 * 	Performs a cycle-by-cycle simulation of the 4-stage pipeline
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * Returns:
 * 	The total number of cycles it takes to execute the instructions.
 * Extra Notes:
 * 	sim_num_insn: the number of instructions in the trace
 */
counter_t runTomasulo(instruction_trace_t* trace)
{
  //initialize instruction queue
	int i;
	for (i = 0; i < INSTR_QUEUE_SIZE; i++) {
		instr_queue[i] = NULL;
	}

  //initialize reservation stations
	for(i = 0; i < RESERV_INT_SIZE; i++){
		all_intRS[i].busy = false;
		all_intRS[i].executing = false;
		all_intRS[i].instruction = NULL;
		all_intRS[i].R0 = -1;
		all_intRS[i].R1 = -1;
		all_intRS[i].T0 = -1;
		all_intRS[i].T1 = -1;
		all_intRS[i].T2 = -1;
		all_intRS[i].instantiation_cycle = 0;
	}
	for(i = 0; i < RESERV_FP_SIZE; i++){
		all_fpRS[i].busy = false;
		all_fpRS[i].executing = false;
		all_fpRS[i].instruction = NULL;
		all_fpRS[i].R0 = -1;
		all_fpRS[i].R1 = -1;
		all_fpRS[i].T0 = -1;
		all_fpRS[i].T1 = -1;
		all_fpRS[i].T2 = -1;	
		all_fpRS[i].instantiation_cycle = 0;
	}

  //initialize functional units
  for (i = 0; i < FU_INT_SIZE; i++) {
	all_intFU[i].inst = NULL;
	all_intFU[i].cycles_to_completion = -1;
	all_intFU[i].rs = -1;
  }
  for (i = 0; i < FU_FP_SIZE; i++) {
	all_fpFU[i].inst = NULL;
	all_fpFU[i].cycles_to_completion = -1;
	all_fpFU[i].rs = -1;
  }

  //initialize map_table to indicate no registers will be written to by future RS's
  int reg;
  for (reg = 0; reg < MD_TOTAL_REGS; reg++) {
	MT[reg] = -1;
  }

	//initialize the cdb
	cdb.T = -1;
	cdb.inst = NULL;
  
  int cycle = 1;
  while (true) {

	CDB_To_retire(cycle);
	execute_To_CDB(cycle);
	issue_To_execute(cycle);
	dispatch_To_issue(cycle);
	fetch_To_dispatch(trace, cycle);

     cycle++;
	
     if (is_simulation_done(sim_num_insn))
        break;
  }
  
  return cycle;
}
/* ECE552 Assignment 3 - END CODE */

/* ECE552 Assignment 3 - BEGIN CODE */
// Additional functions that helped in the development of this program
// Checks if an array contains a certain value
bool contains(int* array, int size, int value){
	int i;
	for(i = 0; i < size; i++){
		if(array[i] == value){
			return true;
		}
	}
	return false;
}
// Enqueue a new quantity at the tail of a linked list
void enqueue(bool int_rs, int rs_index){
	NODE* node = (NODE*)malloc(sizeof(NODE));
	node -> int_rs = int_rs;
	node -> rs_index = rs_index;
	node -> next = NULL;

	NODE* ptr;
	if(HEAD == NULL){
		HEAD = node;
		return;
	}
	for(ptr = HEAD; ptr->next != NULL; ptr = ptr->next){}
	ptr->next = node;
}
// Remove the head from the linked list
NODE* dequeue(void){
	NODE* node = HEAD;
	if(HEAD == NULL){
		return NULL;
	}
	HEAD = HEAD->next;
	return node;
}
// Remove and deallocate a node at a specified index from the linked list
void dequeue_at(NODE* target){
	NODE* node;
	if(HEAD == NULL){
		return;
	}
	if(HEAD == target){
		free(dequeue());
		return;
	}
	for(node = HEAD; (node->next)!=target; node = node->next){}
	node->next = target -> next;
	free(target);
	return;
}
/* ECE552 Assignment 3 - END CODE */
