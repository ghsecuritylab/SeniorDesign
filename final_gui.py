import sys
import mido
import serial
import os
import numpy as np
from PyQt5.QtWidgets import QMainWindow,QApplication,QPushButton,QWidget,QAction,QTabWidget,QVBoxLayout,QHBoxLayout,QLabel,QComboBox,QSlider,QTextEdit,QLineEdit,QGridLayout,QDesktopWidget
from PyQt5.QtGui import QIcon,QColor,QPalette,QFont,QPixmap,QPainter,QPen,QBrush
from PyQt5.QtCore import pyqtSlot,Qt,QSize,QRect

class App(QMainWindow):

    def __init__(self):
        super().__init__()
        self.title = 'Scribes'
        self.setWindowTitle(self.title)
        self.init_menu_bar()
        self.p = self.palette()
        self.setStyleSheet('background-color: rgb(42,41,46); color: rgb(200,209,218)')
        self.central_widget = Central(self)
        self.setCentralWidget(self.central_widget)
        self.showMaximized()
        self.show()

    def init_menu_bar(self):

        # Create menu bar with roots
        menu_bar = self.menuBar() # Create new menu bar
        file = menu_bar.addMenu('File')   # Create new root menu in menu bar
        edit = menu_bar.addMenu('Edit')   # Create new root menu in menu bar

        # Create actions for file root
        new_action = QAction('New',self) # Create new action
        new_action.setShortcut('Ctrl+N')   # Create keyboard shortcut for action
        save_action = QAction('Save',self)   # Create new action
        save_action.setShortcut('Ctrl+S')   # Create keyboard shortcut for action
        quit_action = QAction('Quit',self)   # Create new action
        quit_action.setShortcut('Ctrl+Q')    # Create keyboard shortcut for action

        # Add actions to file root
        file.addAction(save_action) # Add save action to file root
        file.addAction(new_action) # Add new action to file root
        file.addAction(quit_action) # Add quit action to file root

        # Create actions for edit root
        find_action = QAction('Find...',self)  # Create new action
        find_action.setShortcut('Ctrl+F')   # Create keyboard shortcut for action
        replace_action = QAction('Replace...',self)    # Create replace action
        replace_action.setShortcut('Ctrl+R')    # Create keyboard shortcut for action
        find_menu = edit.addMenu('Find')    # Create new sub-menu in edit root
        find_menu.addAction(find_action)    # Add find action to find menu
        find_menu.addAction(replace_action)    # Add replace action to find menu

        # Menu Bar Events
        quit_action.triggered.connect(lambda: sys.exit())    # Exit program when action is taken
        file.triggered.connect(self.selected)

    def selected(self, q):
        print(q.text() + " selected")

class Central(QWidget):

    def __init__(self, parent):
        super().__init__(parent)
        self.layout = QVBoxLayout(self)
        self.remove_background = 'background-color: rgb(0,0,0,0); border-width: 0px; border-radius: 0px;'
        self.format_button = 'background-color: rgb(247,247,247);'
        self.p = self.palette()
        self.p.setColor(self.backgroundRole(), QColor(127,255,127))
        self.init_harmonizer()
        self.init_gatorscribe()
        self.layout.addLayout(self.harmonizer_layout)
        self.layout.addLayout(self.gatorscribe_layout)
        self.layout.setContentsMargins(self.window_width/25,self.window_width/50,self.window_height/50,self.window_height/50)
        self.setLayout(self.layout)
        self.ser = serial.Serial(baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)
        self.past_title = self.title_edit.text()
        self.past_time = self.time_edit.text()
        self.past_key = self.key_edit.text()
        self.past_tempo = self.tempo_edit.text()
        self.past_instrument = self.instrument_pic.icon()
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
        self.dev_lbl.setFont(QFont('Calibri Light',48))  # Edit label font
        self.dev_lbl.setMaximumSize(self.dev_lbl.minimumSizeHint().width()*1.2,self.dev_lbl.minimumSizeHint().height()*1.3)
        self.harmonizer_panel.addWidget(self.dev_lbl,0,0,1,2,Qt.AlignLeft)

        # Create label to describe shown device as MIDI device
        self.midi_lbl = QLabel('MIDI Device')
        self.midi_lbl.setFont(QFont("Calibri Light",12))
        self.midi_lbl.setStyleSheet(self.remove_background + '; color: rgb(42,41,46)')
        self.midi_lbl.setMaximumSize(self.midi_lbl.minimumSizeHint().width()*1.2,self.midi_lbl.minimumSizeHint().height()*0.9)
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
        self.window_width = QDesktopWidget().screenGeometry().width()
        self.window_height = QDesktopWidget().screenGeometry().height()
        # Create layout for Gatorscribe
        self.gatorscribe_layout = QVBoxLayout()
        self.gatorscribe_layout.setSpacing(20)
        #self.setStyleSheet('border-width: 2px; border-style: solid; border-radius: 15px; padding: 2px; border-color: rgb(32,32,32)')

        # Create label to mark gatorscribe area
        self.gatorscribe_lbl = QLabel('Transcribe')
        self.gatorscribe_lbl.setFont(QFont('Calibri Light',14))
        self.gatorscribe_lbl.setStyleSheet('background-color: rgb(128,128,128,64); color: rgb(255,255,255)')

        # Create grid layout for options
        self.options_layout = QGridLayout()
        self.options_layout.setVerticalSpacing(25)

        # Create layout for song title
        self.title_layout = QVBoxLayout()
        self.title_lbl = QLabel('Title')
        self.title_lbl.setFont(QFont('Calibri Light',14))
        self.title_edit = QLineEdit('No Excuses')
        self.title_edit.setStyleSheet(self.remove_background)
        self.title_edit.setFont(QFont('Calibri Light',64))
        self.title_edit.setAlignment(Qt.AlignLeft)
        self.title_edit.setMaxLength(16)
        self.title_edit.setMinimumSize(self.title_edit.minimumSizeHint().width()*1.2,self.title_edit.minimumSizeHint().height()*0.9)
        self.title_edit.editingFinished.connect(self.sendTitle)
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
        self.gatorscribe_start.clicked.connect(self.startGatorscribe)


        # Create time signature layout
        self.time_layout = QVBoxLayout()
        self.time_lbl = QLabel('Time Signature')
        self.time_lbl.setFont(QFont('Calibri Light',14))  # Edit label font
        self.time_edit = QLineEdit('4/4')
        self.time_edit.setStyleSheet(self.remove_background)
        self.time_edit.setFont(QFont('Calibri Light',96))
        self.time_edit.setAlignment(Qt.AlignLeft)
        self.time_edit.setMaxLength(3)
        self.time_edit.setMaximumSize(self.time_edit.minimumSizeHint().width()*1.2,self.time_edit.minimumSizeHint().height()*0.9)
        self.time_edit.editingFinished.connect(self.sendTime)
        self.time_layout.addWidget(self.time_edit)
        self.time_layout.addWidget(self.time_lbl)
        self.time_layout.setSpacing(0)

        # Create key signature layout
        self.key_layout = QVBoxLayout()
        self.key_lbl = QLabel('Key Signature')
        self.key_lbl.setFont(QFont('Calibri Light',14))  # Edit label font
        self.key_edit = QLineEdit()
        self.key_edit.setStyleSheet(self.remove_background)
        self.key_edit.setFont(QFont('Calibri Light',96))
        self.key_edit.setAlignment(Qt.AlignLeft)
        self.key_edit.setMaxLength(3)
        self.key_edit.setMaximumSize(self.key_edit.minimumSizeHint().width()*1.2,self.key_edit.minimumSizeHint().height()*0.9)
        self.key_edit.setText('C#')
        self.key_edit.editingFinished.connect(self.sendKey)
        self.key_layout.addWidget(self.key_edit)
        self.key_layout.addWidget(self.key_lbl)
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
        self.tempo_edit.setText('89')
        self.tempo_layout.addWidget(self.tempo_edit)
        self.tempo_layout.addWidget(self.tempo_lbl)
        self.tempo_layout.setSpacing(0)

        self.instrument_list = ['guitar.png','piano.png','violin.png','trumpet.png','keyboard.png','saxophone.png']
        self.instrument_index = 0

        # Create instrument layout
        self.instrument_layout = QVBoxLayout()
        self.instrument_lbl = QLabel('Instrument')
        self.instrument_lbl.setFont(QFont('Calibri Light',14))  # Edit label font
        self.instrument_lbl.setStyleSheet('color: rgb(170,179,188)')
        self.instrument_pic = QPushButton()
        self.instrument_pic.setStyleSheet(self.remove_background + '; border: 0px')
        self.instrument_pic.setIcon(QIcon(self.instrument_list[self.instrument_index]))
        self.instrument_pic.setIconSize(QSize(self.tempo_edit.minimumSizeHint().width(),self.tempo_edit.minimumSizeHint().height()))
        self.instrument_pic.clicked.connect(self.cycleInstrument)
        self.instrument_layout.addWidget(self.instrument_pic)
        self.instrument_layout.addWidget(self.instrument_lbl)
        self.instrument_layout.setSpacing(0)

        # Fill in options grid layout
        self.options_layout.addLayout(self.title_layout,0,0,1,2,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addWidget(self.gatorscribe_start,0,2,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addLayout(self.time_layout,1,0,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addLayout(self.key_layout,1,1,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addLayout(self.tempo_layout,2,0,Qt.AlignLeft | Qt.AlignVCenter)
        self.options_layout.addLayout(self.instrument_layout,2,1,Qt.AlignLeft | Qt.AlignVCenter)
        self.gatorscribe_layout.addLayout(self.options_layout)

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
        self.drawShapes(event, paint)

    def drawShapes(self, event, paint):
        paint.setBrush(QBrush(QColor(50,209,255,127)))
        paint.setPen(Qt.NoPen)
        rect_width = self.key_lbl.width()/32
        vert_rect = QRect(self.dev_lbl.x()-(rect_width*3),self.dev_lbl.y(),rect_width,self.midi_lbl.y()-self.dev_lbl.y()+self.midi_lbl.height())
        paint.drawRect(vert_rect)
        button_width = self.harmonizer_start.x()+self.harmonizer_start.width()-self.dev_search.x()
        width = self.dev_lbl.width()
        if button_width > width:
            width = button_width
        paint.drawRect(QRect(vert_rect.x() + vert_rect.width(),self.midi_lbl.y(),width+(rect_width*3),self.midi_lbl.height()))
        paint.setBrush(QBrush(QColor(50,209,255,127)))
        paint.drawRect(QRect(self.title_lbl.x(),self.title_edit.y() + self.title_edit.height() - (2*rect_width),self.title_edit.width(),rect_width/2))
        paint.drawRect(QRect(self.time_lbl.x()-(rect_width*3),self.time_edit.y(),rect_width,self.time_lbl.y()-self.time_edit.y()+self.time_lbl.height()))
        paint.setBrush(QBrush(QColor(104,216,196,127)))
        paint.drawRect(QRect(self.key_lbl.x()-(rect_width*3),self.key_edit.y(),rect_width,self.key_lbl.y()-self.key_edit.y()+self.key_lbl.height()))
        paint.setBrush(QBrush(QColor(251,165,125,127)))
        paint.drawRect(QRect(self.tempo_lbl.x()-(rect_width*3),self.tempo_edit.y(),rect_width,self.tempo_lbl.y()-self.tempo_edit.y()+self.tempo_lbl.height()))
        paint.setBrush(QBrush(QColor(157,39,89,127)))
        paint.drawRect(QRect(self.instrument_lbl.x()-(rect_width*3),self.instrument_pic.y(),rect_width,self.instrument_lbl.y()-self.instrument_pic.y()+self.instrument_lbl.height()))

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

    def sendTime(self):
        #Send time flag of 254
        if self.ser.isOpen():
            if self.time_edit.text() is self.past_time:
                return
            else:
                self.ser.write([254])
                self.ser.write([self.time_edit.text()])
                self.past_time = self.time_edit.text()
        else:
            self.connectBoard()
            print('Cannot send time ' + self.time_edit.text() + ' | Board not connected')

    def sendKey(self):
        # Send key flag of 253
        if self.ser.isOpen():
            if self.key_edit.text() is self.past_key:
                return
            else:
                self.ser.write([254])
                self.ser.write([self.key_edit.text()])
                self.past_key = self.key_edit.text()
        else:
            self.connectBoard()
            print('Cannot send time ' + self.key_edit.text() + ' | Board not connected')

    def sendInstrument(self):
        # Send key flag of 252
        if self.ser.isOpen():
            if self.instrument_pic.icon() is self.past_instrument:
                return
            else:
                self.ser.write([252])
                self.ser.write([self.key_edit.text()])
                self.past_key = self.key_edit.text()
        else:
            self.connectBoard()
            print('Cannot send time ' + self.key_edit.text() + ' | Board not connected')

    def startHarmonizer(self):
        if self.harmonizer_start.text() is 'Start':
            self.harmonizer_start.setText('Stop')
            notes = list([ ])
            if (os.path.exists("/dev/tty.usbmodem1462")):
                ser = serial.Serial(port='/dev/tty.usbmodem1462', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)
            elif (os.path.exists("/dev/tty.usbmodem1442")):
                ser = serial.Serial(port='/dev/tty.usbmodem1442', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)
            elif (os.path.exists("/dev/tty.usbserial-A904RDA3")):
                ser = serial.Serial(port='/dev/tty.usbserial-A904RDA3', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)
            else:
                print("No board connected")
                self.harmonizer_start.setText('Start')
                return

            try:
                ser.isOpen()
                print("Serial port is open")
            except:
                print("Error")
                self.harmonizer_start.setText('Start')
                return

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
                        elif (message.bytes()[0] == volume and message.bytes()[1] == master_ch):
                            ser.write([254])
                            ser.write([message.bytes()[2]])
                        elif (message.bytes()[0] == pitch_bend):
                            ser.write([253])
                            ser.write([message.bytes()[2]])
                        elif (message.bytes()[1] == autotune_button):
                            ser.write([252])

                        #print(notes)
                        print(message.bytes())

                        sys.stdout.flush()
            except KeyboardInterrupt:
                pass
        else:
            self.harmonizer_start.setText('Start')
            ser.close()

    def startGatorscribe(self):
        if (os.path.exists("/dev/tty.usbmodem1462")):
            ser = serial.Serial(port='/dev/tty.usbmodem1462', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)
        elif (os.path.exists("/dev/tty.usbmodem1442")):
            ser = serial.Serial(port='/dev/tty.usbmodem1442', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)
        elif (os.path.exists("/dev/tty.usbserial-A904RDA3")):
            ser = serial.Serial(port='/dev/tty.usbserial-A904RDA3', baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, timeout=2)
        else:
            print("No board connected")
            self.harmonizer_start.setText('Start')
            return
        try:
            ser.isOpen()
            print("Serial port is open")
        except:
            print("Error")
            return

        midi_msg = "00"
        if(ser.isOpen()):
            try:
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

            except Exception:
                print("Error")
        else:
            print("Cannot open serial port")

app = QApplication(sys.argv)
ex = App()
ex.setWindowOpacity(0.95)
sys.exit(app.exec_())
