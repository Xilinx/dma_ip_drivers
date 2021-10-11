#!/usr/bin/env gnuplot
#
#
clear
set term png
set term png enhanced font '/usr/share/fonts/liberation/LiberationSans-Regular.ttf' 12
set output 'dataXDMA.png'

#set xlabel 'Time (s)'
set xlabel 'Samp'
set xlabel 'Time/ms'
#set xlabel 'mSec'
#set ylabel 'Amp (V)'
set ylabel 'Lsb'
#set ylabel 'Amp (LSB)'
set title 'ESTHER  DMA data 15-09-21'

dfile='data/out_fmc.bin'

sampl_freq =  125.0e6
sampl_per = 1.0 / sampl_freq
scaleY= 1.0
#scaleX= 1
scaleX= 8e-6
#pretrigger samples = 16536 
offX=0.131
#scaleY= 0.0001729
plot_dec =7
# 200
firstl = 1
endl = 1e6

#
#set xtics  100000
set y2tics 100000
set ytics nomirror
set y2label '4*x' tc lt 2

plot dfile binary format='%8int16' every plot_dec::firstl:0:endl  using ($0*scaleX -offX):(($1)*scaleY) with lines lt 1 lw 1  title 'ch0', \
     dfile binary format='%8int16' every plot_dec::firstl:0:endl  using ($0*scaleX -offX):(($2)*scaleY ) w l lt 1 lw 1  title 'ch0', \
     dfile binary format='%8int16' every plot_dec::firstl:0:endl  using ($0*scaleX -offX):(($3)*scaleY ) with lines lt 2 lw 1  title 'ch1', \
     dfile binary format='%8int16' every plot_dec::firstl:0:endl  using ($0*scaleX -offX):(($4)*scaleY ) with lines lt 2 lw 1  title 'ch1', \
     dfile binary format='%4uint32' every plot_dec::firstl:0:endl  using ($0*2*scaleX):($8) with lines lt 1 lw 2  title 'count' axis x1y2

set term x11
set term wxt
replot
pause -1 "Hit return to continue"
