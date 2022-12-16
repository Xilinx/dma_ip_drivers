#!/usr/bin/env python3
# vim: sta:et:sw=4:ts=4:sts=4
"""
Plot/Control Esther Trigger System
"""
#
# https://www.pythonguis.com/tutorials/qprocess-external-programs/
# https://stackoverflow.com/questions/35056635/how-to-update-a-realtime-plot-and-use-buttons-to-interact-in-pyqtgraph
# https://stackoverflow.com/questions/56918912/how-to-enable-legends-and-change-style-in-pyqtgraph
import sys

# from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5 import QtWidgets
from PyQt5.QtWidgets import QMainWindow, QWidget, QPushButton, QAction, \
        QPlainTextEdit, QLineEdit, QLabel, QFormLayout, QCheckBox

from PyQt5.QtCore import QProcess

# from PyQt5 import QtGui, QtCore
from PyQt5.QtGui import QIcon
import numpy as np
import pyqtgraph as pg

import re
# A regular expression, to extract the % complete.
#progress_re = re.compile("Total complete: (\d+)%")
#progress_re = re.compile("Delay:\s+[+-]?([0-9]*[.])?[0-9]+ us")
#progress_re = re.compile("Delay:\s+[0-9]*[.][0-9]+\sus")
# progress_re = re.compile("Delay:\s+(\d+).(\d+)\sus")
progress_re = re.compile("Delay:\s+(\d+)")


# FILENAME = 'data/out_32.bin'
FILENAME = 'data/out_fmc.bin'
NUMCHANNELS = 3

ACQ_SIZE    = "0x200000"
DECIMATION = 100

A_LVL_HIGH = "9000"
A_LVL_LOW = "-300"
B_LVL_HIGH = "9000"
B_LVL_LOW = "-500"
C_LVL_HIGH = "9000"
C_LVL_LOW = "-500"

class estherTrig(QWidget):
    def __init__(self):
        pg.setConfigOption('background', 'w')  # before loading widget
        super(estherTrig, self).__init__()
        # QtWidgets.QWidget.__init__(self)
        self.init_ui()
        self.proc = None
        self.qt_connections()

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
        self.p1.setLabels(left='axis 1', bottom='Sample')
        self.p1.showAxis('right')
        # call plt.addLegend() BEFORE you create the curves.
        self.pw.addLegend()
        self.pw.show()
        # create a new ViewBox, link the right axis to its coordinate system
        self.p2 = pg.ViewBox()
        self.p1.scene().addItem(self.p2)
        self.p1.getAxis('right').linkToView(self.p2)
        self.p2.setXLink(self.p1)
        self.p1.getAxis('right').setLabel('Count', color='b')  # '#0000ff')

        self.plotCurves = [0]*NUMCHANNELS
        for i in range(NUMCHANNELS):
            self.plotCurves[i] = pg.PlotDataItem(
                [0], [0], pen=pg.mkPen(i, width=2), name="ch{}".format(i))
            self.p1.addItem(self.plotCurves[i])
        # self.pw.setLabels(
        #    title='ADC FMC data', bottom='Sample', left='LSB', right='Count')
        # self.pw.setYRange(-32768, 32768, padding=0.01)
        self.p1.setYRange(-32768, 32768, padding=0.01)
        self.crvAFPRE = pg.PlotDataItem(
            [], [], pen=pg.mkPen(color='#8A2BE2', width=3), name="afpre") #'blueviolet':
        self.crvAF = pg.PlotDataItem(
            [], [], pen=pg.mkPen(color='#00FFFF', width=3), name="af") # 'aqua'
        self.curveCnt = pg.PlotDataItem(
            [], [], pen=pg.mkPen(color='#025b94', width=3), name="count")

        # Add  infinite line with labels
        inf1 = pg.InfiniteLine(movable=True, angle=90, label='x={value:0.2f}',
                       labelOpts={'position':0.1, 'color': (200,200,100),
                           'fill': (200,200,200,50), 'movable': True})
        inf1.setPos([65536, 2])
        self.p1.addItem(inf1)
        self.p2.addItem(self.curveCnt)
        self.p2.addItem(self.crvAFPRE)
        self.p2.addItem(self.crvAF)

        vbox2.addWidget(self.pw)
        self.clearbutton = QPushButton("Clear Plots")
        # hboxD = QtWidgets.QHBoxLayout()
        # decLabel = QLabel("Decimation")
        # self.line_editDec.resize(100, 32)
        # hboxD.addWidget(decLabel)
        # hbox1.addWidget(QtWidgets.QLabel("Decimation "))
        # hboxD.addWidget(self.line_editDec)
        hbox.addLayout(vbox1)
        self.plot16Button = QPushButton("Plot Data")
        self.line_edit = QLineEdit(FILENAME, self)
        vbox2.addWidget(self.line_edit)
        # vbox2.addWidget(self.line_edit32)
        sizePolicy = QtWidgets.QSizePolicy(
            QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        self.clearbutton.setSizePolicy(sizePolicy)
        # vbox1.addWidget(butt)#
        vbox1.addWidget(self.clearbutton)
        verticalSpacer = QtWidgets.QSpacerItem(
            20, 40, QtWidgets.QSizePolicy.Minimum,
            QtWidgets.QSizePolicy.Expanding)

        self.btn = QPushButton("Execute")
        vbox1.addWidget(self.btn)
        self.text = QPlainTextEdit()
        self.text.setReadOnly(True)
        vbox1.addWidget(self.text)
        self.textErr = QPlainTextEdit()
        self.textErr.setReadOnly(True)
        vbox1.addWidget(self.textErr)
        vbox1.addItem(verticalSpacer)
        fbox = QtWidgets.QFormLayout()
        self.atlevelHigh = QLineEdit(A_LVL_HIGH)
        self.atlevelLow = QLineEdit(A_LVL_LOW)
        vbox = QtWidgets.QVBoxLayout()
        vbox.addWidget(self.atlevelHigh)
        vbox.addWidget(self.atlevelLow)
        fbox.addRow(QLabel("A Level"), vbox)
        # vbox1.addLayout(fbox)

        self.btlevelHigh = QLineEdit("500")
        self.btlevelLow = QLineEdit("-9000")
        vbox = QtWidgets.QVBoxLayout()
        vbox.addWidget(self.btlevelHigh)
        vbox.addWidget(self.btlevelLow)
        fbox.addRow(QLabel("B Level"), vbox)
        # vbox1.addLayout(fbox)

        self.ctlevelHigh = QLineEdit("9000")
        self.ctlevelLow = QLineEdit("-500")
        # fbox = QtWidgets.QFormLayout()
        vbox = QtWidgets.QVBoxLayout()
        vbox.addWidget(self.ctlevelHigh)
        vbox.addWidget(self.ctlevelLow)
        fbox.addRow(QLabel("C Level"), vbox)
        self.multParamFloat = QLineEdit("3.1")
        fbox.addRow(QLabel("Mult Param"), self.multParamFloat)
        self.offParam = QLineEdit("0x0000")
        fbox.addRow(QLabel("Off Param"), self.offParam)
        self.acqSize = QLineEdit(ACQ_SIZE)
        fbox.addRow(QLabel("Acq Size"), self.acqSize)
        self.line_editDec = QLineEdit(str(DECIMATION), self)
        fbox.addRow(QLabel("Decimation"), self.line_editDec)
        # self.line_Delay = QLineEdit("10.0", self)
        self.line_Delay = QLabel("0.0", self)
        #self.line_Delay.setReadOnly(True)
        fbox.addRow(QLabel("Delay:"), self.line_Delay)
        vbox1.addLayout(fbox)
        self.chB = QCheckBox("Soft trigger")
        self.chB.setChecked(False)

        vbox1.addWidget(self.chB)
        vbox1.addItem(verticalSpacer)
        # vbox1.addLayout(hboxD)
        vbox1.addWidget(self.plot16Button)
        self.plot16Button.setSizePolicy(sizePolicy)
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
        self.setGeometry(10, 10, 1500, 800)

        # # Create exit action
        # exitAction = QAction(QIcon('exit.png'), '&Exit', self)
        # exitAction.setShortcut('Ctrl+Q')
        # exitAction.setStatusTip('Exit application')
        # exitAction.triggered.connect(self.exitCall)
        # self.curveCnt = pg.PlotDataItem(
            # [0], [0], pen=pg.mkPen(color='#025b94', width=1))

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
        self.btn.pressed.connect(self.start_process)
        self.plot16Button.clicked.connect(self.on_plot16Button_clicked)
        # adding action to the line edit when enter key is pressed
        # line_edit.returnPressed.connect(lambda: do_action())

    def message(self, s):
        self.text.appendPlainText(s)

    def messageErr(self, s):
        self.textErr.appendPlainText(s)

    def toHex(self, val, nbits):
        return hex((val + (1 << nbits)) % (1 << nbits))

    def start_process(self):
        if self.proc is None:  # No process running.
            self.message("Executing process")
            self.proc = QProcess()  # Keep a reference to the QProcess (e.g. on self) while it's running.
            self.proc.readyReadStandardOutput.connect(self.handle_stdout)
            self.proc.readyReadStandardError.connect(self.handle_stderr)
            self.proc.stateChanged.connect(self.handle_state)
            self.proc.finished.connect(self.process_finished)  # Clean up once complete.
            High = self.toHex(int(self.atlevelHigh.text()), 16)
            Low = self.toHex(int(self.atlevelLow.text()), 16)
            # arga =' ./estherdaq -a ' + High + Low[2:]
            arga = High + Low[2:]
            High = self.toHex(int(self.btlevelHigh.text()), 16)
            # arga =' -a ' + High + Low[2:]
            Low = self.toHex(int(self.btlevelLow.text()), 16)
            argb = High + Low[2:]
            High = self.toHex(int(self.ctlevelHigh.text()), 16)
            Low = self.toHex(int(self.ctlevelLow.text()), 16)
            #argc ='--ctrigger ' + High + Low[2:]
            argc = High + Low[2:]
            acqsize = self.acqSize.text()

            argm = self.toHex(int(float(self.multParamFloat.text()) * 2**16), 32)

            command ='./estherdaq  ' '-a ' + arga + ' -b '+ argb + ' -c '+  argc + ' -s ' + acqsize + ' -m ' + argm
            # 0x{:04X}{:04X}'.format(High, Low)
            print(command)
            if self.chB.isChecked() == True:
                argT = '-t'
                # print(self.chB.text()+ " is selected")
            else:
                argT = ''
            self.textErr.clear()
            self.proc.start("./estherdaq", ['-a', arga, '-b',  argb, '-c', argc,
                '-s', acqsize, '-m', argm, argT])
        else:
            self.messageErr("Process not finished")


    def handle_stderr(self):
        data = self.proc.readAllStandardError()
        stderr = bytes(data).decode("utf8")
        print('stderr ' + stderr)
        self.messageErr(stderr)

    def handle_stdout(self):
        data = self.proc.readAllStandardOutput()
        stdout = bytes(data).decode("utf8")
        print('stdout ' + stdout)
        m = progress_re.search(stdout)
        # n = progress_re.findall(stdout)
        if m:
            pc_complete = m.group(1)
            tu = m.groups()
            # print( 'found ' + pc_complete)
            valf = float(m.group(1))
            print("Fval: {:.3f}, Vel: {:.3f}:".format(valf,0.30/(valf *8e-9)))
            self.line_Delay.setText(str(8e-3*valf))
            # print( valf )
        else:
            print('not found')
#             int(pc_complete)
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
        self.proc = None

    def moveplot(self):
        self.t += 1
        self.updateplot()

    def updateplot(self):
        pass
#        print("Update")
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
        data_cnt64 = np.reshape(dataC, (4, -1), order='F')
        x = np.arange(data_mat.shape[1])
        self.p1.setYRange(-9000, 9000, padding=0.01)
        # self.pw.setYRange(-9000, 9000, padding=0.01)
        # self.pw.setLabels(
        # title='16 bit Data', bottom='Sample', left='LSB', right='Count')
        # self.plotcurve2.setData(data_mat[1])
        for i in range(NUMCHANNELS):
            self.plotCurves[i].clear()
            self.plotCurves[i].setData(x*2, data_mat[i*2])
            self.plotCurves[i].setDownsampling(
                ds=DECIMATION, auto=None, method='subsample')
        self.p2.setYRange(-32768, 32768, padding=0.01)
        self.curveCnt.clear()
        #self.p2.setYRange(-161310185, 161509684, padding=0.01)
#        x = np.arange(data_cnt64.shape[1])
        #self.curveCnt.setData(x*2, data_cnt64[3])
        self.curveCnt.setData(x*2, data_mat[7])
        self.curveCnt.setDownsampling(
                ds=DECIMATION, auto=None, method='subsample')
        AFPRE = (data_mat[6] & 0x0001) * 5000 # .astype('int16')
        AF = (data_mat[6] & 0x0002) * 5000 # .astype('int16')

        self.crvAFPRE.setData(x*2, AFPRE )
        self.crvAFPRE.setDownsampling(
                ds=DECIMATION, auto=None, method='subsample')
        self.crvAF.setData(x*2, AF)
        self.crvAF.setDownsampling(
                ds=DECIMATION, auto=None, method='subsample')
        # print("Shape = {0}".format(data_mat.shape[1]))
        # print(data_cnt.shape[1])
        self.updateViews()

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
    app.exec_()
#    sys.exit(app.exec_())


if __name__ == '__main__':
    main()
