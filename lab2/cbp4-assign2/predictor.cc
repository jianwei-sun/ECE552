/* 
	Jianwei Sun 1000009821
	Yi Fang Zhang 1000029284

*/

#include "predictor.h"

/////////////////////////////////////////////////////////////
// 2bitsat
/////////////////////////////////////////////////////////////
int state_2bitsat;

void InitPredictor_2bitsat() {
  /* 
	0 - Super weak not taken
	1 - Weak not taken
	2 - Weak taken
	3 - Strong taken
  */
  state_2bitsat = 1; //Weak not-taken as the intial state
}

bool GetPrediction_2bitsat(UINT32 PC) {
  return state_2bitsat <= 1 ? NOT_TAKEN : TAKEN;
}

void UpdatePredictor_2bitsat(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  if(resolveDir == TAKEN)
	state_2bitsat = ((state_2bitsat + 1) > 3 ? 3 : (state_2bitsat + 1));
  else 
	state_2bitsat = ((state_2bitsat - 1) < 0 ? 0 : (state_2bitsat - 1));
}

/////////////////////////////////////////////////////////////
// 2level
/////////////////////////////////////////////////////////////

void InitPredictor_2level() {

}

bool GetPrediction_2level(UINT32 PC) {

  return TAKEN;
}

void UpdatePredictor_2level(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {

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

