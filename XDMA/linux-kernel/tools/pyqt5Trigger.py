#!/usr/bin/env python3
# vim: sta:et:sw=4:ts=4:sts=4
#
#https://www.pythonguis.com/tutorials/qprocess-external-programs/
# https://stackoverflow.com/questions/35056635/how-to-update-a-realtime-plot-and-use-buttons-to-interact-in-pyqtgraph
# https://stackoverflow.com/questions/56918912/how-to-enable-legends-and-change-style-in-pyqtgraph
import sys
# from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5 import QtWidgets
# from PyQt5 import QtGui, QtCore
import numpy as np
import pyqtgraph as pg

FILENAME = 'data/out_16.bin'
FILENAME32 = 'data/out_32.bin'
NUMCHANNELS = 12

SCALE_32 = 2**12
DECIMATION = 20


class atcaWave(QtWidgets.QWidget):
    def __init__(self):
        pg.setConfigOption('background', 'w')  # before loading widget
        super(atcaWave, self).__init__()
        self.init_ui()
        self.qt_connections()
        # self.plotcurve = pg.PlotCurveItem(pen=pg.mkPen('r', width=3))
        # self.plotcurve2 = pg.PlotCurveItem(pen=pg.mkPen('g', width=3))
        # self.plotwidget.addItem(self.plotcurve)
        # self.plotwidget.addItem(self.plotcurve2)
        # call plt.addLegend() BEFORE you create the curves.
        self.plotwidget.addLegend()
        self.plotCurves = [0]*NUMCHANNELS
        for i in range(NUMCHANNELS):
            self.plotCurves[i] = pg.PlotDataItem(
                [0], [0], pen=pg.mkPen(i, width=2), name="ch{}".format(i))
            self.plotwidget.addItem(self.plotCurves[i])
        self.plotwidget.setLabels(
            title='Window 1', bottom='X Axis', left='LSB', right='Chopp')
        self.plotwidget.setYRange(-32768, 32768, padding=0.01)
        # self.amplitude = 10
        self.t = 0
        self.updateplot()

        # self.timer = pg.QtCore.QTimer()
        # self.timer.timeout.connect(self.moveplot)
        # self.timer.start(500)

    def init_ui(self):
        self.setWindowTitle('ATCA Data 16 bit')
        hbox = QtWidgets.QHBoxLayout()
        vbox1 = QtWidgets.QVBoxLayout()
        # vbox1.addStretch(1) #  add the stretch
        # vbox1.setFixedWidth(50)
        vbox2 = QtWidgets.QVBoxLayout()

        self.plotwidget = pg.PlotWidget()
        self.p1 = self.plotwidget.plotItem
        # self.p1.setLabels(left='axis 1')

        self.p2 = pg.ViewBox()
        self.p1.scene().addItem(self.p2)
        self.p1.getAxis('right').linkToView(self.p2)
        self.p2.setXLink(self.p1)
        self.p2.setYRange(-1, 5)

        self.plotwidget.setLabels(
            title='Window 1', bottom='X Axis', left='LSB', right='Chopp')
        self.curveChp = pg.PlotDataItem(
            [0], [0], pen=pg.mkPen(color='#025b94', width=1))
        self.p2.addItem(self.curveChp)

        vbox2.addWidget(self.plotwidget)
        self.clearbutton = QtWidgets.QPushButton("Clear Plots")
        # self.decreasebutton = QtWidgets.QPushButton("Decrease Amplitude")
        hbox1 = QtWidgets.QHBoxLayout()
        decLabel = QtWidgets.QLabel("Decimation")
        self.line_editDec = QtWidgets.QLineEdit("10", self)
        self.line_editDec.resize(100, 32)
        hbox1.addWidget(decLabel)
        # hbox1.addWidget(QtWidgets.QLabel("Decimation "))
        hbox1.addWidget(self.line_editDec)
        hbox.addLayout(vbox1)
        self.plot16Button = QtWidgets.QPushButton("Plot data 16 bit")
        self.plot16dButton = QtWidgets.QPushButton("Plot data 16 D bit")
        self.plot32Button = QtWidgets.QPushButton("Plot data 32 bit")
        self.line_edit = QtWidgets.QLineEdit(FILENAME, self)
        self.line_edit32 = QtWidgets.QLineEdit(FILENAME32, self)
        vbox1.addWidget(self.clearbutton)
        # vbox.addWidget(self.decreasebutton)
        vbox2.addWidget(self.line_edit)
        vbox2.addWidget(self.line_edit32)
        # butt = QtWidgets.QPushButton("Increa1") butt.setSizePolicy(
        #      QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.MinimumExpanding,
#            QtWidgets.QSizePolicy.MinimumExpanding))
        sizePolicy = QtWidgets.QSizePolicy(
            QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        self.clearbutton.setSizePolicy(sizePolicy)
        # vbox1.addWidget(butt)
        verticalSpacer = QtWidgets.QSpacerItem(
            20, 40, QtWidgets.QSizePolicy.Minimum,
            QtWidgets.QSizePolicy.Expanding)
        vbox1.addItem(verticalSpacer)
        vbox1.addLayout(hbox1)
        vbox1.addWidget(self.plot16Button)
        vbox1.addWidget(self.plot16dButton)
        vbox1.addWidget(self.plot32Button)
        self.plot16Button.setSizePolicy(sizePolicy)
        self.plot16dButton.setSizePolicy(sizePolicy)
        self.plot32Button.setSizePolicy(sizePolicy)
        hbox.addLayout(vbox1, 1)
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
        self.show()

    def qt_connections(self):
        self.clearbutton.clicked.connect(self.on_clearbutton_clicked)
        # self.decreasebutton.clicked.connect(self.on_decreasebutton_clicked)
        self.plot16Button.clicked.connect(self.on_plot16Button_clicked)
        self.plot16dButton.clicked.connect(self.on_plot16dButton_clicked)
        self.plot32Button.clicked.connect(self.on_plot32Button_clicked)
        # adding action to the line edit when enter key is pressed
        # line_edit.returnPressed.connect(lambda: do_action())

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
        self.curveChp.clear()

    def on_clearbutton_clicked(self):
        print("clear")
        self.clear()

    # def on_decreasebutton_clicked(self):
        # print ("Amplitude decreased")
        # self.amplitude -= 1
        # self.updateplot()

    def on_plot16Button_clicked(self):
        # print ("Amplitude decreased")
        self.draw16()

    def on_plot16dButton_clicked(self):
        self.draw16d()

    def on_plot32Button_clicked(self):
        # print ("Amplitude decreased")
        self.draw32()

    def draw16d(self):
        data = np.fromfile(FILENAME, dtype='int16')
# ‘F’ means to readthe elements using Fortran-like index order,
# with the first index changing fastest,
        filename = self.line_edit.text()
        data = np.fromfile(filename, dtype='int16')
        data_mat = np.reshape(data, (32, -1), order='F')
        # for i in range(3):
        # self.graphicsView.plot(data_mat[i], pen=(i,3))
        # self.graphicsView.plot(data_mat[0], pen='r')
        # self.graphicsView.plot(data_mat[1], pen='g')
        # self.plotcurve.setData(data_mat[0])
        # self.plotcurve2.setData(data_mat[1])
        self.plotwidget.setYRange(-32768, 32768, padding=0.01)
        self.plotwidget.setLabels(
            title='16 bit Data', bottom='Sample', left='LSB', right='Chopp')
        decim = int(self.line_editDec.text())
        print(" decim is {0}".format(int(decim)))
        for i in range(NUMCHANNELS):
            self.plotCurves[i].clear()
        for i in [0, 1, 2, 3]:
            self.plotCurves[i].setPen(
                pen=pg.mkPen(i, width=2),  name="cha{}".format(i + 28))
            self.plotCurves[i].setDownsampling(
                ds=decim, auto=None, method='subsample')
        self.plotCurves[0].setData(data_mat[28])
        self.plotCurves[1].setData(data_mat[29])
        self.plotCurves[2].setData(data_mat[30])
        self.plotCurves[3].setData(data_mat[31])
        self.curveChp.setData(data_mat[31] & 0x01)
        self.updateViews()

    def draw16(self):
        filename = self.line_edit.text()
        data = np.fromfile(filename, dtype='int16')
# ‘F’ means to readthe elements using Fortran-like index order,
# with the first index changing fastest,
        self.plotwidget.setYRange(-32768, 32768, padding=0.01)
        self.plotwidget.setLabels(
            title='16 bit Data', bottom='Sample', left='LSB', right='Chopp')
        data_mat = np.reshape(data, (32, -1), order='F')
        # self.plotcurve2.setData(data_mat[1])
        for i in range(NUMCHANNELS):
            self.plotCurves[i].clear()
            self.plotCurves[i].setData(data_mat[i])
            self.plotCurves[i].setDownsampling(
                ds=DECIMATION, auto=None, method='subsample')
        self.curveChp.setData(data_mat[31] & 0x01)
        self.curveChp.setDownsampling(
            ds=DECIMATION, auto=None, method='subsample')
        self.updateViews()

    def draw32(self):
        filename = self.line_edit32.text()
        data = np.fromfile(filename, dtype='int32')
        data_mat = np.reshape(data, (32, -1), order='F')
        self.plotwidget.setLabels(
            title='32 bit Data', bottom='Sample', left='LSB', right='Chopp')
        # data_u64 = np.fromfile(filename, dtype='u8')
        # data_u64 = np.reshape(data_u64, (8, -1), order='F')
        self.plotwidget.setYRange(
            -32768 * 4, 32768 * 4, padding=0.01)
        for i in range(NUMCHANNELS):
            self.plotCurves[i].clear()
            self.plotCurves[i].setData(data_mat[i]/SCALE_32)
        self.curveChp.setData((data_mat[31] & 0x0001))
        self.curveChp.setDownsampling(
            ds=DECIMATION, auto=None, method='subsample')
        self.updateViews()

# Handle view resizing
    def updateViews(self):
        #  view has resized; update auxiliary views to match
        self.p2.setGeometry(self.p1.vb.sceneBoundingRect())
        # need to re-update linked axes since this was called
        # incorrectly while views had different shapes.
        # (probably this should be handled in ViewBox.resizeEvent)
        self.p2.linkedViewChanged(self.p1.vb, self.p2.XAxis)


def main():
    app = QtWidgets.QApplication(sys.argv)
    app.setApplicationName('ATCA Data 16 bit')
    ex = atcaWave()
    sys.exit(app.exec_())


if __name__ == '__main__':
    main()
