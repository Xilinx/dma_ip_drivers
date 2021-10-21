#!/usr/bin/env python3
# vim: sta:et:sw=4:ts=4:sts=4
#
# https://www.pythonguis.com/tutorials/qprocess-external-programs/
# https://stackoverflow.com/questions/35056635/how-to-update-a-realtime-plot-and-use-buttons-to-interact-in-pyqtgraph
# https://stackoverflow.com/questions/56918912/how-to-enable-legends-and-change-style-in-pyqtgraph
import sys
# from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5 import QtWidgets
from PyQt5.QtWidgets import QMainWindow, QWidget, QPushButton, QAction, \
        QPlainTextEdit

from PyQt5.QtCore import QProcess

# from PyQt5 import QtGui, QtCore
from PyQt5.QtGui import QIcon
import numpy as np
import pyqtgraph as pg

# FILENAME = 'data/out_32.bin'
FILENAME = 'data/out_fmc.bin'
NUMCHANNELS = 3

SCALE_32 = 1
from PyQt5.QtWidgets import QMainWindow, QWidget, QPushButton, QAction,QPlainTextEdit
DECIMATION = 20


class estherTrig(QtWidgets.QWidget):
    def __init__(self):
        pg.setConfigOption('background', 'w')  # before loading widget
        super(estherTrig, self).__init__()
        # QtWidgets.QWidget.__init__(self)
        self.init_ui()
        self.p = None
        self.qt_connections()

        # self.amplitude = 10
        self.t = 0
        self.updateplot()

        # self.timer = pg.QtCore.QTimer()
        # self.timer.timeout.connect(self.moveplot)
        # self.timer.start(500)

    def init_ui(self):

        self.setWindowTitle('ADC FMC Data')
        hbox = QtWidgets.QHBoxLayout()
        vbox1 = QtWidgets.QVBoxLayout()
        # vbox1.addStretch(1) #  add the stretch
        # vbox1.setFixedWidth(50)
        vbox2 = QtWidgets.QVBoxLayout()
        self.pw = pg.PlotWidget()
        self.pw.setWindowTitle('pyqtgraph example: MultiplePlotAxes')
        self.p1 = self.pw.plotItem
        self.p1.setLabels(left='axis 1')
        # call plt.addLegend() BEFORE you create the curves.
        self.pw.addLegend()
        self.pw.show()
        # create a new ViewBox, link the right axis to its coordinate system
        self.p2 = pg.ViewBox()
        self.p1.showAxis('right')
        self.p1.scene().addItem(self.p2)
        self.p1.getAxis('right').linkToView(self.p2)
        self.p2.setXLink(self.p1)
        self.p1.getAxis('right').setLabel('axis2', color='b')  # '#0000ff')

        self.plotCurves = [0]*NUMCHANNELS
        for i in range(NUMCHANNELS):
            self.plotCurves[i] = pg.PlotDataItem(
                [0], [0], pen=pg.mkPen(i, width=2), name="ch{}".format(i))
            self.p1.addItem(self.plotCurves[i])
        # self.pw.setLabels(
        #    title='ADC FMC data', bottom='Sample', left='LSB', right='Count')
        self.pw.setYRange(-32768, 32768, padding=0.01)
        self.curveCnt = pg.PlotCurveItem(
            [0], [0], pen=pg.mkPen(color='#025b94', width=3), name="count")
        # self.p1.setLabels(left='axis 1')

        # self.p2.setYRange(-1, 5)

        # self.pw.setLabels(
        # title='Window 1', bottom='X Axis', left='LSB', right='Count')

        self.p2.addItem(self.curveCnt)
        # self.p2.addItem(self.plotCurves[0])

        vbox2.addWidget(self.pw)
        self.clearbutton = QPushButton("Clear Plots")
        # self.decreasebutton = QtWidgets.QPushButton("Decrease Amplitude")
        hboxD = QtWidgets.QHBoxLayout()
        decLabel = QtWidgets.QLabel("Decimation")
        self.line_editDec = QtWidgets.QLineEdit("10", self)
        self.line_editDec.resize(100, 32)
        hboxD.addWidget(decLabel)
        # hbox1.addWidget(QtWidgets.QLabel("Decimation "))
        hboxD.addWidget(self.line_editDec)
        hbox.addLayout(vbox1)
        self.plot16Button = QPushButton("Plot Data")
        # self.plot16dButton = QtWidgets.QPushButton("Plot data 16 D bit")
        # self.plot32Button = QtWidgets.QPushButton(",Plot data 32 bit")
        self.line_edit = QtWidgets.QLineEdit(FILENAME, self)
        # self.line_edit32 = QtWidgets.QLineEdit(FILENAME32, self)
        # vbox.addWidget(self.decreasebutton)#
        vbox2.addWidget(self.line_edit)
        # vbox2.addWidget(self.line_edit32)
        # butt = QtWidgets.QPushButton("Increa1") butt.setSizePolicy(
        #      QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.MinimumExpanding,
#            QtWidgets.QSizePolicy.MinimumExpanding))
        self.curveCnt = pg.PlotDataItem(
            [0], [0], pen=pg.mkPen(color='#025b94', width=1))
        sizePolicy = QtWidgets.QSizePolicy(
            QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        self.clearbutton.setSizePolicy(sizePolicy)
        # vbox1.addWidget(butt)#
        vbox1.addWidget(self.clearbutton)
        verticalSpacer = QtWidgets.QSpacerItem(
            20, 40, QtWidgets.QSizePolicy.Minimum,
            QtWidgets.QSizePolicy.Expanding)

        self.btn = QPushButton("Execute")
        self.text = QPlainTextEdit()
        self.text.setReadOnly(True)

        vbox1.addWidget(self.btn)
        vbox1.addWidget(self.text)
        vbox1.addItem(verticalSpacer)
        fbox = QtWidgets.QFormLayout()
        self.atlevelHigh = QtWidgets.QLineEdit("9000")
        self.atlevelLow = QtWidgets.QLineEdit("-9000")
        vbox3 = QtWidgets.QVBoxLayout()
        vbox3.addWidget(self.atlevelHigh)
        vbox3.addWidget(self.atlevelLow)
        fbox.addRow(QtWidgets.QLabel("A Level"), vbox3)
        vbox1.addLayout(fbox)

        self.btlevelHigh = QtWidgets.QLineEdit("9000")
        self.btlevelLow = QtWidgets.QLineEdit("-9000")
        fbox = QtWidgets.QFormLayout()
        vbox3 = QtWidgets.QVBoxLayout()
        vbox3.addWidget(self.btlevelHigh)
        vbox3.addWidget(self.btlevelLow)
        fbox.addRow(QtWidgets.QLabel("B Level"), vbox3)
        vbox1.addLayout(fbox)

        self.ctlevelHigh = QtWidgets.QLineEdit("9000")
        self.btn.pressed.connect(self.start_process)
        self.ctlevelLow = QtWidgets.QLineEdit("-9000")
        fbox = QtWidgets.QFormLayout()
        vbox3 = QtWidgets.QVBoxLayout()
        vbox3.addWidget(self.ctlevelHigh)
        vbox3.addWidget(self.ctlevelLow)
        self.btn.pressed.connect(self.start_process)
        fbox.addRow(QtWidgets.QLabel("C Level"), vbox3)
        vbox1.addLayout(fbox)

        vbox1.addItem(verticalSpacer)
        vbox1.addLayout(hboxD)
        vbox1.addWidget(self.plot16Button)
        # vbox1.addWidget(self.plot16dButton)
        # vbox1.addWidget(self.plot32Button)#
        self.btn.pressed.connect(self.start_process)
        self.plot16Button.setSizePolicy(sizePolicy)
        # self.plot16dButton.setSizePolicy(sizePolicy)
        # self.plot32Button.setSizePolicy(sizePolicy)
        # hbox.addLayout(vbox1, 1)
        # hbox.addStretch()
        line = QtWidgets.QFrame(self)
        # line.setObjectName(QtWidgets.QStringLiteral("line"))
        line.setObjectName("line")
        line.setFrameShape(QtWidgets.QFrame.VLine)
        line.setFrameShadow(QtWidgets.QFrame.Sunken)

        hbox.addWidget(line)
# verticalLayout->addWidget(line)

        hbox.addLayout(vbox2, 3)
        self.setLayout(hbox)
        self.setGeometry(10, 10, 1000, 600)

        # # Create exit action
        # exitAction = QAction(QIcon('exit.png'), '&Exit', self)
        # exitAction.setShortcut('Ctrl+Q')
        # exitAction.setStatusTip('Exit application')
        # exitAction.triggered.connect(self.exitCall)
        self.curveCnt = pg.PlotDataItem(
            [0], [0], pen=pg.mkPen(color='#025b94', width=1))

        # # Create menu bar and add action
        # menuBar = self.menuBar()
        # fileMenu = menuBar.addMenu('&File')
        # # fileMenu.addAction(newAction)
        # # fileMenu.addAction(openAction)
        # fileMenu.addAction(exitAction)

        self.show()

    def qt_connections(self):
        self.clearbutton.clicked.connect(self.on_clearbutton_clicked)
        # self.decreasebutton.clicked.connect(self.on_decreasebutton_clicked)
        self.plot16Button.clicked.connect(self.on_plot16Button_clicked)
        self.btn.pressed.connect(self.start_process)
        # self.plot16dButton.clicked.connect(self.on_plot16dButton_clicked)
        # self.plot32Button.clicked.connect(self.on_plot32Button_clicked)
        # adding action to the line edit when enter key is pressed
        # line_edit.returnPressed.connect(lambda: do_action())
        # call plt.addLegend() BEFORE you create the curves.

    def message(self, s):
        self.text.appendPlainText(s)

    def start_process(self):
        if self.p is None:  # No process running.
            self.message("Executing process")
            self.p = QProcess()  # Keep a reference to the QProcess (e.g. on self) while it's running.
            self.p.readyReadStandardOutput.connect(self.handle_stdout)
            self.p.readyReadStandardError.connect(self.handle_stderr)
            self.p.stateChanged.connect(self.handle_state)
            self.p.finished.connect(self.process_finished)  # Clean up once complete.
            self.p.start("python3", ['dummy_script.py'])

    def handle_stderr(self):
        data = self.p.readAllStandardError()
        stderr = bytes(data).decode("utf8") 
        self.message(stderr)

    def handle_stdout(self):
        data = self.p.readAllStandardOutput()
        stdout = bytes(data).decode("utf8")
        self.message(stdout)

    def handle_state(self, state):
        states = {
            QProcess.NotRunning: 'Not running',
            QProcess.Starting: 'Starting',
            QProcess.Running: 'Running',
        }
        state_name = states[state]
        self.message(f"State changed: {state_name}")

    def process_finished(self):
        self.message("Process finished.")
        self.p = None 

    def moveplot(self):
        self.t += 1
        self.updateplot()

    def updateplot(self):
        print("Update")
        # data1 = self.amplitude*np.sin(np.linspace(0,30,121)+self.t)
        # self.plotcurve.setData(data1)

    def clear(self):
        for i in range(NUMCHANNELS):
            self.plotCurves[i].setData([])
        self.curveCnt.clear()

    def on_clearbutton_clicked(self):
        print("clear")
        self.clear()

    # def on_decreasebutton_clicked(self):
        # print ("Amplitude decreased")
        # self.amplitude -= 1
        # self.updateplot()

    def on_plot16Button_clicked(self):
        print ("ploting 16")
        self.draw16()

    # def on_plot32Button_clicked(self):
        # # print ("Amplitude decreased")
        # self.draw32()

    def draw16(self):
        filename = self.line_edit.text()
        data = np.fromfile(filename, dtype='int16')
        dataC = np.fromfile(filename, dtype='uint32')
# ‘F’ means to readthe elements using Fortran-like index order,
# with the first index changing fastest,
        data_mat = np.reshape(data, (8, - 1), order='F')
        data_cnt = np.reshape(dataC, (4, -1), order='F')
        x = np.arange(data_mat.shape[1])
        self.pw.setYRange(-9000, 9000, padding=0.01)
        # self.pw.setLabels(
        # title='16 bit Data', bottom='Sample', left='LSB', right='Count')
        # self.plotcurve2.setData(data_mat[1])
        for i in range(NUMCHANNELS):
            self.plotCurves[i].clear()
            self.plotCurves[i].setData(x*2, data_mat[i*2])
            self.plotCurves[i].setDownsampling(
                ds=DECIMATION, auto=None, method='subsample')
        self.curveCnt.clear()
        #self.p2.setYRange(-161310185, 161509684, padding=0.01)
        x = np.arange(data_cnt.shape[1])
        self.curveCnt.setData(x*2, data_cnt[3])
        print(data_cnt[3])
        print(x)
        # self.curveCnt.setDownsampling(
            # ds=DECIMATION, auto=None, method='subsample')
        self.updateViews()

    # def draw32(self):
        # filename = self.line_edit32.text()
        # data = np.fromfile(filename, dtype='int32')
        # data_mat = np.reshape(data, (32, -1), order='F')
        # self.plotwidget.setLabels(
        # title='32 bit Data', bottom='Sample', left='LSB', right='Chopp')
        # # data_u64 = np.fromfile(filename, dtype='u8')
        # # data_u64 = np.reshape(data_u64, (8, -1), order='F')
        # self.plotwidget.setYRange(
        # -32768 * 4, 32768 * 4, padding=0.01)
        # for i in range(NUMCHANNELS):
        # self.plotCurves[i].clear()
        # self.plotCurves[i].setData(da,ta_mat[i]/SCALE_32)
        # self.curveChp.setData((data_mat[31] & 0x0001))
        # self.curveChp.setDownsampling(
        # ds=DECIMATION, auto=None, method='subsample')
        # self.updateViews()

# Handle view resizing
    def updateViews(self):
        #  view has resized; update auxiliary views to match
        self.p2.setGeometry(self.p1.vb.sceneBoundingRect())
        # need to re-update linked axes since this was called
        # incorrectly while views had different shapes.
        # (probably this should be handled in ViewBox.resizeEvent)
        self.p2.linkedViewChanged(self.p1.vb, self.p2.XAxis)

    def exitCall(self):
        print('Exit app')


def main():
    app = QtWidgets.QApplication(sys.argv)
    app.setApplicationName('Esther Data 16 bit')
    myapp = estherTrig()
    # myapp.show()
    sys.exit(app.exec_())


if __name__ == '__main__':
    main()
