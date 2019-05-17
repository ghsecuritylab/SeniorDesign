# Spring 2018 ECE Senior Design
### Danny Gonzalez and Tyler Tucker
[![ECE](http://dwoodard.ece.ufl.edu/images/ece2.png)](https://www.ece.ufl.edu/)

## Gatorscribe
We created a monophonic pitch detection system that records a sample directly from an instrument such as a guitar and converts it to MIDI format. Our embedded ARM software takes into account four characteristics for each note: duration, start time, pitch, and velocity. Before recording, the artist provides a song name, a time signature, a set BPM (beats per minute), and a playback instrument on our desktop Qt GUI. All of these settings are reflected in the final MIDI file, which can be used with any popular MIDI computer program.

## Vocal Harmonizer
After producing a working demonstration of *Gatorscribe*, we created a real-time vocal harmonizer that takes input from a singer through a microphone and layers the output with several pitch-shifted versions of their voice. The user has a desktop GUI interface that allows them to choose what notes in a set musical key they would like to shift their voice to. Also, they can use a MIDI-compliant musical device such as a keyboard to play notes that our embedded software will interpret as desired pitch shifts for their voice. The final effect sounds like a choir of the same singer harmonizing simultaneously.
<br/>
<br/>
<br/>
<br/>
![Gatorscribe](https://raw.githubusercontent.com/Danngonz3/SeniorDesign/master/GUIs/Gatorscribe.png)
*Gatorscribe User Interface*

## Tech
We used the following technologies in some capacity during our development.

| Resource | Category |
| ------ | ------ |
| [ARM](https://www.arm.com/products/silicon-ip-cpu) | Microprocessor used for embedded code |
| [Altium](https://www.altium.com/) | PCB (Printed Circuit Board) Design Software |
| [Qt Library](https://www.qt.io/) | Desktop Python GUIs |
| [MIDI](https://en.wikipedia.org/wiki/MIDI) | Digital music file structure |
