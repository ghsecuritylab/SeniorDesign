/*
 * psola.c
 *
 * Created: 3/13/2018 3:53:58 PM
 *  Author: Daniel Gonzalez
 */ 

#include "psola.h"
#include <stdlib.h>


#define RING_BUFFER_SIZE 8192 
#define RING_BUFFER_SIZE_D2 (RING_BUFFER_SIZE >> 1) 
#define RING_BUFFER_MASK (RING_BUFFER_SIZE-1)

#define PI_F 3.14159265358f 

/************************ Static variables *********************/ 
static float input_ring_buffer[RING_BUFFER_SIZE];
static float output_ring_buffer[RING_BUFFER_SIZE];

static uint32_t readPos; 
static uint32_t inPtr; 
static uint32_t outPtr;
static uint32_t samplesLeftInPeriod; 
static int32_t inputPeriodLength; 
static float currentPitch; 
static float currentShift; 
/************************ Static variables *********************/

void PSOLA_init(void)
{
	arm_fill_f32(0.0F, input_ring_buffer, RING_BUFFER_SIZE); 
	arm_fill_f32(0.0F, output_ring_buffer, RING_BUFFER_SIZE);
	
	readPos = RING_BUFFER_SIZE - WIN_SIZE; 
	inPtr = 0; 
	outPtr = 0; 
	samplesLeftInPeriod = 0; 
	inputPeriodLength = 100; 
	currentPitch = 20.0f; 
	currentShift = 1.0f; 
}

void pitchCorrect(float* input, float *output, float inputPitch, float shift_amount) 
{
	
	uint32_t size = WIN_SIZE; 
	uint32_t curInSample = 0; 
	uint32_t curOutSample = 0; 
	uint32_t outLag;
	uint32_t inHalfAway;  
	float window_value; 
	float correctedPitchScale; 
	float correctedPitchIn; 
	
	// Do error checking
	if (currentPitch < 40.0f) 
	{
		correctedPitchIn = 40.0f; 
		correctedPitchScale = 1.0f;
	}
	else 
	{
		correctedPitchIn = currentPitch; 
		correctedPitchScale = currentShift;
	}

	float inputPeriodLengthRecip = 1.0f / inputPeriodLength; 
	float periodRatio = 1.0f / correctedPitchScale; 
	while(size > 0)
	{
		input_ring_buffer[(inPtr+WIN_SIZE) & RING_BUFFER_MASK] = input[curInSample]; 
		
		if (samplesLeftInPeriod == 0)
		{
			outLag = 1; 
			
			inHalfAway = (inPtr + RING_BUFFER_SIZE_D2) & RING_BUFFER_MASK;
                
            if (inHalfAway < RING_BUFFER_SIZE_D2) 
			{
                /* The zero element of the input buffer lies
                    in (inptr, inHalfAway] */
                if (outPtr < inHalfAway || outPtr > inPtr) {
                    // The current input element lags current
                    // synthesis pitch marker.
                    outLag = 0;
                }
            } 
			else 
			{
                /* The zero element of the input buffer lies
                    in (inHalfAway, inptr] */
                if (outPtr > inPtr && outPtr < inHalfAway) {
                    // The current input element lags current
                    // synthesis pitch marker.
                    outLag = 0;
                }
            }
			
			while(outLag == 1)
			{
				// set outPtr about the sample at which we OLA 
				outPtr = (outPtr + (uint32_t)((float)inputPeriodLength * periodRatio)) & RING_BUFFER_MASK; 
				
				// OLA 
				for (int32_t olaIdx = -inputPeriodLength; olaIdx < inputPeriodLength; olaIdx++)
				{
					window_value = (1.0f + arm_cos_f32(PI_F * (float)olaIdx * inputPeriodLengthRecip)) * 0.5f; 
					output_ring_buffer[(uint32_t)(olaIdx + (int64_t)outPtr) & RING_BUFFER_MASK] += 
						window_value * input_ring_buffer[(uint32_t)(olaIdx + (int64_t)inPtr) & RING_BUFFER_MASK]; 
				}
				
				if (inHalfAway < RING_BUFFER_SIZE_D2) 
				{
					/* The zero element of the input buffer lies
						in (inptr, inHalfAway] */
					if (outPtr < inHalfAway || outPtr > inPtr) {
						// The current input element lags current
						// synthesis pitch marker.
						outLag = 0;
					}
				} 
				else 
				{
					/* The zero element of the input buffer lies
						in (inHalfAway, inptr] */
					if (outPtr > inPtr && outPtr < inHalfAway) {
						// The current input element lags current
						// synthesis pitch marker.
						outLag = 0;
					}
				}		
			}
			
			// assume uniform frequency within window 
			samplesLeftInPeriod = inputPeriodLength;
		}
		
		--samplesLeftInPeriod; 
		
		// after processing 
		output[curOutSample] = output_ring_buffer[readPos];
		
		// delete and inc/wrap output ring buffer index 
		output_ring_buffer[readPos] = 0.0f; 
		readPos = (readPos+1) & RING_BUFFER_MASK; 
		
		// inc/wrap input ring buffer index 
		inPtr = (inPtr+1) & RING_BUFFER_MASK; 
		
		curOutSample++; 
		curInSample++; 
		size--; 
	}
	inputPeriodLength = (uint32_t)((float)PSOLA_SAMPLE_RATE / correctedPitchIn);
	currentPitch = inputPitch; 
	currentShift = shift_amount; 
}


