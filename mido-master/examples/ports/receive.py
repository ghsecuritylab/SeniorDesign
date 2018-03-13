#!/usr/bin/env python
"""
Receive messages from the input port and print them out.
"""
from __future__ import print_function
import sys
import mido
import serial 

notes = list([ ])

ser = serial.Serial(port='/dev/tty.usbmodem1462', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)

try:    
    ser.isOpen()
    print("Serial port is open")
except: 
    print("Error")
    exit() 

try:
    with mido.open_input('Oxygen 49') as port:
        print('Using {}'.format(port))
        print('Waiting for messages...')
        for message in port:
            if (message.bytes()[0] == 144): 
                # add note to array to send 
                notes.append(message.bytes()[1])
                
            elif (message.bytes()[0] == 128): 
                # remove note from array to send 
                notes.remove(message.bytes()[1])
            
            #msg = ser.read().decode('ascii')

            
            for k in range(len(notes)):
                #result = str(notes[k])
                #ser.write(str.encode(result) - 0x30)
                temp = notes[k]
                ser.write([temp])
            
                
          
            ser.write([0])
            
            print(notes)

            sys.stdout.flush()
except KeyboardInterrupt:
    pass
