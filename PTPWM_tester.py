#! /usr/bin/python
#-*-coding: utf-8 -*-

from PTPWM import PTPWM, PTPWMsin
from time import sleep
w1 = PTPWMsin (1)
print ('made PWM')
w1.set_sin_freq(200,1,1)
print ('set initial frequency')
w1.start()
print ('started')
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
del w1
sleep (0.5)
