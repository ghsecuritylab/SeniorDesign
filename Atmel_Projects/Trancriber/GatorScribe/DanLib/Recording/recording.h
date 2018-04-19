/*
 * recording.h
 *
 * Created: 1/10/2018 9:34:52 AM
 *  Author: Daniel Gonzalez
 */ 


#ifndef RECORDING_H_
#define RECORDING_H_

#include "DanLib.h"
#include "pitchyinfast.h"

#define MAX_NUM_EVENTS 10000

void start_recording(midi_event_t *events, uint32_t *number_of_events, uint32_t bpm, midi_instrument_t playback_instrument, time_signature_t time_signature , key_signature_t key_signature, char *title); 

#endif /* RECORDING_H_ */