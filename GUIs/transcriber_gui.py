import sys
import mido
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
        self.blue = QColor(50,209,255)
        self.green = QColor(104,216,196)
        self.orange = QColor(251,165,125)
        self.magenta = QColor(157,39,89)
        self.yellow = QColor(177,176,148)
        self.init_gatorscribe()
        self.layout.setSpacing(self.window_height/20)
        self.layout.addLayout(self.horizontal_menu)
        self.layout.addLayout(self.options_layout)
        self.layout.addStretch()
        self.layout.setContentsMargins(self.window_width/25,self.window_width/50,self.window_height/50,self.window_height/50)
        self.setLayout(self.layout)
        self.ser = serial.Serial()
        self.opacity = 0.9
        self.timer = QTimer()   
        self.timer.setInterval(200)
        self.timer.timeout.connect(self.read_serial_port)  
        self.timer.start()
        self.connectBoard()

    def get_midi_file(self):    
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

    def read_serial_port(self): 
        # decode serial message from board 
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

        # Create start button for gatorscribe
        self.start_btn = QPushButton('Start')
        self.start_btn.setFont(QFont('Calibri',self.window_width/128))
        self.start_btn.setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent')
        self.start_btn.setMinimumSize(self.start_btn.minimumSizeHint().width()*2,self.start_btn.minimumSizeHint().height())
        self.start_btn.pressed.connect(lambda: self.start_btn.setStyleSheet('background-color: rgb(192,192,192,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
        self.start_btn.released.connect(lambda: self.start_btn.setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
        # self.start_btn.clicked.connect(self.sendTitle)
        # self.start_btn.clicked.connect(lambda: self.sendTime(self.time_index))
        # self.start_btn.clicked.connect(self.sendKey)
        # self.start_btn.clicked.connect(self.sendTempo)
        # self.start_btn.clicked.connect(self.sendInstrument)
        self.start_btn.clicked.connect(self.startGatorscribe)

        # Create horizontal menu
        self.horizontal_menu.addLayout(self.title_layout)
        self.horizontal_menu.addWidget(self.start_btn)
        self.horizontal_menu.setAlignment(self.start_btn,Qt.AlignLeft)
        self.horizontal_menu.addStretch()

        # Class variables for key signature
        self.major_minor = 0
        self.current_key = 'C'
        self.major_keys_dict = {'C':0, 'G':1, 'D':2, 'A':3, 'E':4, 'B':5, 'F#':6, 'C#':7, 'F':-1, 'Bb':-2, 'Eb':-3, 'Ab':-4, 'Db':-5, 'Gb':-6, 'Cb':-7}
        self.minor_keys_dict = {'A':0, 'E':1, 'B':2, 'F#':3, 'C#':4, 'G#':5, 'D#':6, 'A#':7, 'D':-1, 'G':-2, 'C':-3, 'F':-4, 'Bb':-5, 'Eb':-6, 'Ab':-7}

        # Class variables for instruments
        self.instrument_dict = {'guitar.png':27, 'piano.png':0, 'violin.png':40, 'trumpet.png':56, 'synth.png':80, 'sax.png':64}
        self.current_instrument = 'piano.png'

        # Array to hold all colors
        self.color_array = [self.blue, self.purple, self.green, self.orange, self.magenta, self.yellow]

        # Create all components of the instrument menu
        self.initInstrumentMenu()

        # Create all components of the main menu
        self.initMainMenu()

        # Create all components of the time menu
        self.initTimeMenu()

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
            pen_color = QColor(200,209,218)
            pen_color.setAlpha(alpha*self.opacity)
            paint.setPen(pen_color)
            paint.setFont(QFont('Calibri Light',self.window_width/92))
            key = 'Major'
            if self.major_minor:
                key = 'Minor'
            paint.drawText(QRect(self.key_btn.x()+self.key_btn.width()+(rect_width),self.key_btn.y()+self.key_btn.height()-(rect_width*8),self.key_btn.width(),rect_width*5),Qt.AlignLeft,key)
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
                if i > 4:
                    i = 0
                else:
                    i = i + 1
            paint.setPen(QColor(200,209,218))
            paint.setFont(QFont('Calibri Light',self.window_width/128))
            top_widget = self.options_layout.itemAtPosition(0,0).widget()
            paint.drawText(QRect(top_widget.x(),top_widget.y()-(rect_width*5),top_widget.x(),rect_width*5),Qt.AlignLeft,'Major Keys')
            middle_widget = self.options_layout.itemAtPosition(self.options_layout.rowCount()/2,0).widget()
            right_widget = self.options_layout.itemAtPosition(2,self.options_layout.columnCount()-1).widget()
            bottom_widget = self.options_layout.itemAtPosition(self.options_layout.rowCount()-1,0).widget()
            paint.drawText(QRect(bottom_widget.x(),bottom_widget.y()+bottom_widget.height()+(rect_width*3),bottom_widget.x(),rect_width*5),Qt.AlignLeft,'Minor Keys')
            paint.setBrush(QColor(200,209,218,32))
            paint.setPen(QColor(200,209,218,0))
            path = QPainterPath()
            path.addRoundedRect(QRectF(top_widget.x()-(rect_width*5),top_widget.y()-(rect_width),right_widget.x()+right_widget.width()+(rect_width*10)-top_widget.x(),(middle_widget.y()-top_widget.y())-(rect_width*2)),10,10)
            paint.drawPath(path)
            path.addRoundedRect(QRectF(middle_widget.x()-(rect_width*5),middle_widget.y()-(rect_width),right_widget.x()+right_widget.width()+(rect_width*10)-top_widget.x(),(middle_widget.y()-top_widget.y())-(rect_width*2)),10,10)
            paint.drawPath(path)


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
            QTest.qWait(10)
            self.fadeOptions(self.options_layout)
            self.opacity = self.opacity - 0.1

    # Fade in function that calls fadeOptions for increasingly larger alpha
    def fadeIn(self):
        self.opacity = 0.0
        while self.opacity <= 0.9:
            QTest.qWait(10)
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
        self.time_button.setMaximumSize(self.time_button.minimumSizeHint().width()*1.2,self.time_button.minimumSizeHint().height()*0.9)
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
        self.key_btn.setFont(QFont('Calibri Light',self.window_width/20))
        self.key_btn.setMaximumSize(self.key_btn.minimumSizeHint().width()*2.4,self.key_btn.minimumSizeHint().height()*0.9)
        self.key_btn.clicked.connect(self.constructKeyMenu)
        self.key_layout.addWidget(self.key_btn)
        self.key_layout.addWidget(self.key_lbl)
        self.key_layout.setAlignment(self.key_btn,Qt.AlignLeft)
        self.key_layout.setAlignment(self.key_lbl,Qt.AlignLeft)
        self.key_layout.setSpacing(0)

         # Create tempo layout
        self.tempo_layout = QVBoxLayout()
        self.tempo_lbl = QLabel('BPM')
        self.tempo_lbl.setFont(QFont('Calibri Light',self.window_width/128))  # Edit label font
        self.tempo_lbl.setStyleSheet('color: rgb(170,179,188)')
        self.tempo_lbl.setMaximumSize(self.tempo_lbl.minimumSizeHint().width(),self.tempo_lbl.height())
        self.tempo_edit = QLineEdit()
        self.tempo_edit.setStyleSheet(self.remove_background)
        self.tempo_edit.setFont(QFont('Calibri Light',self.window_width/20))
        self.tempo_edit.setAlignment(Qt.AlignLeft)
        self.tempo_edit.setMaxLength(3)
        self.tempo_edit.setMaximumSize(self.tempo_edit.minimumSizeHint().width()*2.4,self.tempo_edit.minimumSizeHint().height()*0.9)
        self.tempo_edit.setText('100')
        #self.tempo_edit.editingFinished.connect(self.sendTempo)
        self.tempo_layout.addWidget(self.tempo_edit)
        self.tempo_layout.addWidget(self.tempo_lbl)
        self.tempo_layout.setAlignment(self.tempo_lbl,Qt.AlignLeft)
        self.tempo_layout.setAlignment(self.tempo_edit,Qt.AlignLeft)
        self.tempo_layout.setSpacing(0)

        # Create instrument layout
        self.instrument_layout = QVBoxLayout()
        self.instrument_lbl = QLabel('Instrument')
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
        for key,value in self.major_keys_dict.items():
            button = QPushButton(key)
            button.setStyleSheet(self.remove_background)
            button.setFont(QFont('Calibri Light',self.window_width/40))
            button.setMaximumSize(button.minimumSizeHint().width()*1.2,button.minimumSizeHint().height()*0.9)
            button.clicked.connect(lambda: self.changeKey(0))
            button.clicked.connect(self.constructMainMenu)
            self.options_layout.addWidget(button,row,column,Qt.AlignLeft)
            if column < 4:
                column = column + 1
            else:
                column = 0
                row = row + 1
        for key,value in self.minor_keys_dict.items():
            button = QPushButton(key)
            button.setStyleSheet(self.remove_background)
            button.setFont(QFont('Calibri Light',self.window_width/40))
            button.setMaximumSize(button.minimumSizeHint().width()*1.2,button.minimumSizeHint().height()*0.9)
            button.clicked.connect(lambda: self.changeKey(1))
            button.clicked.connect(self.constructMainMenu)
            self.options_layout.addWidget(button,row,column,Qt.AlignLeft)
            if column < 4:
                column = column + 1
            else:
                column = 0
                row = row + 1

        self.paint_code = 3
        self.fadeIn()

    def initInstrumentMenu(self):

        self.pic_size = QSize(self.window_height/6,self.window_height/6)

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
        else:
            self.ser = serial.Serial(baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=0.1)
            print("No board connected")
            return
        try:
            self.ser.isOpen()
            print("Serial port is open")
        except:
            print("Error")
            self.start_btn.setText('Start')
            return

    def changeTime(self,number):
        self.time_index = number
        self.constructMainMenu()

    def changeKey(self, major_minor):
        self.major_minor = major_minor
        button = self.sender()
        self.current_key = button.text()

    def changeInstrument(self,instrument):
        self.current_instrument = instrument
        #print(self.current_instrument)

    def sendStartFlag(self):
        if self.ser.isOpen():
            self.ser.write([255])
        else:
            self.connectBoard()
            print('Cannot send info')

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
            self.ser.write([self.major_minor])
            if self.major_minor:
                self.ser.write([self.minor_keys_dict.get(self.current_key)])
            else:
                self.ser.write([self.major_keys_dict.get(self.current_key)])
        else:
            self.connectBoard()
            if self.major_minor:
                print('Cannot send key ' + str(self.minor_keys_dict[self.current_key]) + ' | Board not connected')
            else:
                print('Cannot send key ' + str(self.major_keys_dict[self.current_key]) + ' | Board not connected')

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
            self.sendStartFlag() #255 
            self.sendTitle()
            self.sendTime() 
            self.sendKey() 
            self.sendTempo() 
            self.sendInstrument() 
            print("Recording!")
        else:
            print("Cannot open serial port")

app = QApplication(sys.argv)
ex = App()
ex.setWindowOpacity(0.99)
sys.exit(app.exec_())
