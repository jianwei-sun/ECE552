/* 
	Jianwei Sun 1000009821
	Yi Fang Zhang 1000029284

*/

#include "predictor.h"

#define TWO_BIT_SAT_TABLE_SIZE 8192
/////////////////////////////////////////////////////////////
// 2bitsat
/////////////////////////////////////////////////////////////
/*
	represents each saturation counter as a byte
*/
unsigned char *state_table_2bitsat; 

void InitPredictor_2bitsat() { 
  /* 
	0 - Super weak not taken
	1 - Weak not taken
	2 - Weak taken
	3 - Strong taken
  */
  state_table_2bitsat = (unsigned char*)malloc(TWO_BIT_SAT_TABLE_SIZE/8*sizeof(unsigned char)); 
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
unsigned char *bht_2level;
unsigned char **pht_pointers;

void InitPredictor_2level() {
  /*
	allocating memory:
		bht is a table of 512 BHR, each represented as an unsigned char
		pht_pointers is a table of 8 pht table pointers, each pointing to a table of 64 bimodal counters 
  */
  bht_2level = (unsigned char*)malloc(512*sizeof(unsigned char));
  pht_pointers = (unsigned char**)malloc(8*sizeof(unsigned char*));
  int i, j;
  for(i = 0; i < 8; i++){
    /*
	creating each of the 8 pht tables with 64 entries
    */
    *(pht_pointers + i) = (unsigned char*)malloc(64*sizeof(unsigned char));
    unsigned char* single_PHT_table = *(pht_pointers + i);
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
  unsigned char* PHT_table;
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
  unsigned char* PHT_table;
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

void InitPredictor_openend() {

}

bool GetPrediction_openend(UINT32 PC) {

  return TAKEN;
}

void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {

}

