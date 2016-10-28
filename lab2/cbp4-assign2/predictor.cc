/* 
	Jianwei Sun 1000009821
	Yi Fang Zhang 1000029284

*/

#include "predictor.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define for_endian(size) for (int i = 0; i < size; ++i)
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define for_endian(size) for (int i = size - 1; i >= 0; --i)
#else
#error "Endianness not detected"
#endif

#define printb(value)                                   \
({                                                      \
        typeof(value) _v = value;                       \
        __printb((typeof(_v) *) &_v, sizeof(_v));       \
})

void __printb(void *value, size_t size)
{
        uint8_t byte;
        size_t blen = sizeof(byte) * 8;
        uint8_t bits[blen + 1];

        bits[blen] = '\0';
        for_endian(size) {
                byte = ((uint8_t *) value)[i];
                memset(bits, '0', blen);
                for (int j = 0; byte && j < blen; ++j) {
                        if (byte & 0x80)
                                bits[j] = '1';
                        byte <<= 1;
                }
                printf("%s ", bits);
        }
        printf("\n");
}

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
#define FIRST_BLOCK_PC_MASK 0x00007FFC  //Based on the above fixed number
#define INIT_BIMODAL_STATE 3 //Weak not taken for a 3 bit counter
#define INIT_USABILITY_LEVEL 0  //No usefulness for a 2 bit counter
#define GHR_LENGTH 512

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

//Data structures
typedef struct TBlocks{
  int bimodal[TBLOCK_SIZE];
  UINT32 tag[TBLOCK_SIZE]; 
  int u[TBLOCK_SIZE];
} TBlock;


//Function prototypes
int update_bimodal_counter(int, bool);
void increment_u(TBlock*, int);
void decrement_u(TBlock*, int);
void allocate_row(TBlock*, int, bool, int, int);
void update_bimodal(TBlock*, int, bool);
bool get_prediction(int);
void update_GHR(bool);
UINT32 hash1(UINT32,int);
UINT32 hash2(UINT32,int);
void random_break(void);


//Global variables
UINT32 GHR[(GHR_LENGTH+32)/32] = {0};
TBlock **all_Tblocks;
TBlock *predict_block;
int predict_block_index;
int predict_block_was_last;
UINT32 hash1_results[NUMBER_T_BLOCKS];
UINT32 hash2_results[NUMBER_T_BLOCKS];
int hash_lengths[NUMBER_T_BLOCKS] = {0,5,10,18,32,64,84,150,512};
int counter = 0; 

//Debugging
int T0_gave_prediction = 0;
int T0_correct_prediction = 0;
int T_gave_prediction = 0;
int T_correct_prediction = 0;

//Initializes all the data structures
void InitPredictor_openend() {
	int i,j;
	all_Tblocks = (TBlock**)malloc(NUMBER_T_BLOCKS*sizeof(TBlock*));
	for(i = 0; i < NUMBER_T_BLOCKS; i++){
		*(all_Tblocks + i) = (TBlock*)calloc(1,sizeof(TBlock));
		for(j = 0; j < TBLOCK_SIZE; j++){
			((*(all_Tblocks + i)) -> bimodal)[j] = (int)INIT_BIMODAL_STATE;
			((*(all_Tblocks + i)) -> u)[j] = (int)INIT_USABILITY_LEVEL;
			((*(all_Tblocks + i)) -> tag)[j] = 0;
		}
	}
	return;
}

bool GetPrediction_openend(UINT32 PC) {
	//Compute all of the hashes
	int i;
	TBlock *block;
	bool prediction;
	//Get the prediction from the T0 block as the default prediction
	prediction = get_prediction(((all_Tblocks[0])->bimodal)[(PC&FIRST_BLOCK_PC_MASK)>>2]);
	predict_block = all_Tblocks[0];
	predict_block_index = 0;
	predict_block_was_last = 0;
	for(i = NUMBER_T_BLOCKS-1; i > 0; i--){
		//hash1_results[i] = (hash1(PC, hash_lengths[i])%TBLOCK_SIZE);
		hash1_results[i] = (hash1(PC, hash_lengths[i]));
		hash2_results[i] = hash2(PC, hash_lengths[i]);		
		//hash1_results[i] = PC%TBLOCK_SIZE;
		//hash2_results[i] = PC;
		block = all_Tblocks[i];
		if(((block->tag)[hash1_results[i]]) == hash2_results[i]){  //Get the prediction if tags match			
			T_gave_prediction= T_gave_prediction+1;
			prediction = get_prediction((block->bimodal)[hash1_results[i]]);
			predict_block = block;
			predict_block_index = i;
			predict_block_was_last = (i == (NUMBER_T_BLOCKS-1));

			break;
		} else {  //Otherwise, look at the next shorter-history-length tblock
			continue;
		}
	}
	i = (i<0) ? 0 : i; //Make sure is not -1
	if(i == 0) {T0_gave_prediction++;}
	return prediction;
}

void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	/*counter ++;
	if(counter > 1000000){
		random_break();
	}*/
	if(predict_block_index == 0){
		if(resolveDir == predDir){
			T0_correct_prediction++;}
	}else{
		if(resolveDir == predDir){
			T_correct_prediction++;}
	}
	//Update bimodal
	if(predict_block_index == 0){
		update_bimodal(predict_block,(PC&FIRST_BLOCK_PC_MASK)>>2,resolveDir);
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
	update_GHR(resolveDir);
	return;
}

void usage_stats(void){
	printf("PRINTING USAGE STATS:\n");
	int i,j,num_use[NUMBER_T_BLOCKS] = {0};
	TBlock* block;
	for(i = 0; i < NUMBER_T_BLOCKS; i++){
		block = all_Tblocks[i];
		for(j=0;j<TBLOCK_SIZE;j++){
			if((block->u)[j] > 0){
				num_use[i]++;
				//printf("Block%d Entry%d u value%d\n",i,j,(block->u)[j]);
			}
		}
	}
	
	printf("T0_gave_prediction:    %d\n",T0_gave_prediction);
	printf("T0_correct_prediction: %d\n",T0_correct_prediction);
	printf("T_gave_prediction:     %d\n",T_gave_prediction);
	printf("T_correct_prediction:  %d\n",T_correct_prediction);
	printf("\n");
	for(i = 0; i < NUMBER_T_BLOCKS; i++){
		printf("T%d number of useful entries: %d\n", i, num_use[i]);
	}
	printf("\nnumber of u increments: %d\n",counter);
	printf("\nGHR:\n");
	for(i=0;i<(GHR_LENGTH+32)/32;i++){
		printb(GHR[i]);
	}
	printb(0x21219393);
	printb(100);
	printf("\n\n");
}

void random_break(void){
	return;
}
void increment_u(TBlock* tblock, int i){
	counter++;
	(tblock->u)[i] = (((tblock->u)[i] + 1) > 3) ? 3 : ((tblock->u)[i] + 1);
	//(tblock->u)[i] = ((tblock->u)[i] + 1);
	return;
}
void decrement_u(TBlock* tblock, int i){
	(tblock->u)[i] = (((tblock->u)[i] - 1) < 0) ? 0 : ((tblock->u)[i] - 1);
	return;
}
void allocate_row(TBlock* tblock, int index, bool taken, int tag, int u){
	(tblock->tag)[index] = tag;
	(tblock->u)[index] = u;
	(tblock->bimodal)[index] = (taken == TAKEN) ? 4 : 3;
	return;
}
void update_bimodal(TBlock* tblock, int index, bool taken){
	int current = (tblock->bimodal)[index];
	if(taken == TAKEN){
		(tblock->bimodal)[index] = ((current + 1) > 7) ? 7 : (current + 1);
	} else {
		(tblock->bimodal)[index] = ((current - 1) < 0) ? 0 : (current - 1);
	}
	return;
}
bool get_prediction(int bimodal_count){
	if(bimodal_count < 4){
		return NOT_TAKEN;
	} else {
		return TAKEN;
	}
}
void update_GHR(bool taken){
	int GHR_size = (GHR_LENGTH+32)/32;
	int i = GHR_size - 1;
	UINT32 msb, old_msb;
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
	//Need 13 bits to index 8192 entries
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
	/*return PC;
	int full_ints = length/32;
	int ratio = length%32;
	UINT32 hash, accumulator = 0, prime = 31;
	int num_GHRs = GHR_LENGTH/32+1;
	int i;
	for(i = 0; i < ratio; i++){
		accumulator |= (GHR[num_GHRs - full_ints - 1]&(1<<i));
	}
	for(i = 0; i < full_ints; i++){
		accumulator ^= (prime * GHR[num_GHRs-i-1]);
	}	

	hash = max(PC, accumulator);
	if(hash == accumulator)
	{
		hash = PC*PC + accumulator;
	}
	else
	{
		hash = accumulator*accumulator + accumulator + PC;
	}
	return hash;*/
}

UINT32 hash2(UINT32 PC, int length){
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

	/*int full_ints = length/32;
	int ratio = length%32;
	UINT32 accumulator = 0, prime = 1500450271, offset = 3267000013;
	int num_GHRs = GHR_LENGTH/32+1;
	int i;
	for(i = 0; i < ratio; i++){
		accumulator |= (GHR[num_GHRs - full_ints - 1]&(1<<i));
	}
	accumulator += offset;
	for(i = 0; i < full_ints; i++){
		accumulator ^= (prime * GHR[num_GHRs-i-1]);
	}	
	accumulator ^= (prime * PC);
	return accumulator;*/
}

/*UINT32 hash2(UINT32 PC, int level){
	UINT32 GHR_int = 0;
	int mask = 0xFFFFFFFF;
	int i;
	int hash;


	for

	if(level < 5)
	{
		mask = mask >> (32 - (2 << level));
		GHR_int = GHR[0] & mask;
	}
	else if (level == 5)
	{
		GHR_int = GHR[0];
	}
	else
	{
		level = level - 5;
		for(i=0; i < level ; i++)
		{
			GHR_int = GHR[i] * 33 + GHR_int;			
		}	
	}

}
*/
/*
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
}*/
