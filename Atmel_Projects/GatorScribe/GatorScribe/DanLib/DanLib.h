/*
 * DanLib.h
 *
 * Created: 12/22/2017 4:17:28 AM
 *  Author: Daniel Gonzalez
 */ 


#ifndef DANLIB_H_
#define DANLIB_H_

#include "audio.h"
#include "LCDLib.h"
#include "vocoder.h"
#include "DYWA/dywapitchtrack.h"
#include "psola.h"

enum harmonies 
{
	OCTAVE_DOWN = -12, 
	MAJOR_2ND_BELOW = -10, 
	MAJOR_3RD_BELOW = -8, 
	PERFECT_4TH_BELOW = -7, 
	PERFECT_5TH_BELOW = -5, 
	MINOR_6TH_BELOW = -3, 
	DIMINISHED_7TH_BELOW = -1, 
	ROOT = 0, 
	MAJOR_2ND_ABOVE = 2, 
	MAJOR_3RD_ABOVE = 4, 
	PERFECT_4TH_ABOVE = 5, 
	PERFECT_5TH_ABOVE = 7, 
	MINOR_6TH_ABOVE = 9, 
	DIMINISHED_7TH_ABOVE = 11, 
	OCTAVE_UP = 12
};

#define HALF_STEP 1.059463094359f
#define POWER_THRESHOLD 0.000001f


#endif /* DANLIB_H_ */