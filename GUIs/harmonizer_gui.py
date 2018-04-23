# Add key signature list
# Seperate GUIs
#
import sys
import mido
import serial
import asyncio
import concurrent
import os
import numpy as np
from PyQt5.QtWidgets import QMainWindow,QApplication,QPushButton,QWidget,QAction,QTabWidget,QLayout,QVBoxLayout,QHBoxLayout,QLabel,QComboBox,QSlider,QTextEdit,QLineEdit,QGridLayout,QDesktopWidget,QSizePolicy,QGraphicsOpacityEffect
from PyQt5.QtGui import QIcon,QColor,QPalette,QFont,QPixmap,QPainter,QPen,QBrush,QLinearGradient
from PyQt5.QtCore import pyqtSlot,Qt,QSize,QRect,QTimer
from PyQt5.QtTest import QTest

class App(QMainWindow):

    def __init__(self):
        super().__init__()
        self.title = 'Scribes'
        self.setWindowTitle(self.title)
        self.p = self.palette()
        self.setStyleSheet('background-color: rgb(42,41,46); color: rgb(200,209,218)')
        self.central_widget = Central(self)
        self.setCentralWidget(self.central_widget)
        self.resize(1920,400)  # resolution for mac is 1920 x 1200
        self.central_widget.open_midi_port() 
        self.show()

class Central(QWidget):

    def __init__(self, parent):
        super().__init__(parent)
        self.layout = QVBoxLayout(self)
        self.window_width = QDesktopWidget().screenGeometry().width()
        self.window_height = QDesktopWidget().screenGeometry().height()
        self.remove_background = 'background-color: rgb(0,0,0,0); border-width: 0px; border-radius: 0px;'
        self.format_button = 'background-color: rgb(247,247,247);'
        self.p = self.palette()
        self.p.setColor(self.backgroundRole(), QColor(127,255,127))
        self.purple = QColor(93,107,68)
        self.blue = QColor(50,209,255)
        self.green = QColor(104,216,196)
        self.orange = QColor(251,165,125)
        self.magenta = QColor(157,39,89)
        self.yellow = QColor(177,176,148)
        self.base_key = 60
        self.starting_key_column = 4
        self.major_pos = [0,3]
        self.minor_pos = [1,3]
        self.chord_pos = [4,0]
        self.prev_midi_status = -1 
        self.chord_harmonies = [False,False,False,False,False,False,False,False,False]
        self.ser = serial.Serial()
        self.midi_port_open = 0; 
        self.midi_device_buttons = [QPushButton(" "), QPushButton(" "), QPushButton(" "), QPushButton(" ")]
        self.connectBoard()
        self.init_harmonizer()
        self.sendResetToBoard()
        self.layout.setSpacing(self.window_height/20)
        self.layout.addLayout(self.harmonizer_layout)
        self.layout.addStretch()
        self.layout.setContentsMargins(self.window_width/25,self.window_width/50,self.window_height/50,self.window_height/50)
        self.setLayout(self.layout)
        self.opacity = 0.9
        self.timer = QTimer()   
        self.timer.setInterval(500)
        self.timer.timeout.connect(self.board_reset_check)  
        self.timer.start()  

    def board_reset_check(self):
        if self.ser.isOpen():
            try: 
                serial_msg = self.ser.read() #.decode('ascii')
                s = list(serial_msg)
                if (len(s) > 0):
                    if (s[0] < 128):
                        status = serial_msg.decode('ascii')
                        if (status == "0"):
                            self.restartGUI()
            except: 
                self.ser.close() 

    def lambdaChooseKey(self, m):
        return lambda: self.chooseKey(m) 

    def lambdaChangeChordHarmonies(self, m):
        return lambda: self.changeChordHarmonies(m) 
 
    def init_harmonizer(self):
        self.gray_text = 'color: rgb(200,209,218)'

        # Create layout to encapsulate harmonizer portion
        self.harmonizer_layout = QGridLayout()

        # Create grid layout for user harmonizer control
        self.harmonizer_layout.setSpacing(10)

        # Look for devices and create label to display current device
        self.device_list = mido.get_input_names()
        self.dev_lbl = QLabel()
        self.dev_lbl.setStyleSheet(self.remove_background + "; color: rgb(59,202,243,244)")
        if self.device_list:
            self.dev_lbl.setText(mido.get_input_names()[0])  # Set label to first device in list
        else:
            self.dev_lbl.setText('No Device')  # Set default label if not device is found
        self.dev_lbl.setFont(QFont('Calibri Light',64))  # Edit label font
        self.dev_lbl.setMaximumSize(self.dev_lbl.minimumSizeHint().width()*1.2,self.dev_lbl.minimumSizeHint().height()*1.3)
        self.harmonizer_layout.addWidget(self.dev_lbl,0,0,1,5,Qt.AlignLeft)

        # Create labels for all keys 
        self.key_array = ['C','C#/Db','D','D#/Eb','E','F','F#/Gb','G','G#/Ab','A','A#/Bb','B']
        self.key_buttons = [QPushButton(" ")] * 12
        row = 0
        column = self.starting_key_column
        i = 0
        for key in self.key_array:
            self.key_buttons[i] = QPushButton(key)
            self.key_buttons[i].setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")
            self.key_buttons[i].setFont(QFont('Calibri Light',46))
            self.key_buttons[i].setMaximumSize(self.key_buttons[i].minimumSizeHint().width()*1.2,self.key_buttons[i].minimumSizeHint().height())
            self.key_buttons[i].clicked.connect(self.lambdaChooseKey(i))
            self.harmonizer_layout.addWidget(self.key_buttons[i],row,column,Qt.AlignCenter)
            i = i + 1
            if column < self.starting_key_column+3:
                column = column + 1
            else:
                column = self.starting_key_column
                row = row + 1

        # Major key 
        self.major_button = QPushButton('Major')
        self.major_button.setStyleSheet(self.remove_background + "; color: rgb(59,202,243,244)")
        self.major_button.setFont(QFont('Calibri Light',46))
        self.major_button.setMaximumSize(self.major_button.minimumSizeHint().width()*1.2,self.major_button.minimumSizeHint().height())
        self.major_button.clicked.connect(lambda: self.chooseKeyMode("Major"))
        self.harmonizer_layout.addWidget(self.major_button,self.major_pos[0],self.major_pos[1],Qt.AlignCenter)

        # Minor Key 
        self.minor_button = QPushButton('Minor')
        self.minor_button.setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")
        self.minor_button.setFont(QFont('Calibri Light',46))
        self.minor_button.setMaximumSize(self.minor_button.minimumSizeHint().width()*1.2,self.minor_button.minimumSizeHint().height())
        self.minor_button.clicked.connect(lambda: self.chooseKeyMode("Minor"))
        self.harmonizer_layout.addWidget(self.minor_button,self.minor_pos[0],self.minor_pos[1],Qt.AlignCenter)

         # Show default key signature
        self.current_key_button = self.key_buttons[4]
        self.setOriginalKey()

        button = QLabel('Harmonies')
        button.setStyleSheet(self.remove_background + "; color: rgb(59,202,243,244)")
        button.setFont(QFont('Calibri Light',46))
        button.setMaximumSize(button.minimumSizeHint().width(),button.minimumSizeHint().height())
        self.harmonizer_layout.addWidget(button,self.chord_pos[0]-1,0,Qt.AlignLeft | Qt.AlignTop)

        button = QLabel(' ')
        button.setStyleSheet(self.remove_background + "; color: rgb(200,209,218,244)")
        button.setFont(QFont('Calibri Light',46))
        button.setMaximumSize(button.minimumSizeHint().width()*1.2,button.minimumSizeHint().height())
        self.harmonizer_layout.addWidget(button,self.chord_pos[0]-1,0,Qt.AlignCenter)

        #Show 9 harmony indicators 
        self.harmony_array = ['Octave Below   ', '3rd Below  ', '5th Below   ', '6th Below   ', '3rd Above   ', '5th Above   ', '6th Above   ', 'Octave Above   ', 'Autotune   ']
        self.harmony_buttons = [QPushButton(" ")] * 9
        row = self.chord_pos[0]
        column = self.chord_pos[1]
        i = 0
        for harmony in self.harmony_array:
            self.harmony_buttons[i] = QPushButton(harmony)
            self.harmony_buttons[i].setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")
            self.harmony_buttons[i].setFont(QFont('Calibri Light',36))
            self.harmony_buttons[i].setMaximumSize(self.harmony_buttons[i].minimumSizeHint().width(),self.harmony_buttons[i].minimumSizeHint().height())
            self.harmony_buttons[i].clicked.connect(self.lambdaChangeChordHarmonies(i))
            self.harmonizer_layout.addWidget(self.harmony_buttons[i],row,column,Qt.AlignCenter)
            column = column + 1 
            i = i + 1

        # Create search button 
        self.search_button = QPushButton('Search')
        self.search_button.setFont(QFont('Calibri',14))
        self.search_button.setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent')
        self.search_button.setMinimumSize(self.search_button.minimumSizeHint().width()*2,self.search_button.minimumSizeHint().height())
        self.search_button.pressed.connect(lambda: self.search_button.setStyleSheet('background-color: rgb(192,192,192,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
        self.search_button.released.connect(lambda: self.search_button.setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
        self.search_button.clicked.connect(self.searchMidi)
        self.harmonizer_layout.addWidget(self.search_button,1,0,Qt.AlignLeft)

        # Create restart button 
        self.restart_button = QPushButton('Restart')
        self.restart_button.setFont(QFont('Calibri',14))
        self.restart_button.setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent')
        self.restart_button.setMinimumSize(self.restart_button.minimumSizeHint().width()*2,self.restart_button.minimumSizeHint().height())
        self.restart_button.pressed.connect(lambda: self.restart_button.setStyleSheet('background-color: rgb(192,192,192,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
        self.restart_button.released.connect(lambda: self.restart_button.setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
        self.restart_button.clicked.connect(self.restartGUI)
        self.harmonizer_layout.addWidget(self.restart_button,1,1,Qt.AlignLeft)

    def searchMidi(self): 
        if self.midi_port_open:
            self.midi_port.close() 
            self.midi_port_open = False
        midi_device = mido.get_input_names()
        for i in range(len(midi_device)): 
            self.midi_device_buttons[i] = QPushButton(midi_device[i])
            self.midi_device_buttons[i].setFont(QFont('Calibri',14))
            self.midi_device_buttons[i].setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent')
            self.midi_device_buttons[i].setMinimumSize(self.midi_device_buttons[i].minimumSizeHint().width()*2,self.midi_device_buttons[i].minimumSizeHint().height())
            self.midi_device_buttons[i].pressed.connect(lambda: self.midi_device_buttons[i].setStyleSheet('background-color: rgb(192,192,192,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
            self.midi_device_buttons[i].released.connect(lambda: self.midi_device_buttons[i].setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
            self.midi_device_buttons[i].clicked.connect(lambda: self.midi_device_clicked(self.midi_device_buttons[i])) #self.open_midi_port(midi_device[i]); search_button.deleteLater)
            self.harmonizer_layout.addWidget(self.midi_device_buttons[i],i,2,Qt.AlignLeft)

    def midi_device_clicked(self, midi_button): 
        self.open_midi_port(midi_button.text()) 
        for i in range(len(self.midi_device_buttons)): 
            self.midi_device_buttons[0].deleteLater()  

    # callback for restart button 
    def restartGUI(self): 
        self.connectBoard() 

        self.resetKeyAndHarmonies() 

        if self.midi_port_open:
            self.midi_port.close() 
            self.midi_port_open = False
        self.open_midi_port() 

    def sendResetToBoard(self): 
        if self.ser.isOpen():
            self.ser.write([255, 255, 255])

    def setOriginalKey(self): 
        old_key_button = self.current_key_button
        old_key_button.setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")
        self.current_key_midi_num = 64 # E 
        self.current_key_mode = "Major"
        self.current_key_button = self.key_buttons[4]
        self.current_key_button.setStyleSheet(self.remove_background + "; color: rgb(59,202,243,244)")

        # Dim minor 
        self.minor_button.setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")

        # Highlight major 
        self.major_button.setStyleSheet(self.remove_background + "; color: rgb(59,202,243,244)")


    def resetKeyAndHarmonies(self): 
        self.setOriginalKey() 
        
        for i in range(0,9): 
            self.chord_harmonies[i] = False 

        row = self.chord_pos[0]
        column = self.chord_pos[1]
        for i in range(len(self.harmony_buttons)): #.harmony_array:
            harmony_button = self.harmony_buttons[i] #self.harmonizer_layout.itemAtPosition(row,column).widget()
            harmony_button.setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")
            column = column + 1 

        self.sendResetToBoard()
        
    def chooseKeyMode(self, mode): 
        if self.ser.isOpen():
            button = self.sender()
            button.setStyleSheet(self.remove_background + "; color: rgb(59,202,243,244)")
            if(mode == "Major"):
                if(self.current_key_mode != "Major"):
                    # dim minor button 
                    self.minor_button.setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")
                    self.current_key_mode = "Major"
                    self.current_key_midi_num -= 3
                    self.sendKeyChange(self.current_key_midi_num) 
            elif(mode == "Minor"):
                if (self.current_key_mode != "Minor"):
                    # dim major button 
                    self.major_button.setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")
                    self.current_key_mode = "Minor"
                    self.current_key_midi_num += 3
                    self.sendKeyChange(self.current_key_midi_num)

    def chooseKey(self, key_offset):
        if self.ser.isOpen():
            old_key_button = self.current_key_button
            old_key_button.setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")
            self.current_key_button = self.sender()
            self.current_key_button.setStyleSheet(self.remove_background + "; color: rgb(59,202,243,244)")
            self.current_key_midi_num = self.base_key + key_offset
            if(self.current_key_mode == "Minor"): 
                self.current_key_midi_num +=3
            self.sendKeyChange(self.current_key_midi_num)

    def sendKeyChange(self, midi_key_num):
        if self.ser.isOpen():
            self.ser.write([255, 255, midi_key_num])
            print(midi_key_num)
        else: 
            print('Cannot send key')

    def changeChordHarmonies(self, chord_idx): 
        if self.ser.isOpen():   
            self.chord_harmonies[chord_idx] ^= True 
            if (self.chord_harmonies[chord_idx]):
                self.harmony_buttons[chord_idx].setStyleSheet(self.remove_background + "; color: rgb(59,202,243,244)")
            else: 
                self.harmony_buttons[chord_idx].setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")
            self.ser.write([176, 32, 0]) # tells board that it's a button from one of the channels (MIDI keyboard uses + and - buttons sometimes and sends same message)
            self.ser.write([192, chord_idx])

    def connectBoard(self):
        if self.ser.isOpen(): 
            self.ser.close() 
        if (os.path.exists("/dev/tty.usbmodem1462")):
            self.ser = serial.Serial(port='/dev/tty.usbmodem1462', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=0.001)
        elif (os.path.exists("/dev/tty.usbmodem1442")):
            self.ser = serial.Serial(port='/dev/tty.usbmodem1442', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=0.001)
        elif (os.path.exists("/dev/tty.usbserial-A904RDA2")):
            self.ser = serial.Serial(port='/dev/tty.usbserial-A904RDA2', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=0.001)
        #elif (os.path.exists("/dev/tty.usbserial-A904RDA3")):
        #    self.ser = serial.Serial(port='/dev/tty.usbserial-A904RDA3', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=0.001)
        else:
            self.ser = serial.Serial(baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=0.001)
            print("Error, no board connected")
            return
        try:
            if(self.ser.isOpen()): 
                print("Serial port is open")
            else:
                print("Error, no board connected")
        except:
            print("Error, no board connected")
            return

    def message_callback(self, message): 
        if (message.bytes()[0] == 192 and self.prev_midi_status == 176): 
            chord_idx = message.bytes()[1]
            if (chord_idx < 9):
                self.chord_harmonies[chord_idx] ^= True 
                harmony = self.harmonizer_layout.itemAtPosition(self.chord_pos[0],chord_idx).widget()
                if (self.chord_harmonies[chord_idx]):
                    harmony.setStyleSheet(self.remove_background + "; color: rgb(59,202,243,244)")
                else: 
                    harmony.setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")
            
        self.prev_midi_status = message.bytes()[0]
        if self.ser.isOpen():   
            self.ser.write(message.bytes())
        print(message.bytes())

    def open_midi_port(self, device=None):
        if(device == None): 
            midi_device = mido.get_input_names()
            if (midi_device):
                device = midi_device[0]
        
        if (device):
            self.midi_port = mido.open_input(device)
            self.midi_port.callback = self.message_callback
            self.midi_port_open = True
            self.dev_lbl.setText(mido.get_input_names()[0])
            print("Midi device connected")
        else: 
            self.dev_lbl.setText('No Device')
            print("Error, no midi device connected")

    # def paintEvent(self, event):
    #     paint = QPainter()
    #     paint.begin(self)
    #     self.drawShapes(event, paint)

    # def drawShapes(self, event, paint):
    #     color = self.blue
    #     alpha = 240
    #     color.setAlpha(alpha)
    #     paint.setBrush(QBrush(color))
    #     paint.setPen(Qt.NoPen)
    #     rect_width = int(self.window_width/250)
    #     width = self.dev_lbl.width()
    #     top_rect = QRect(self.midi_lbl.x()*0.9,self.midi_lbl.y()*1.1,self.midi_lbl.width()*1.2,self.midi_lbl.height()*0.5)
    #     paint.drawRect(top_rect)
    #     paint.drawRect(top_rect.x() + top_rect.width(),top_rect.y() + (top_rect.height()-rect_width),self.dev_lbl.x() + self.dev_lbl.width() - (top_rect.x() + top_rect.width()),rect_width)


app = QApplication(sys.argv)
ex = App()
ex.setWindowOpacity(0.99)
sys.exit(app.exec_())