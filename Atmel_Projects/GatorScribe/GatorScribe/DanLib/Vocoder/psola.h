/*
 * psola.h
 *
 * Created: 3/13/2018 3:54:09 PM
 *  Author: Daniel Gonzalez
 */ 


#ifndef PSOLA_H_
#define PSOLA_H_

#include "asf.h"
#include "DMA_Audio.h"
#include "arm_math.h"

#define PSOLA_SAMPLE_RATE 46500 //23250 

#define DEFAULT_BUFFER_SIZE 1024 
#define STEP_SIZE (WIN_SIZE/NUM_OF_OVERLAPS)
#define WIN_SIZE_D2 (WIN_SIZE>>1)


void PSOLA_init(void); 
void pitchCorrect(float* input, float *output, float inputPitch, float shift_amount); 
#endif /* PSOLA_H_ */