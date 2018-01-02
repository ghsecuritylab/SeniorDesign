#include <stdint.h> /* For standard interger types (int16_t) */
#include <stdlib.h> /* For call to malloc */
#include "Yin.h"

/***************************** Static Variables Start *****************************/
static Yin yin; 
/***************************** Static Variables End *****************************/

/**
 * Step 1: Calculates the squared difference of the signal with a shifted version of itself.
 * @param buffer Buffer of samples to process. 
 *
 * This is the Yin algorithms tweak on autocorellation. Read http://audition.ens.fr/adc/pdf/2002_JASA_YIN.pdf
 * for more details on what is in here and why it's done this way.
 */
static void Yin_difference(int16_t* buffer)
{
	int16_t i;
	int16_t tau;
	float32_t delta;
	// float32_t multOutput; 

	/* Calculate the difference for difference shift values (tau) for the half of the samples */
	for(tau = 0 ; tau < yin.halfBufferSize; tau++)
	{
		/* Take the difference of the signal with a shifted version of itself, then square it.
		 * (This is the Yin algorithm's tweak on autocorrelation) */ 
		
		/* compute first value to avoid having to zero out buffer */ 
		delta = buffer[0] - buffer[tau];
		yin.yinBuffer[tau] = delta*delta; 
		for(i = 1; i < yin.halfBufferSize; i++)
		{
			//__SSUB16 here maybe ... look at example. need to be in int32 first 
			delta = buffer[i] - buffer[i + tau];
			yin.yinBuffer[tau] += delta * delta;
			
			// Why is this slower??? 
			//arm_mult_f32(&delta, &delta, &multOutput, 1);
			//arm_add_f32(&yin.yinBuffer[tau], &multOutput, &yin.yinBuffer[tau], 1);
		}
	}
}


/**
 * Step 2: Calculate the cumulative mean on the normalised difference calculated in step 1
 * This goes through the Yin autocorellation values and finds out roughly where shift is which 
 * produced the smallest difference
 */
static void Yin_cumulativeMeanNormalizedDifference(void)
{
	int16_t tau;
	float32_t runningSum = 0;
	yin.yinBuffer[0] = 1;

	/* Sum all the values in the autocorellation buffer and nomalise the result, replacing
	 * the value in the autocorellation buffer with a cumulative mean of the normalised difference */
	for (tau = 1; tau < yin.halfBufferSize; tau++) {
		runningSum += yin.yinBuffer[tau];
		
		// float32_ting point divide here 
		yin.yinBuffer[tau] *= tau / runningSum;
	}
}

/**
 * Step 3: Search through the normalised cumulative mean array and find values that are over the threshold
 * @return Shift (tau) which caused the best approximate autocorellation. -1 if no suitable value is found over the threshold.
 */
static int16_t Yin_absoluteThreshold(void){
	int16_t tau;

	/* Search through the array of cumulative mean values, and look for ones that are over the threshold 
	 * The first two positions in yinBuffer are always so start at the third (index 2) */
	for (tau = 2; tau < yin.halfBufferSize ; tau++) {
		if (yin.yinBuffer[tau] < yin.threshold) {
			while (tau + 1 < yin.halfBufferSize && yin.yinBuffer[tau + 1] < yin.yinBuffer[tau]) {
				tau++;
			}
			/* found tau, exit loop and return
			 * store the probability
			 * From the YIN paper: The yin.threshold determines the list of
			 * candidates admitted to the set, and can be interpreted as the
			 * proportion of aperiodic power tolerated
			 * within a periodic signal.
			 *
			 * Since we want the periodicity and not aperiodicity:
			 * periodicity = 1 - aperiodicity */
			yin.probability = 1 - yin.yinBuffer[tau];
			break;
		}
	}

	/* if no pitch found, tau => -1 */
	if (tau == yin.halfBufferSize || yin.yinBuffer[tau] >= yin.threshold) {
		tau = -1;
		yin.probability = 0;
	}

	return tau;
}

/**
 * Step 5: Interpolate the shift value (tau) to improve the pitch estimate.
 * @param  tauEstimate [description]
 * @return             [description]
 *
 * The 'best' shift value for autocorellation is most likely not an interger shift of the signal.
 * As we only autocorellated using integer shifts we should check that there isn't a better fractional 
 * shift value.
 */
static float32_t Yin_parabolicInterpolation(int16_t tauEstimate) {
	float32_t betterTau;
	int16_t x0;
	int16_t x2;
	
	/* Calculate the first polynomial coeffcient based on the current estimate of tau */
	if (tauEstimate < 1) {
		x0 = tauEstimate;
	} 
	else {
		x0 = tauEstimate - 1;
	}

	/* Calculate the second polynomial coeffcient based on the current estimate of tau */
	if (tauEstimate + 1 < yin.halfBufferSize) {
		x2 = tauEstimate + 1;
	} 
	else {
		x2 = tauEstimate;
	}

	/* Algorithm to parabolically interpolate the shift value tau to find a better estimate */
	if (x0 == tauEstimate) {
		if (yin.yinBuffer[tauEstimate] <= yin.yinBuffer[x2]) {
			betterTau = (float32_t)tauEstimate;
		} 
		else {
			betterTau = (float32_t)x2;
		}
	} 
	else if (x2 == tauEstimate) {
		if (yin.yinBuffer[tauEstimate] <= yin.yinBuffer[x0]) {
			betterTau = (float32_t)tauEstimate;
		} 
		else {
			betterTau = (float32_t)x0;
		}
	} 
	else {
		float32_t s0, s1, s2;
		s0 = yin.yinBuffer[x0];
		s1 = yin.yinBuffer[tauEstimate];
		s2 = yin.yinBuffer[x2];
		// fixed AUBIO implementation, thanks to Karl Helgason:
		// (2.0f * s1 - s2 - s0) was incorrectly multiplied with -1
		betterTau = (float32_t)(tauEstimate) + (s2 - s0) / (2.0 * (2.0 * s1 - s2 - s0));
	}

	return betterTau;
}
/***************************** Static Functions End *****************************/

/***************************** Public Functions Start *****************************/

/**
 * Initialise the Yin pitch detection object
 * @param bufferSize Length of the audio buffer to analyse
 * @param threshold  Allowed uncertainty (e.g 0.05 will return a pitch with ~95% probability)
 */
void Yin_init(int16_t bufferSize, float32_t threshold){
	/* Initialise the fields of the Yin structure passed in */
	yin.bufferSize = bufferSize;
	yin.halfBufferSize = bufferSize / 2;
	yin.probability = 0.0;
	yin.threshold = threshold;
	
	/* Allocate the autocorellation buffer */
	yin.yinBuffer = (float32_t *) malloc(sizeof(float32_t)* yin.halfBufferSize);
}

/**
 * Runs the Yin pitch detection algortihm
 * @param  buffer Buffer of samples to analyse
 * @return        Fundamental frequency of the signal in Hz. Returns -1 if pitch can't be found
 */
float32_t Yin_getPitch(int16_t* buffer){
	int16_t tauEstimate = -1;
	float32_t pitchInHertz = -1;

	/* Step 1: Calculates the squared difference of the signal with a shifted version of itself. */
	Yin_difference(buffer);
	
	/* Step 2: Calculate the cumulative mean on the normalised difference calculated in step 1 */
	Yin_cumulativeMeanNormalizedDifference();
	
	/* Step 3: Search through the normalised cumulative mean array and find values that are over the threshold */
	tauEstimate = Yin_absoluteThreshold();
	
	/* Step 5: Interpolate the shift value (tau) to improve the pitch estimate. */
	if(tauEstimate != -1){
		pitchInHertz = YIN_SAMPLING_RATE / Yin_parabolicInterpolation(tauEstimate);
	}
	return pitchInHertz;
}

/**
 * Certainty of the pitch found 
 * @return     Returns the certainty of the note found as a decimal (i.e 0.3 is 30%)
 */
float32_t Yin_getProbability(void){
	return yin.probability;
}

/***************************** Public Functions End *****************************/