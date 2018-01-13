#ifndef Yin_h
#define Yin_h

#include <stdint.h>
#include <math.h>
#include "arm_math.h"

//#define YIN_SAMPLING_RATE 46500
#define YIN_SAMPLING_RATE 23250

#define YIN_DEFAULT_THRESHOLD 0.05 

/**
 * @struct  Yin
 * @breif	Object to encapsulate the parameters for the Yin pitch detection algorithm 
 */
typedef struct _Yin {
	int16_t bufferSize;			/**< Size of the audio buffer to be analysed */
	int16_t halfBufferSize;		/**< Half the buffer length */
	float32_t* yinBuffer;		/**< Buffer that stores the results of the intermediate processing steps of the algorithm */
	float32_t probability;		/**< Probability that the pitch found is correct as a decimal (i.e 0.85 is 85%) */
	float32_t threshold;		/**< Allowed uncertainty in the result as a decimal (i.e 0.15 is 15%) */
	float32_t* deltaBuffer; 
	float32_t* runningSum; 
} Yin;

/**
 * Initialise the Yin pitch detection object
 * @param bufferSize Length of the audio buffer to analyse
 * @param threshold  Allowed uncertainty (e.g 0.05 will return a pitch with ~95% probability)
 */
void yin_init(int16_t bufferSize, float32_t threshold);

/**
 * Runs the Yin pitch detection algortihm
 * @param  buffer Buffer of samples to analyse
 * @return        Fundamental frequency of the signal in Hz. Returns -1 if pitch can't be found
 */
float32_t yin_getPitch(int16_t* buffer);

/**
 * Certainty of the pitch found 
 * @return     Returns the certainty of the note found as a decimal (i.e 0.3 is 30%)
 */
float32_t yin_getProbability(void);

void yin_free_buffer(void); 

#endif
