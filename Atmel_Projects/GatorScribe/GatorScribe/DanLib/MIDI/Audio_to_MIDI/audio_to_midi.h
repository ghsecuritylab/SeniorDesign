/*
 * audio_to_midi.h
 *
 * Created: 1/3/2018 7:15:10 AM
 *  Author: Daniel Gonzalez
 */ 


#ifndef AUDIO_TO_MIDI_H_
#define AUDIO_TO_MIDI_H_

#include "DanLib.h"

typedef struct midi_note 
{
	int16_t note_number; 
	int16_t velocity; 
}midi_note_t;

typedef struct midi_event
{
	int16_t note_number;
	int16_t velocity;
    float rhythm;
}midi_event_t;

void start_recording(uint32_t bpm, midi_instrument_t playback_instrument, time_signature_t time_signature , key_signature_t key_signature, char *title);
#endif /* AUDIO_TO_MIDI_H_ */
