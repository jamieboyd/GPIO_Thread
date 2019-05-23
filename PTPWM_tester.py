#! /usr/bin/python
#-*-coding: utf-8 -*-

from PTSimpleGPIO import PTSimpleGPIO
from PTPWM import PTPWM, PTPWMsin
from time import sleep
from array import array

# play a scale with sin thread
tone = 1.12246
semi_tone = 1.05946
init_freq = 110
freq = init_freq

print ('Sine wave Thread playing a major scale on channel 1')
# make the object and set initial frequency
w1 = PTPWMsin (1)
w1.set_sin_freq(init_freq,1,0)
# start the sinThread running
w1.start()
for ii in range (0,2):
    for i in range (1,8):
        sleep (0.2)
        if i ==3 or i ==7:
            freq *= semi_tone
            print ('semi tone')
        else:
            freq *= tone
            print ('tone')
        w1.set_sin_freq(round (freq),1,1)
    print ("Octave frequency = ", round (freq)) 
sleep (0.5)
# stop
w1.stop()

print ('Setting off state to HIGH')
w1.set_PWM_off_state(0, 1, 0)
sleep (0.5)
w1.set_PWM_off_state(0, 1, 0)
print ("setting off state back to LOW")
sleep (0.5)

print ('Playing a Minor Scale on channel 1')
freq=init_freq
w1.set_sin_freq(init_freq,1,0)
w1.start()
for ii in range (0,2):
    for i in range (1,8):
        sleep (0.2)
        if i ==2 or i ==5:
            freq *= semi_tone
            print ('semi tone')
        else:
            freq *= tone
            print ('tone')
        w1.set_sin_freq(round (freq),1,1)
    print ("Octave frequency = ", round (freq))
sleep (0.2)
w1.stop()

print ('making a PWM thread in Mark Space Mode on Channel 2')
a1 = array('i', (i for i in range (0, 1000)))
w1 = PTPWM(PTSimpleGPIO.MODE_FREQ, 44000, 1000, 0, 100, 9, 0)
print ('Object made')
w1.add_channel(2,0,PTPWM.PWM_MARK_SPACE,0,0,a1)
print ('channel added')
w1.set_PWM_enable(1,2,1)
print ('enabled')
sleep (0.5)
w1.start_train()
print ('train started')
w1.wait_on_busy (10)
w1.set_PWM_enable(0,2,1)
print ('train stopped')
sleep (0.5)


