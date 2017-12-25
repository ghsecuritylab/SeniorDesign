//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Tue Feb 16 20:00:41 PST 2016
// Last Modified: Tue Feb 16 20:16:17 PST 2016
// Filename:      midifile/src-programs/createmidifile2.cpp
// Syntax:        C++11
//
// Description:   Demonstration of how to create a Multi-track MIDI file
//                with convenience functions.
//

#include "MidiFile.h"
#include <iostream>

using namespace std;

/* Time signature */
#define TOP 4
#define BOTTOM 4
#define MAJOR 0
#define MINOR 1

typedef enum {
    Cmajor = 0,
    Gmajor = 1,
    Dmajor = 2,
    Amajor = 3,
    Emajor = 4,
    Bmajor = 5,
    Fsmajor = 6,
    Csmajor = 7,
    Fmajor = -1,
    Bbmajor = -2,
    Ebmajor = -3,
    Abmajor = -4,
    Dbmajor = -5,
    Gbmajor = -6,
    Cbmajor = -7
} majorKeys_t;

typedef enum {
    Aminor = 0,
    Eminor = 1,
    Bminor = 2,
    Fsminor = 3,
    Csminor = 4,
    Gsminor = 5,
    Dsminor = 6,
    Asminor = 7,
    Dminor = -1,
    Gminor = -2,
    Cminor = -3,
    Fminor = -4,
    Bbminor = -5,
    Ebminor = -6,
    Abminor = -7
} minorKeys_t;

int main(int argc, char** argv) {
    MidiFile midifile;
    midifile.addTracks(1);    // Add another two tracks to the MIDI file
    int tpq = 120;            // ticks per quarter note
    midifile.setTicksPerQuarterNote(tpq);
    
    // melody to write to MIDI track 1: (60 = middle C)
    // C5   C  G  G  A  A  G- F  F  E  E  D  D  C-
    int melody[50]  = {72,72,79,79,81,81,79,77,77,76,76,74,74,72,-1};
    int mrhythm[50] = { 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2,-1};
    
    int actiontick = 0;
    int track      = 0;
    
    // Add some expression track (track 0) messages:
    midifile.addTrackName(track, actiontick, "Twinkle, Twinkle Little Star");
    midifile.addTempo(track, actiontick, 104.0);
    midifile.addTimeSignature(track, actiontick, TOP, BOTTOM, 24, 8);
    midifile.addKeySignature(track, actiontick, Cmajor, MAJOR);
    
    // Store melody line in track 1
    int i = 0;
    int velocity   = 64;
    int channel    = 0;
    track          = 1;
    
    midifile.addTrackName(track, actiontick, "Melody");
    midifile.addPatchChange(track, actiontick, channel, 40); // 40=violin
    
    while (melody[i] >= 0) {
        midifile.addNoteOn(track, actiontick, channel, melody[i], velocity);
        actiontick += tpq * mrhythm[i];
        midifile.addNoteOff(track, actiontick, channel, melody[i], velocity);
        i++;
    }
    
    midifile.sortTracks();         // ensure tick times are in correct order
    midifile.write("Twinkle, Twinkle Little Star.mid"); // write Standard MIDI File twinkle.mid
    return 0;
}


