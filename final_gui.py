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
        self.init_harmonizer()
        self.init_gatorscribe()
        self.layout.setSpacing(self.window_height/20)
        self.layout.addLayout(self.harmonizer_layout)
        self.layout.addLayout(self.horizontal_menu)
        self.layout.addLayout(self.options_layout)
        self.layout.addStretch()
        self.layout.setContentsMargins(self.window_width/25,self.window_width/50,self.window_height/50,self.window_height/50)
        self.setLayout(self.layout)
        self.ser = serial.Serial()
        self.opacity = 0.9
        self.timer = QTimer()
        self.past_title = self.title_edit.text()
        self.past_time = 0
        self.past_key = self.key_edit.text()
        self.past_tempo = self.tempo_edit.text()
        self.past_instrument = 27
        self.connectBoard()

    def init_harmonizer(self):

        self.gray_text = 'color: rgb(200,209,218)'
        # Create layout to encapsulate harmonizer portion
        self.harmonizer_layout = QVBoxLayout()
        self.harmonizer_layout.addSpacing(5)

        # Create label to specify Harmonizer portion
        self.harmonizer_lbl = QLabel(' Harmonize')
        self.harmonizer_lbl.setFont(QFont('Calibri',12))
        self.harmonizer_lbl.setStyleSheet('background-color: rgb(128,128,128,64); color: rgb(255,255,255)')
        self.harmonizer_layout.setAlignment(self.harmonizer_lbl,Qt.AlignLeft)

        # Create grid layout for user harmonizer control
        self.harmonizer_panel = QGridLayout()
        self.harmonizer_layout.addLayout(self.harmonizer_panel)
        self.harmonizer_layout.setAlignment(self.harmonizer_panel,Qt.AlignLeft)
        self.harmonizer_layout.setSpacing(5)

        # Look for devices and create label to display current device
        self.device_list = mido.get_input_names()
        self.dev_lbl = QLabel()
        self.dev_lbl.setStyleSheet(self.gray_text)
        if self.device_list:
            self.dev_lbl.setText(mido.get_input_names()[0])  # Set label to first device in list
        else:
            self.dev_lbl.setText('Oxygen 49')  # Set default label if not device is found
        self.dev_lbl.setFont(QFont('Calibri Light',64))  # Edit label font
        self.dev_lbl.setMaximumSize(self.dev_lbl.minimumSizeHint().width()*1.2,self.dev_lbl.minimumSizeHint().height()*1.3)
        self.harmonizer_panel.addWidget(self.dev_lbl,0,0,1,2,Qt.AlignLeft)

        # Create label to describe shown device as MIDI device
        self.midi_lbl = QLabel('MIDI Device')
        self.midi_lbl.setFont(QFont("Calibri Light",12))
        self.midi_lbl.setStyleSheet(self.remove_background + '; color: rgb(42,41,46)')
        self.midi_lbl.resize(self.midi_lbl.minimumSizeHint().width()*1.2,self.midi_lbl.minimumSizeHint().height()*1.2)
        self.harmonizer_panel.addWidget(self.midi_lbl,1,0,1,2,Qt.AlignLeft)

        # Create button to search for MIDI devices
        self.dev_search = QPushButton('Search')
        self.dev_search.setFont(QFont('Calibri',14))
        self.dev_search.setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent')
        self.dev_search.setMinimumSize(self.dev_search.minimumSizeHint().width()*2,self.dev_search.minimumSizeHint().height())
        self.dev_search.pressed.connect(lambda: self.dev_search.setStyleSheet('background-color: rgb(192,192,192,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
        self.dev_search.released.connect(lambda: self.dev_search.setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
        self.harmonizer_panel.addWidget(self.dev_search,2,0,Qt.AlignHCenter)

        # Create start button
        self.harmonizer_start = QPushButton('Start')
        self.harmonizer_start.setFont(QFont('Calibri',14))
        self.harmonizer_start.setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent')
        self.harmonizer_start.setMinimumSize(self.harmonizer_start.minimumSizeHint().width()*2,self.harmonizer_start.minimumSizeHint().height())
        self.harmonizer_start.clicked.connect(self.startHarmonizer)
        self.harmonizer_start.pressed.connect(lambda: self.harmonizer_start.setStyleSheet('background-color: rgb(192,192,192,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
        self.harmonizer_start.released.connect(lambda: self.harmonizer_start.setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
        self.harmonizer_panel.addWidget(self.harmonizer_start,2,1,Qt.AlignHCenter)

        self.harmonizer_panel.setRowMinimumHeight(2,self.dev_search.minimumSizeHint().height()*2)

        self.rectangle_label = QLabel()
        self.pixmap = QPixmap(10,60)
        self.pixmap.fill(QColor(32,64,128))
        self.rectangle_label.setPixmap(self.pixmap)


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
        self.title_lbl.setFont(QFont('Calibri Light',14))
        self.title_lbl.setStyleSheet(self.remove_background + '; color: rgb(42,41,46)')
        self.title_lbl.setMaximumSize(self.title_lbl.minimumSizeHint().width()*1.5,self.title_lbl.minimumSizeHint().height()*1.2)
        self.title_lbl.setContentsMargins(self.title_lbl.minimumSizeHint().width()/5,self.title_lbl.minimumSizeHint().width()/5,self.title_lbl.minimumSizeHint().width()/5,self.title_lbl.minimumSizeHint().width()/5)
        self.title_edit = QLineEdit('Song Title')
        self.title_edit.setStyleSheet(self.remove_background)
        self.title_edit.setFont(QFont('Calibri Light',64))
        self.title_edit.setAlignment(Qt.AlignLeft)
        self.title_edit.setMaxLength(30)
        self.title_edit.setMinimumSize(self.title_edit.minimumSizeHint().width()*1.2,self.title_edit.minimumSizeHint().height()*0.9)
        self.title_edit.editingFinished.connect(self.editTitle)
        self.title_edit.editingFinished.connect(self.sendTitle)

        self.editing_title = False
        self.finishing_title = False
        self.grad_position = 0.0
        self.title_layout.addWidget(self.title_edit)
        self.title_layout.addWidget(self.title_lbl)
        self.title_layout.setSpacing(0)

        # Create start button for gatorscribe
        self.gatorscribe_start = QPushButton('Start')
        self.gatorscribe_start.setFont(QFont('Calibri',14))
        self.gatorscribe_start.setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent')
        self.gatorscribe_start.setMinimumSize(self.gatorscribe_start.minimumSizeHint().width()*2,self.gatorscribe_start.minimumSizeHint().height())
        self.gatorscribe_start.pressed.connect(lambda: self.gatorscribe_start.setStyleSheet('background-color: rgb(192,192,192,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))
        self.gatorscribe_start.released.connect(lambda: self.gatorscribe_start.setStyleSheet('background-color: rgb(127,127,127,127); border-width: 2px; border-style: outset; border-radius: 5px; padding: 2px; border-color: transparent'))

        # Create horizontal menu
        self.horizontal_menu.addLayout(self.title_layout)
        self.horizontal_menu.addWidget(self.gatorscribe_start)
        self.horizontal_menu.setAlignment(self.gatorscribe_start,Qt.AlignLeft)
        self.horizontal_menu.addStretch()

        self.instrument_index = 1

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

    # Create function to cycle through instrument options when instrument symbol is clicked
    def cycleInstrument(self):
        if self.instrument_index < 5:
            self.instrument_index = self.instrument_index + 1
        else:
            self.instrument_index = 0
        self.instrument_pic.setIcon(QIcon(self.instrument_list[self.instrument_index]))

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
        button_width = self.harmonizer_start.x()+self.harmonizer_start.width()-self.dev_search.x()
        width = self.dev_lbl.width()
        if button_width > width:
            width = button_width
        top_rect = QRect(self.midi_lbl.x()*0.95,self.midi_lbl.y()*0.99,self.midi_lbl.width()*1.1,self.midi_lbl.height()*1.2)
        paint.drawRect(top_rect)
        paint.drawRect(top_rect.x() + top_rect.width(),top_rect.y() + (top_rect.height()-rect_width),self.dev_lbl.x() + self.dev_lbl.width() - (top_rect.x() + top_rect.width()),rect_width)
        title_rect = QRect(self.title_lbl.x()+self.title_lbl.width(),self.title_lbl.y(),(self.gatorscribe_start.x()+self.gatorscribe_start.width())-(self.title_lbl.x()+self.title_lbl.width()),rect_width/2)
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
            paint.drawRect(QRect(self.key_lbl.x()-(rect_width*3),self.key_edit.y(),rect_width,self.key_lbl.y()-self.key_edit.y()+self.key_lbl.height()))
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
            QTest.qWait(20)
            self.fadeOptions(self.options_layout)
            self.opacity = self.opacity - 0.1

    # Fade in function that calls fadeOptions for increasingly larger alpha
    def fadeIn(self):
        self.opacity = 0.0
        while self.opacity <= 0.9:
            QTest.qWait(20)
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
        self.time_lbl.setFont(QFont('Calibri Light',14))  # Edit label font
        self.time_lbl.setMaximumSize(self.time_lbl.minimumSizeHint().width(),self.time_lbl.height())
        self.time_button = QPushButton(self.time_list[self.time_index])
        self.time_button.setStyleSheet(self.remove_background)
        self.time_button.setFont(QFont('Calibri Light',96))
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
        self.key_lbl.setFont(QFont('Calibri Light',14))  # Edit label font
        self.key_lbl.setAlignment(Qt.AlignLeft)
        self.key_edit = QLineEdit()
        self.key_edit.setStyleSheet(self.remove_background)
        self.key_edit.setFont(QFont('Calibri Light',96))
        self.key_edit.setAlignment(Qt.AlignLeft)
        self.key_edit.setMaxLength(3)
        self.key_edit.setMaximumSize(self.key_edit.minimumSizeHint().width()*1.2,self.key_edit.minimumSizeHint().height()*0.9)
        self.key_edit.setText('CM')
        self.key_edit.editingFinished.connect(self.sendKey)
        self.key_layout.addWidget(self.key_edit)
        self.key_layout.addWidget(self.key_lbl)
        self.key_layout.setAlignment(self.key_edit,Qt.AlignLeft)
        self.key_layout.setAlignment(self.key_lbl,Qt.AlignLeft)
        self.key_layout.setSpacing(0)

         # Create tempo layout
        self.tempo_layout = QVBoxLayout()
        self.tempo_lbl = QLabel('BPM')
        self.tempo_lbl.setFont(QFont('Calibri Light',14))  # Edit label font
        self.tempo_lbl.setStyleSheet('color: rgb(170,179,188)')
        self.tempo_lbl.setMaximumSize(self.tempo_lbl.minimumSizeHint().width(),self.tempo_lbl.height())
        self.tempo_edit = QLineEdit()
        self.tempo_edit.setStyleSheet(self.remove_background)
        self.tempo_edit.setFont(QFont('Calibri Light',96))
        self.tempo_edit.setAlignment(Qt.AlignLeft)
        self.tempo_edit.setMaxLength(3)
        self.tempo_edit.setMaximumSize(self.tempo_edit.minimumSizeHint().width()*1.2,self.tempo_edit.minimumSizeHint().height()*0.9)
        self.tempo_edit.setText('100')
        self.tempo_edit.editingFinished.connect(self.sendTempo)
        self.tempo_layout.addWidget(self.tempo_edit)
        self.tempo_layout.addWidget(self.tempo_lbl)
        self.tempo_layout.setAlignment(self.tempo_lbl,Qt.AlignLeft)
        self.tempo_layout.setAlignment(self.tempo_edit,Qt.AlignLeft)
        self.tempo_layout.setSpacing(0)

        self.instrument_list = ['guitar.png','piano.png','violin.png','trumpet.png','synth.png','sax.png']


        # Create instrument layout
        self.instrument_layout = QVBoxLayout()
        self.instrument_lbl = QLabel('Instrument')
        self.instrument_lbl.setFont(QFont('Calibri Light',14))  # Edit label font
        self.instrument_lbl.setStyleSheet('color: rgb(170,179,188)')
        self.instrument_lbl.setAlignment(Qt.AlignLeft)
        self.instrument_pic = QPushButton()
        self.instrument_pic.setStyleSheet(self.remove_background)
        self.instrument_pic.setIcon(QIcon(self.instrument_list[self.instrument_index]))
        self.instrument_pic.setIconSize(QSize(self.key_edit.height(),self.key_edit.height()))
        self.instrument_pic.clicked.connect(lambda: self.constructInstrumentMenu())
        self.instrument_layout.addWidget(self.instrument_pic)
        self.instrument_layout.addWidget(self.instrument_lbl)
        self.instrument_layout.setAlignment(self.instrument_pic,Qt.AlignLeft)
        self.instrument_layout.setAlignment(self.instrument_lbl,Qt.AlignLeft)
        self.instrument_layout.setSpacing(0)

    def initTimeMenu(self):

        self.button_44 = QPushButton('4/4')
        self.button_44.setStyleSheet(self.remove_background)
        self.button_44.setFont(QFont('Calibri Light',96))
        self.button_44.setMaximumSize(self.button_44.minimumSizeHint().width(),self.button_44.minimumSizeHint().height())
        self.button_44.clicked.connect(lambda: self.sendTime(0))

        self.button_34 = QPushButton('3/4')
        self.button_34.setStyleSheet(self.remove_background)
        self.button_34.setFont(QFont('Calibri Light',96))
        self.button_34.setMaximumSize(self.time_button.width(),self.time_button.height())
        self.button_34.clicked.connect(lambda: self.sendTime(1))

        self.button_24 = QPushButton('2/4')
        self.button_24.setStyleSheet(self.remove_background)
        self.button_24.setFont(QFont('Calibri Light',96))
        self.button_24.setMaximumSize(self.time_button.width(),self.time_button.height())
        self.button_24.clicked.connect(lambda: self.sendTime(2))

        self.button_68 = QPushButton('6/8')
        self.button_68.setStyleSheet(self.remove_background)
        self.button_68.setFont(QFont('Calibri Light',96))
        self.button_68.setMaximumSize(self.time_button.width(),self.time_button.height())
        self.button_68.clicked.connect(lambda: self.sendTime(3))

    def initInstrumentMenu(self):
        self.guitar = QPushButton()
        self.guitar.setStyleSheet(self.remove_background)
        self.guitar.setIcon(QIcon('guitar.png'))
        self.guitar.setIconSize(QSize(self.dev_lbl.height(),self.dev_lbl.height()))
        self.guitar.setSizePolicy(QSizePolicy.Preferred,QSizePolicy.Expanding)
        self.guitar.clicked.connect(lambda: self.sendInstrument(27,0))

        self.piano = QPushButton()
        self.piano.setStyleSheet(self.remove_background)
        self.piano.setIcon(QIcon('piano.png'))
        self.piano.setIconSize(QSize(self.dev_lbl.height(),self.dev_lbl.height()))
        self.piano.setSizePolicy(QSizePolicy.Preferred,QSizePolicy.Expanding)
        self.piano.clicked.connect(lambda: self.sendInstrument(0,1))

        self.violin = QPushButton()
        self.violin.setStyleSheet(self.remove_background)
        self.violin.setIcon(QIcon('violin.png'))
        self.violin.setIconSize(QSize(self.dev_lbl.height(),self.dev_lbl.height()))
        self.violin.setSizePolicy(QSizePolicy.Preferred,QSizePolicy.Expanding)
        self.violin.clicked.connect(lambda: self.sendInstrument(40,2))

        self.trumpet = QPushButton()
        self.trumpet.setStyleSheet(self.remove_background)
        self.trumpet.setIcon(QIcon('trumpet.png'))
        self.trumpet.setIconSize(QSize(self.dev_lbl.height(),self.dev_lbl.height()))
        self.trumpet.setSizePolicy(QSizePolicy.Preferred,QSizePolicy.Expanding)
        self.trumpet.clicked.connect(lambda: self.sendInstrument(56,3))

        self.synth = QPushButton()
        self.synth.setStyleSheet(self.remove_background)
        self.synth.setIcon(QIcon('synth.png'))
        self.synth.setIconSize(QSize(self.dev_lbl.height(),self.dev_lbl.height()))
        self.synth.setSizePolicy(QSizePolicy.Preferred,QSizePolicy.Expanding)
        self.synth.clicked.connect(lambda: self.sendInstrument(80,4))

        self.sax = QPushButton()
        self.sax.setStyleSheet(self.remove_background)
        self.sax.setIcon(QIcon('sax.png'))
        self.sax.setIconSize(QSize(self.dev_lbl.height(),self.dev_lbl.height()))
        self.sax.setSizePolicy(QSizePolicy.Preferred,QSizePolicy.Expanding)
        self.sax.clicked.connect(lambda: self.sendInstrument(64,5))

    def constructMainMenu(self):
        self.fadeOut()
        self.clearGridLayout()

        self.time_button.setText(self.time_list[self.time_index])
        self.instrument_pic.setIcon(QIcon(self.instrument_list[self.instrument_index]))
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
            self.ser = serial.Serial(port='/dev/tty.usbmodem1462', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)
        elif (os.path.exists("/dev/tty.usbmodem1442")):
            self.ser = serial.Serial(port='/dev/tty.usbmodem1442', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)
        elif (os.path.exists("/dev/tty.usbserial-A904RDA3")):
            self.ser = serial.Serial(port='/dev/tty.usbserial-A904RDA3', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)
        else:
            self.ser = serial.Serial(baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)
            print("No board connected")
            return
        try:
            self.ser.isOpen()
            print("Serial port is open")
        except:
            print("Error")
            self.harmonizer_start.setText('Start')
            return

    def sendTitle(self):
        #Send title flag of 255
        if self.ser.isOpen():
            if self.title_edit.text() is self.past_title:
                return
            else:
                self.ser.write([255])
                self.ser.write([self.title_edit.text()])
                self.past_title = self.title_edit.text()
        else:
            self.connectBoard()
            print('Cannot send title ' + self.title_edit.text() + ' | Board not connected')

    def sendTime(self,number):
        #Send time flag of 254
        self.time_index = number
        if self.ser.isOpen():
            if number is self.past_time:
                self.constructMainMenu()
                return
            else:
                self.ser.write([254])
                self.ser.write([number])
                self.past_time = number
        else:
            self.connectBoard()
            print('Cannot send time ' + self.time_list[number] + ' | Board not connected')
        self.constructMainMenu()



    def sendKey(self):
        # Send key flag of 253
        if self.ser.isOpen():
            if self.key_edit.text() is self.past_key:
                return
            else:
                self.ser.write([254])

                # Update -----------------------------------------
                self.ser.write([self.key_edit.text()])
                self.past_key = self.key_edit.text()
        else:
            self.connectBoard()
            print('Cannot send key ' + self.key_edit.text() + ' | Board not connected')

    def sendTempo(self):
        # Send key flag of 252
        if self.ser.isOpen():
            if self.tempo_edit.text() is self.past_tempo:
                return
            else:
                self.ser.write([251])

                # Update --------------------------------------------
                self.ser.write([self.tempo_edit.text()])
                self.past_tempo = self.tempo_edit.text()
        else:
            self.connectBoard()
            print('Cannot send tempo ' + self.tempo_edit.text() + ' | Board not connected')

    def sendInstrument(self,number,index):
        self.instrument_index = index
        # Send key flag of 251
        if self.ser.isOpen():
            if number is self.past_instrument:
                self.constructMainMenu()
                return
            else:
                self.ser.write([251])
                self.ser.write([number])
                self.past_instrument = number
        else:
            self.connectBoard()
            print('Cannot send instrument ' + self.instrument_pic.icon().name() + ' | Board not connected')
        self.constructMainMenu()

    def startHarmonizer(self):
        if self.harmonizer_start.text() is 'Start':
            self.harmonizer_start.setText('Stop')
            notes = list([ ])
            self.connectBoard

            note_on = 144
            note_off = 128
            volume = 176
            harmony_ch = 77
            master_ch = 76
            autotune_button = 82
            pitch_bend = 224

            try:
                with mido.open_input('Oxygen 49') as port:
                    print('Using {}'.format(port))
                    print('Waiting for messages...')
                    for message in port:
                        if (message.bytes()[0] == 144):
                            # add note to array to send
                            notes.append(message.bytes()[1])
                            for k in range(len(notes)):
                                self.ser.write([notes[k]])
                            self.ser.write([0]) # null terminated

                        elif (message.bytes()[0] == 128):
                            # remove note from array to send
                            notes.remove(message.bytes()[1])
                            for k in range(len(notes)):
                                self.ser.write([notes[k]])
                            self.ser.write([0]) # null terminated
                        elif (message.bytes()[0] == volume and message.bytes()[1] == harmony_ch):
                            self.ser.write([255])
                            self.ser.write([message.bytes()[2]])
                        elif (message.bytes()[0] == volume and message.bytes()[1] == master_ch):
                            self.ser.write([254])
                            self.ser.write([message.bytes()[2]])
                        elif (message.bytes()[0] == pitch_bend):
                            self.ser.write([253])
                            self.ser.write([message.bytes()[2]])
                        elif (message.bytes()[1] == autotune_button):
                            self.ser.write([252])

                        #print(notes)
                        print(message.bytes())

                        sys.stdout.flush()
            except KeyboardInterrupt:
                pass
        else:
            self.harmonizer_start.setText('Start')
            self.ser.close()

    def startGatorscribe(self):
        if (os.path.exists("/dev/tty.usbmodem1462")):
            self.ser = serial.Serial(port='/dev/tty.usbmodem1462', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)
        elif (os.path.exists("/dev/tty.usbmodem1442")):
            self.ser = serial.Serial(port='/dev/tty.usbmodem1442', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)
        elif (os.path.exists("/dev/tty.usbserial-A904RDA3")):
            self.ser = serial.Serial(port='/dev/tty.usbserial-A904RDA3', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)
        else:
            print("No board connected")
            self.harmonizer_start.setText('Start')
            return
        try:
            self.ser.isOpen()
            print("Serial port is open")
        except:
            print("Error")
            return

        midi_msg = "00"
        if(self.ser.isOpen()):
            try:
                while(1):
                    msg = self.ser.read().decode('ascii')
                    if (msg == '-'):
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

                        x.tofile('my_song.mid') # simply name of file

            except Exception:
                print("Error")
        else:
            print("Cannot open serial port")

app = QApplication(sys.argv)
ex = App()
ex.setWindowOpacity(0.95)
sys.exit(app.exec_())
