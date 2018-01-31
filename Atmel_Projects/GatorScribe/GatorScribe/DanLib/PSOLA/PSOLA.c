/*
 * PSOLA.c
 *
 * Created: 1/29/2018 5:06:45 PM
 *  Author: Daniel Gonzalez
 */ 

#include "PSOLA.h"
#include "DMA_Audio.h"

static float32_t gInFIFO[PROCESS_BUF_SIZE];
static float32_t gOutFIFO[PROCESS_BUF_SIZE];
static float32_t gFFTworksp[2*PROCESS_BUF_SIZE];
static float32_t gLastPhase[PROCESS_BUF_SIZE/2+1];
static float32_t gSumPhase[PROCESS_BUF_SIZE/2+1];
static float32_t gOutputAccum[2*PROCESS_BUF_SIZE];
static float32_t gAnaFreq[PROCESS_BUF_SIZE];
static float32_t gAnaMagn[PROCESS_BUF_SIZE];
static float32_t gSynFreq[PROCESS_BUF_SIZE];
static float32_t gSynMagn[PROCESS_BUF_SIZE];

void PSOLA_init(uint32_t bufferSize)
{
	memset(gInFIFO, 0, bufferSize*sizeof(float32_t));
	memset(gOutFIFO, 0, bufferSize*sizeof(float32_t));
	memset(gFFTworksp, 0, 2*bufferSize*sizeof(float32_t));
	memset(gLastPhase, 0, (bufferSize/2+1)*sizeof(float32_t));
	memset(gSumPhase, 0, (bufferSize/2+1)*sizeof(float32_t));
	memset(gOutputAccum, 0, 2*bufferSize*sizeof(float32_t));
	memset(gAnaFreq, 0, bufferSize*sizeof(float32_t));
	memset(gAnaMagn, 0, bufferSize*sizeof(float32_t));
}

void pitch_shift_do(float32_t * outData, uint32_t pitch_shift, cvec_t *mags_and_phases)
{
	double magn, phase, tmp, window, real, imag;
	double freqPerBin, expct;
	long i,k, qpd, index;

	/* set up some handy variables */
	freqPerBin = FFT_SAMPLE_RATE/(double)FFT_FRAME_SIZE;
	expct = 2.*M_PI*(double)STEP_SIZE/(double)FFT_FRAME_SIZE;
	
	/* ***************** ANALYSIS ******************* */
	/* this is the analysis step */
	for (k = 0; k <= FRAME_SIZE_2; k++) {
		
		magn = mags_and_phases->norm[k]; 
		phase = mags_and_phases->phas[k]; 

		/* compute phase difference */
		tmp = phase - gLastPhase[k];
		gLastPhase[k] = phase;

		/* subtract expected phase difference */
		tmp -= (double)k*expct;

		/* map delta phase into +/- Pi interval */
		qpd = tmp/M_PI;
		if (qpd >= 0) qpd += qpd&1;
		else qpd -= qpd&1;
		tmp -= M_PI*(double)qpd;

		/* get deviation from bin frequency from the +/- Pi interval */
		tmp = NUM_OF_OVERLAPS*tmp/(2.*M_PI);

		/* compute the k-th partials' true frequency */
		tmp = (double)k*freqPerBin + tmp*freqPerBin;

		/* store magnitude and true frequency in analysis arrays */
		gAnaMagn[k] = magn;
		gAnaFreq[k] = tmp;
	}
	
	memset(gSynMagn, 0, FFT_FRAME_SIZE*sizeof(float32_t));
	memset(gSynFreq, 0, FFT_FRAME_SIZE*sizeof(float32_t));
	for (k = 0; k <= FRAME_SIZE_2; k++) {
		index = k*pitch_shift;
		if (index <= FRAME_SIZE_2) {
			gSynMagn[index] += gAnaMagn[k];
			gSynFreq[index] = gAnaFreq[k] * pitch_shift;
		}
	}
	
	/* ***************** SYNTHESIS ******************* */
	/* this is the synthesis step */
	for (k = 0; k <= FRAME_SIZE_2; k++) {

		/* get magnitude and true frequency from synthesis arrays */
		magn = gSynMagn[k];
		tmp = gSynFreq[k];

		/* subtract bin mid frequency */
		tmp -= (double)k*freqPerBin;

		/* get bin deviation from freq deviation */
		tmp /= freqPerBin;

		/* take number of overlaps into account */
		tmp = 2.*M_PI*tmp/NUM_OF_OVERLAPS;

		/* add the overlap phase advance back in */
		tmp += (double)k*expct;

		/* accumulate delta phase to get bin phase */
		gSumPhase[k] += tmp;
		phase = gSumPhase[k];

		/* get real and imag part and re-interleave */
		gFFTworksp[2*k] = magn*cos(phase);
		gFFTworksp[2*k+1] = magn*sin(phase);
	}

	/* zero negative frequencies */
	for (k = FFT_FRAME_SIZE+2; k < 2*FFT_FRAME_SIZE; k++) gFFTworksp[k] = 0.;

	/* do inverse transform */
	//smbFft(gFFTworksp, FFT_FRAME_SIZE, 1);

	/* do windowing and add to output accumulator */
	/*
	for(k=0; k < FFT_FRAME_SIZE; k++) {
		gOutputAccum[k] += hanning[k]*gFFTworksp[2*k]; 
	}
	*/
	// problem for output 
	for (k = 0; k < STEP_SIZE; k++) gOutFIFO[k] = gOutputAccum[k];

	/* shift accumulator */
	memmove(gOutputAccum, gOutputAccum+STEP_SIZE, FFT_FRAME_SIZE*sizeof(float));

}