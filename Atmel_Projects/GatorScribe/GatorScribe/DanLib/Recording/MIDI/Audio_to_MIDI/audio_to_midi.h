/*
 * audio_to_midi.h
 *
 * Created: 1/3/2018 7:15:10 AM
 *  Author: Daniel Gonzalez
 */ 


#ifndef AUDIO_TO_MIDI_H_
#define AUDIO_TO_MIDI_H_
#include "pitchyinfast.h"

#define NO_NOTE -1 
#define END_OF_RECORDING -2 

typedef struct midi_note 
{
	int16_t note_number; 
	int16_t velocity; 
}midi_note_t;

typedef struct midi_event
{
	int16_t note_number;
	int16_t velocity;
	float rhythm; // 0.25 = 16th, 0.5 = eight, 1 = quater, 2 = half, 3 = .half, 4 = whole
}midi_event_t;

void get_midi_note(float32_t *buffer, midi_note_t *note, aubio_pitchyinfast_t *object);
void get_midi_note_name(char *note_name, int16_t note_number);
void get_frequency_str(char *freq_name, int16_t note_number);

#endif /* AUDIO_TO_MIDI_H_ */
