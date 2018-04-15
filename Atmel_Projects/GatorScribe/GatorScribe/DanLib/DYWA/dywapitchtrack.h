#ifndef dywapitchtrack__H
#define dywapitchtrack__H

#include "asf.h"
#include "DMA_Audio.h"

// structure to hold tracking data
typedef struct _dywapitchtracker {
	float	_prevPitch;
	int		_pitchConfidence;
} dywapitchtracker;


// return -1.0 if no pitch was found (sound too low, noise, etc..)
float computeWaveletPitch(float * samples); 

#endif

