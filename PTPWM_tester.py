#! /usr/bin/python
#-*-coding: utf-8 -*-

from PTSimpleGPIO import PTSimpleGPIO, SingletonForGPIO
from PTPWM import PTPWM, PTPWMsin
from time import sleep
from array import array


tone = 1.12246
semi_tone = 1.05946
init_freq = 110

w1 = PTPWMsin (1)
print ('made object')
w1.set_sin_freq(init_freq,1,0)
print ('initial frequency', w1.get_sin_freq(1))
freq = init_freq
w1.set_PWM_off_state(1, 1, 0)
sleep (0.5)
w1.set_PWM_off_state(0, 1, 0)
print ("off state set and reset")
w1.start()
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
sleep (0.5)


a1 = array('i', (i for i in range (0, 1000)))
w2 = PTPWM(PTSimpleGPIO.MODE_FREQ, 44000, 1000, 0, 100, 9, 0)
w2.add_channel(2,0,PTPWM.PWM_MARK_SPACE,0,0,a1)
w2.set_PWM_enable(1,2,1)
w2.start_train()
print ('enabled')
sleep (9)
w2.set_PWM_enable(0,2,1)

w2.stop_train()
sleep (0.5)
del w2
sleep (0.5)
