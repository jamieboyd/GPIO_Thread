#! /usr/bin/python
#-*-coding: utf-8 -*-

from PTPWM import PTPWM, PTPWMsin
from time import sleep
w1 = PTPWMsin (1)
w1.set_sin_freq(200,1,1)
w1.start()
freq = 200.0
for i in range (0,480):
    if i % 50 == 0:
        print ("frequency = ", round (freq))
    sleep (0.05)
    freq *= 1.01
    w1.set_sin_freq(round (freq),1,1)
    
print ("End frequency = ", round (freq))
sleep (2)
w1.stop()
sleep (0.2)
del w1
