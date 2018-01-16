//
//  MidiFile.h
//  my_midi_lib_in_c
//
//  Created by Daniel Gonzalez on 1/15/18.
//  Copyright © 2018 Daniel Gonzalez. All rights reserved.
//

#ifndef MidiFile_h
#define MidiFile_h

#include <stdio.h>
#include "playback_instrument.h"
#include "key_signature.h"
#include "time_signature.h"

#define TICKS_PER_QUARTER_NOTE 120

typedef struct midi_event
{
    int16_t note_number;
    int16_t velocity;
    //float rhythm;
//    int tick_on;
//    int tick_off;
    int rhythm; 
}midi_event_t;

typedef struct MidiFile
{
    char * midi_file_data;
    char track_name[128];
    
    
}MidiFile_t;

void write_midi_file(uint32_t bpm, midi_instrument_t playback_instrument, time_signature_t *time_signature , key_signature_t *key_signature, char *title, midi_event_t *events, uint32_t number_of_events); 

#endif /* MidiFile_h */
