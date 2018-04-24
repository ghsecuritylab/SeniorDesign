import sys
import mido
import struct
import serial
import os
import numpy as np
from PyQt5.QtWidgets import QMainWindow,QApplication,QPushButton,QWidget,QAction,QTabWidget,QLayout,QVBoxLayout,QHBoxLayout,QLabel,QComboBox,QSlider,QTextEdit,QLineEdit,QGridLayout,QDesktopWidget,QSizePolicy,QGraphicsOpacityEffect
from PyQt5.QtGui import QIcon,QColor,QPalette,QFont,QPixmap,QPainter,QPainterPath,QPen,QBrush,QLinearGradient
from PyQt5.QtCore import pyqtSlot,Qt,QSize,QRect,QTimer,QRectF
from PyQt5.QtTest import QTest

class App(QMainWindow):

    def __init__(self):
        super().__init__()
        self.title = 'Gatorscribe'
        self.setWindowIcon(QIcon('uf_logo.png'))
        self.setWindowTitle(self.title)
        self.p = self.palette()
        self.setStyleSheet('background-color: rgb(42,41,46); color: rgb(200,209,218)')
        self.central_widget = Central(self)
        self.setCentralWidget(self.central_widget)
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
        self.blue = QColor(50,200,255)
        self.green = QColor(104,236,196)
        self.orange = QColor(251,165,125)
        self.magenta = QColor(157,39,89)
        self.yellow = QColor(177,176,148)
        self.init_gatorscribe()
        self.layout.setSpacing(self.window_height/40)
        self.layout.addLayout(self.horizontal_menu)
        self.layout.addLayout(self.options_layout)
        self.layout.addStretch()
        self.layout.setContentsMargins(self.window_width/25,self.window_width/50,self.window_height/15,self.window_height/50)
        self.setLayout(self.layout)
        self.ser = serial.Serial()
        self.opacity = 0.9
        self.timer = QTimer()
        self.timer.setInterval(180)
        self.timer.timeout.connect(self.read_serial_port)
        self.timer.start()
        self.connectBoard()

    def get_midi_file(self):
        if (self.ser.isOpen()):
            msg = self.ser.read().decode('ascii')
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

                msg = self.ser.read().decode('ascii')
                #print(msg)

            x.tofile('my_song.mid') # simply name of file
            print("Created MIDI File")
            self.start_btn.setText("Start")
        else:
            print("No board connected")
            self.connectBoard()

    def read_serial_port(self):
        if self.ser.isOpen():
            # decode serial message from board
            try:
                serial_msg = self.ser.read() #.decode('ascii')
                s = list(serial_msg)
                #print(s)
                if (len(s) > 0):
                    if (s[0] < 128):
                        msg = serial_msg.decode('ascii')
                        if (msg == '-'):
                            self.get_midi_file()

                    if (s[0] == 255): # read bpm
                        serial_msg = self.ser.read() #.decode('ascii')
                        s = list(serial_msg)
                        self.tempo_edit.setText(str(s[0]))
                        #print(s)

                    elif (s[0] == 254): # start recording
                        self.startGatorscribe()
                    elif(s[0] == 253):
                        self.reset_start_stop()
                    elif(s[0] == 252):
                        print("Getting pitch")
                        self.getPitch()
            except:
                self.start_btn.setText("Reconnect")
                self.ser.close()

    def getPitch(self): 
        s = [0,0,0,0]
        for i in range(0,4): 
            serial_msg = self.ser.read() #.decode('ascii')
            s[i] = list(serial_msg)[0]
        aa= bytearray(s) 
        values = struct.unpack('<f', aa)
        self.input_freq = values[0]
        print(values[0]) 
        self.updateTuner()

    def find_nearest(self,array,value):
        idx = (np.abs(array-value)).argmin()
        return (idx)

    def updateTuner(self):
        freq_index = self.find_nearest(self.freq_array, self.input_freq)
        self.closest_freq = self.freq_array[freq_index]
        lower_index = freq_index-1
        lower_freq = self.freq_array[lower_index]
        higher_index = freq_index+1
        higher_freq = self.freq_array[higher_index]
        left_freq = (lower_freq + self.closest_freq) / 2
        right_freq = (higher_freq + self.closest_freq) / 2
        print("Lower Frequency: " + str(lower_freq))
        print("Closest Frequency: " + str(self.closest_freq))
        print("Higher Frequency: " + str(higher_freq))
        if (self.input_freq < self.closest_freq):
            self.percent_change = (self.input_freq - self.closest_freq)/(self.closest_freq - left_freq)
        else:
            self.percent_change = (self.input_freq - self.closest_freq)/(right_freq - self.closest_freq)
        print("P change = " + str(self.percent_change))
        print("Index is " + str(freq_index))
        print(self.freq_dict[self.freq_array[freq_index]])
        print(self.freq_dict[self.freq_array[lower_index]])
        print(self.freq_dict[self.freq_array[higher_index]])
        self.update()

    def init_gatorscribe(self):
        # Create label to mark gatorscribe area
        self.gatorscribe_lbl = QLabel('Transcribe')
        self.gatorscribe_lbl.setFont(QFont('Calibri Light',12))
        self.gatorscribe_lbl.setStyleSheet('background-color: rgb(128,128,128,64); color: rgb(255,255,255)')

        # Create grid layout for options
        self.options_layout = QGridLayout()
        self.options_layout.setSpacing(self.window_width/50)
        self.options_layout.setContentsMargins(int(self.window_width/50),int(self.window_width/50),int(self.window_width/50),int(self.window_width/50))

        # Create grid layout for title and start
        self.horizontal_menu = QHBoxLayout()
        # Create layout for song title
        self.title_layout = QVBoxLayout()
        self.title_lbl = QLabel('Title')
        self.title_lbl.setFont(QFont('Calibri Light',self.window_width/128))
        self.title_lbl.setStyleSheet(self.remove_background + '; color: rgb(42,41,46)')
        self.title_lbl.setMaximumSize(self.title_lbl.minimumSizeHint().width()*1.5,self.title_lbl.minimumSizeHint().height()*1.5)
        self.title_lbl.setContentsMargins(self.title_lbl.minimumSizeHint().width()/5,self.title_lbl.minimumSizeHint().width()/5,self.title_lbl.minimumSizeHint().width()/5,self.title_lbl.minimumSizeHint().width()/5)
        self.title_edit = QLineEdit('Song Title')
        self.title_edit.setStyleSheet(self.remove_background)
        self.title_edit.setFont(QFont('Calibri Light',64))
        self.title_edit.setAlignment(Qt.AlignLeft)
        self.title_edit.setMaxLength(30)
        self.title_edit.setMinimumSize(self.title_edit.minimumSizeHint().width()*1.2,self.title_edit.minimumSizeHint().height()*0.9)
        self.title_edit.editingFinished.connect(self.editTitle)

        self.editing_title = False
        self.finishing_title = False
        self.grad_position = 0.0
        self.title_layout.addWidget(self.title_edit)
        self.title_layout.addWidget(self.title_lbl)
        self.title_layout.setSpacing(0)

        # Create tuner button
        self.tuner_btn = QPushButton('Tuner')
        self.tuner_btn.setFont(QFont('Calibri',self.window_width/128))
        self.tuner_btn.setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent')
        self.tuner_btn.setMinimumSize(self.tuner_btn.minimumSizeHint().width()*2,self.tuner_btn.minimumSizeHint().height())
        self.tuner_btn.pressed.connect(lambda: self.tuner_btn.setStyleSheet('background-color: rgb(192,192,192,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
        self.tuner_btn.released.connect(lambda: self.tuner_btn.setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
        self.tuner_btn.clicked.connect(self.toggleTuner)

        # Create start button for gatorscribe
        self.start_btn = QPushButton('Start')
        self.start_btn.setFont(QFont('Calibri',self.window_width/128))
        self.start_btn.setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent')
        self.start_btn.setMinimumSize(self.start_btn.minimumSizeHint().width()*2,self.start_btn.minimumSizeHint().height())
        self.start_btn.pressed.connect(lambda: self.start_btn.setStyleSheet('background-color: rgb(192,192,192,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
        self.start_btn.released.connect(lambda: self.start_btn.setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
        self.start_btn.clicked.connect(self.sendStartStopFlag)

        # Create horizontal menu
        self.horizontal_menu.addLayout(self.title_layout)
        self.horizontal_menu.addWidget(self.tuner_btn)
        self.horizontal_menu.addWidget(self.start_btn)
        self.horizontal_menu.setAlignment(self.tuner_btn,Qt.AlignLeft)
        self.horizontal_menu.setAlignment(self.start_btn,Qt.AlignLeft)
        #self.horizontal_menu.addStretch()

        # Class variables for key signature
        self.current_key = 'C Major\nA minor'
        self.midi_keys_dict = {'C Major\nA minor':0, 'G Major\nE minor':1, 'D Major\nB minor':2, 'A Major\nF# minor':3, 'E Major\nC# minor':4,
            'B Major\nG# minor':5, 'F# Major\nD# minor':6, 'C# Major\nA# minor':7, 'F Major\nD minor':-1, 'Bb Major\nG minor':-2, 'Eb Major\nC minor':-3,
            'Ab Major\nF minor':-4, 'Db Major\nBb minor':-5, 'Gb Major\nEb minor':-6, 'Cb Major\nAb minor':-7}
       # self.minor_keys_dict = {'A Minor':0, 'E':1, 'B':2, 'F#':3, 'C#':4, 'G#':5, 'D#':6, 'A#':7, 'D':-1, 'G':-2, 'C':-3, 'F':-4, 'Bb':-5, 'Eb':-6, 'Ab':-7}

        # Class variables for instruments
        self.instrument_dict = {'guitar.png':27, 'piano.png':0, 'violin.png':40, 'trumpet.png':56, 'synth.png':80, 'sax.png':64}
        self.current_instrument = 'piano.png'

        # Array to hold all colors
        self.color_array = [self.blue, self.yellow, self.green, self.orange, self.magenta]

        # Look up table for set frequencies
        self.freq_array = np.array([8.176,8.662,9.177,9.723,10.301,10.913,11.562,12.250,12.978,13.750,14.568,15.434,
                            16.35,17.32,18.35,19.45,20.60,21.83,23.12,24.50,25.96,27.50,29.14,30.87,32.70,34.65,36.71,38.89,41.20,43.65,
                            46.25,49.00,51.91,55.00,58.27,61.74,65.41,69.30,73.42,77.78,82.41,87.31,92.50,98.00,103.83,110.00,116.54,123.47,
                            130.81,138.59,146.83,155.56,164.81,174.61,185.00,196.00,207.65,220.00,233.08,246.94,261.63,277.18,293.66,311.13,
                            329.63,349.23,369.99,392.00,415.30,440.00,466.16,493.88,523.25,554.37,587.33,622.25,659.25,698.46,739.99,783.99,
                            830.61,880.00,932.33,987.77,1046.50,1108.73,1174.66,1244.51,1318.51,1396.91,1479.98,1567.98,1661.22,1760.00,1864.66,
                            1975.53,2093.00,2217.46,2349.32,2489.02,2637.02,2793.83,2959.96,3135.96,3322.44,3520.00,3729.31,3951.07,4186.01,4434.92,
                            4698.63,4978.03,5274.04,5587.65,5919.91,6271.93,6644.88,7040.00,7458.62,7902.13])

        self.note_array = [ 'C','C#','D','D#','E','F','F#','G','G#','A','A#','B',
                            'C','C#','D','D#','E','F','F#','G','G#','A','A#','B',
                            'C','C#','D','D#','E','F','F#','G','G#','A','A#','B',
                            'C','C#','D','D#','E','F','F#','G','G#','A','A#','B',
                            'C','C#','D','D#','E','F','F#','G','G#','A','A#','B',
                            'C','C#','D','D#','E','F','F#','G','G#','A','A#','B',
                            'C','C#','D','D#','E','F','F#','G','G#','A','A#','B',
                            'C','C#','D','D#','E','F','F#','G','G#','A','A#','B',
                            'C','C#','D','D#','E','F','F#','G','G#','A','A#','B',
                            'C','C#','D','D#','E','F','F#','G','G#','A','A#','B']

        self.percent_change = 0.0
        self.input_freq = 395.0
        self.freq_dict = dict(zip(self.freq_array,self.note_array))
        self.closest_freq = self.freq_array[40]

        # Create all components of the instrument menu
        self.initInstrumentMenu()

        # Create all components of the main menu
        self.initMainMenu()

        # Create all components of the time menu
        self.initTimeMenu()

        # Create all components of the tuner menu
        self.initTunerMenu()

        # Fill in options grid layout
        self.options_layout.addLayout(self.time_layout,0,0,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addLayout(self.key_layout,0,1,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addLayout(self.tempo_layout,1,0,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addLayout(self.instrument_layout,1,1,Qt.AlignLeft | Qt.AlignVCenter)

    def resizeEvent(self, event):
        self.gatorscribe_lbl.resize(self.width(),self.gatorscribe_lbl.height())

    def paintEvent(self, event):
        paint = QPainter()
        paint.begin(self)
        self.drawShapes(event, paint, self.paint_code)

    def drawShapes(self, event, paint, paint_code):
        color = self.blue
        alpha = 240
        color.setAlpha(alpha)
        paint.setBrush(QBrush(color))
        paint.setPen(Qt.NoPen)
        rect_width = int(self.window_width/250)
        title_rect = QRect(self.title_lbl.x()+self.title_lbl.width(),self.title_lbl.y(),(self.start_btn.x()+self.start_btn.width())-(self.title_lbl.x()+self.title_lbl.width()),rect_width/2)
        if self.editing_title or self.finishing_title:
            grad = QLinearGradient(title_rect.x(),title_rect.y() + title_rect.height(),title_rect.x() + title_rect.width(),title_rect.y() + title_rect.height())
            color_1 = QColor()
            color_2 = QColor()
            if self.editing_title:
                color_1 = QColor(255,255,255)
                color_2 = QColor(self.blue)
            if self.finishing_title:
                color_1 = QColor(self.blue)
                color_2 = QColor(255,255,255)
            grad.setColorAt(self.grad_position,color_1)
            grad.setColorAt(self.grad_position+0.05,color_2)
            paint.setBrush(QBrush(grad))
        paint.drawRect(title_rect)
        paint.drawRect(QRect(self.title_lbl.x(),self.title_lbl.y(),self.title_lbl.width(),self.title_lbl.height()))

        if paint_code is 0:
            color.setAlpha(alpha*self.opacity)
            paint.setBrush(QBrush(color))
            paint.drawRect(QRect(self.time_lbl.x()-(rect_width*3),self.time_button.y(),rect_width,self.time_lbl.y()-self.time_button.y()+self.time_lbl.height()))
            color = self.green
            color.setAlpha(alpha*self.opacity)
            paint.setBrush(QBrush(color))
            paint.drawRect(QRect(self.key_lbl.x()-(rect_width*3),self.key_btn.y(),rect_width,self.key_lbl.y()-self.key_btn.y()+self.key_lbl.height()))
            color = self.orange
            color.setAlpha(alpha*self.opacity)
            paint.setBrush(QBrush(color))
            paint.drawRect(QRect(self.tempo_lbl.x()-(rect_width*3),self.tempo_edit.y(),rect_width,self.tempo_lbl.y()-self.tempo_edit.y()+self.tempo_lbl.height()))
            color = self.magenta
            color.setAlpha(alpha*self.opacity)
            paint.setBrush(QBrush(color))
            paint.drawRect(QRect(self.instrument_lbl.x()-(rect_width*3),self.instrument_pic.y(),rect_width,self.instrument_lbl.y()-self.instrument_pic.y()+self.instrument_lbl.height()))
        elif paint_code is 1:
            color.setAlpha(alpha*self.opacity)
            paint.setBrush(QBrush(color))
            paint.drawRect(QRect(self.button_44.x()-(rect_width*3),self.button_44.y(),rect_width,self.button_44.height()))
            color = self.green
            color.setAlpha(alpha*self.opacity)
            paint.setBrush(QBrush(color))
            paint.drawRect(QRect(self.button_34.x()-(rect_width*3),self.button_34.y(),rect_width,self.button_34.height()))
            color = self.orange
            color.setAlpha(alpha*self.opacity)
            paint.setBrush(QBrush(color))
            paint.drawRect(QRect(self.button_24.x()-(rect_width*3),self.button_24.y(),rect_width,self.button_24.height()))
            color = self.magenta
            color.setAlpha(alpha*self.opacity)
            paint.setBrush(QBrush(color))
            paint.drawRect(QRect(self.button_68.x()-(rect_width*3),self.button_68.y(),rect_width,self.button_68.height()))
        elif paint_code is 2:
            color = self.blue
            color.setAlpha(alpha*self.opacity)
            paint.setBrush(QBrush(color))
            paint.drawRect(QRect(self.guitar.x()-(rect_width*3),self.guitar.y(),rect_width,self.guitar.height()))
            color = self.green
            color.setAlpha(alpha*self.opacity)
            paint.setBrush(QBrush(color))
            paint.drawRect(QRect(self.piano.x()-(rect_width*3),self.piano.y(),rect_width,self.piano.height()))
            color = self.orange
            color.setAlpha(alpha*self.opacity)
            paint.setBrush(QBrush(color))
            paint.drawRect(QRect(self.violin.x()-(rect_width*3),self.violin.y(),rect_width,self.violin.height()))
            color = self.magenta
            color.setAlpha(alpha*self.opacity)
            paint.setBrush(QBrush(color))
            paint.drawRect(QRect(self.trumpet.x()-(rect_width*3),self.trumpet.y(),rect_width,self.trumpet.height()))
            color = self.purple
            color.setAlpha(alpha*self.opacity)
            paint.setBrush(QBrush(color))
            paint.drawRect(QRect(self.synth.x()-(rect_width*3),self.synth.y(),rect_width,self.synth.height()))
            color = self.yellow
            color.setAlpha(alpha*self.opacity)
            paint.setBrush(QBrush(color))
            paint.drawRect(QRect(self.sax.x()-(rect_width*3),self.sax.y(),rect_width,self.sax.height()))
        elif paint_code is 3:
            i = 0
            items = (self.options_layout.itemAt(i) for i in range(self.options_layout.count()))
            for widget in items:
                color = self.color_array[i]
                color.setAlpha(alpha*self.opacity)
                paint.setBrush(QBrush(color))
                current_widget = widget.widget()
                paint.drawRect(QRect(current_widget.x()-(rect_width*3),current_widget.y(),rect_width,current_widget.height()))
                if i > 3:
                    i = 0
                else:
                    i = i + 1
            top_widget = self.options_layout.itemAtPosition(0,0).widget()
            middle_widget = self.options_layout.itemAtPosition(self.options_layout.rowCount()/2,0).widget()
            right_widget = self.options_layout.itemAtPosition(2,self.options_layout.columnCount()-1).widget()
            bottom_widget = self.options_layout.itemAtPosition(self.options_layout.rowCount()-1,0).widget()
            paint.setBrush(QColor(200,209,218,20))
            path = QPainterPath()
            path.addRoundedRect(QRectF(top_widget.x()-(rect_width*5),top_widget.y()-(rect_width*2),right_widget.x()+right_widget.width()+(rect_width*10)-top_widget.x(),(middle_widget.y())-(rect_width)),10,10)
            paint.drawPath(path)
        elif paint_code is 4:
            base_line_y = self.exit_btn.y()-self.options_layout.rowMinimumHeight(0)/2.2
            base_line_x = self.exit_btn.x() - ((self.tuner_btn.x()+self.tuner_btn.width()) - (self.exit_btn.x()+self.exit_btn.width()))
            base_line_width = (2*((self.tuner_btn.x()+self.tuner_btn.width()) - (self.exit_btn.x()+self.exit_btn.width())))+self.exit_btn.width()
            paint.setBrush(QColor(50,200,255, 255))
            paint.drawRect(QRect(base_line_x,base_line_y,base_line_width,self.window_height/500))
            marker_height = self.options_layout.rowMinimumHeight(0)/2
            marker_y = base_line_y - (marker_height/2)
            paint.setBrush(QColor(50,200,255, 60))
            paint.drawRect(QRect(self.exit_btn.x()+(self.exit_btn.width()/2),marker_y,self.window_height/500,marker_height))
            marker_x = self.exit_btn.x()+(self.exit_btn.width()/2) + (self.percent_change*(base_line_width/2))
            marker_width = self.window_height/250
            if(abs(self.percent_change) < 0.1): 
                paint.setBrush(QColor(104,236,196, 255))
            else: 
                paint.setBrush(QColor(QColor(157,39,89, 255)))
            
            paint.drawRect(QRect(marker_x,marker_y,marker_width,marker_height))
            self.tuner_note.setText(self.freq_dict[self.closest_freq])

    # Traverse through input layout to change the transparency of each widget
    def fadeOptions(self, layout):
        if layout is not None:
            items = (layout.itemAt(i) for i in range(layout.count()))
            for item in items:
                widget = item.widget()
                if widget is not None:
                    op = QGraphicsOpacityEffect(widget)
                    op.setOpacity(self.opacity)
                    widget.setGraphicsEffect(op)
                    widget.setAutoFillBackground(True)
                else:
                    self.fadeOptions(item.layout())
                self.update()

    # Fade out function that calls fadeOptions for increasingly smaller alpha
    def fadeOut(self):
        self.opacity = 0.9
        while self.opacity >= 0:
            QTest.qWait(0.5)
            self.fadeOptions(self.options_layout)
            self.opacity = self.opacity - 0.1

    # Fade in function that calls fadeOptions for increasingly larger alpha
    def fadeIn(self):
        self.opacity = 0.0
        while self.opacity <= 0.9:
            QTest.qWait(0.5)
            self.fadeOptions(self.options_layout)
            self.opacity = self.opacity + 0.1
        self.hide()
        self.show()

    def clearGridLayout(self):
        if self.options_layout is not None:
            items = (self.options_layout.itemAt(i) for i in reversed(range(self.options_layout.count())))
            for item in items:
                if item is not None:
                    layout = item.layout()
                    if layout is not None:
                        self.options_layout.removeItem(layout)
                        layout.setParent(None)
                    else:
                        widget = item.widget()
                        if widget is not None:
                            self.options_layout.removeWidget(widget)
                            widget.setParent(None)
        self.options_layout.setRowMinimumHeight(0,0)
        painter = QPainter()
        painter.eraseRect(0,self.title_lbl.y(),self.window_width,self.window_height-self.title_edit.y())
        self.hide()
        self.show()

    def initMainMenu(self):
        self.paint_code = 0
        # Time signature table
        self.time_list = ['4/4','3/4','2/4','6/8']
        self.time_index = 0

        # Create time signature layout
        self.time_layout = QVBoxLayout()
        self.time_lbl = QLabel('Time Signature')
        self.time_lbl.setFont(QFont('Calibri Light',self.window_width/128))  # Edit label font
        self.time_lbl.setMaximumSize(self.time_lbl.minimumSizeHint().width(),self.time_lbl.height())

        self.time_button = QPushButton(self.time_list[self.time_index])
        self.time_button.setStyleSheet(self.remove_background)
        self.time_button.setFont(QFont('Calibri Light',self.window_width/20))
        self.time_button.setMaximumSize(self.time_button.minimumSizeHint().width()*1.2,self.time_button.minimumSizeHint().height())
        self.time_button.clicked.connect(self.constructTimeMenu)

        self.time_layout.addWidget(self.time_button)
        self.time_layout.addWidget(self.time_lbl)
        self.time_layout.setAlignment(self.time_lbl,Qt.AlignLeft)
        self.time_layout.setAlignment(self.time_button,Qt.AlignLeft)
        self.time_layout.setSpacing(0)

        # Create key signature layout
        self.key_layout = QVBoxLayout()
        self.key_lbl = QLabel('Key Signature')
        self.key_lbl.setFont(QFont('Calibri Light',self.window_width/128))  # Edit label font
        self.key_lbl.setAlignment(Qt.AlignLeft)
        self.key_btn = QPushButton(self.current_key)
        self.key_btn.setStyleSheet(self.remove_background)
        self.key_btn.setFont(QFont('Calibri Light',self.window_width/39))
        self.key_btn.setMaximumSize(self.key_btn.minimumSizeHint().width()*1.2,self.key_btn.minimumSizeHint().height())
        self.key_btn.clicked.connect(self.constructKeyMenu)
        self.key_layout.addWidget(self.key_btn)
        self.key_layout.addWidget(self.key_lbl)
        self.key_layout.setAlignment(self.key_btn,Qt.AlignLeft)
        self.key_layout.setAlignment(self.key_lbl,Qt.AlignLeft)
        self.key_layout.setSpacing(0)

         # Create tempo layout
        self.tempo_layout = QVBoxLayout()
        self.tempo_lbl = QLabel('Tempo')
        self.tempo_lbl.setFont(QFont('Calibri Light',self.window_width/128))  # Edit label font
        self.tempo_lbl.setStyleSheet('color: rgb(170,179,188)')
        self.tempo_lbl.setMaximumSize(self.tempo_lbl.minimumSizeHint().width(),self.tempo_lbl.height())
        self.tempo_edit = QLineEdit()
        self.tempo_edit.setStyleSheet(self.remove_background)
        self.tempo_edit.setFont(QFont('Calibri Light',self.window_width/20))
        self.tempo_edit.setAlignment(Qt.AlignLeft)
        self.tempo_edit.setMaxLength(3)
        self.tempo_edit.setMaximumSize(self.tempo_edit.minimumSizeHint().width()*2.4,self.tempo_edit.minimumSizeHint().height())
        self.tempo_edit.setText('100')
        #self.tempo_edit.editingFinished.connect(self.sendTempo)
        self.tempo_layout.addWidget(self.tempo_edit)
        self.tempo_layout.addWidget(self.tempo_lbl)
        self.tempo_layout.setAlignment(self.tempo_lbl,Qt.AlignLeft)
        self.tempo_layout.setAlignment(self.tempo_edit,Qt.AlignLeft)
        self.tempo_layout.setSpacing(0)

        # Create instrument layout
        self.instrument_layout = QVBoxLayout()
        self.instrument_lbl = QLabel('Playback Instrument')
        self.instrument_lbl.setFont(QFont('Calibri Light',self.window_width/128))  # Edit label font
        self.instrument_lbl.setStyleSheet('color: rgb(170,179,188)')
        self.instrument_lbl.setAlignment(Qt.AlignLeft)
        self.instrument_pic = QPushButton()
        self.instrument_pic.setStyleSheet(self.remove_background)
        self.instrument_pic.setIcon(QIcon(self.current_instrument))
        self.instrument_pic.setIconSize(QSize(self.key_btn.height(),self.key_btn.height()))
        self.instrument_pic.clicked.connect(lambda: self.constructInstrumentMenu())
        self.instrument_layout.addWidget(self.instrument_pic)
        self.instrument_layout.addWidget(self.instrument_lbl)
        self.instrument_layout.setAlignment(self.instrument_pic,Qt.AlignLeft)
        self.instrument_layout.setAlignment(self.instrument_lbl,Qt.AlignLeft)
        self.instrument_layout.setSpacing(0)

    def initTimeMenu(self):

        self.button_44 = QPushButton('4/4')
        self.button_44.setStyleSheet(self.remove_background)
        self.button_44.setFont(QFont('Calibri Light',self.window_width/20))
        self.button_44.setMaximumSize(self.button_44.minimumSizeHint().width(),self.button_44.minimumSizeHint().height())
        self.button_44.clicked.connect(lambda: self.changeTime(0))

        self.button_34 = QPushButton('3/4')
        self.button_34.setStyleSheet(self.remove_background)
        self.button_34.setFont(QFont('Calibri Light',self.window_width/20))
        self.button_34.setMaximumSize(self.time_button.width(),self.time_button.height())
        self.button_34.clicked.connect(lambda: self.changeTime(1))

        self.button_24 = QPushButton('2/4')
        self.button_24.setStyleSheet(self.remove_background)
        self.button_24.setFont(QFont('Calibri Light',self.window_width/20))
        self.button_24.setMaximumSize(self.time_button.width(),self.time_button.height())
        self.button_24.clicked.connect(lambda: self.changeTime(2))

        self.button_68 = QPushButton('6/8')
        self.button_68.setStyleSheet(self.remove_background)
        self.button_68.setFont(QFont('Calibri Light',self.window_width/20))
        self.button_68.setMaximumSize(self.time_button.width(),self.time_button.height())
        self.button_68.clicked.connect(lambda: self.changeTime(3))

    def constructKeyMenu(self):
        self.fadeOut()
        self.clearGridLayout()

        row = 0
        column = 0
        i = 0
        for key,value in self.midi_keys_dict.items():
            button = QPushButton(key)
            button.setStyleSheet(self.remove_background)
            button.setFont(QFont('Calibri',self.window_width/70))
            button.setMaximumSize(button.minimumSizeHint().width(),button.minimumSizeHint().height())
            button.clicked.connect(self.changeKey)
            button.clicked.connect(self.constructMainMenu)
            self.options_layout.addWidget(button,row,column,Qt.AlignLeft)
            if column < 4:
                column = column + 1
            else:
                column = 0
                row = row + 2
            i = i + 1

        self.paint_code = 3
        self.fadeIn()

    def initInstrumentMenu(self):
        self.pic_size = 0.8*QSize(self.window_height/6,self.window_height/6)

        self.guitar = QPushButton()
        self.guitar.setStyleSheet(self.remove_background)
        self.guitar.setIcon(QIcon('guitar.png'))
        self.guitar.setIconSize(self.pic_size)
        self.guitar.setSizePolicy(QSizePolicy.Preferred,QSizePolicy.Expanding)
        self.guitar.clicked.connect(lambda: self.changeInstrument('guitar.png'))
        self.guitar.clicked.connect(self.constructMainMenu)

        self.piano = QPushButton()
        self.piano.setStyleSheet(self.remove_background)
        self.piano.setIcon(QIcon('piano.png'))
        self.piano.setIconSize(self.pic_size)
        self.piano.setSizePolicy(QSizePolicy.Preferred,QSizePolicy.Expanding)
        self.piano.clicked.connect(lambda: self.changeInstrument('piano.png'))
        self.piano.clicked.connect(self.constructMainMenu)

        self.violin = QPushButton()
        self.violin.setStyleSheet(self.remove_background)
        self.violin.setIcon(QIcon('violin.png'))
        self.violin.setIconSize(self.pic_size)
        self.violin.setSizePolicy(QSizePolicy.Preferred,QSizePolicy.Expanding)
        self.violin.clicked.connect(lambda: self.changeInstrument('violin.png'))
        self.violin.clicked.connect(self.constructMainMenu)

        self.trumpet = QPushButton()
        self.trumpet.setStyleSheet(self.remove_background)
        self.trumpet.setIcon(QIcon('trumpet.png'))
        self.trumpet.setIconSize(self.pic_size)
        self.trumpet.setSizePolicy(QSizePolicy.Preferred,QSizePolicy.Expanding)
        self.trumpet.clicked.connect(lambda: self.changeInstrument('trumpet.png'))
        self.trumpet.clicked.connect(self.constructMainMenu)

        self.synth = QPushButton()
        self.synth.setStyleSheet(self.remove_background)
        self.synth.setIcon(QIcon('synth.png'))
        self.synth.setIconSize(self.pic_size)
        self.synth.setSizePolicy(QSizePolicy.Preferred,QSizePolicy.Expanding)
        self.synth.clicked.connect(lambda: self.changeInstrument('synth.png'))
        self.synth.clicked.connect(self.constructMainMenu)

        self.sax = QPushButton()
        self.sax.setStyleSheet(self.remove_background)
        self.sax.setIcon(QIcon('sax.png'))
        self.sax.setIconSize(self.pic_size)
        self.sax.setSizePolicy(QSizePolicy.Preferred,QSizePolicy.Expanding)
        self.sax.clicked.connect(lambda: self.changeInstrument('sax.png'))
        self.sax.clicked.connect(self.constructMainMenu)

    def initTunerMenu(self):
        self.exit_btn = QPushButton('')
        self.exit_btn.setFont(QFont('Calibri',self.window_width/128))
        self.exit_btn.setStyleSheet('background-color: rgb(127,127,127,0); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent')
        self.exit_btn.setMaximumSize(self.exit_btn.minimumSizeHint().width()*3,self.exit_btn.minimumSizeHint().height())

        self.tuner_note = QLabel('E')
        self.tuner_note.setFont(QFont('Calibri Light',80))
        self.tuner_note.setStyleSheet('background-color: rgb(128,128,128,0); color: rgb(255,255,255)')
        self.tuner_note.setAlignment(Qt.AlignHCenter)

    def constructMainMenu(self):
        self.fadeOut()
        self.clearGridLayout()

        self.time_button.setText(self.time_list[self.time_index])
        self.instrument_pic.setIcon(QIcon(self.current_instrument))
        self.key_btn.setText(self.current_key)
        # Fill in options grid layout
        self.options_layout.addLayout(self.time_layout,0,0,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addLayout(self.key_layout,0,1,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addLayout(self.tempo_layout,1,0,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addLayout(self.instrument_layout,1,1,Qt.AlignLeft | Qt.AlignVCenter)

        self.paint_code = 0
        self.fadeIn()

    def constructTimeMenu(self):
        self.fadeOut()
        self.clearGridLayout()

        self.options_layout.addWidget(self.button_44,0,0,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addWidget(self.button_34,0,1,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addWidget(self.button_24,1,0,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addWidget(self.button_68,1,1,Qt.AlignLeft | Qt.AlignVCenter)

        self.paint_code = 1
        self.fadeIn()

    def constructInstrumentMenu(self):
        self.fadeOut()
        self.clearGridLayout()

        self.options_layout.addWidget(self.guitar,0,0,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addWidget(self.piano,0,1,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addWidget(self.violin,0,2,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addWidget(self.trumpet,1,0,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addWidget(self.synth,1,1,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addWidget(self.sax,1,2,Qt.AlignLeft | Qt.AlignVCenter)

        self.paint_code = 2
        self.fadeIn()

    def constructTunerMenu(self):
        self.fadeOut()
        self.clearGridLayout()
        self.options_layout.setRowMinimumHeight(0,self.button_44.minimumSizeHint().height()*3)
        self.options_layout.addWidget(self.exit_btn,1,0,Qt.AlignVCenter)

        self.options_layout.addWidget(self.tuner_note, 0, 0, Qt.AlignVCenter | Qt.AlignTop)

        print(self.options_layout.rowMinimumHeight(0))
        self.paint_code = 4
        self.fadeIn()


    def editTitle(self):
        self.editing_title = True
        self.grad_position = 0.0
        while self.grad_position <= 0.95:
            QTest.qWait(1)
            self.update()
            self.grad_position = self.grad_position + 0.05
        self.editing_title = False
        self.hide()
        self.show()

    def connectBoard(self):
        if (os.path.exists("/dev/tty.usbmodem1462")):
            self.ser = serial.Serial(port='/dev/tty.usbmodem1462', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=0.2)
        elif (os.path.exists("/dev/tty.usbmodem1442")):
            self.ser = serial.Serial(port='/dev/tty.usbmodem1442', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=0.2)
        elif (os.path.exists("/dev/tty.usbserial-A904RDA3")):
            self.ser = serial.Serial(port='/dev/tty.usbserial-A904RDA3', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=0.1)
        if(self.ser.isOpen()):
            print("Serial port is open")
            self.reset_board()
            self.start_btn.setText('Start')
        else:
            print("No board connected")
            self.start_btn.setText('Reconnect')

    def toggleTuner(self):
        if (self.tuner_btn.text() == 'Tuner'):
            self.tuner_btn.setText('Back')
            self.sendStartTuning()
            self.constructTunerMenu();
        else:
            self.tuner_btn.setText('Tuner')
            self.sendStopTuning()
            self.constructMainMenu();
            

    def changeTime(self,number):
        self.time_index = number
        self.constructMainMenu()

    def changeKey(self):
        button = self.sender()
        self.current_key = button.text()

    def changeInstrument(self,instrument):
        self.current_instrument = instrument

    def reset_start_stop(self):
        self.start_btn.setText("Start")

    def reset_board(self): 
        if self.ser.isOpen():
            try:
                self.ser.write([252])
            except:
                return
        else:
            print('Reconnecting...')
            self.connectBoard()

    def sendStartStopFlag(self):
        if self.ser.isOpen():
            try:
                self.ser.write([255])
            except:
                return
        else:
            print('Reconnecting...')
            self.connectBoard()

    def sendStartTuning(self):
        if self.ser.isOpen():
            try:
                self.ser.write([254])
            except:
                return
        else:
            print('Reconnecting...')
            self.connectBoard()

    def sendStopTuning(self):
        if self.ser.isOpen():
            try:
                self.ser.write([253])
            except:
                return
        else:
            print('Reconnecting...')
            self.connectBoard()

    def sendTitle(self):
        if self.ser.isOpen():
            self.ser.write(self.title_edit.text().encode('ascii'))
            self.ser.write([0])
        else:
            self.connectBoard()
            print('Cannot send title ' + self.title_edit.text() + ' | Board not connected')

    def sendTime(self):
        if self.ser.isOpen():
            self.ser.write([self.time_index])
        else:
            self.connectBoard()
            print('Cannot send time ' + self.time_list[number] + ' | Board not connected')

    def sendKey(self):
        if self.ser.isOpen():
            self.ser.write([self.midi_keys_dict.get(self.current_key) + 7]) # can't send negative numbers. add 7 on board
        else:
            self.connectBoard()
            print('Cannot send key. Board not connected')

    def sendTempo(self):
        if self.ser.isOpen():
            self.ser.write([int(self.tempo_edit.text())])
        else:
            self.connectBoard()
            print('Cannot send tempo ' + self.tempo_edit.text() + ' | Board not connected')

    def sendInstrument(self):
        if self.ser.isOpen():
            self.ser.write([self.instrument_dict.get(self.current_instrument)])
        else:
            self.connectBoard()
            print('Cannot send instrument ' + self.current_instrument + ' | Board not connected')
        self.constructMainMenu()

    def startGatorscribe(self):
        if(self.ser.isOpen()):
            self.sendTitle()
            self.sendTime()
            self.sendKey()
            self.sendTempo()
            self.sendInstrument()
            self.start_btn.setText("Stop")
            print("Recording!")
        else:
            print("Cannot open serial port")

app = QApplication(sys.argv)
ex = App()
ex.setWindowOpacity(0.99)
sys.exit(app.exec_())
