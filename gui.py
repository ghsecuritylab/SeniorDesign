# Add key signature list
# Seperate GUIs
#
import sys
import mido
import serial
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
        self.setWindowIcon(QIcon('uf_logo.png'))
        self.setWindowTitle(self.title)
        self.p = self.palette()
        self.setStyleSheet('background-color: rgb(42,41,46); color: rgb(200,209,218)')
        self.central_widget = Central(self)
        self.setCentralWidget(self.central_widget)
        self.showMaximized()
        self.show()
        self.central_widget.startHarmonizer() 

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
        self.starting_key_column = 6
        self.current_key_pos = [1,self.starting_key_column]
        self.current_key_midi_num = 64 # E 
        self.current_key_mode = "Major"
        self.major_pos = [0,5]
        self.minor_pos = [1,5]
        self.chord_pos = [3,0]
        self.init_harmonizer()
        self.layout.setSpacing(self.window_height/20)
        self.layout.addLayout(self.harmonizer_layout)
        self.layout.addStretch()
        self.layout.setContentsMargins(self.window_width/25,self.window_width/50,self.window_height/50,self.window_height/50)
        self.setLayout(self.layout)
        self.ser = serial.Serial()
        self.opacity = 0.9
        self.timer = QTimer()
        self.connectBoard()
        self.chord_harmonies = [False,False,False,False,False,False,False,False,False]
        self.restartChordHarms() 

    def init_harmonizer(self):

        self.gray_text = 'color: rgb(200,209,218)'

        # Create layout to encapsulate harmonizer portion
        self.harmonizer_layout = QGridLayout()

        # Create grid layout for user harmonizer control
        self.harmonizer_layout.setSpacing(10)

        # Look for devices and create label to display current device
        self.device_list = mido.get_input_names()
        self.dev_lbl = QLabel()
        self.dev_lbl.setStyleSheet(self.gray_text)
        if self.device_list:
            self.dev_lbl.setText(mido.get_input_names()[0])  # Set label to first device in list
        else:
            self.dev_lbl.setText('No Device')  # Set default label if not device is found
        self.dev_lbl.setFont(QFont('Calibri Light',64))  # Edit label font
        self.dev_lbl.setMaximumSize(self.dev_lbl.minimumSizeHint().width()*1.2,self.dev_lbl.minimumSizeHint().height()*1.3)
        self.harmonizer_layout.addWidget(self.dev_lbl,0,0,1,5,Qt.AlignLeft)

        # Create label to describe shown device as MIDI device
        self.midi_lbl = QLabel('MIDI Device')
        self.midi_lbl.setFont(QFont("Calibri Light",12))
        self.midi_lbl.setStyleSheet(self.remove_background + '; color: rgb(42,41,46)')
        self.midi_lbl.resize(self.midi_lbl.minimumSizeHint().width()*1.2,self.midi_lbl.minimumSizeHint().height()*1.2)
        self.harmonizer_layout.addWidget(self.midi_lbl,1,0,1,5,Qt.AlignLeft)

        # Create labels for all keys 
        self.key_array = ['C','C#/Db','D','D#/Eb','E','F','F#/Gb','G','G#/Ab','A','A#/Bb','B']
        row = 0
        column = self.starting_key_column
        for key in self.key_array:
            button = QPushButton(key)
            button.setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")
            button.setFont(QFont('Calibri Light',46))
            button.setMaximumSize(button.minimumSizeHint().width()*1.2,button.minimumSizeHint().height())
            button.clicked.connect(self.chooseKey)
            self.harmonizer_layout.addWidget(button,row,column,Qt.AlignCenter)
            if column < 9:
                column = column + 1
            else:
                column = self.starting_key_column
                row = row + 1

        # Show default key signature
        button = self.harmonizer_layout.itemAtPosition(self.current_key_pos[0],self.current_key_pos[1]).widget()
        button.setStyleSheet(self.remove_background + "; color: rgb(59,202,243,244)")

        # Major key 
        button = QPushButton('Major')
        button.setStyleSheet(self.remove_background + "; color: rgb(59,202,243,244)")
        button.setFont(QFont('Calibri Light',46))
        button.setMaximumSize(button.minimumSizeHint().width()*1.2,button.minimumSizeHint().height())
        button.clicked.connect(self.chooseKeyMode)
        self.harmonizer_layout.addWidget(button,self.major_pos[0],self.major_pos[1],Qt.AlignCenter)

        # Minor Key 
        button = QPushButton('Minor')
        button.setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")
        button.setFont(QFont('Calibri Light',46))
        button.setMaximumSize(button.minimumSizeHint().width()*1.2,button.minimumSizeHint().height())
        button.clicked.connect(self.chooseKeyMode)
        self.harmonizer_layout.addWidget(button,self.minor_pos[0],self.minor_pos[1],Qt.AlignCenter)

        #Show 9 harmony indicators 
        self.harmony_array = ['Octave Down', 'Low 3rd', 'Low 5th', 'Low 6th', 'High 3rd', 'High 5th', 'High 6th', 'Octave Up']
        row = self.chord_pos[0]
        column = self.chord_pos[1]
        for harmony in self.harmony_array:
            button = QPushButton(harmony)
            button.setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")
            button.setFont(QFont('Calibri Light',36))
            button.setMaximumSize(button.minimumSizeHint().width(),button.minimumSizeHint().height())
            button.clicked.connect(self.changeChordHarmonies)
            self.harmonizer_layout.addWidget(button,row,column,Qt.AlignCenter)
            column = column + 1 

        # Create search and refresh buttons
        # self.search_button = QPushButton('Search')
        # self.search_button.setFont(QFont('Calibri',14))
        # self.search_button.setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent')
        # self.search_button.setMinimumSize(self.search_button.minimumSizeHint().width()*2,self.search_button.minimumSizeHint().height())
        # self.search_button.pressed.connect(lambda: self.search_button.setStyleSheet('background-color: rgb(192,192,192,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
        # self.search_button.released.connect(lambda: self.search_button.setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
        # self.harmonizer_layout.addWidget(self.search_button,2,0,1,2,Qt.AlignCenter)

        # self.rectangle_label = QLabel()
        # self.pixmap = QPixmap(10,60)
        # self.pixmap.fill(QColor(32,64,128))
        # self.rectangle_label.setPixmap(self.pixmap)

    def paintEvent(self, event):
        paint = QPainter()
        paint.begin(self)
        self.drawShapes(event, paint)

    def drawShapes(self, event, paint):
        color = self.blue
        alpha = 240
        color.setAlpha(alpha)
        paint.setBrush(QBrush(color))
        paint.setPen(Qt.NoPen)
        rect_width = int(self.window_width/250)
        width = self.dev_lbl.width()
        top_rect = QRect(self.midi_lbl.x()*0.95,self.midi_lbl.y()*0.99,self.midi_lbl.width()*1.1,self.midi_lbl.height()*1.2)
        paint.drawRect(top_rect)
        paint.drawRect(top_rect.x() + top_rect.width(),top_rect.y() + (top_rect.height()-rect_width),self.dev_lbl.x() + self.dev_lbl.width() - (top_rect.x() + top_rect.width()),rect_width)

    def sendKeyChange(self, midi_key_num):
        if self.ser.isOpen():
            self.ser.write([255])
            self.ser.write([255])
            self.ser.write([midi_key_num])
            print(midi_key_num)
        else: 
            self.connectBoard()
            print('Cannot send key')

    def restartChordHarms(self): 
        for i in range(0,9): 
            self.chord_harmonies[i] = False 
        if self.ser.isOpen():
            self.ser.write([255])
            self.ser.write([255])
            self.ser.write([255])
        else: 
            self.connectBoard()
            print('Error restarting')

    def chooseKeyMode(self): 
        button = self.sender()
        button.setStyleSheet(self.remove_background + "; color: rgb(59,202,243,244)")
        index = self.harmonizer_layout.indexOf(button)
        current_button_pos = self.harmonizer_layout.getItemPosition(index)
        if(current_button_pos[0] == self.major_pos[0] and current_button_pos[1] == self.major_pos[1]):
            if(self.current_key_mode != "Major"):
                # dim minor button 
                minor_button = self.harmonizer_layout.itemAtPosition(self.minor_pos[0],self.minor_pos[1]).widget()
                minor_button.setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")
                self.current_key_mode = "Major"
                self.current_key_midi_num += 4
                self.sendKeyChange(self.current_key_midi_num) 
        elif(current_button_pos[0] == self.minor_pos[0] and current_button_pos[1] == self.minor_pos[1]): 
            if (self.current_key_mode != "Minor"):
                # dim major button 
                major_button = self.harmonizer_layout.itemAtPosition(self.major_pos[0],self.major_pos[1]).widget()
                major_button.setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")
                self.current_key_mode = "Minor"
                self.current_key_midi_num -= 4
                self.sendKeyChange(self.current_key_midi_num)

    def chooseKey(self):
        old_key_button = self.harmonizer_layout.itemAtPosition(self.current_key_pos[0],self.current_key_pos[1]).widget()
        old_key_button.setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")
        button = self.sender()
        button.setStyleSheet(self.remove_background + "; color: rgb(59,202,243,244)")
        index = self.harmonizer_layout.indexOf(button)
        current_button_pos = self.harmonizer_layout.getItemPosition(index)
        self.current_key_pos = current_button_pos 
        self.current_key_midi_num = self.base_key + (4*self.current_key_pos[0]) + (self.current_key_pos[1]-self.starting_key_column)
        if(self.current_key_mode == "Minor"): 
            self.current_key_midi_num -= 4
        self.sendKeyChange(self.current_key_midi_num)

    def changeChordHarmonies(self): 
        button = self.sender()
        index = self.harmonizer_layout.indexOf(button)
        current_button_pos = self.harmonizer_layout.getItemPosition(index)
        chord_idx = current_button_pos[1] - self.chord_pos[1]
        self.chord_harmonies[chord_idx] ^= True 
        if (self.chord_harmonies[chord_idx]):
            button.setStyleSheet(self.remove_background + "; color: rgb(59,202,243,244)")
        else: 
            button.setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")
        self.ser.write([192])
        self.ser.write([chord_idx])

    def connectBoard(self):
        if (os.path.exists("/dev/tty.usbmodem1462")):
            self.ser = serial.Serial(port='/dev/tty.usbmodem1462', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=0.001)
        elif (os.path.exists("/dev/tty.usbmodem1442")):
            self.ser = serial.Serial(port='/dev/tty.usbmodem1442', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=0.001)
        elif (os.path.exists("/dev/tty.usbserial-A904RDA3")):
            self.ser = serial.Serial(port='/dev/tty.usbserial-A904RDA3', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=0.001)
        else:
            self.ser = serial.Serial(baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=0.001)
            print("No board connected")
            return
        try:
            self.ser.isOpen()
            print("Serial port is open")
        except:
            print("Error")
            self.harmonizer_start.setText('Start')
            return

    def message_callback(self, message): 
        if (message.bytes()[0] == 192): 
            chord_idx = message.bytes()[1]
            self.chord_harmonies[chord_idx] ^= True 
            harmony = self.harmonizer_layout.itemAtPosition(self.chord_pos[0],chord_idx).widget()
            if (self.chord_harmonies[chord_idx]):
                harmony.setStyleSheet(self.remove_background + "; color: rgb(59,202,243,244)")
            else: 
                harmony.setStyleSheet(self.remove_background + "; color: rgb(200,209,218,50)")

            
        self.ser.write(message.bytes())
        print(message.bytes())

    def startHarmonizer(self):
        port = mido.open_input(mido.get_input_names()[0])
        port.callback = self.message_callback


app = QApplication(sys.argv)
ex = App()
ex.setWindowOpacity(0.95)
sys.exit(app.exec_())