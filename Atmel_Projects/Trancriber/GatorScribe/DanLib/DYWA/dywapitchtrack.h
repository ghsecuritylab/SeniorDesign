#ifndef dywapitchtrack__H
#define dywapitchtrack__H

#include "asf.h"
#include "DMA_Audio.h"

#define DYW_SAMPLING_RATE 23600.0f
/*47200.0f*/
//#define DYW_SAMPLING_RATE 46503.0f 
#define WIN_SIZE 2048

// structure to hold tracking data
typedef struct _dywapitchtracker {
	float	_prevPitch;
	int		_pitchConfidence;
} dywapitchtracker;


// return -1.0 if no pitch was found (sound too low, noise, etc..)
float computeWaveletPitch(float * samples); 

#endif

