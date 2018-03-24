#ifndef dywapitchtrack__H
#define dywapitchtrack__H

#include "asf.h"
#include "DMA_Audio.h"

/*#define DYW_SAMPLING_RATE 23250*/
#define DYW_SAMPLING_RATE 46503.0f 

// structure to hold tracking data
typedef struct _dywapitchtracker {
	float	_prevPitch;
	int		_pitchConfidence;
} dywapitchtracker;


// return -1.0 if no pitch was found (sound too low, noise, etc..)
float computeWaveletPitch(float * samples); 

#endif

