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
elif (os.path.exists("/dev/tty.usbmodem1442")): 
    ser = serial.Serial(port='/dev/tty.usbmodem1442', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)
elif (os.path.exists("/dev/tty.usbserial-A904RDA3")): 
    ser = serial.Serial(port='/dev/tty.usbserial-A904RDA3', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)
else: 
    print("No board connected")
    exit() 
    
    
try:    
    ser.isOpen()
    print("Serial port is open")
except: 
    print("Error")
    exit() 
    
note_on = 144 
note_off = 128 
volume = 176 
dry_ch = 7
harmony_ch = 84 
master_ch = 5 
reverb_vol_ch = 72
chorus_vol_ch = 73
chorus_speed_ch = 93 
delay_feedback_ch = 91
delay_speed_ch = 71
delay_vol_ch = 74
autotune_button = 192 
pitch_bend = 224 

try:
    with mido.open_input('Oxygen 49') as port:
        print('Using {}'.format(port))
        print('Waiting for messages...')
        for message in port:
            
            ser.write(message.bytes())
            print(message.bytes())

            sys.stdout.flush()
except KeyboardInterrupt:
    pass
