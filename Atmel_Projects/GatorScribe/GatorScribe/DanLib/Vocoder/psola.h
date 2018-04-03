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

#define PSOLA_SAMPLE_RATE 46503.0f 

#define MINIMUM_PITCH 100.0f 
#define NO_SHIFT 1.0f 
#define END_OF_SHIFTS -1.0f
#define MAX_NUM_SHIFTS 11 // always doing root (1.0 shift) 
#define NO_PITCH_BEND 64

#define WEIRD_OFFSET 600

void PSOLA_init(void); 
void create_harmonies(float* input, float *output, float inputPitch, float *pitch_shifts_in, float harm_volume, float dry_volume); 

#endif /* PSOLA_H_ */