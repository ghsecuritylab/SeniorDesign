/*
 * PSOLA.c
 *
 * Created: 1/29/2018 5:06:45 PM
 *  Author: Daniel Gonzalez
 */ 

#include "PSOLA.h"
#include <time.h>
#include <stdlib.h>

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
COMPILER_ALIGNED(WIN_SIZE) static float envelope_shift[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE_D2) static float syn_idx_array[WIN_SIZE_D2];
COMPILER_ALIGNED(WIN_SIZE_D2) static float idx_numbers[WIN_SIZE_D2];


void PSOLA_init(void)
{
	srand(4783746); // randomize seed
	rand(); 
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
		idx_numbers[i] = (float)i; 
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
	arm_scale_f32(gAnaFreq, shift_amount, gAnaFreq, WIN_SIZE_D2); 

	/* ***************** PROCESSING ******************* */
	arm_fill_f32(0.0, gSynFreq, WIN_SIZE_D2); 
	arm_scale_f32(idx_numbers, shift_amount, (float *)syn_idx_array, WIN_SIZE_D2); 
	for (k = 0; k < WIN_SIZE_D2; k++) 
	{
		index = (uint32_t)syn_idx_array[k]; 
		if (index <= WIN_SIZE_D2) {

#ifdef AUTOTUNE
			gSynMagn[index] += mags_and_phases->norm[k] ;
			gSynFreq[index] = gAnaFreq[k];
#else 
			gSynMagn[index] += mags_and_phases->norm[k] / mags_and_phases->env[k] * mags_and_phases->env[index]; 
			//gSynMagn[index] += mags_and_phases->norm[k] ;
			gSynFreq[index] = gAnaFreq[k]; // (gAnaFreq[k] * shift_amount);// 0.0; //rand() % (FFT_SAMPLE_RATE - 10) + 10.0; //0.0;
#endif 
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
	uint32_t k, idx; 
	
	/*
	arm_max_f32(gSynMagn, WIN_SIZE_D2, &max_fft_norm2, &tmp);
	arm_conv_f32(gSynMagn, WIN_SIZE_D2, (float *)envelope_filter, envelope_filter_length, temp_envelope);
	arm_max_f32(&temp_envelope[envelope_filter_length/2], mags_and_phases->length, &max_fft_filt_norm2, &tmp);
	envelop_scale2 = max_fft_norm2 / max_fft_filt_norm2;
	for (i = 0; i < mags_and_phases->length; i++)
		envelope_shift[i] = temp_envelope[i + envelope_filter_length/2] * envelop_scale2;
		*/ 	
	float sin_value, cos_value; 
	arm_copy_f32(mags_and_phases->phas, prevAnaPhase, WIN_SIZE_D2); 
	for (k = 0; k < WIN_SIZE_D2; k++)
	{		
		//if (gSynFreq[k] > 0)
	//	gSynMagn[k] = gSynMagn[k] * (mags_and_phases->env[k] / envelope_shift[k]);// + Abs(mags_and_phases->env[k] - envelope_shift[k])/Max(mags_and_phases->env[k],envelope_shift[k]) ); 
		 
		// get real and imag part and re-interleave
		idx = k << 1; 
		arm_sin_cos_f32(gSumPhase[k], &sin_value, &cos_value);
		gFFTworksp[idx] = gSynMagn[k]*cos_value;
		gFFTworksp[idx+1] = gSynMagn[k]*sin_value; 
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