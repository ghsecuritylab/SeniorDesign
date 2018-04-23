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
#include "DYWA/dywapitchtrack.h"
#include "psola.h"


typedef struct harmony 
{
	volatile float freq; 
	volatile uint32_t idx; // index of frequency... also midi note 
	volatile bool active; 
}harmony_t;

#define MAX_NUM_KEYS_HARMONIES 10 // not including root  

#define POWER_THRESHOLD 0.000001f
#define ONE_OVER_64 0.015625000F

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

enum scale_steps
{
	W = 2, 
	H = 1
};


// scale correct directions
#define SCALE_DOWN -1.0f
#define SCALE_NONE 0.0f 
#define SCALE_UP 1.0f
#define SCALE_CORRECT_HISTORY_SIZE 8
#define SCALE_CORRECT_HISTORY_MASK (SCALE_CORRECT_HISTORY_SIZE-1)


typedef uint32_t scale_t; 

#define KEY_OF_E 64 

typedef struct chord
{
	float freq; 
	bool active; 
}chord_t;

#define MASTER_VOL_BASE 0.6f

#define NOTE_ON 144
#define NOTE_OFF 128 
#define SLIDER 176
#define PITCH_WHEEL 224
#define CH_BUTTON 192
#define DRY_VOLUME_CH 7
#define HARM_VOLUME_CH 84 
#define MASTER_VOLUME_CH 5 
#define REVERB_CH 72 
#define CHORUS_VOLUME_CH 73 
#define CHORUS_SPEED_CH 93 
#define DELAY_FEEDBACK_CH 91
#define DELAY_SPEED_CH 71 
#define DELAY_VOLUME_CH 74 
#endif /* DANLIB_H_ */