/*
 * audio_to_midi.h
 *
 * Created: 1/3/2018 7:15:10 AM
 *  Author: Daniel Gonzalez
 */ 


#ifndef AUDIO_TO_MIDI_H_
#define AUDIO_TO_MIDI_H_
#include "pitchyinfast.h"

typedef struct midi_note 
{
	int16_t note_number; 
	int16_t velocity; 
}midi_note_t;

void get_midi_note(float32_t *buffer, midi_note_t *note, aubio_pitchyinfast_t *object);
void get_midi_note_name(char *note_name, int16_t note_number);
void get_frequency_str(char *freq_name, int16_t note_number);

#endif /* AUDIO_TO_MIDI_H_ */
