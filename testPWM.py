#! /usr/bin/python
#-*-coding: utf-8 -*-

from PTPWM import PTPWM
from PTPWM import PTPWMsin
from time import sleep
w1 =PTPWMsin (1)
sleep (0.05)
w1.set_sin_freq(200,1,1)
sleep (0.05)
w1.start()
sleep (0.5)
freq = 200.0
for i in range (0,20):
    freq *= 1.1
    w1.set_sin_freq(round (freq),1,1)
    sleep (0.5)
w1.stop()
sleep (0.2)
del w1
