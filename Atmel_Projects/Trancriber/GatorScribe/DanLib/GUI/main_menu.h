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

#define NUMBER_OF_MAIN_MENU_BUTTONS 6

#define TITLE_TEXT_Y 58
#define TITLE_TEXT_X 85
#define BPM_TEXT_X	134
#define BPM_TEXT_Y	242
#define INSTRUMENT_TEXT_X 341
#define INSTRUMENT_TEXT_Y BPM_TEXT_Y
#define TIME_SIG_TEXT_X BPM_TEXT_X
#define TIME_SIG_TEXT_Y 151
#define KEY_SIG_TEXT_X INSTRUMENT_TEXT_X
#define KEY_SIG_TEXT_Y TIME_SIG_TEXT_Y


void main_menu(uint32_t *bpm, midi_instrument_t *playback_instrument, 
	time_signature_t *time_signature , key_signature_t *key_signature, char *title); 
