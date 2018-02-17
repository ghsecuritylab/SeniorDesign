/*
 * Vocoder.h
 *
 * Created: 1/29/2018 5:06:57 PM
 *  Author: Daniel Gonzalez
 */ 


#ifndef VOCODER_H_
#define VOCODER_H_

#include "asf.h"
#include "DMA_Audio.h"
#include "arm_math.h"

#define FFT_SAMPLE_RATE 24000
#define STEP_SIZE (WIN_SIZE/NUM_OF_OVERLAPS)
#define WIN_SIZE_D2 (WIN_SIZE>>1)

typedef struct {
	uint32_t length;  /**< length of buffer = (requested length)/2 + 1 */
	float *norm;   /**< norm array of size ::cvec_t.length */
	float *phas;   /**< phase array of size ::cvec_t.length */
	float *env;		// pointer to envelope starting at filterlength>>1 to correct for FIR phase change 
	float *unshiftedEnv; // pointer to beginning of envelope buffer 
} cvec_t;

typedef struct {
	uint32_t length;  /**< length of buffer */
	float *data;   /**< data vector of length ::fvec_t.length */
} fvec_t;


void Vocoder_init(void);
void pitch_shift_do(float shift_amount, cvec_t *mags_and_phases);
void get_harmonized_output(float * outData, cvec_t *mags_and_phases, arm_rfft_fast_instance_f32 *fftInstance); 

#endif /* VOCODER_H_ */