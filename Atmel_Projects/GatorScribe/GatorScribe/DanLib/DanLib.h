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

#define MAX_NUM_HARMONIES 10 

#define POWER_THRESHOLD 0.000001f
#define ONE_OVER_64 0.015625000F

#define HARMONY_VOLUME_FLAG 255 
#define MASTER_VOLUME_FLAG 254
#define PITCH_BEND_FLAG 253
#define AUTOTUNE_FLAG 252 
#define REVERB_VOLUME_FLAG 251 
#define CHORUS_VOLUME_FLAG 250
#define DELAY_VOLUME_FLAG 246
#define DELAY_SPEED_FLAG 248
#define DRY_VOLUME_FLAG 247
#define DELAY_FEEDBACK_FLAG 249
#define CHORUS_SPEED_FLAG 245 


#define NOTE_ON 144
#define NOTE_OFF 128 
#define SLIDER 176
#define PITCH_WHEEL 224
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