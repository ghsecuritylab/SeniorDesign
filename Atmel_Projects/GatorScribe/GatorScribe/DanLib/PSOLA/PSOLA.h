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

#define FFT_SAMPLE_RATE YIN_FFT_SAMPLING_RATE
#define STEP_SIZE (WIN_SIZE/NUM_OF_OVERLAPS)
#define FFT_FRAME_SIZE (WIN_SIZE)
#define FRAME_SIZE_2 (FFT_FRAME_SIZE>>1)

typedef struct {
	uint32_t length;  /**< length of buffer = (requested length)/2 + 1 */
	float *norm;   /**< norm array of size ::cvec_t.length */
	float *phas;   /**< phase array of size ::cvec_t.length */
} cvec_t;



void PSOLA_init(void);
void pitch_shift_do(float * outData, float shift_amount, cvec_t *mags_and_phases, arm_rfft_fast_instance_f32 *fftInstance); 

#endif /* PSOLA_H_ */