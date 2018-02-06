#! /usr/bin/python
#-*-coding: utf-8 -*-

import PTSimpleGPIO
from PTSimpleGPIO import PTSimpleGPIO, Pulse, PulseStretch, Train, Infinite_train, MajorScale
from array import array
from math import pi, cos
from abc import ABCMeta, abstractmethod
from time import sleep
from array import array as array

t1Pin=4
t1=Train (PTSimpleGPIO.INIT_FREQ, 110, 0.5, 0.5, t1Pin, PTSimpleGPIO.ACC_MODE_SLEEPS_AND_OR_SPINS)
scaler = MajorScale ()
t1.add_endFunc (scaler)
t1.do_trains (21)

"""
a1 = array('f', (0.01*i for i in range (0, 100)))
t1.set_array_endFunc (a1, 0, 0)
t1.add_endFunc (scaler)
    
    t2Pin = 18
    t1 = Infinite_train (PTSimpleGPIO.INIT_FREQ, 30, 0.75, t1Pin, PTSimpleGPIO.ACC_MODE_SLEEPS_AND_OR_SPINS)
    t1.start_train()
    
    set_array_endFunc (self, flt_array, freq_or_duty, is_locking):
    pulser = Pulser (120)
    t1.add_endFunc (pulser)
    t2 = Train (PTSimpleGPIO.INIT_FREQ, 220, 0.5, 0.5, t2Pin, PTSimpleGPIO.ACC_MODE_SLEEPS_AND_OR_SPINS)
    scaler = MajorScale ()
    t2.add_endFunc (scaler)
    t1.do_trains(5000)
    #t2.do_trains (22)
    for i in range(0,20,1):
        sleep (0.3)
        print ("t1 trains remaining =", t1.is_busy ())
        print ("t2 trains remaining =", t2.is_busy (), "\n")
    print (t1.wait_on_busy (5))
    #print ("t2 waited 20 secs=", t2.wait_on_busy (20))
    #t1=None
    #t2=None

    p1=PulseStretch(0.005, 0.05, 1, 18, PTSimpleGPIO.ACC_MODE_SLEEPS)
    p1.add_endFunc()
    p1.do_pulses (100)
    p1.wait_on_busy (100)
    p1=None

   
    (self, mode, pulseDelayOrTrainFreq, pulseDurationOrTrainDutyCycle, nPulsesOrTrainDuration, gpio_pin, accuracy_level):
    
    sleep(2)
    t1=None
    t1=Infinite_train (0.5, 2.1e3, t1Pin,ptGPIO.ACC_MODE_SLEEPS_AND_SPINS)
    t1.start_train()
    p2.do_pulse()
    sleep(3)
    t1.stop_train()
    
    # software PWM
    t1Pin = 23
    t1=Infinite_train (2, 0.025, 50, t1Pin, PtGPIO.ACC_MODE_SLEEPS_AND_SPINS)
    t1.start_train()
    for i in range (25, 125, 1):
         t1.set_duty_cycle (i * 0.001)
         sleep (.2)
   
    t1.stop_train()
    t1=None
 
    # hardware PWM
    pwm = PWM (4096, PWM.clock_from_freq_and_range (50, 4096), 0, 410)
    
    for i in range (410, 205, -16):
        pwm.set_pulse_len (i)
        sleep (1)
    for i in range (205, 410, 16):
        pwm.set_pulse_len (i)
        sleep (1)
    pwm = None
    """
