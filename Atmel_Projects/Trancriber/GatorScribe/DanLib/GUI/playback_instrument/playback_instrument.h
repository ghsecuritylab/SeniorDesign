/*
 * playback_instrument.h
 *
 * Created: 12/27/2017 3:37:18 AM
 *  Author: Daniel Gonzalez
 */ 


#ifndef PLAYBACK_INSTRUMENT_H_
#define PLAYBACK_INSTRUMENT_H_

typedef enum midi_instrument
{
	GUITAR = 27, // clean electric guitar
	PIANO = 0, // acoustic grand piano
	VIOLIN = 40,
	TRUMPET = 56,
	SYNTH = 80, // square synth lead
	SAX = 64 // soprano sax
}midi_instrument_t;


midi_instrument_t instrument_menu(void); 




#endif /* PLAYBACK_INSTRUMENT_H_ */