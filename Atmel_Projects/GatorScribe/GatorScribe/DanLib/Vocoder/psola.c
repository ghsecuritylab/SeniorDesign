/*
 * psola.c
 *
 * Created: 3/13/2018 3:53:58 PM
 *  Author: Daniel Gonzalez
 */ 

#include "psola.h"
#include <stdlib.h>


#define RING_BUFFER_SIZE 4096
#define RING_BUFFER_SIZE_D2 (RING_BUFFER_SIZE >> 1) 
#define RING_BUFFER_MASK (RING_BUFFER_SIZE-1)

#define PI_F 3.14159265358f 

/************************ Static variables *********************/ 
COMPILER_ALIGNED(RING_BUFFER_SIZE) static float input_ring_buffer[RING_BUFFER_SIZE];
COMPILER_ALIGNED(RING_BUFFER_SIZE) static float output_ring_buffer[RING_BUFFER_SIZE];

static uint32_t readPos; 
static uint32_t inPtr; 
static uint32_t outPtrList[MAX_NUM_SHIFTS];
static uint32_t current_num_shifts; 
static uint32_t saved_samplesLeftInPeriod[MAX_NUM_SHIFTS]; 
static int32_t inputPeriodLength; 
static float currentPitch; 
static float prev_pitch_shifts[MAX_NUM_SHIFTS]; 
static float window[10*WIN_SIZE]; // sufficiently large window array  
/************************ Static variables *********************/

void PSOLA_init(void)
{
	arm_fill_f32(0.0f, input_ring_buffer, RING_BUFFER_SIZE); 
	arm_fill_f32(0.0f, output_ring_buffer, RING_BUFFER_SIZE);
	arm_fill_f32(0.0f, window, 10*WIN_SIZE); 
	arm_fill_f32(1.0f, prev_pitch_shifts, MAX_NUM_SHIFTS); 
	prev_pitch_shifts[0] = 1.0f; 
	prev_pitch_shifts[1] = -1.0f; 
	
	for(uint32_t i = 0; i < MAX_NUM_SHIFTS; i++)
	{
		outPtrList[i] = 0; 
		saved_samplesLeftInPeriod[i] = 0; 
	}
		
	current_num_shifts = 1; // always doing root 
	
	readPos = RING_BUFFER_SIZE - WIN_SIZE; // + WEIRD_OFFSET; 
	inPtr = 0; 
	inputPeriodLength = PSOLA_SAMPLE_RATE / MINIMUM_PITCH; 
	currentPitch = MINIMUM_PITCH; 
}

// assumes valid pitch shifts 
void create_harmonies(float* input, float *output, float inputPitch, float *pitch_shifts_in, float harmony_vol,  float autotune_flag, float autotune_vol)
{
	uint32_t i, w; 
	int32_t olaIdx; 
	
	uint32_t saved_inPtr = inPtr; 
	uint32_t outPtr; 
	uint32_t pitch_idx = 0; 
	
	uint32_t starting_input_ptr = inPtr + WIN_SIZE; 
	for (i = 0; i < WIN_SIZE; i++)
	{
		input_ring_buffer[(starting_input_ptr++) & RING_BUFFER_MASK] = input[i]; 
	}
			
	uint32_t outLag;
	uint32_t inHalfAway;
	float periodRatio;
	float inputPeriodLengthRecip = 1.0f / inputPeriodLength;
	uint32_t samplesLeftInPeriod = 0; 
	
	float scale = 1.0f / log2f((float)(current_num_shifts+1));
	if (current_num_shifts > 1) 
		scale *= 1.2f; 
	
	// pre-compute window function	
	for (olaIdx = 0, w = 0; olaIdx < 2*inputPeriodLength; olaIdx++, w++)
	{
		window[w] = scale * (1.0f - arm_cos_f32(PI_F * (float)olaIdx * inputPeriodLengthRecip)) * 0.5f;
	}
		
	// for each pitch shift 
	while(pitch_idx < MAX_NUM_SHIFTS && pitch_shifts_in[pitch_idx] > 0.0f)
	{
		periodRatio = 1.0f / pitch_shifts_in[pitch_idx]; 
		inPtr = saved_inPtr; 

		// find closest harmony within previous harmony list to better match the phase 
		outPtr = outPtrList[0];
		samplesLeftInPeriod = saved_samplesLeftInPeriod[0];
		if (pitch_idx > 0)
		{
			float tmp; 
			float min = Abs(pitch_shifts_in[pitch_idx] - prev_pitch_shifts[0]); 
			for (i = 1; i < current_num_shifts; i++)
			{
				tmp = Abs(pitch_shifts_in[pitch_idx] - prev_pitch_shifts[i]);
				if (tmp < min)
				{
					outPtr = outPtrList[i]; 
					samplesLeftInPeriod = saved_samplesLeftInPeriod[i]; 
					min = tmp; 
				}
			}
		}

		for (i = 0; i < WIN_SIZE; i++)
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
					if (pitch_idx == 0 && autotune_flag == 1)
					{
						for (olaIdx = -inputPeriodLength, w = 0; olaIdx < inputPeriodLength; olaIdx++, w++)
						{
							output_ring_buffer[(uint32_t)(olaIdx + (int64_t)outPtr) & RING_BUFFER_MASK] +=
								autotune_vol * window[w] * input_ring_buffer[(uint32_t)(olaIdx + (int64_t)inPtr + LAG_OFFSET) & RING_BUFFER_MASK];
						}
					}
					else 
					{
						for (olaIdx = -inputPeriodLength, w = 0; olaIdx < inputPeriodLength; olaIdx++, w++)
						{
							output_ring_buffer[(uint32_t)(olaIdx + (int64_t)outPtr) & RING_BUFFER_MASK] +=
								harmony_vol * window[w] * input_ring_buffer[(uint32_t)(olaIdx + (int64_t)inPtr + LAG_OFFSET) & RING_BUFFER_MASK];
						}
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
		}

		saved_samplesLeftInPeriod[pitch_idx] = samplesLeftInPeriod; 
		outPtrList[pitch_idx] = outPtr; 
		pitch_idx++;  
	}
	
	for(i = 0; i < WIN_SIZE; i++)
	{
		output[i] = output_ring_buffer[readPos]; 	
		output_ring_buffer[readPos] = 0.0f;
		readPos = (readPos+1) & RING_BUFFER_MASK;
	}
	
	// averaging -- not used currently 
	//if (pitch_idx > 1) //  scale = 2.0f / (float)current_num_shifts ; // just in case 
	//arm_scale_f32(output, 2.0f, output, WIN_SIZE); 
	
	currentPitch = inputPitch; 
	inputPeriodLength = (uint32_t)(PSOLA_SAMPLE_RATE / currentPitch);
	current_num_shifts = pitch_idx; 
	arm_copy_f32(pitch_shifts_in, prev_pitch_shifts, MAX_NUM_SHIFTS); 
}


