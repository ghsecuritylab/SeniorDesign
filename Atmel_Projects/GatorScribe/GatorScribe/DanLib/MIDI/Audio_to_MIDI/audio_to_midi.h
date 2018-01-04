/*
 * audio_to_midi.h
 *
 * Created: 1/3/2018 7:15:10 AM
 *  Author: Daniel Gonzalez
 */ 


#ifndef AUDIO_TO_MIDI_H_
#define AUDIO_TO_MIDI_H_

typedef struct midi_note 
{
	int16_t note_number; 
	int16_t  velocity; 
}midi_note_t;

typedef struct midi_event
{
	int16_t note_number;
	int16_t velocity;
	uint8_t rhythm; 
}midi_event_t;

void get_midi_note(int16_t *buffer, midi_note_t *midi_note);
void get_midi_note_name(char *str, int16_t note_number); 

#endif /* AUDIO_TO_MIDI_H_ */