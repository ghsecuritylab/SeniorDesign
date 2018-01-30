/*
 * PSOLA.h
 *
 * Created: 1/29/2018 5:06:57 PM
 *  Author: Daniel Gonzalez
 */ 


#ifndef PSOLA_H_
#define PSOLA_H_

#include "asf.h"
#include "arm_math.h"

#define DEFAULT_BUFFER_SIZE 512
#define FIXED_BITS        16
#define FIXED_WBITS       0
#define FIXED_FBITS       15
#define Q15_RESOLUTION   (1 << (FIXED_FBITS - 1))
#define LARGEST_Q15_NUM   32767

void PSOLA_init(uint32_t bufferLen); 
void pitchCorrect(float32_t* input, int Fs, float inputPitch, float desiredPitch); 

#endif /* PSOLA_H_ */