#include "dywapitchtrack.h"
#include <math.h>
#include "arm_math.h"
#include <stdlib.h>
#include <string.h> // for memset

#ifndef DBL_MAX
#define DBL_MAX 1.79769e+308
#endif

// returns 1 if power of 2
static inline int32_t _power2p(int32_t value) {
	if (value == 0) return 1;
	if (value == 2) return 1;
	if (value & 0x1) return 0;
	return (_power2p(value >> 1));
}

// count number of bits
static inline int32_t _bitcount(int32_t value) {
	if (value == 0) return 0;
	if (value == 1) return 1;
	if (value == 2) return 2;
	return _bitcount(value >> 1) + 1;
}

// closest power of 2 above or equal
static inline int32_t _ceil_power2(int32_t value) {
	if (_power2p(value)) return value;
	
	if (value == 1) return 2;
	int32_t j, i = _bitcount(value);
	int32_t res = 1;
	for (j = 0; j < i; j++) res <<= 1;
	return res;
}

// closest power of 2 below or equal
static inline int32_t _floor_power2(int32_t value) {
	if (_power2p(value)) return value;
	return _ceil_power2(value)/2;
}

// 2 power
static int32_t _2power(int32_t i) {
	int32_t res = 1, j;
	for (j = 0; j < i; j++) res <<= 1;
	return res;
}


COMPILER_ALIGNED(WIN_SIZE) static float sam[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static int32_t distances[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static int32_t mins[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static int32_t maxs[WIN_SIZE];

float computeWaveletPitch(float * samples) 
{
	float pitchF = 0.0;
	int32_t i, j;
	float si, si1;
	float power; 
	arm_power_f32(samples, WIN_SIZE>>2, &power); 
	if (power < POWER_THRESHOLD) return pitchF; 
	
	arm_copy_f32(samples, sam, WIN_SIZE); 
	int32_t curSamNb = WIN_SIZE;
	int32_t nbMins, nbMaxs;
	
	// algorithm parameters
	int32_t maxFLWTlevels = 6;
	float maxF = 3000.;
	int32_t differenceLevelsN = 3;
	float maximaThresholdRatio = 0.75;
	
	float ampltitudeThreshold;  
	float theDC = 0.0;
	uint32_t temp_idx; 
	float maxValue;
	float minValue; 
	
	{ // compute ampltitudeThreshold and theDC
		//first compute the DC and maxAMplitude
		arm_mean_f32(sam, WIN_SIZE, &theDC); 
		arm_max_f32(sam, WIN_SIZE, &maxValue, &temp_idx); 
		arm_min_f32(sam, WIN_SIZE, &minValue, &temp_idx); 
		maxValue = maxValue - theDC;
		minValue = minValue - theDC;
		float amplitudeMax = (maxValue > -minValue ? maxValue : -minValue);
		
		ampltitudeThreshold = amplitudeMax*maximaThresholdRatio;		
	}
	
	// levels, start without downsampling..
	int32_t curLevel = 0;
	float curModeDistance = -1.;
	int32_t delta;
	
	while(1) 
	{
		// delta
		delta = DYW_SAMPLING_RATE/(_2power(curLevel)*maxF);
				
		if (curSamNb < 2) return pitchF;
		
		// compute the first maximums and minumums after zero-crossing
		// store if greater than the min threshold
		// and if at a greater distance than delta
		float dv, previousDV = -1000;
		nbMins = nbMaxs = 0;   
		int32_t lastMinIndex = -1000000;
		int32_t lastmaxIndex = -1000000;
		int32_t findMax = 0;
		int32_t findMin = 0;
		for (i = 1; i < curSamNb; i++) 
		{
			si = sam[i] - theDC;
			si1 = sam[i-1] - theDC;
			
			if (si1 <= 0 && si > 0) {findMax = 1; findMin = 0; }
			if (si1 >= 0 && si < 0) {findMin = 1; findMax = 0; }
			
			// min or max ?
			dv = si - si1;
			
			if (previousDV > -1000) 
			{
				if (findMin && previousDV < 0 && dv >= 0) 
				{ 
					// minimum
					if (Abs(si1) >= ampltitudeThreshold) 
					{
						if (i - 1 > lastMinIndex + delta)
						{
							mins[nbMins++] = i - 1;
							lastMinIndex = i - 1;
							findMin = 0;
						} 
					} 
				}
				
				if (findMax && previousDV > 0 && dv <= 0) 
				{
					// maximum
					if (Abs(si1) >= ampltitudeThreshold) 
					{
						if (i -1 > lastmaxIndex + delta) 
						{
							maxs[nbMaxs++] = i - 1;
							lastmaxIndex = i - 1;
							findMax = 0;
						} 
					} 
				}
			}
			previousDV = dv;
		}
		
		if (nbMins == 0 && nbMaxs == 0) 
		{
			// no best distance !
			return pitchF;
		}
		
		// maxs = [5, 20, 100,...]
		// compute distances
		int32_t d;
		memset(distances, 0, WIN_SIZE*sizeof(int32_t));
		for (i = 0 ; i < nbMins ; i++) 
		{
			for (j = 1; j < differenceLevelsN; j++) 
			{
				if (i+j < nbMins) 
				{
					d = Abs(mins[i] - mins[i+j]);
					distances[d] = distances[d] + 1;
				}
			}
		}
		for (i = 0 ; i < nbMaxs ; i++) 
		{
			for (j = 1; j < differenceLevelsN; j++) 
			{
				if (i+j < nbMaxs) 
				{
					d = Abs(maxs[i] - maxs[i+j]);
					distances[d] = distances[d] + 1;
				}
			}
		}
		
		// find best summed distance
		int32_t bestDistance = -1;
		int32_t bestValue = -1;
		for (i = 0; i< curSamNb; i++) 
		{
			int32_t summed = 0;
			for (j = -delta ; j <= delta ; j++) 
			{
				if (i+j >=0 && i+j < curSamNb)
					summed += distances[i+j];
			}
			if (summed == bestValue) 
			{ // break tie 
				if (i == 2*bestDistance)
					bestDistance = i;
			} 
			else if (summed > bestValue) 
			{
				bestValue = summed;
				bestDistance = i;
			}
		}
		
		// averaging
		float distAvg = 0.0;
		float nbDists = 0;
		for (j = -delta ; j <= delta ; j++) 
		{
			if (bestDistance+j >=0 && bestDistance+j < WIN_SIZE) 
			{	// don't really understand this averaging scheme 
				int32_t nbDist = distances[bestDistance+j];
				if (nbDist > 0) {
					nbDists += nbDist;
					distAvg += (bestDistance+j)*nbDist;
				}
			}
		}
		// this is our mode distance !
		distAvg /= nbDists;
		
		// continue the levels ?
		if (curModeDistance > -1.) 
		{
			float similarity = Abs(distAvg*2 - curModeDistance);
			if (similarity <= 2*delta) 
			{
				// two consecutive similar mode distances : ok !
				pitchF = DYW_SAMPLING_RATE/(_2power(curLevel-1)*curModeDistance);
				return pitchF;
			}
		}
		
		// not similar, continue next level
		curModeDistance = distAvg;
		
		curLevel = curLevel + 1;
		if (curLevel >= maxFLWTlevels) 
		{
			// put "max levels reached, exiting"
			return pitchF;
		}
		
		// downsample
		if (curSamNb < 2) 
		{
			return pitchF;
		}
		
		for (i = 0; i < curSamNb/2; i++)
		{
			sam[i] = (sam[2*i] + sam[2*i + 1]) * 0.5;
		}
		curSamNb /= 2;
	}
	
	return pitchF;
}


