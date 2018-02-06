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
//static const float TwoPi = 2.0f * M_PI;
static const float OneOverPi = 0.318309886f; 
static const float Overlap_x_OneOverTwoPi = (float)NUM_OF_OVERLAPS * 0.15915494309f;
static const float TwoPi_d_Overlap =  2.0f * M_PI / (float)NUM_OF_OVERLAPS;
static const float ifft_scale = 2.0/((float)FFT_FRAME_SIZE*(float)NUM_OF_OVERLAPS); 

static const float freqPerBin = (float)FFT_SAMPLE_RATE/(float)FFT_FRAME_SIZE;
static const float expct = 2.0f * M_PI / (float)NUM_OF_OVERLAPS;// expected phase shift


static float gLastPhase[WIN_SIZE/2+1];
static float gSumPhase[WIN_SIZE/2+1];
COMPILER_ALIGNED(2*WIN_SIZE) float gFFTworksp[2*WIN_SIZE];
COMPILER_ALIGNED(2*WIN_SIZE) static float gOutputAccum[2*WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float gAnaFreq[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float gAnaMagn[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float gSynFreq[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float gSynMagn[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float scaled_hanning[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float ifft_real_values[WIN_SIZE];

static void smbFft(float *fftBuffer, long fftFrameSize, long sign)
/* 
	FFT routine, (C)1996 S.M.Bernsee. Sign = -1 is FFT, 1 is iFFT (inverse)
	Fills fftBuffer[0...2*fftFrameSize-1] with the Fourier transform of the
	time domain data in fftBuffer[0...2*fftFrameSize-1]. The FFT array takes
	and returns the cosine and sine parts in an interleaved manner, ie.
	fftBuffer[0] = cosPart[0], fftBuffer[1] = sinPart[0], asf. fftFrameSize
	must be a power of 2. It expects a complex input signal (see footnote 2),
	ie. when working with 'common' audio signals our input signal has to be
	passed as {in[0],0.,in[1],0.,in[2],0.,...} asf. In that case, the transform
	of the frequencies of interest is in fftBuffer[0...fftFrameSize].
*/
{
	float wr, wi, arg, *p1, *p2, temp;
	float tr, ti, ur, ui, *p1r, *p1i, *p2r, *p2i;
	long i, bitm, j, le, le2, k;

	for (i = 2; i < 2*fftFrameSize-2; i += 2) {
		for (bitm = 2, j = 0; bitm < 2*fftFrameSize; bitm <<= 1) {
			if (i & bitm) j++;
			j <<= 1;
		}
		if (i < j) {
			p1 = fftBuffer+i; p2 = fftBuffer+j;
			temp = *p1; *(p1++) = *p2;
			*(p2++) = temp; temp = *p1;
			*p1 = *p2; *p2 = temp;
		}
	}
	for (k = 0, le = 2; k < (long)(log10f(fftFrameSize)/log10f(2.)+.5); k++) {
		le <<= 1;
		le2 = le>>1;
		ur = 1.0;
		ui = 0.0;
		arg = Pi / (le2>>1);
		wr = arm_cos_f32(arg);
		wi = sign*arm_sin_f32(arg);
		for (j = 0; j < le2; j += 2) {
			p1r = fftBuffer+j; p1i = p1r+1;
			p2r = p1r+le2; p2i = p2r+1;
			for (i = j; i < 2*fftFrameSize; i += le) {
				tr = *p2r * ur - *p2i * ui;
				ti = *p2r * ui + *p2i * ur;
				*p2r = *p1r - tr; *p2i = *p1i - ti;
				*p1r += tr; *p1i += ti;
				p1r += le; p1i += le;
				p2r += le; p2i += le;
			}
			tr = ur*wr - ui*wi;
			ui = ur*wi + ui*wr;
			ur = tr;
		}
	}
}


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
		gLastPhase[i] = 0.0; 
		gSumPhase[i] = 0.0; 
	}
	for (i = 0; i < 2*WIN_SIZE; i++)
	{
		gFFTworksp[i] = 0.0; 
		gOutputAccum[i] = 0.0; 
	}
	arm_scale_f32((float *)hanning, ifft_scale, scaled_hanning, WIN_SIZE); 
}

void pitch_shift_do(float * outData, uint32_t pitch_shift, cvec_t *mags_and_phases)
{
	float tmp;
	uint32_t k, qpd, index;

	/* ***************** ANALYSIS ******************* */
	/* this is the analysis step */
	for (k = 0; k <= FRAME_SIZE_2; k++) {
		/* compute phase difference */
		tmp = mags_and_phases->phas[k] - gLastPhase[k];
		gLastPhase[k] = mags_and_phases->phas[k]; 

		/* subtract expected phase difference */
		tmp -= (float)k*expct;

		/* map delta phase into +/- Pi interval */
		qpd = tmp*OneOverPi;
		if (qpd >= 0) 
			qpd += qpd & 1;
		else 
			qpd -= qpd & 1;
		tmp -= Pi*(float)qpd;

		/* get deviation from bin frequency from the +/- Pi interval */
		tmp = Overlap_x_OneOverTwoPi*tmp;

		/* compute the k-th partials' true frequency */
		//tmp = (float)k*freqPerBin + tmp*freqPerBin;
		//tmp = freqPerBin * ((float)k + tmp); 

		/* store magnitude and true frequency in analysis arrays */
		gAnaMagn[k] = mags_and_phases->norm[k];
		gAnaFreq[k] = freqPerBin * ((float)k + tmp); 
	}
	
	/* ***************** PROCESSING ******************* */
	
	arm_fill_f32(0.0, gSynFreq, WIN_SIZE); 
	arm_fill_f32(0.0, gSynMagn, WIN_SIZE); 
	for (k = 0; k <= FRAME_SIZE_2; k++) {
		index = k*pitch_shift;
		if (index <= FRAME_SIZE_2) {
			gSynMagn[index] += gAnaMagn[k];
			gSynFreq[index] = gAnaFreq[k] * pitch_shift;
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
		tmp += (float)k*expct;

		// accumulate delta phase to get bin phase 
		gSumPhase[k] += tmp;

		// get real and imag part and re-interleave 
		gFFTworksp[k<<1] = gSynMagn[k]*arm_cos_f32(gSumPhase[k]);
		gFFTworksp[(k<<1)+1] = gSynMagn[k]*arm_sin_f32(gSumPhase[k]);
	}
	/* zero negative frequencies */
	arm_fill_f32(0.0, &gFFTworksp[FFT_FRAME_SIZE+2], FFT_FRAME_SIZE - 2); 

	/* do inverse transform */
	//smbFft(gFFTworksp, FFT_FRAME_SIZE, 1);

	/* do windowing and add to output accumulator */
	for(k=0; k < WIN_SIZE; k++) 
		ifft_real_values[k] = gFFTworksp[k<<1]; 

	arm_mult_f32(scaled_hanning, ifft_real_values, ifft_real_values, WIN_SIZE); 
	arm_add_f32(gOutputAccum, ifft_real_values, gOutputAccum, WIN_SIZE); 
	
	// output 
	arm_copy_f32(gOutputAccum, outData, STEP_SIZE); 

	/* shift accumulator */
	arm_copy_f32(&gOutputAccum[WIN_SIZE], gOutputAccum, WIN_SIZE); 
}