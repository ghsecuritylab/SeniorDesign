#!/usr/bin/env python
"""
Receive messages from the input port and print them out.
"""
from __future__ import print_function
import mido
import serial 
import os 





if (os.path.exists("/dev/tty.usbmodem1462")):
    ser = serial.Serial(port='/dev/tty.usbmodem1462', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=0.001)
elif (os.path.exists("/dev/tty.usbmodem1442")): 
    ser = serial.Serial(port='/dev/tty.usbmodem1442', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=0.001)
elif (os.path.exists("/dev/tty.usbserial-A904RDA3")): 
    ser = serial.Serial(port='/dev/tty.usbserial-A904RDA3', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=0.001)
else: 
    print("No board connected")
    exit() 

try:    
    ser.isOpen()
    print("Serial port is open")
except: 
    print("Error")
    exit() 

class MIDI: 
    def __init__(self):
        self.chord_harmonies = [False,False,False,False,False,False,False,False,False]
    
    def message_callback(self, message): 
        if (message.bytes()[0] == 192): 
            self.chord_harmonies[message.bytes()[1]] ^= True 
            #print(chord_harmonies)
        
        ser.write(message.bytes())
        print(message.bytes())

port = mido.open_input(mido.get_output_names()[0])
print('Using {}'.format(port))
print('Waiting for messages...')

midi = MIDI()
port.callback = midi.message_callback

while(True):   

    read_byte = ser.read(1).decode('ISO-8859-1')
    if (read_byte == 0):
        for i in range (0,9): 
            midi.chord_harmonies[i] = False 

