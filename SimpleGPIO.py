#! /usr/bin/python
#-*-coding: utf-8 -*-

import ptSimpleGPIO
from array import array
from math import pi, cos
from abc import ABCMeta, abstractmethod
from time import sleep
"""
    PTSimpleGPIO does control of GPIO access using the C++ module ptSimpleGPIO,
    which provides pulsed thread timing for setting GPIO lines high or low.
    Sublasses of SimpleGPIO are PTSimpleGPIO (Pulse, Train, Infinite_train, each
    controlled by a C++ thread)
"""
class PTSimpleGPIO (object, metaclass = ABCMeta):
    """
    PTSimpleGPIO defines constants for accuracy level for timing:
    ACC_MODE_SLEEPS relies solely on sleeping for timing, which may not be accurate beyond the ms scale
    ACC_MODE_SLEEPS_AND_SPINS sleeps for first part of a time period, but wakes early and spins in a tight loop
    until the the end of the period, checking each time through the loop against calculated end time.
    It is more processor intensive, but much more accurate. How soon before the end of the period the thread
    wakes is set by the constant kSLEEPTURNAROUND in gpioThread.h. You need to re-run setup if it is changed.
    ACC_MODE_SLEEPS_AND_OR_SPINS checks the time before sleeping, so if thread is delayed for some reason, the sleeping is
    countermanded for that time period, and the thread goes straight to spinning, or even skips spinning entirely
    """
    ACC_MODE_SLEEPS = 0
    ACC_MODE_SLEEPS_AND_SPINS =1
    ACC_MODE_SLEEPS_AND_OR_SPINS =2

    """
    PTSimpleGPIO defines constants for initializing with pulse based (low time, high time, number of pulses) or
    frequency based (frequency, duty cycle, total duration) paramaters. Same information, presented in a different way,
    use what is convenient for your application.
    """
    INIT_PULSES =1
    INIT_FREQ=2

    """
    PTSimpleGPIO defines contants for special endFuncs from ptSimpleGPIO that take the next value from an array of either
    Duty Cycles or Frequencies
    """
    ARRAY_MODE_DUTY =0
    ARRAY_MODE_FREQ =1
   
    """
    Because PTSimpleGPIO is an abstact base class, we explicitly declare the __init__ method to be abstract
    """
    @abstractmethod
    def __init__():
        pass
    
    """
    Setters and Getters for pulse delay (low time in seconds) and duration (high time in seconds) for each pulse.
    Defined in the base class because all subclasses, Pulse, Train, Infinite_train, have these properties. 
    """
    def set_delay (self, new_delay):
        return ptSimpleGPIO.modDelay(self.task_ptr, new_delay)

    def set_duration (self, new_duration):
        return ptSimpleGPIO.modDur(self.task_ptr, new_duration)

    def get_delay (self):
        return ptSimpleGPIO.getPulseDelay(self.task_ptr)

    def get_duration (self):
        return ptSimpleGPIO.getPulseDuration(self.task_ptr)

    """
    Setters and Getters for frequency (Hz) and duty cycle (0 to 1), same information as
    pulse delay and pulse duration, different format. Note behaviour of setters: Changing
    one of frequency, duty cycle, or train duration will adjust low time, high time, and
    number of pulses as needed to keep the other two of frequency, duty cycle,
    or train duration from changing.
    """
    def set_frequency (self, new_frequency):
        return ptSimpleGPIO.modTrainFreq (self.task_ptr, new_frequency)

    def set_duty_cycle (self, new_duty_cycle):
        return ptSimpleGPIO.modTrainDuty (self.task_ptr, new_duty_cycle)

    def get_frequency (self):
        return ptSimpleGPIO.getTrainFrequency (self.task_ptr)

    def get_dutycycle (self):
        return ptSimpleGPIO.getTrainDutyCycle (self.task_ptr)
    """
    is_busy returns 0 if a thread is not currently performing its
    task, i.e., trains or pulses, else it returns number of trains
    or pulses left for it to do
    """
    def is_busy (self):
        return ptSimpleGPIO.isBusy(self.task_ptr)

    """
    wait_on_busy does not return until the thread is no longer busy
    or the time out expires. It returns 0 if the task ended, 1 if the
    time out expired. Don't call wait_on_busy on an Infinite_train
    Don't wait on a thread with an endFunc from a Python object installed, or you will get GILled 
    """
    def wait_on_busy(self, waitSecs):
        if (ptSimpleGPIO.hasEndFunc (self.task_ptr)):
            nWaits = waitSecs * 10
            with nogil:
                pass
            for iWait in range (0,nWaits, 1):
                sleep (0.1)
                nTasks = ptSimpleGPIO.isBusy(self.task_ptr)
                if nTasks == 0:
                    break
            return (nTasks > 0)
        else:
            return ptSimpleGPIO.waitOnBusy(self.task_ptr, waitSecs)

    """
    set_pin sets the GPIO pin that is used by the thread, if you want to
    change it after initializing it. Broadcom numbering is always used.
    set is_locking to non-zero to get the mutex lock first, if the task
    is currently active, else pin might change in middle of a pulse
    """
    def set_pin (self, pin_number, is_locking):
        return ptSimpleGPIO.setPin(self.task_ptr, pin_number, is_locking)

    """
    set_level allows you to set the pin used by the thread to HIGH or
    LOW state directly, not through the thread. If used while a thread
    is active, the state you set may be overwritten almost immediately by the
    thread.
    """
    def set_level (self, level, is_locking):
        return ptSimpleGPIO.setLevel(self.task_ptr, level, is_locking)
    
    """
    add_endFunc passes a Python object with a method named endFunc to the C++ thread
    The default object to pass is self. If set, the object's endFunc is run at the end
    of every Pulse, at the end of every Train, or when an Infinite_train is stopped.
    An endFunc gets *args with current state of timing of the thread's task in the
    order: args[0] =trainFrequency (Hz); args[1] = trainDutyCycle (0 -1);
    args [2] = total train Duration (seconds), args[3] = pulse Delay in microseconds,
    args [4] = pulse Duration in microseconds, args [5] = number of pulses
    """
    def add_endFunc (self, dataMode, addObj = None):
        if addObj is None and hasattr(self, "endFunc"):
            ptSimpleGPIO.setEndFunc (self.task_ptr, self)
        elif (hasattr(addObj, "endFunc")):
            ptSimpleGPIO.setEndFuncObj (self.task_ptr, addObj, dataMode)
            addObj.task_ptr = self.task_ptr
    """
    Unsets the endFunc for the thread so it is no longer run at the end of each task.
    """
    def clear_endFunc (self):
         ptSimpleGPIO.unSetEndFunc (self.task_ptr)

    """
    reports if the GPIO thread object has an endFunc installed
    """

    def has_endFunc (self):
        return ptSimpleGPIO.hasEndFunc (self.task_ptr)


    def set_array_endFunc (self, flt_array, freq_or_duty, is_locking):
        self.flt_array = flt_array
        ptSimpleGPIO.setArrayEndFunc (self.task_ptr, self.flt_array, freq_or_duty, is_locking)


        
""" Pulse class is a task used to do a single low-to-high (direction =0) or high-to-low
    (direction = 1) pulse on a GPIO pin. Delay and Duration are in seconds, Broadcom numbering
    is always used for GPIO pin. Setters and getters for delay and duration are defined in PTSimpleGPIO
""" 
class Pulse (PTSimpleGPIO):
   
    def __init__ (self, delay, duration, direction, gpio_pin, accuracy_level):
        self.task_ptr = ptSimpleGPIO.pulse(delay, duration, direction, gpio_pin, accuracy_level)

    """
    does a single pulse, as previously configured. GPIO pin is held in starting state (high or low,
    depending on direction when initied) for delay, then toggled and held for duration,
    when it is set back to starting state.
    """
    def do_pulse(self):
        ptSimpleGPIO.doTask(self.task_ptr)

    """
    Does n_pulses of pulses, one immediatley following the other, with duration and delay between
    pulses as configured, posibly calling an endFunc after each pulse.
    """
    def do_pulses (self, n_pulses):
        ptSimpleGPIO.doTasks(self.task_ptr, n_pulses)

"""
    PulseStretch is a subclass of Pulse with an endFunc defined. The endFunc stretches out the delay between
    pulses by a factor of 1.05 after each pulse.

"""
class PulseStretch (Pulse):

    def endFunc (self, *args):
        #trainFrequency=args[0] in Hz
        #trainDutyCycle=args[1] 0 - 1
        #trainDuration=args[2] in seconds
        #pulseDelay = args[3] in microseconds
        #pulseDur=args[4] in microseconds
        #nPulses = args[5]
        ptSimpleGPIO.modDelay(self.task_ptr, (args[3] * 1.05)*1e-06)


"""
    Train class is a task to output a defined length train of pulses on a GPIO pin
"""
class Train (PTSimpleGPIO):
    
    def __init__ (self, mode, pulseDelayOrTrainFreq, pulseDurationOrTrainDutyCycle, nPulsesOrTrainDuration, gpio_pin, accuracy_level):
        if mode == PTSimpleGPIO.INIT_PULSES:
            self.task_ptr = ptSimpleGPIO.trainDelayDur (pulseDelayOrTrainFreq, pulseDurationOrTrainDutyCycle, nPulsesOrTrainDuration, gpio_pin, accuracy_level)
        elif mode == PTSimpleGPIO.INIT_FREQ:
            self.task_ptr = ptSimpleGPIO.trainFreqDuty (pulseDelayOrTrainFreq, pulseDurationOrTrainDutyCycle, nPulsesOrTrainDuration, gpio_pin, accuracy_level)
		
    def do_train (self):
        return ptSimpleGPIO.doTask(self.task_ptr)
	
    def do_trains (self, n_trains):
        return ptSimpleGPIO.doTasks(self.task_ptr, n_trains)

    def set_pulse_number (self, new_pulse_number):
        return ptSimpleGPIO.modTrainLength (self.task_ptr, new_pulse_number)

    def set_train_duration (self, new_train_duration):
        return ptSimpleGPIO.modTrainDur(self.task_ptr, new_train_duration)

    def get_pulse_number (self):
        return ptSimpleGPIO.getPulseNumber (self.task_ptr)

    def get_train_duration (self):
        return ptSimpleGPIO.getTrainDuration (self.task_ptr)
    
  
"""
    Infinite_train is a class to output a continuous train of pulses on a GPIO pin. Number of pulses
    is not tracked. It starts when started and stops when stopped. An endFunc, when in effect,
    is called when the train is stopped. The duty cycle, frequency (or pulse delay and pulse duration)
    can be set while the train is running, using the functions from the base class. 
"""
class Infinite_train (PTSimpleGPIO):

    def __init__ (self, mode, pulseDelayOrTrainFreq, pulseDurationOrTrainDutyCycle, gpio_pin, accuracy_level):
        if mode == PTSimpleGPIO.INIT_PULSES:
            self.task_ptr = ptSimpleGPIO.trainDelayDur (pulseDelayOrTrainFreq, pulseDurationOrTrainDutyCycle, 0, gpio_pin, accuracy_level)
        elif mode == PTSimpleGPIO.INIT_FREQ:
            self.task_ptr = ptSimpleGPIO.trainFreqDuty (pulseDelayOrTrainFreq, pulseDurationOrTrainDutyCycle, 0, gpio_pin, accuracy_level)

    """
    Starts an infinite train, as configured, endlessly repeating pulses
    """
    def start_train (self):
        ptSimpleGPIO.startTrain (self.task_ptr)

    """
    Stops an infinite train previously started
    """
    def stop_train (self):
        ptSimpleGPIO.stopTrain (self.task_ptr)

    """
    waiting on an infnite train might be a good premise for existentialist theatre, but is not so useful
    so we override the method and raise an error. is_busy for an infinite train tells you the useful
    information that the train is currently running, or is stopped.
    """
    def wait_on_busy(self, waitSecs):
        raise NotImplementedError("Waiting on an infinite train is not supported.")

"""
The Simple_Output class directly sets a gpio pin hi or low with no thread involvement. It is inited
with the pin number you want to use, and the level (0 =low, 1 =high) you want at start.
"""
class Simple_output (PTSimpleGPIO):
    """Class does simple set hi/set low with no thread
    """
    def __init__(self, pin_number, level):
        self.task_ptr = ptSimpleGPIO.output (pin_number, level)

    """
    Sets the level of the pin (0=low, 1 = high)
    """
    def  set_level (self, level):
        ptSimpleGPIO.setOutput (self.task_ptr, level)

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
	#		    0           1           2           3	    4	        5	    6
	#		    7	        8	    9	        10	    11		12	    13
	#		    14          15	    16          17	    18		19	    20
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

    
