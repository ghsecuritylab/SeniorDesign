/*
 * PSOLA.c
 *
 * Created: 1/29/2018 5:06:45 PM
 *  Author: Daniel Gonzalez
 */ 

#include "PSOLA.h"

extern const float hanning[1024];

static const float Pi = M_PI; 
static const float OneOverTwoPi = 0.15915494309f; 
static const float TwoPi = 2.0f * M_PI;
static const float OneOverPi = 0.318309886f; 
static const float Overlap_x_OneOverTwoPi = (float)NUM_OF_OVERLAPS * 0.15915494309f;
static const float TwoPi_d_Overlap =  2.0f * M_PI / (float)NUM_OF_OVERLAPS;

static const float ifft_scale = 4.0 / (float)NUM_OF_OVERLAPS; // 4.0 = 2 (only using half the spectrum) * 2 (removed scaling from original fft) 

static const float freqPerBin = (float)FFT_SAMPLE_RATE/(float)WIN_SIZE;
static const float oneOverFreqPerBin = (float)WIN_SIZE / (float)FFT_SAMPLE_RATE; 
static const float expct = 2.0f * M_PI / (float)NUM_OF_OVERLAPS;


COMPILER_ALIGNED(WIN_SIZE_D2) static float prevAnaPhase[WIN_SIZE_D2];
COMPILER_ALIGNED(WIN_SIZE_D2) static float gSumPhase[WIN_SIZE_D2];
COMPILER_ALIGNED(2*WIN_SIZE) float gFFTworksp[2*WIN_SIZE];
COMPILER_ALIGNED(2*WIN_SIZE) static float gOutputAccum[2*WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float gAnaFreq[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float gSynFreq[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float gSynMagn[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float scaled_hanning[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float ifft_real_values[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE_D2) static float omega[WIN_SIZE_D2]; 

void PSOLA_init(void)
{
	uint32_t i; 
	for (i = 0; i < WIN_SIZE; i++)
	{
		gAnaFreq[i] = 0.0;
		gSynFreq[i] = 0.0; 
		gSynMagn[i] = 0.0; 
	}
	for (i = 0; i < 2*WIN_SIZE; i++)
	{
		gFFTworksp[i] = 0.0; 
		gOutputAccum[i] = 0.0; 
	}
	arm_scale_f32((float *)hanning, ifft_scale, scaled_hanning, WIN_SIZE); 
	for(i = 0; i < WIN_SIZE_D2; i++)
	{
		omega[i] = (float)i * expct; 
		prevAnaPhase[i] = 0.0;
		gSumPhase[i] = 0.0;
	}
}

static inline float princarg(float inPhase)
{
	return (inPhase - (float)(round(inPhase*OneOverTwoPi)) * TwoPi); 
}

void pitch_shift_do(float shift_amount, cvec_t *mags_and_phases)
{
	float tmp; 
	uint32_t k, index;
	
	/* ***************** ANALYSIS ******************* */
	/* this is the analysis step */
	for (k = 0; k < WIN_SIZE_D2; k++) {
		/* compute phase difference */
		tmp = mags_and_phases->phas[k] - prevAnaPhase[k]; 
			
		/* subtract expected phase difference */
		tmp -= omega[k]; 
		
		/* map delta phase into +/- Pi interval */
		tmp = princarg(tmp); 

		// get deviation from bin frequency from the +/- Pi interval
		tmp = Overlap_x_OneOverTwoPi*tmp;
		
		// compute the k-th partials' true frequency 
		tmp = ((float)k + tmp)*freqPerBin; 
		
		/* store true frequency in analysis arrays */
		gAnaFreq[k] = tmp; 
	}
	
	/* ***************** PROCESSING ******************* */
	arm_fill_f32(0.0, gSynFreq, WIN_SIZE_D2); 
	//arm_fill_f32(0.0, gSynMagn, WIN_SIZE_D2); 
	for (k = 0; k < WIN_SIZE_D2; k++) 
	{
		index = k*shift_amount;
		if (index <= WIN_SIZE_D2) {
			gSynMagn[index] += mags_and_phases->norm[k];
			gSynFreq[index] = 0.0; //(gAnaFreq[k] * shift_amount);
		}
	}
	
	/* ***************** SYNTHESIS ******************* */
	/* this is the synthesis step */
	for (k = 0; k < WIN_SIZE_D2; k++) 
	{
		// get true frequency from synthesis arrays 
		tmp = gSynFreq[k];

		// subtract bin mid frequency 
		tmp -= (float)k*freqPerBin;

		// get bin deviation from freq deviation 
		tmp *= oneOverFreqPerBin;

		// take number of overlaps into account 
		tmp = TwoPi_d_Overlap*tmp;

		// add the overlap phase advance back in 
		tmp += omega[k]; 

		// accumulate delta phase to get bin phase 
		gSumPhase[k] += tmp;
	}
}


void get_harmonized_output(float * outData, cvec_t *mags_and_phases, arm_rfft_fast_instance_f32 *fftInstance)
{
	uint32_t k; 
	for (k = 0; k < WIN_SIZE_D2; k++)
	{
		prevAnaPhase[k] = mags_and_phases->phas[k]; 
		// get real and imag part and re-interleave
		gFFTworksp[k<<1] = gSynMagn[k]*arm_cos_f32(gSumPhase[k]);
		gFFTworksp[(k<<1)+1] = gSynMagn[k]*arm_sin_f32(gSumPhase[k]);
	}
		
	/* do inverse transform */
	arm_rfft_fast_f32(fftInstance, gFFTworksp, ifft_real_values, 1);

	arm_mult_f32(scaled_hanning, ifft_real_values, ifft_real_values, WIN_SIZE);
	arm_add_f32(gOutputAccum, ifft_real_values, gOutputAccum, WIN_SIZE);
		
	// output
	arm_copy_f32(gOutputAccum, outData, STEP_SIZE);
		
	/* shift accumulator */
	arm_copy_f32(&gOutputAccum[STEP_SIZE], gOutputAccum, WIN_SIZE);
	
	/* zero out synthesis mags */ 
	arm_fill_f32(0.0, gSynMagn, WIN_SIZE_D2); 
}