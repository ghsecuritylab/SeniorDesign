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
static float currentShifts[11]; 
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
	currentShifts[0] = 1.0f; 
	currentShifts[1] = -1.0f; 
}

void create_harmonies(float* input, float *output, float inputPitch, float *pitch_shifts_in) 
{
	
	uint32_t saved_inPtr = inPtr; 
	uint32_t saved_outPtr = outPtr; 
	uint32_t saved_samplesLeftInPeriod = samplesLeftInPeriod; 
	uint32_t pitch_idx = 0; // safely divide for averaging later on 
	
	uint32_t starting_input_ptr = inPtr + WIN_SIZE; 
	for (uint32_t i = 0; i < WIN_SIZE; i++)
	{
		input_ring_buffer[(starting_input_ptr++) & RING_BUFFER_MASK] = input[i]; 
	}
		
	uint32_t size;
	uint32_t outLag;
	uint32_t inHalfAway;
	float window_value, periodRatio;
	float inputPeriodLengthRecip = 1.0f / inputPeriodLength;
	while(currentShifts[pitch_idx] > 0.0f)
	{
		size = WIN_SIZE; 
		periodRatio = 1.0f / currentShifts[pitch_idx++]; 
		samplesLeftInPeriod = saved_samplesLeftInPeriod; 
		inPtr = saved_inPtr; 
		outPtr = saved_outPtr; 
		while(size > 0)
		{		
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
		
			// inc/wrap input ring buffer index 
			inPtr = (inPtr+1) & RING_BUFFER_MASK; 
		
			size--; 
		}
	}
	
	for(uint32_t i = 0; i < WIN_SIZE; i++)
	{
		output[i] = output_ring_buffer[readPos]; 	
		output_ring_buffer[readPos] = 0.0f;
		readPos = (readPos+1) & RING_BUFFER_MASK;
	}
	
	if (pitch_idx < 1) pitch_idx = 1; // just in case 
	arm_scale_f32(output, 1.0f / (float)pitch_idx, output, WIN_SIZE); 
	inputPeriodLength = (uint32_t)((float)PSOLA_SAMPLE_RATE / currentPitch);
	currentPitch = inputPitch; 
	pitch_idx = 0; 
	while(pitch_shifts_in[pitch_idx] > 0.0f)
	{
		currentShifts[pitch_idx] = pitch_shifts_in[pitch_idx]; 
		pitch_idx++; 
	}
	currentShifts[pitch_idx] = -1.0f; 
}


