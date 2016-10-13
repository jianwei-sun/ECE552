/* 
	Jianwei Sun 1000009821
	Yi Fang Zhang 1000029284

*/

#include "predictor.h"

#define TWO_BIT_SAT_TABLE_SIZE 8192
/////////////////////////////////////////////////////////////
// 2bitsat
/////////////////////////////////////////////////////////////
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
  while(--i >= 0){
	*(state_table_2bitsat + i) = 1;
  }
}

bool GetPrediction_2bitsat(UINT32 PC) {
  int state = *(state_table_2bitsat + (int)(PC & (UINT32)0x3FF));
  return state <= 1 ? NOT_TAKEN : TAKEN;
}

void UpdatePredictor_2bitsat(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  int current_state = *(state_table_2bitsat + (int)(PC & (UINT32)0x3FF));
  if(resolveDir == TAKEN)
	current_state = ((current_state + 1) > 3 ? 3 : (current_state + 1));
  else 
	current_state = ((current_state - 1) < 0 ? 0 : (current_state - 1));
  *(state_table_2bitsat + (int)(PC & (UINT32)0x3FF)) = current_state;
}

/////////////////////////////////////////////////////////////
// 2level
/////////////////////////////////////////////////////////////
unsigned char *bht_2level;
unsigned char **pht_pointers;
void InitPredictor_2level() {
  bht_2level = (unsigned char*)malloc(512*sizeof(unsigned char));
  pht_pointers = (unsigned char**)malloc(64*sizeof(unsigned char*));
  int i, j;
  for(i = 0; i < 64; i++){
    *(pht_pointers + i) = (unsigned char*)malloc(8*sizeof(unsigned char));
    unsigned char* single_PHT_table = *(pht_pointers + i);
    for(j = 0; j < 8; j++){
      *(single_PHT_table + i ) = 1;
    }
  }
}

bool GetPrediction_2level(UINT32 PC) {
  int PHT_index, BHT_index, state, BHR;
  unsigned char* PHT_table;
  PHT_index = (int)(PC & (UINT32)0x7);
  BHT_index = (int)((PC & (UINT32)0xFF8) >> 3);
  BHR = *(bht_2level + BHT_index) & 0x3F;
  PHT_table = *(pht_pointers + BHR);
  state = *(PHT_table + PHT_index);
  return state <= 1 ? NOT_TAKEN : TAKEN;
}

void UpdatePredictor_2level(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  int PHT_index, BHT_index, state, BHR;
  unsigned char* PHT_table;
  PHT_index = (int)(PC & (UINT32)0x7);
  BHT_index = (int)((PC & (UINT32)0xFF8) >> 3);
  BHR = *(bht_2level + BHT_index) & 0x3F;
  PHT_table = *(pht_pointers + BHR);
  state = *(PHT_table + PHT_index);
  if(resolveDir == TAKEN){
	state = ((state + 1) > 3 ? 3 : (state + 1));
	BHR = (BHR << 1) | 0x01;
  }
  else{
	state = ((state - 1) < 0 ? 0 : (state - 1));
        BHR = (BHR << 1);
  }  
  BHR = BHR & 0x3F;
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

/*
unsigned char *bht_2level;
unsigned char **pht_pointers;
void InitPredictor_2level() {
  bht_2level = (unsigned char*)malloc(512*sizeof(unsigned char));
  pht_pointers = (unsigned char**)malloc(8*sizeof(unsigned char*));
  int i, j;
  for(i = 0; i < 8; i++){
    *(pht_pointers + i) = (unsigned char*)malloc(64*sizeof(unsigned char));
    unsigned char* single_PHT_table = *(pht_pointers + i);
    for(j = 0; j < 64; j++){
      *(single_PHT_table + i ) = 1;
    }
  }
}

bool GetPrediction_2level(UINT32 PC) {
  int PHT_index, BHT_index, state, BHR;
  unsigned char* PHT_table;
  PHT_index = (int)(PC & (UINT32)0x7);
  BHT_index = (int)((PC & (UINT32)0xFF8) >> 3);
  BHR = *(bht_2level + BHT_index) & 0x3F;
  PHT_table = *(pht_pointers + PHT_index);
  state = *(PHT_table + BHR);
  return state <= 1 ? NOT_TAKEN : TAKEN;
}

void UpdatePredictor_2level(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  int PHT_index, BHT_index, state, BHR;
  unsigned char* PHT_table;
  PHT_index = (int)(PC & (UINT32)0x7);
  BHT_index = (int)((PC & (UINT32)0xFF8) >> 3);
  BHR = *(bht_2level + BHT_index) & 0x3F;
  PHT_table = *(pht_pointers + PHT_index);
  state = *(PHT_table + BHR);
  if(resolveDir == TAKEN){
	state = ((state + 1) > 3 ? 3 : (state + 1));
	BHR = (BHR << 1) | 0x01;
  }
  else{
	state = ((state - 1) < 0 ? 0 : (state - 1));
        BHR = (BHR << 1);
  }  
  BHR = BHR & 0x3F;
  *(PHT_table + BHR) = state;
  *(bht_2level + BHT_index) = BHR;
}

*/

