#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Jan 18 23:37:59 2018

@author: danielgonzalez
"""

import serial 
import numpy as np 

#ser = serial.Serial(port='/dev/tty.usbmodem1462', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)
ser = serial.Serial(port='/dev/tty.usbserial-A904RDA3', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)

try:    
    ser.isOpen()
    print("Serial port is open")
except: 
    print("Error")
    exit() 

midi_msg = "00"
if(ser.isOpen()): 
    while(1): 
        msg = ser.read().decode('ascii')
        if (msg == '-'): 
            msg = ser.read().decode('ascii')
            x = np.array([], dtype='uint8')
            i = 0 
            while(msg != '-'): 
                if (i == 1): 
                    midi_msg = midi_msg + msg
                    x = np.append(x, np.array([int(midi_msg,16)], dtype="uint8"))
                    i = 0
                else:  
                    midi_msg = msg
                    i = i + 1

                msg = ser.read().decode('ascii')
                
            x.tofile('my_song.mid') # simply name of file 
            print("Created MIDI File")
        #else: 
         #   print(msg)
else: 
    print("Cannot open serial port")
            