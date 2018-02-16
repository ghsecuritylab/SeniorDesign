/* dywapitchtrack.c
 
 Dynamic Wavelet Algorithm Pitch Tracking library
 Released under the MIT open source licence
  
 Copyright (c) 2010 Antoine Schmitt
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

#include "dywapitchtrack.h"
#include <math.h>
#include "arm_math.h"
#include <stdlib.h>
#include <string.h> // for memset


//**********************
//       Utils
//**********************

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

//******************************
// the Wavelet algorithm itself
//******************************

int32_t dywapitch_neededsamplecount(int32_t minFreq) {
	int32_t nbSam = 3*DYW_SAMPLING_RATE/minFreq; // 1017. for 130 Hz
	nbSam = _ceil_power2(nbSam); // 1024
	return nbSam;
}

COMPILER_ALIGNED(WIN_SIZE) static float sam[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static int32_t distances[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static int32_t mins[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static int32_t maxs[WIN_SIZE];

static float _dywapitch_computeWaveletPitch(float * samples) 
{
	float pitchF = 0.0;
	int32_t i, j;
	float si, si1;
	
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
	
	{ // compute ampltitudeThreshold and theDC
		//first compute the DC and maxAMplitude
		float maxValue = -DBL_MAX;
		float minValue = DBL_MAX;
		for (i = 0; i < WIN_SIZE;i++) 
		{
			si = sam[i];
			theDC = theDC + si;
			if (si > maxValue) maxValue = si;
			if (si < minValue) minValue = si;
		}
		theDC = theDC/WIN_SIZE;
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
			sam[i] = (sam[2*i] + sam[2*i + 1])/2.;
		}
		curSamNb /= 2;
	}
	
	return pitchF;
}

// ***********************************
// the dynamic postprocess
// ***********************************

/***
It states: 
 - a pitch cannot change much all of a sudden (20%) (impossible humanly,
 so if such a situation happens, consider that it is a mistake and drop it. 
 - a pitch cannot float or be divided by 2 all of a sudden : it is an
 algorithm side-effect : divide it or float it by 2. 
 - a lonely voiced pitch cannot happen, nor can a sudden drop in the middle
 of a voiced segment. Smooth the plot. 
***/

static inline float _dywapitch_dynamicprocess(dywapitchtracker *pitchtracker, float pitch) {
	
	// equivalence
	if (pitch == 0.0) pitch = -1.0;
	
	//
	float estimatedPitch = -1;
	float acceptedError = 0.2f;
	int32_t maxConfidence = 5;
	
	if (pitch != -1) {
		// I have a pitch here
		
		if (pitchtracker->_prevPitch == -1) {
			// no previous
			estimatedPitch = pitch;
			pitchtracker->_prevPitch = pitch;
			pitchtracker->_pitchConfidence = 1;
			
		} else if (abs(pitchtracker->_prevPitch - pitch)/pitch < acceptedError) {
			// similar : remember and increment pitch
			pitchtracker->_prevPitch = pitch;
			estimatedPitch = pitch;
			pitchtracker->_pitchConfidence = min(maxConfidence, pitchtracker->_pitchConfidence + 1); // maximum 3
			
		} else if ((pitchtracker->_pitchConfidence >= maxConfidence-2) && abs(pitchtracker->_prevPitch - 2.*pitch)/(2.*pitch) < acceptedError) {
			// close to half the last pitch, which is trusted
			estimatedPitch = 2.*pitch;
			pitchtracker->_prevPitch = estimatedPitch;
			
		} else if ((pitchtracker->_pitchConfidence >= maxConfidence-2) && abs(pitchtracker->_prevPitch - 0.5*pitch)/(0.5*pitch) < acceptedError) {
			// close to twice the last pitch, which is trusted
			estimatedPitch = 0.5*pitch;
			pitchtracker->_prevPitch = estimatedPitch;
			
		} else {
			// nothing like this : very different value
			if (pitchtracker->_pitchConfidence >= 1) {
				// previous trusted : keep previous
				estimatedPitch = pitchtracker->_prevPitch;
				pitchtracker->_pitchConfidence = max(0, pitchtracker->_pitchConfidence - 1);
			} else {
				// previous not trusted : take current
				estimatedPitch = pitch;
				pitchtracker->_prevPitch = pitch;
				pitchtracker->_pitchConfidence = 1;
			}
		}
		
	} else {
		// no pitch now
		if (pitchtracker->_prevPitch != -1) {
			// was pitch before
			if (pitchtracker->_pitchConfidence >= 1) {
				// continue previous
				estimatedPitch = pitchtracker->_prevPitch;
				pitchtracker->_pitchConfidence = max(0, pitchtracker->_pitchConfidence - 1);
			} else {
				pitchtracker->_prevPitch = -1;
				estimatedPitch = -1.;
				pitchtracker->_pitchConfidence = 0;
			}
		}
	}
	
	// put "_pitchConfidence="&pitchtracker->_pitchConfidence
	if (pitchtracker->_pitchConfidence >= 1) {
		// ok
		pitch = estimatedPitch;
	} else {
		pitch = -1;
	}
	
	// equivalence
	if (pitch == -1) pitch = 0.0;
	
	return pitch;
}


// ************************************
// the API main entry point32_ts
// ************************************

void dywapitch_inittracking(dywapitchtracker *pitchtracker) {
	pitchtracker->_prevPitch = -1.;
	pitchtracker->_pitchConfidence = -1;
}

float dywapitch_computepitch(dywapitchtracker *pitchtracker, float * samples) {
	float raw_pitch = _dywapitch_computeWaveletPitch(samples);
	return _dywapitch_dynamicprocess(pitchtracker, raw_pitch);
}



