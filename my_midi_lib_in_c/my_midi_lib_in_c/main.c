//
//  main.c
//  my_midi_lib_in_c
//
//  Created by Daniel Gonzalez on 1/15/18.
//  Copyright © 2018 Daniel Gonzalez. All rights reserved.
//

#include <stdio.h>
#include "MidiFile.h"

int main(int argc, const char * argv[]) {
    
    uint32_t bpm = 104;
    midi_instrument_t playback_instrument = VIOLIN;
    time_signature_t time_signature = {4,4, FOUR_FOUR};
    key_signature_t key_signature = {C_MAJOR, MAJOR};
    char title[128] = "Twinkle, Twinkle Little Star";
    midi_event_t events[100];
    uint32_t number_of_events = 14;

    
    events[0].note_number = 72;
    events[0].rhythm = 1;
    events[0].velocity = 64;
    
    events[1].note_number = 72;
    events[1].rhythm = 1;
    events[1].velocity = 64;
    
    events[2].note_number = 79;
    events[2].rhythm = 1;
    events[2].velocity = 64;
    
    events[3].note_number = 79;
    events[3].rhythm = 1;
    events[3].velocity = 64;
    
    events[4].note_number = 81;
    events[4].rhythm = 1;
    events[4].velocity = 64;
    
    events[5].note_number = 81;
    events[5].rhythm = 1;
    events[5].velocity = 64;
    
    events[6].note_number = 79;
    events[6].rhythm = 2;
    events[6].velocity = 64;
    
    events[7].note_number = 77;
    events[7].rhythm = 1;
    events[7].velocity = 64;
    
    events[8].note_number = 77;
    events[8].rhythm = 1;
    events[8].velocity = 64;
    
    events[9].note_number = -1;
    events[9].rhythm = 2;
    events[9].velocity = 64;
    
    events[10].note_number = 76;
    events[10].rhythm = 1;
    events[10].velocity = 64;
    
    events[11].note_number = 74;
    events[11].rhythm = 1;
    events[11].velocity = 64;
    
    events[12].note_number = 74;
    events[12].rhythm = 1;
    events[12].velocity = 64;
    
    events[13].note_number = 72;
    events[13].rhythm = 2;
    events[13].velocity = 64;
    
    write_midi_file(bpm, playback_instrument, &time_signature, &key_signature, title, events, number_of_events);
    
    return 0;
}
