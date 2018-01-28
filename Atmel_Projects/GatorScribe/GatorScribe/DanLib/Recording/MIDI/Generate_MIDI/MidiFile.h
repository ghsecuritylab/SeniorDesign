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
#include "audio_to_midi.h"

#define TICKS_PER_QUARTER_NOTE 120
#define CLOCKS_PER_CLICK 24
#define NUM_32NDS_PER_QUARTER 8

void write_midi_file(uint32_t bpm, midi_instrument_t playback_instrument, time_signature_t *time_signature , key_signature_t *key_signature, char *title, midi_event_t *events, uint32_t number_of_events); 

#endif /* MidiFile_h */
