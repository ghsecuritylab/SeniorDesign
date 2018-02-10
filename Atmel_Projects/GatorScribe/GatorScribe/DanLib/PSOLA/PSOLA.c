/*
 * PSOLA.c
 *
 * Created: 1/29/2018 5:06:45 PM
 *  Author: Daniel Gonzalez
 */ 

#include "PSOLA.h"
#include "DMA_Audio.h"
#include <math.h>
#include "fastmath.h"

static const float Pi = M_PI; 
//static const float OneOverTwoPi = 0.15915494309f; 
static const float TwoPi = 2.0f * M_PI;
static const float OneOverPi = 0.318309886f; 
static const float Overlap_x_OneOverTwoPi = (float)NUM_OF_OVERLAPS * 0.15915494309f;
static const float TwoPi_d_Overlap =  2.0f * M_PI / (float)NUM_OF_OVERLAPS;
static const float ifft_scale = 2.0 / (float)NUM_OF_OVERLAPS; // 2.0 /((float)FRAME_SIZE_2*(float)NUM_OF_OVERLAPS); 

static const float freqPerBin = (float)FFT_SAMPLE_RATE/(float)FFT_FRAME_SIZE;
static const float expct = 2.0f * M_PI / (float)NUM_OF_OVERLAPS;// expected phase shift


static float prevAnaPhase[WIN_SIZE/2+1];

static float gSumPhase[WIN_SIZE/2+1];
COMPILER_ALIGNED(2*WIN_SIZE) float gFFTworksp[2*WIN_SIZE];
COMPILER_ALIGNED(2*WIN_SIZE) static float gOutputAccum[2*WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float gAnaFreq[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float gAnaMagn[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float gSynFreq[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float gSynMagn[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float scaled_hanning[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float ifft_real_values[WIN_SIZE];

static float omega[FRAME_SIZE_2 + 1]; 

void PSOLA_init(void)
{
	uint32_t i; 
	for (i = 0; i < WIN_SIZE; i++)
	{
		gAnaFreq[i] = 0.0;
		gAnaMagn[i] = 0.0;  
		gSynFreq[i] = 0.0; 
		gSynMagn[i] = 0.0; 
	}
	for (i = 0; i < WIN_SIZE/2+1; i++)
	{
		prevAnaPhase[i] = 0.0; 
		gSumPhase[i] = 0.0; 
	}
	for (i = 0; i < 2*WIN_SIZE; i++)
	{
		gFFTworksp[i] = 0.0; 
		gOutputAccum[i] = 0.0; 
	}
	arm_scale_f32((float *)hanning, ifft_scale, scaled_hanning, WIN_SIZE); 
	for(i = 0; i <= FRAME_SIZE_2; i++)
	{
		omega[i] = (float)i * expct; 
	}
}

static float princarg(float inPhase)
{
	return (inPhase - (float)(round(inPhase/TwoPi)) * TwoPi); 
}

void pitch_shift_do(float * outData, float shift_amount, cvec_t *mags_and_phases, arm_rfft_fast_instance_f32 *fftInstance)
{
	float tmp; 
	uint32_t k, index;
	int32_t qpd; 
	
	int32_t temp_a; 
	/* ***************** ANALYSIS ******************* */
	/* this is the analysis step */
	for (k = 0; k <= FRAME_SIZE_2; k++) {
		/* compute phase difference */
		tmp = mags_and_phases->phas[k] - prevAnaPhase[k]; 
		prevAnaPhase[k] = mags_and_phases->phas[k]; 

		/* subtract expected phase difference */
		tmp -= omega[k]; // can store these values in an array 
		
		// different phase unwrapping: 
		tmp = princarg(tmp); 

		/* map delta phase into +/- Pi interval */
		//qpd = (int32_t)(tmp*OneOverPi);
		
		// raz code //
		/*
		uint32_t signMap = (uint32_t)qpd >> 31; 
		signMap ^= 1; 
		qpd += (int32_t)signMap; 
		qpd >>= 1; 
		tmp -= (float)qpd * TwoPi; 
		// raz code //
		*/ 
		/*
		if (qpd >= 0) 
			qpd += qpd & 1;
		else 
			qpd -= qpd & 1;
			
		tmp -= Pi*(float)qpd;
		*/ 
		// get deviation from bin frequency from the +/- Pi interval
		tmp = Overlap_x_OneOverTwoPi*tmp;
		
		// compute the k-th partials' true frequency 
		tmp = ((float)k + tmp)*freqPerBin; 
		
		
		/* store magnitude and true frequency in analysis arrays */
		gAnaMagn[k] = mags_and_phases->norm[k];
		gAnaFreq[k] = tmp; 
	    //gAnaFreq[k] = (expct * (float)k ) + tmp; //raz code 
	}
	
	/* ***************** PROCESSING ******************* */
	
	arm_fill_f32(0.0, gSynFreq, WIN_SIZE); 
	arm_fill_f32(0.0, gSynMagn, WIN_SIZE); 
	for (k = 0; k <= FRAME_SIZE_2; k++) 
	{
		index = k*shift_amount;
		if (index <= FRAME_SIZE_2) {
			gSynMagn[index] += gAnaMagn[k]; // can just use mags_and_phases->norm[k];
			gSynFreq[index] = gAnaFreq[k] * shift_amount;
		}
	}
	
	/* ***************** SYNTHESIS ******************* */
	/* this is the synthesis step */
	for (k = 0; k <= FRAME_SIZE_2; k++) 
	{
		// get true frequency from synthesis arrays 
		tmp = gSynFreq[k];

		// subtract bin mid frequency 
		tmp -= (float)k*freqPerBin;

		// get bin deviation from freq deviation 
		tmp /= freqPerBin;

		// take number of overlaps into account 
		tmp = TwoPi_d_Overlap*tmp;

		// add the overlap phase advance back in 
		tmp += omega[k]; 

		// accumulate delta phase to get bin phase 
		gSumPhase[k] += tmp;

		// get real and imag part and re-interleave 
		gFFTworksp[k<<1] = gSynMagn[k]*arm_cos_f32(gSumPhase[k]);
		gFFTworksp[(k<<1)+1] = gSynMagn[k]*arm_sin_f32(gSumPhase[k]);
	}
	/* zero negative frequencies */
	//arm_fill_f32(0.0, &gFFTworksp[FFT_FRAME_SIZE+2], FFT_FRAME_SIZE - 2); 

	/* do inverse transform */	
	arm_rfft_fast_f32(fftInstance, gFFTworksp, ifft_real_values, 1); 

	arm_mult_f32(scaled_hanning, ifft_real_values, ifft_real_values, WIN_SIZE); 
	arm_add_f32(gOutputAccum, ifft_real_values, gOutputAccum, WIN_SIZE); 
	
	// output 
	arm_copy_f32(gOutputAccum, outData, STEP_SIZE); 
	
	/* shift accumulator */
	//arm_copy_f32(&gOutputAccum[STEP_SIZE], gOutputAccum, WIN_SIZE-STEP_SIZE);
	for(k = 0; k < WIN_SIZE; k++)
		gOutputAccum[k] = gOutputAccum[k + STEP_SIZE]; 
}