#!/usr/bin/env python
"""
Receive messages from the input port and print them out.
"""
from __future__ import print_function
import sys
import mido
import serial 
import os 

notes = list([ ])

if (os.path.exists("/dev/tty.usbmodem1462")):
    ser = serial.Serial(port='/dev/tty.usbmodem1462', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)
else: 
    ser = serial.Serial(port='/dev/tty.usbmodem1442', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)

try:    
    ser.isOpen()
    print("Serial port is open")
except: 
    print("Error")
    exit() 
    
note_on = 144 
note_off = 128 
volume = 176 
harmony_ch = 77 

try:
    with mido.open_input('Oxygen 49') as port:
        print('Using {}'.format(port))
        print('Waiting for messages...')
        for message in port:
            if (message.bytes()[0] == 144): 
                # add note to array to send 
                notes.append(message.bytes()[1])
                for k in range(len(notes)):
                    ser.write([notes[k]])
                ser.write([0]) # null terminated 
                
            elif (message.bytes()[0] == 128): 
                # remove note from array to send 
                notes.remove(message.bytes()[1])
                for k in range(len(notes)):
                    ser.write([notes[k]])
                ser.write([0]) # null terminated 
            elif (message.bytes()[0] == volume and message.bytes()[1] == harmony_ch):
                ser.write([255])
                ser.write([message.bytes()[2]])
                

            
           
            
            #print(notes)
            print(message.bytes())

            sys.stdout.flush()
except KeyboardInterrupt:
    pass
