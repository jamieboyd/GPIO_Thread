#! /usr/bin/python
#-*-coding: utf-8 -*-

from PTSimpleGPIO import PTSimpleGPIO, SingletonForGPIO
from PTPWM import PTPWM, PTPWMsin
from time import sleep
from array import array

"""
a1 = array ('i', (i for i in range (0, 100)))
w0 = PTPWM (PTSimpleGPIO.MODE_PULSES, 1000, 100, 0, 1e04, 1000, 1)
sleep (0.1)
w0.add_channel (1, 0, PTPWM.PWM_MARK_SPACE, 0, 0, a1)
sleep (0.1)
w0.set_PWM_enable (1, 1, 0)
w0.start_train()
sleep (10)
w0.stop_train()
w0.set_PWM_enable (0, 1, 0)
sleep (0.1)
del w0
sleep (0.1)
"""
tone = 1.12246
semi_tone = 1.05946
init_freq = 110

w1 = PTPWMsin (1)
w1.set_sin_freq(init_freq,1,0)
w1.start()
freq = init_freq
print ("starting frequency = ", freq)
for ii in range (0,4):
    for i in range (1,8):
        sleep (0.5)
        if i ==3 or i ==7:
            freq *= semi_tone
            print ('semi tone')
        else:
            freq *= tone
            print ('tone')
        w1.set_sin_freq(round (freq),1,1)
    print ("Octave frequency = ", round (freq)) 
sleep (0.5)

w1.stop()
sleep (1)
freq=init_freq
w1.set_sin_freq(init_freq,1,0)
w1.start()
for ii in range (0,4):
    for i in range (1,8):
        sleep (0.5)
        if i ==2 or i ==5:
            freq *= semi_tone
            print ('semi tone')
        else:
            freq *= tone
            print ('tone')
        w1.set_sin_freq(round (freq),1,1)
    print ("Octave frequency = ", round (freq))
sleep (0.5)

w1.stop()
sleep (0.1)
del w1
sleep (0.1)
