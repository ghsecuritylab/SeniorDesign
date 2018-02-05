/*
 * PSOLA.h
 *
 * Created: 1/29/2018 5:06:57 PM
 *  Author: Daniel Gonzalez
 */ 


#ifndef PSOLA_H_
#define PSOLA_H_

#include "asf.h"
#include "pitchyinfast.h"
#include "cvec.h" // prolly take out later

#define FFT_SAMPLE_RATE YIN_FFT_SAMPLING_RATE
#define STEP_SIZE (WIN_SIZE/NUM_OF_OVERLAPS)
#define FFT_FRAME_SIZE (WIN_SIZE)
#define FRAME_SIZE_2 (FFT_FRAME_SIZE/2)



void PSOLA_init(void);
void pitch_shift_do(float * outData, uint32_t pitch_shift, cvec_t *mags_and_phases); 

#endif /* PSOLA_H_ */