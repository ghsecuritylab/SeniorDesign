/*
 * PSOLA.h
 *
 * Created: 1/29/2018 5:06:57 PM
 *  Author: Daniel Gonzalez
 */ 


#ifndef PSOLA_H_
#define PSOLA_H_

#include "asf.h"
#include "arm_math.h"
#include "pitchyinfast.h"

#define NUM_OF_OVERLAPS 4
#define FFT_SAMPLE_RATE YIN_FFT_SAMPLING_RATE
#define STEP_SIZE (PROCESS_BUF_SIZE/NUM_OF_OVERLAPS)
#define FFT_FRAME_SIZE (PROCESS_BUF_SIZE)
#define FRAME_SIZE_2 (FFT_FRAME_SIZE/2)

void PSOLA_init(uint32_t bufferSize);
void pitch_shift_do(float32_t * outData, uint32_t pitch_shift, cvec_t *mags_and_phases); 

#endif /* PSOLA_H_ */