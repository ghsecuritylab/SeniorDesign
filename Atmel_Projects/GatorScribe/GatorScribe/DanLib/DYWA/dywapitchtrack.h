#ifndef dywapitchtrack__H
#define dywapitchtrack__H

#include "asf.h"
#include "DMA_Audio.h"

#define DYW_SAMPLING_RATE 24000.0f //23250.0f
#define POWER_THRESHOLD 0.000001f

// return 0.0 if no pitch was found (sound too low, noise, etc..)
float computeWaveletPitch(float * samples); 

#endif

