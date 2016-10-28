/* 
	Jianwei Sun 1000009821
	Yi Fang Zhang 1000029284

*/

#include "predictor.h"
#include <math.h>

#define TWO_BIT_SAT_TABLE_SIZE 8192
/////////////////////////////////////////////////////////////
// 2bitsat
/////////////////////////////////////////////////////////////
/*
	represents each saturation counter as a byte
*/
int *state_table_2bitsat; 

void InitPredictor_2bitsat() { 
  /* 
	0 - Super weak not taken
	1 - Weak not taken
	2 - Weak taken
	3 - Strong taken
  */
  state_table_2bitsat = (int*)malloc(TWO_BIT_SAT_TABLE_SIZE/8*sizeof(int)); 
  int i = TWO_BIT_SAT_TABLE_SIZE/8;
  /*
	initializing all saturation counters to weak not taken
  */
  while(--i >= 0){
	*(state_table_2bitsat + i) = 1;
  }
}

bool GetPrediction_2bitsat(UINT32 PC) {
  /*
 	get the 10 least significant bits PC
	use the 10 bit quantity to index the state table
  */
  int state = *(state_table_2bitsat + (int)(PC & (UINT32)0x3FF)); 
  /*
	determines prediction as TAKEN/NOT_TAKEN
  */
 return state <= 1 ? NOT_TAKEN : TAKEN;
}

void UpdatePredictor_2bitsat(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  /*
	get the current state of the bimodal counter
  */
  int current_state = *(state_table_2bitsat + (int)(PC & (UINT32)0x3FF));
  /*
	Updates the bimodel counter:
		if branch was taken, increment counter
		else if branch was not taken, decrement counter
  */
  if(resolveDir == TAKEN)
	current_state = ((current_state + 1) > 3 ? 3 : (current_state + 1));
  else 
	current_state = ((current_state - 1) < 0 ? 0 : (current_state - 1));
  /*
	write new state back into the state table
  */
  *(state_table_2bitsat + (int)(PC & (UINT32)0x3FF)) = current_state;
}

/////////////////////////////////////////////////////////////
// 2level
/////////////////////////////////////////////////////////////
/*
	initializing bht and pht tables as pointers
	
*/
int *bht_2level;
int **pht_pointers;

void InitPredictor_2level() {
  /*
	allocating memory:
		bht is a table of 512 BHR, each represented as an unsigned char
		pht_pointers is a table of 8 pht table pointers, each pointing to a table of 64 bimodal counters 
  */
  bht_2level = (int*)malloc(512*sizeof(int));
  pht_pointers = (int**)malloc(8*sizeof(int*));
  int i, j;
  for(i = 0; i < 8; i++){
    /*
	creating each of the 8 pht tables with 64 entries
    */
    *(pht_pointers + i) = (int*)malloc(64*sizeof(int));
    int* single_PHT_table = *(pht_pointers + i);
    /*
	initialize all bimodal counters to be weak not taken
    */
    for(j = 0; j < 64; j++){
      *(single_PHT_table + i ) = 1;
    }
  }
}

bool GetPrediction_2level(UINT32 PC) {
  int PHT_index, BHT_index, state, BHR;
  int* PHT_table;
  /*
	determine which of the 8 PHT tables to look at based on last 3 bits of PC
	determine which BHR to look at based on the next 9 bits of PC
  */
  PHT_index = (int)(PC & (UINT32)0x7);
  BHT_index = (int)((PC & (UINT32)0xFF8) >> 3);
  /*
	reading value of BHR, truncating to 6 bits
  */
  BHR = *(bht_2level + BHT_index) & 0x3F;
  /*
	determine which bimodal counter of PHT table to look at 
  */
  PHT_table = *(pht_pointers + PHT_index);
  /*
	get state of the bimodal counter
	return TAKEN/NOT_TAKEN
  */
  state = *(PHT_table + BHR);
  return state <= 1 ? NOT_TAKEN : TAKEN;
}

void UpdatePredictor_2level(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  /*
	same proceedure as above to get current state
  */
  int PHT_index, BHT_index, state, BHR;
  int* PHT_table;
  PHT_index = (int)(PC & (UINT32)0x7);
  BHT_index = (int)((PC & (UINT32)0xFF8) >> 3);
  BHR = *(bht_2level + BHT_index) & 0x3F;
  PHT_table = *(pht_pointers + PHT_index);
  state = *(PHT_table + BHR);
  /*
	Updating the state of bimodal counter (same proceedure as 2bit bimodal predictor)
	add branch outcome to corresponding BHR
  */
  if(resolveDir == TAKEN){
	state = ((state + 1) > 3 ? 3 : (state + 1));
	BHR = (BHR << 1) | 0x01;
  }
  else{
	state = ((state - 1) < 0 ? 0 : (state - 1));
        BHR = (BHR << 1);
  }

  /*
	Mask BHR to 6 bits
  */  
  BHR = BHR & 0x3F;
  /*
	update state and BHR in their respective tables
  */
  *(PHT_table + BHR) = state;
  *(bht_2level + BHT_index) = BHR;
}

/////////////////////////////////////////////////////////////
// openend
/////////////////////////////////////////////////////////////
#define NUMBER_T_BLOCKS 9
#define TBLOCK_SIZE 8192
#define FIRST_BLOCK_SIZE 8192 //Fixed number
#define FIRST_BLOCK_PC_MASK 0x000001FF  //Based on the above fixed number
#define INIT_BIMODAL_STATE 3 //Weak not taken for a 3 bit counter
#define INIT_USABILITY_LEVEL 0  //No usefulness for a 2 bit counter
#define GHR_LENGTH 512
#define TAG_SIZE 11
#define RESET_PERIOD 500000

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

//Data structures
typedef struct TBlocks{
  int *bimodal;
  UINT32 *tag; 
  int *u;
} TBlock;


//Function prototypes
int update_bimodal_counter(int, bool);
void increment_u(TBlock*, int);
void decrement_u(TBlock*, int);
void allocate_row(TBlock*, int, bool, UINT32, int);
void update_bimodal(TBlock*, int, bool);
bool get_prediction(int);
void update_GHR(bool);
UINT32 hash1(UINT32,int);
UINT32 hash2(UINT32,int);

//Global variables
UINT32 GHR[(GHR_LENGTH+32)/32] = {0};
TBlock **all_Tblocks;
TBlock *predict_block;
int predict_block_index;
int predict_block_was_last;
UINT32 hash1_results[NUMBER_T_BLOCKS];
UINT32 hash2_results[NUMBER_T_BLOCKS];
int hash_lengths[NUMBER_T_BLOCKS] = {0,5,10,18,32,64,84,150,512};
int TSIZES[NUMBER_T_BLOCKS] = {512,896,1024,1024,1024,1024,1024,1024,1024};
int branch_counter = 0;

//Initializes all the data structures
void InitPredictor_openend() {
	int i,j;
	//all_Tblocks is an array of TBlock pointers
	all_Tblocks = (TBlock**)malloc(NUMBER_T_BLOCKS*sizeof(TBlock*));
	for(i = 0; i < NUMBER_T_BLOCKS; i++){
		//For each TBlock, allocate size for the various pointers
		*(all_Tblocks + i) = (TBlock*)calloc(1,sizeof(TBlock));
		(*(all_Tblocks + i)) -> bimodal = (int*)malloc(TSIZES[i]*sizeof(int));
		(*(all_Tblocks + i)) -> tag = (UINT32*)malloc(TSIZES[i]*sizeof(UINT32));
		(*(all_Tblocks + i)) -> u = (int*)malloc(TSIZES[i]*sizeof(int));
		//For each pointer type, allocate according to how much space is given to the TBlock
		for(j = 0; j < TSIZES[i]; j++){
			((*(all_Tblocks + i)) -> bimodal)[j] = (int)INIT_BIMODAL_STATE;
			((*(all_Tblocks + i)) -> u)[j] = (int)INIT_USABILITY_LEVEL;
			((*(all_Tblocks + i)) -> tag)[j] = (UINT32)0;
		}
	}
	return;
}

bool GetPrediction_openend(UINT32 PC) {
	branch_counter++;
	//Compute all of the hashes
	int i;
	TBlock *block;
	bool prediction;
	//Get the prediction from the T0 block as the default prediction
	prediction = get_prediction(((all_Tblocks[0])->bimodal)[PC%TSIZES[0]]);
	//Record details about prediction to make accessing easier in UpdatePredictor
	predict_block = all_Tblocks[0];
	predict_block_index = 0;
	predict_block_was_last = 0;
	for(i = NUMBER_T_BLOCKS-1; i > 0; i--){
		//Iterate from longest history block to the shortest history block
		hash1_results[i] = hash1(PC, hash_lengths[i])%(TSIZES[i]);
		hash2_results[i] = hash2(PC, hash_lengths[i])%(0x00000001<<TAG_SIZE);		
		block = all_Tblocks[i];
		if(*((block->tag)+hash1_results[i]) == hash2_results[i]){  //Get the prediction if tags match
			//And break and use the resulting prediction as TAGE's prediction			
			prediction = get_prediction((block->bimodal)[hash1_results[i]]);
			predict_block = block;
			predict_block_index = i;
			predict_block_was_last = (i == (NUMBER_T_BLOCKS-1));
			break;
		} else {  //Otherwise, loop k at the next shorter-history-length tblock
			continue;
		}
	}
	return prediction;
}

void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	//Update bimodal
	if(predict_block_index == 0){
		update_bimodal(predict_block,PC%TSIZES[0],resolveDir);
	} else {
		update_bimodal(predict_block,hash1_results[predict_block_index],resolveDir);
	}
	if(predDir == resolveDir){      //ON CORRECT PREDICTIONS
		increment_u(predict_block,hash1_results[predict_block_index]);
	} else {			//ON INCORRECT PREDICTIONS
		if(predict_block_was_last == 1){  //Prediction came from last Tblock
			
		} else {  //Prediction came from not last Tblock
			int i = predict_block_index;
			TBlock *block;
			//Allocating
			for(i = predict_block_index + 1; i < NUMBER_T_BLOCKS; i++){
				block = all_Tblocks[i];
				if((block->u)[hash1_results[i]] == 0){
					allocate_row(block,hash1_results[i],resolveDir,hash2_results[i],INIT_USABILITY_LEVEL);
					break;
				} else {
					decrement_u(block,hash1_results[i]);
					continue;
				}
			}
		}
	}
	//Update the global history register to track the resolveDir
	update_GHR(resolveDir);
	//Reset occasionally for small improvement
}
void increment_u(TBlock* tblock, int i){
	//Increment u by one, making sure not to overflow
	(tblock->u)[i] = (((tblock->u)[i] + 1) > 3) ? 3 : ((tblock->u)[i] + 1);
	return;
}
void decrement_u(TBlock* tblock, int i){
	//Decrement u by one, making sure not to underflow
	(tblock->u)[i] = (((tblock->u)[i] - 1) < 0) ? 0 : ((tblock->u)[i] - 1);
	return;
}
void allocate_row(TBlock* tblock, int index, bool taken, UINT32 tag, int u){
	//Creates an entry in a Tblock given initialization variables
	(tblock->tag)[index] = tag;
	(tblock->u)[index] = u;
	(tblock->bimodal)[index] = (taken == TAKEN) ? 4 : 3;
	return;
}
void update_bimodal(TBlock* tblock, int index, bool taken){
	//Update the bimodal counter given the branch state
	//Ensures no overflowing or underflowing occurs
	int current = (tblock->bimodal)[index];
	if(taken == TAKEN){
		(tblock->bimodal)[index] = ((current + 1) > 7) ? 7 : (current + 1);
	} else {
		(tblock->bimodal)[index] = ((current - 1) < 0) ? 0 : (current - 1);
	}
	return;
}
bool get_prediction(int bimodal_count){
	//Parse the bimodal counter to yield a boolean result
	if(bimodal_count < 4){
		return NOT_TAKEN;
	} else {
		return TAKEN;
	}
}
void update_GHR(bool taken){
	//Essentially shifts everything by one location
	int GHR_size = (GHR_LENGTH+32)/32;
	int i = GHR_size - 1;
	UINT32 msb, old_msb;
	//Records the new branch result
	if(taken == TAKEN){
		old_msb = 0x00000001;
	} else {
		old_msb = 0x00000000;
	}
	for(i = GHR_size - 1; i >= 0; i--){
		msb = (GHR[i] & 0x80000000) >> 31;
		GHR[i] = GHR[i] << 1;
		GHR[i] = GHR[i] | old_msb;
		old_msb = msb;
	}
	msb = 0x80000000;
	for(i = 0; i < (32 - (GHR_LENGTH%32)); i++){
		GHR[0] = GHR[0] & ~msb;
		msb = msb >> 1;
	}
	return;
}
UINT32 hash1(UINT32 PC, int length){
	//Takes a width of 13 and folds the GHR and PC into a stack, then XOR
	int i = 0, ratio = length%32,full_ints = length/32,num_GHRs = GHR_LENGTH/32+1;
	UINT32 hash = 0, temp = PC>>2, accumulator = 0;
	while(temp != 0){
		hash ^= (temp&0x00001FFF);
		temp = temp >> 13;
	}
	for(i = 0; i < ratio; i++){
		accumulator |= (GHR[num_GHRs - full_ints - 1]&(1<<i));
	}
	while(accumulator != 0){
		hash ^= (accumulator&0x00001FFF);
		accumulator = accumulator >> 13;
	}
	for(i = 0; i < full_ints; i++){
		temp = GHR[num_GHRs-i-1];
		while(temp != 0){
			hash ^= (temp&0x00001FFF);
			temp = temp >> 13;
		}
	}
	return hash;
}

UINT32 hash2(UINT32 PC, int length){
	//Takes a width of 7 and folds the GHR and PC into a stack, then XOR
	int i = 0, ratio = length%32,full_ints = length/32,num_GHRs = GHR_LENGTH/32+1;
	UINT32 hash = 0, temp = PC>>2, accumulator = 0;
	while(temp != 0){
		hash ^= (temp&0x00001FFF);
		temp = temp >> 7;
	}
	for(i = 0; i < ratio; i++){
		accumulator |= (GHR[num_GHRs - full_ints - 1]&(1<<i));
	}
	while(accumulator != 0){
		hash ^= (accumulator&0x00001FFF);
		accumulator = accumulator >> 7;
	}
	for(i = 0; i < full_ints; i++){
		temp = GHR[num_GHRs-i-1];
		while(temp != 0){
			hash ^= (temp&0x00001FFF);
			temp = temp >> 7;
		}
	}
	return hash;
}


