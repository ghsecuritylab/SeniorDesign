Daniel Gonzalez and Tyler Tucker Senior Design

The goal:
Create a (semi) real-time music transcriber for a piano and hopefully a guitar. We will be using machine learning (specifically a hiearchichal Kalman filter) to detect monophomic notes and create a midi file. We will extract four characteristics: duration, start time, pitch, and velocity of the note being played.

The MIDI file will then be sent to a laptop over USB or UART, and translated to sheet music using 3rd party software. We can also playback what the system learned with any 3rd party MIDI player.

Finally, we will have an LCD that will allow the user to set the song name, time signature, BPM, and key of the song. The user will then play along to a metronome that is outputted from the processor, as well as sound in/sound out from the instrument.

We'll only be able to record a finite length of a recording, for this reason, we'll likely need to incorpoarte an SDRAM to hold the MIDI information.

That's all for now, folks. Stay tuned. 

