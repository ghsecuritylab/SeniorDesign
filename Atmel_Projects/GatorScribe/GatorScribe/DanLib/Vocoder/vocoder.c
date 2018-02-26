/*
 * Vocoder.c
 *
 * Created: 1/29/2018 5:06:45 PM
 *  Author: Daniel Gonzalez
 */ 

#include "vocoder.h"
#include <stdlib.h>

extern const float hanning[1024];

static const float OneOverTwoPi = 0.15915494309f; 
static const float TwoPi = 2.0f * M_PI;
static const float Overlap_x_OneOverTwoPi = (float)NUM_OF_OVERLAPS * 0.15915494309f;
static const float TwoPi_d_Overlap =  2.0f * M_PI / (float)NUM_OF_OVERLAPS;

static const float ifft_scale = 4.0f / (float)NUM_OF_OVERLAPS; // 4.0 = 2 (only using half the spectrum) * 2 (removed scaling from original fft) 

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

/* Spectral envelope */ 
COMPILER_ALIGNED(WIN_SIZE) static float shift_envelope[WIN_SIZE];
extern float envelope_filter[];
extern uint32_t envelope_filter_length;

void Vocoder_init(void)
{
	uint32_t i; 
	for (i = 0; i < WIN_SIZE; i++)
	{
		gAnaFreq[i] = 0.0f;
		gSynFreq[i] = 0.0f; 
		gSynMagn[i] = 0.0f; 
	}
	for (i = 0; i < 2*WIN_SIZE; i++)
	{
		gFFTworksp[i] = 0.0f; 
		gOutputAccum[i] = 0.0f; 
	}
	for(i = 0; i < WIN_SIZE_D2; i++)
	{
		prevAnaPhase[i] = 0.0f;
		gSumPhase[i] = 0.0f;
	}
	
	/* Create ifft hanning window with appropriate scaling factor */ 
	arm_scale_f32((float *)hanning, ifft_scale, scaled_hanning, WIN_SIZE);
}

static inline float princarg(float inPhase)
{
	return (inPhase - (float)(round(inPhase*OneOverTwoPi)) * TwoPi); 
}

void pitch_shift_do(float shift_amount, cvec_t *mags_and_phases)
{
	float tmp; 
	uint32_t k, target;
	/* ***************** ANALYSIS ******************* */
	/* this is the analysis step */
	for (k = 0; k < WIN_SIZE_D2; k++) {
		/* compute phase difference */
		tmp = mags_and_phases->phas[k] - prevAnaPhase[k]; 
			
		/* subtract expected phase difference */
		tmp -= (float)k * expct; 
		
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
	arm_fill_f32(0.0f, gSynFreq, WIN_SIZE_D2); 
	arm_scale_f32(gAnaFreq, shift_amount, gAnaFreq, WIN_SIZE_D2);
	for (k = 0; k < WIN_SIZE_D2; k++) 
	{
		target = (float)k * shift_amount; 
		if (target <= WIN_SIZE_D2) 
		//if (target <= 200 && target > 2) 
		{
#ifdef AUTOTUNE
			gSynMagn[target] += mags_and_phases->norm[k] ;
			gSynFreq[target] = gAnaFreq[k];
#else 
			gSynMagn[target] += mags_and_phases->norm[k] * mags_and_phases->env[target] / mags_and_phases->env[k];
			gSynFreq[target] = gAnaFreq[k];
#endif 
		}
	}
	
	/* ***************** SYNTHESIS ******************* */
	/* this is the synthesis step */
	for (k = 0; k < WIN_SIZE_D2; k++) 
	{
		// subtract bin mid frequency from true frequency from synthesis arrays 
		tmp = gSynFreq[k] - (float)k * freqPerBin;

		// get bin deviation from freq deviation 
		tmp *= oneOverFreqPerBin;

		// take number of overlaps into account 
		tmp = TwoPi_d_Overlap*tmp;

		// add the overlap phase advance back in 
		tmp += (float)k * expct; 

		// accumulate delta phase to get bin phase 
		gSumPhase[k] += tmp;
	}
}

void get_harmonized_output(float * outData, cvec_t *mags_and_phases, arm_rfft_fast_instance_f32 *fftInstance, bool harmonize_flag )
{
	uint32_t k; 
	float sin_value, cos_value;
	
	if (harmonize_flag)
	{
		/* calculate shift envelope -- todo: try different filter cutoffs */ 
		//arm_conv_f32(gSynMagn, WIN_SIZE_D2, (float *)envelope_filter, envelope_filter_length, shift_envelope);
		//float *shift_env = &shift_envelope[envelope_filter_length>>1];
			
		//arm_scale_f32(gSynMagn, 4.0f, gSynMagn, WIN_SIZE_D2); // scaling... basically volume of harmonizer... can control this with a knob!!!
		//arm_mult_f32(gSynMagn, mags_and_phases->env, gSynMagn, WIN_SIZE_D2); // scaling from original envelope
		for (k = 0; k < WIN_SIZE_D2; k++)
		{
			/* scale by synth envelope - adding small term to avoid dividing by zero */ 
			//gSynMagn[k] /= (shift_env[k] + 0.000001f); //Abs(mags_and_phases->env[k] - shift_env[k]) / shift_env[k];  //Abs(2.0f*mags_and_phases->env[k] - shift_env[k]) / shift_env[k];
				
			/* Get real and imag part and interleave */ 
			gSumPhase[k] = princarg(gSumPhase[k]); 
			arm_sin_cos_f32(gSumPhase[k], &sin_value, &cos_value);
			gFFTworksp[2*k] = gSynMagn[k]*cos_value;
			gFFTworksp[2*k+1] = gSynMagn[k]*sin_value;
		}
	}
	else 
	{
		//arm_scale_f32(gSynMagn, 4.0f, gSynMagn, WIN_SIZE_D2); // scaling... basically volume of harmonizer... can control this with a knob!!!
		for (k = 0; k < WIN_SIZE_D2; k++)
		{	
			/* Get real and imag part and interleave */ 
			gSumPhase[k] = princarg(gSumPhase[k]); 
			arm_sin_cos_f32(gSumPhase[k], &sin_value, &cos_value);
			gFFTworksp[2*k] = gSynMagn[k]*cos_value;
			gFFTworksp[2*k+1] = gSynMagn[k]*sin_value;
		}
	}

	/* do inverse transform */
	arm_rfft_fast_f32(fftInstance, gFFTworksp, ifft_real_values, 1);
	
	/* Window and overlap & add */ 
	arm_mult_f32(scaled_hanning, ifft_real_values, ifft_real_values, WIN_SIZE);
	arm_add_f32(gOutputAccum, ifft_real_values, gOutputAccum, WIN_SIZE);
		
	/* Copy data to output buffer */ 
	if(harmonize_flag)
		arm_copy_f32(gOutputAccum, outData, STEP_SIZE);
	//else 
		//arm_fill_f32(0.0f, outData, STEP_SIZE); 
		
	/* shift accumulator */
	arm_copy_f32(&gOutputAccum[STEP_SIZE], gOutputAccum, WIN_SIZE);
	
	/* zero out synthesis mags */ 
	arm_fill_f32(0.0f, gSynMagn, WIN_SIZE_D2); 
	
	/* Save previous phases */ 
	arm_copy_f32(mags_and_phases->phas, prevAnaPhase, WIN_SIZE_D2);
}