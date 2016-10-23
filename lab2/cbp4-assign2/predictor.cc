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
#define N_BUDGET 17

#define NUMBER_T_BLOCKS 7
#define TBLOCK_SIZE 1024
#define INIT_BIMODAL_STATE 3
#define INIT_USABILITY_LEVEL 0
#define HISTORY_LENGTH 150

#define FIRST_BLOCK_SIZE 8192

typedef struct TBlocks{
  int bimodal[TBLOCK_SIZE];
  unsigned int tag[TBLOCK_SIZE]; 
  int u[TBLOCK_SIZE];
} TBlock;

unsigned int hash_1(UINT32, int*, int);
int entry_exists(TBlock*, unsigned int);
bool to_take(int);
int update_bimodal_counter(int, bool);
int find_empty(TBlock*);
void decrement_all_useful(TBlock*);
int rand_select_Tblock(int);

int* FirstBlock;
TBlock **all_Tblocks;
int BHR[HISTORY_LENGTH] = {0};
int history_depths[NUMBER_T_BLOCKS] = {2,4,8,17,36,73,150};

unsigned int hash_results[NUMBER_T_BLOCKS] = {0};
int pred_results[NUMBER_T_BLOCKS] = {0};
int mux_results[NUMBER_T_BLOCKS] = {0};

void InitPredictor_openend() {
	int i, j;
	FirstBlock = (int*)malloc(FIRST_BLOCK_SIZE*sizeof(int));
	for(i = 0; i < FIRST_BLOCK_SIZE; i++){
		FirstBlock[i] = INIT_BIMODAL_STATE;
	}
	
	all_Tblocks = (TBlock**)malloc(NUMBER_T_BLOCKS*sizeof(TBlock*));
	for(i = 0; i < NUMBER_T_BLOCKS; i++){
		*(all_Tblocks + i) = (TBlock*)calloc(1,sizeof(TBlock));
		for(j = 0; j < TBLOCK_SIZE; j++){
			((*(all_Tblocks + i)) -> bimodal)[j] = (int)INIT_BIMODAL_STATE;
			((*(all_Tblocks + i)) -> u)[j] = (int)INIT_USABILITY_LEVEL;
		}
	}
}

bool GetPrediction_openend(UINT32 PC) {
	int i;
	for(i = 0; i < NUMBER_T_BLOCKS; i++){
		int entry_index;
		hash_results[i] = hash_1(PC, BHR, history_depths[i]);
		entry_index = entry_exists(*(all_Tblocks+i), hash_results[i]);
		if(entry_index == -1){
			mux_results[i] = 0;
		} else {
			pred_results[i] = ((*(all_Tblocks + i)) -> bimodal)[entry_index];
			mux_results[i] = 1;
		}
	}
	
	int outcome = FirstBlock[(PC>>2)&8191];
	for(i = NUMBER_T_BLOCKS - 1; i >=0; i--){
		if(mux_results[i] == 1){
			outcome = pred_results[i];
			break;
		}
	} 
	return to_take(outcome);
}

void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	int i = 0, j, entry_index, not_useful_index, target_tblock;
	TBlock *tblock = NULL;
	for(i = NUMBER_T_BLOCKS - 1; i >= 0; i--){
		if(mux_results[i] == 1){
			break;
		}
	}
	if(i<0){
		i = 0;
	}
	
	// i is the index of which T block gave the correct prediction
	if(i == 0 && mux_results[i] == 0){	//Result came from T0
		FirstBlock[(PC>>2)&8191] = update_bimodal_counter(FirstBlock[(PC>>2)&8191],resolveDir);
	} 
	else {	//Otherwise result came from one of the Tblocks
		tblock = *(all_Tblocks + i);
		entry_index = entry_exists(tblock, hash_results[i]);
		(tblock->bimodal)[entry_index] = update_bimodal_counter((tblock->bimodal)[entry_index],resolveDir);	
	}
	
	if(resolveDir != predDir){	//If the prediction was false
		if(tblock == NULL){	//Start search at T1
			target_tblock = rand_select_Tblock(0);
		} else {	//Otherwise, start search from i + 1
			target_tblock = rand_select_Tblock(i + 1);
		}
		tblock = *(all_Tblocks + target_tblock);
		not_useful_index = find_empty(tblock);
		if(not_useful_index != -1){
			tblock->tag[not_useful_index] = hash_results[i];
			tblock->bimodal[not_useful_index] = INIT_BIMODAL_STATE;
			tblock->u[not_useful_index] = INIT_USABILITY_LEVEL;
		} else {
			decrement_all_useful(tblock);	
		}	
			
	}

	for(j = HISTORY_LENGTH - 1; j >= 1; j--){
		BHR[j] = BHR[j-1];
	}
	BHR[0] = (resolveDir == TAKEN) ? 1 : 0;
}

int entry_exists(TBlock* tblock, unsigned int hash){
  int i;
  for(i = 0; i < TBLOCK_SIZE; i++){
    if(((tblock->tag)[i] == hash)){
      return i;
    }
  }
  return -1;
}

unsigned int hash_1(UINT32 PC, int* HR, int length){
  unsigned int hash = (unsigned int)PC;
  int i;
  for(i = 0; i < length; i++){
    hash = (unsigned int)(pow((double)2, (double)(length - i - 1))*(double)(HR[i])) + hash;
  }
  return hash;
}

bool to_take(int prediction){
	return (prediction <= 3) ? NOT_TAKEN : TAKEN;
}

int update_bimodal_counter(int current, bool taken){
	if(taken == TAKEN){
		return ((current + 1) > 7) ? 7 : (current + 1);
	} else {
		return ((current - 1) < 0) ? 0 : (current - 1);
	}
}

int find_empty(TBlock* tblock){
	int i;
	for(i = 0; i < TBLOCK_SIZE; i++){
		if((tblock->u)[i] == 0){
			return i;
		}
	}
	return -1;
}

void decrement_all_useful(TBlock* tblock){
	int i;
	for(i = 0; i < TBLOCK_SIZE; i++){
		(tblock->u)[i] = (((tblock->u)[i] - 1) < 0) ? 0 : ((tblock->u)[i] - 1);
	}
}

int rand_select_Tblock(int start){
	//start is where we start searching
	int i, result = 0;
	if(start == NUMBER_T_BLOCKS){
		return NUMBER_T_BLOCKS-1;
	}
	for(i = 0; i < (NUMBER_T_BLOCKS-start); i++){
		if((rand()%3) > 0){	// two-thirds chance
			result = i;
			break;
		}
	}
	return start+result;
}
