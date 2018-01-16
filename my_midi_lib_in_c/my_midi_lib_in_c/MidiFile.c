//
//  MidiFile.c
//  my_midi_lib_in_c
//
//  Created by Daniel Gonzalez on 1/15/18.
//  Copyright © 2018 Daniel Gonzalez. All rights reserved.
//

#include "MidiFile.h"

#define CLOCKS_PER_CLICK 24
#define NUM_32NDS_PER_QUARTER 8

static unsigned char out[10000];
static int outIdx = 0;

static inline void write_out(char val)
{
    out[outIdx++] = val;
}

static inline void writeBigEndianULong(uint32_t value)
{
    union { char bytes[4]; uint32_t l; } data;
    data.l = value;
    write_out(data.bytes[3]);
    write_out(data.bytes[2]);
    write_out(data.bytes[1]);
    write_out(data.bytes[0]);
}

static inline void writeBigEndianUShort(uint16_t value)
{
    union { char bytes[2]; ushort us; } data;
    data.us = value;
    write_out(data.bytes[1]);
    write_out(data.bytes[0]);
}

static inline void writeVLValue(uint32_t value)
{
    unsigned char bytes[4];
    // most significant 7 bits
    bytes[0] = (unsigned char)((value >> 21) & 0x7f);
    bytes[1] = (unsigned char)((value >> 14) & 0x7f);
    bytes[2] = (unsigned char)((value >> 7)  & 0x7f);
    // least significant 7 bits
    bytes[3] = (unsigned char)(value & 0x7f);
    
    int start = 0;
    while ((start<4) && (bytes[start] == 0))  start++;
    int i;
    for (i=start; i<3; i++) {
        bytes[i] = bytes[i] | 0x80;
        write_out(bytes[i]);
    }
    write_out(bytes[3]);
}

void write_midi_file(uint32_t bpm, midi_instrument_t playback_instrument, time_signature_t *time_signature , key_signature_t *key_signature, char *title, midi_event_t *events, uint32_t number_of_events)
{
    char ch;
    int i;
    uint32_t longdata;
    char endoftrack[4] = {0, 0xff, 0x2f, 0x00};
    outIdx = 0;
    // 1. The characters "MThd"
    ch = 'M'; write_out(ch);
    ch = 'T'; write_out(ch);
    ch = 'h'; write_out(ch);
    ch = 'd'; write_out(ch);
    
    // 2. write the size of the header (always a "6" stored in unsigned long
    longdata = 6;
    writeBigEndianULong(longdata);
    
    // 3. MIDI file format, type 0, 1, or 2
    writeBigEndianUShort(0x0001);
    
    // 4. write out the number of tracks.
    writeBigEndianUShort(0x0002);
    
    // 5. write out the number of ticks per quarternote.
    writeBigEndianUShort(TICKS_PER_QUARTER_NOTE);
    
    // 6. Write header track
    ch = 'M'; write_out(ch);
    ch = 'T'; write_out(ch);
    ch = 'r'; write_out(ch);
    ch = 'k'; write_out(ch);
    
    // size of track
    uint8_t size_of_title = 0;
    while(title[size_of_title++]); // size of title
    longdata = 29 + size_of_title - 1;
    writeBigEndianULong(longdata);
    
    // track name (title)
    write_out(0x00); // tick
    write_out(0xFF);
    write_out(0x03);
    write_out(size_of_title-1);
    for (i = 0; i < size_of_title-1; i++)
        write_out(title[i]);
    
    // tempo
    int microseconds = (int)(60.0 / bpm * 1000000.0 + 0.5);
    write_out(0x00); // tick
    write_out(0xFF);
    write_out(0x51);
    write_out(0x03);
    write_out((microseconds >> 16) & 0xff);
    write_out((microseconds >> 8) & 0xff);
    write_out((microseconds >> 0) & 0xff);

    // time signature
    int base2 = 0;
    while( time_signature->bottom >>=1) base2++;
    write_out(0x00); // tick
    write_out(0xFF);
    write_out(0x58);
    write_out(0x04);
    write_out(0xff & time_signature->top);
    write_out(0xff & base2);
    write_out(0xff & CLOCKS_PER_CLICK);
    write_out(0xff & NUM_32NDS_PER_QUARTER);
    
    // key signature
    write_out(0x00); // tick
    write_out(0xff);
    write_out(0x59);
    write_out(0x02);
    write_out(0xff & key_signature->key);
    write_out(0xff & key_signature->mode);
    
    // end of header track
    for (i = 0; i < 4; i++)
        write_out(endoftrack[i]);
    
    
    
    // 7. Write melody track
    ch = 'M'; write_out(ch);
    ch = 'T'; write_out(ch);
    ch = 'r'; write_out(ch);
    ch = 'k'; write_out(ch);
    
    // size of track name
    char track_name[] = "Melody";
    uint8_t size_of_track_name = 0;
    while(track_name[size_of_track_name++]); // size of track name
    
    // size of melody
    longdata = 11 + number_of_events*8 + size_of_track_name - 1;
    writeBigEndianULong(longdata);
    
    // track name
    write_out(0x00); // tick
    write_out(0xFF);
    write_out(0x03);
    write_out(size_of_track_name-1);
    for (i = 0; i < size_of_track_name-1; i++)
        write_out(track_name[i]);
    
    // patch name
    uint8_t channel = 0;
    write_out(0x00); // tick
    write_out(0xc0 | (0x0f & channel));
    write_out(0x7f & playback_instrument);
    
    // write out track data
    int actiontick = 0;
    for (i = 0; i < number_of_events; i++)
    {
        actiontick = 0;
        // note on
        writeVLValue(actiontick);
        write_out(0x90 | (0x0f & channel));
        write_out(events[i].note_number & 0x7f);
        write_out(events[i].velocity & 0x7f);
        
        actiontick += 120 * events[i].rhythm;
        
        // note off
        writeVLValue(actiontick);
        write_out(0x80 | (0x0f & channel));
        write_out(events[i].note_number & 0x7f);
        write_out(events[i].velocity & 0x7f);
    }
    
    // end of header track
    for (i = 0; i < 4; i++)
        write_out(endoftrack[i]);
    
    for (i = 0; i < outIdx; i++)
    {
        printf("%02x", out[i]);
        if ((i+1) % 16 == 0)
            printf("\n");
        else if ((i+1) % 2 == 0)
            printf(" ");
    }

}
