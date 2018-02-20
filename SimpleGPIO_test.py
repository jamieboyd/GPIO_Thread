#! /usr/bin/python
#-*-coding: utf-8 -*-

import PTSimpleGPIO
from PTSimpleGPIO import PTSimpleGPIO, Pulse, PulseStretch, Train, Infinite_train, MajorScale
from array import array
from math import pi, cos
from abc import ABCMeta, abstractmethod
from time import sleep
from array import array as array

class PulseStretch (Pulse):
    def __init__ (self, delay, duration, polarity, gpio_pin, accuracy_level):
        super.__init__ (self, delay, duration, polarity, gpio_pin, accuracy_level)
        set_endFunc_obj (self, 1, 0)
    
    def endFunc (self, *args):
        #pulseDelay = args[0] in microseconds
        #pulseDur=args[1] in microseconds
        #nPulses = args[2]
        ptSimpleGPIO.modDelay(self.task_ptr, (args[0] * 1.05)*1e-06)

"""
MajorScale is a class with an endFunc designed to be used with a Train
task, with GPIO pin connected to a speaker or piezo buzzer.
It increases the frequency of the starting tone to play an
ascending major scale for as many trains as you configure. Start at
220 Hz and run 22 trains to play 3 octaves in A major. Make the train duty cycle
0.5 and the train duration 0.5 seconds or so.
"""
class MajorScale (object):

    def __init__(self):
        self.iTone =0
        
    def endFunc (self, *args):
        #trainFrequency=args[0]
        #trainDutyCycle=args[1]
        #trainDuration=args[2]
        #pulseDelay = args[3]
        #pulseDur=args[4]
        #nPulses = args[5]
        # major scale:
        #       do          re          me          fa          so          la          ti          do
        #  	    tone        tone        semitone    tone        tone        tone        semitone
	#	    0           1           2           3	    4	        5	    6
	#	    7           8	    9	        10	    11	        12          13
	#	    14          15	    16          17	    18          19          20
        if self.iTone % 7 == 2 or self.iTone % 7 == 6:
            newFrequency = args[0] * 1.05946  # semitone
            if self.iTone % 7 == 6:
                newFrequency= round (newFrequency) # so floating point roundoff does not accumulate
                print ('octave', ((self.iTone + 1)/7), newFrequency)
        else:
            newFrequency = args[0] * 1.12246  # tone
            #print ('Tone')
 
        self.iTone +=1
        ptSimpleGPIO.modTrainFreq (self.task_ptr, newFrequency)

"""
Pulser is class with an endFunc designed to be used with a Train
task with GPIO pin hooked up to an LED. Pulser modifies the duty cycle
of the train sinusoidally between 0.2 and 0.8 to give a gentle
pulsating effect. Try Pulser with a period of 100 added to
a Train task of 1000 Hz trains of 0.01 seconds, starting duty
cycle of 0.2, and do 1000 trains 
"""
class Pulser (object):

    """ for fast endFunc, we precompute the values. A floating point array of
    size period is filled with an inverted cosine wave scaled in domain
    (0 to period -> 0 to 2 * pi) and range (-1 to +1 -> 0.2 to 0.8)
    """
    def __init__(self, period):
        self.period = period
        self.periodArray = array ('f', (0.5 - 0.3 * cos (2 * pi * i/self.period) for i in range (0, self.period, 1)))
        self.iArray = 0

    """
    The endFunc iterates through the precomputed array of duty cycles, setting the
    duty cycle of the train to adjust the brightness
    The mod operator is used instead of resetting to 0 when iArray gets to period
    """
    def endFunc (self, *args):
        #trainFrequency=args[0]
        #trainDutyCycle=args[1]
        #trainDuration=args[2]
        #pulseDelay = args[3]
        #pulseDur=args[4]
        #nPulses = args[5]
        ptSimpleGPIO.modTrainDuty (self.task_ptr, self.periodArray [(self.iArray % self.period)])
        self.iArray += 1


t1Pin=23
t1=Train (PTSimpleGPIO.INIT_FREQ, 110, 0.5, 0.5, t1Pin, PTSimpleGPIO.ACC_MODE_SLEEPS_AND_OR_SPINS)
#scaler = MajorScale ()
#t1.add_endFunc (scaler)
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
