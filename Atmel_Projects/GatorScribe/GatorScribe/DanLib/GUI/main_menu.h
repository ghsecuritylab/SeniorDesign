/*
 * main_menu.h
 *
 * Created: 12/26/2017 3:20:19 AM
 *  Author: Daniel Gonzalez
 */ 

#include "playback_instrument.h"
#include "song_title.h"
#include "tempo.h"
#include "time_signature.h"
#include "key_signature.h"

void main_menu(uint32_t *bpm, midi_instrument_t *playback_instrument, 
	time_signature_t *time_signature , key_signature_t *key_signature, char *title); 
