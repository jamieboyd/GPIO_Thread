#! /usr/bin/python
#-*-coding: utf-8 -*-

import ptSimpleGPIO
from abc import ABCMeta, abstractmethod
from time import sleep

"""
A simple singleton class to ensure only one instance of certain GPIO classes like PWM
lifted from https://sourcemaking.com/design_patterns/singleton/python/1
"""
class SingletonForGPIO (type):

    def __init__ (cls, name, bases, attrs, **kwargs):
        super().__init__(nam, bases, attrs)
        cls._instance = None

    def __call__(cls, *args, **kwargs):
        if cls._instance is None:
            cls._instance = super().__call__(*args, **kwargs)
        return cls._instance

"""
    PTSimpleGPIO does control of GPIO access using the C++ module ptSimpleGPIO,
    which provides pulsed thread timing for setting GPIO lines high or low.
    Sublasses of PTSimpleGPIO are Pulse, Train, and Infinite_train
    Each instance of Pulse, Train, or Infinite_train is controlled by its own C++ thread
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
    PTSimpleGPIO defines constants for working with frequency based (frequency, duty cycle, total duration) or
    time based (low time, high time, number of pulses) paramaters when creating a train, or passing information to
    an endFunction. Same information, presented in a different way, use what is convenient for your application.
    """
    MODE_FREQ=0
    MODE_PULSES=1
    
    """
    cosine_duty_cycle_array fills a passed-in array with a cosine wave with a period of period points, with
    values ranging from offset - scaling (which must be greater than 0) to offset + scaling (which must be
    less than 1). The range is limited from 0 to 1 because its intended use is to set duty cycle.
    """
    @staticmethod
    def cosine_duty_cycle_array (array, period, offset, scaling):
        return ptSimpleGPIO.cosDutyCycleArray (array, period, offset, scaling)

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

    def get_duty_cycle (self):
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
    or the time out expires. It returns 0 if the task ended, or the
    number of tasks left to do if the time out expired.
    Don't call wait_on_busy on an Infinite_train
    Don't call ptSimpleGPIO.waitOnBusy on a thread with an endFunc from a Python object installed,
    or you will get GILled 
    """
    def wait_on_busy(self, waitSecs):
        if (self.respectTheGIL == True):
            nWaits = waitSecs * 10
            for iWait in range (0,nWaits, 1):
                sleep (0.1)
                nTasks = ptSimpleGPIO.isBusy(self.task_ptr)
                if nTasks == 0:
                    break
            return (nTasks)
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


    def get_pin (self):
        return ptSimpleGPIO.getPin(self.task_ptr)


    """
    set_level allows you to set the pin used by the thread to HIGH or
    LOW state directly, not through the thread. If used while a thread
    is active, the state you set may be overwritten almost immediately by the
    thread.
    """
    def set_level (self, level, is_locking):
        return ptSimpleGPIO.setLevel(self.task_ptr, level, is_locking)


    def get_polarity (self):
        return ptSimpleGPIO.getPolarity (self.task_ptr)
    
    """
    set_endFunc_obj passes a Python object with a method named endFunc to the C++ thread
    The default object to pass is self. If set, the object's endFunc is run at the end
    of every Train, or at the end of every Pulse, and after every pulse in an Infinite train
    An endFunc gets *args with current state of timing of the thread's task in either
    frequency/Duty cycle format (dataMode = 0: args[0] =trainFrequency (Hz);
    args[1] = trainDutyCycle (0 -1); args [2] = total train Duration (seconds);
    args [3] = number of tasks left to do) or Delay/Duration format
    (dataMode = nonZero: args[0] = pulse Delay in microseconds, args [1] = pulse Duration
    in microseconds, args [2] = number of pulses, args [3] = number of tasks left to do)
    """
    def set_endFunc_obj (self, dataMode, is_locking, addObj = None):
        
        if addObj is None and hasattr(self, "endFunc"):
            ptSimpleGPIO.setEndFunc (self.task_ptr, self, dataMode, is_locking)
        elif (hasattr(addObj, "endFunc")):
            ptSimpleGPIO.setEndFuncObj (self.task_ptr, addObj, dataMode, is_locking)
            addObj.task_ptr = self.task_ptr
        else:
            return False
        self.respectTheGIL = True
        return True
        
    """
    Unsets the endFunc for the thread so it is no longer run at the end of each task.
    """
    def clear_endFunc (self):
         ptSimpleGPIO.unsetEndFunc (self.task_ptr)
         self.respectTheGIL = False

    """
    reports if the GPIO thread object has an endFunc installed
    """
    def has_endFunc (self):
        return ptSimpleGPIO.hasEndFunc (self.task_ptr)

    """
    sets a C++ endFunction that updates either duty cycle (freq_or_duty =0) or frequency
    (freq_or_duty = nonzero from a passed-in Python floating point array (flt_array) 
    """
    def set_array_endFunc (self, flt_array, freq_or_duty, is_locking):
        self.flt_array = flt_array
        ptSimpleGPIO.setArrayEndFunc (self.task_ptr, self.flt_array, freq_or_duty, is_locking)
        self.respectTheGIL = False


""" Pulse class is a task used to do a single low-to-high (direction =0) or high-to-low
    (direction = 1) pulse on a GPIO pin. Delay and Duration are in seconds, Broadcom numbering
    is always used for GPIO pin. Setters and getters for delay and duration are defined in PTSimpleGPIO
""" 
class Pulse (PTSimpleGPIO):
   
    def __init__ (self, gpio_pin, polarity, delay, duration, accuracy_level):
        self.task_ptr = ptSimpleGPIO.newDelayDur(gpio_pin, polarity, delay, duration, 1, accuracy_level)
        self.respectTheGIL = False
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
    
    def cancel_pulses (self):
        ptSimpleGPIO.unDoTasks (self.task_ptr)

    def get_pulse_len (self):
        return ptSimpleGPIO.getTrainDuration (self.task_ptr)
"""
    Train class is a task to output a defined length train of pulses on a GPIO pin
"""
class Train (PTSimpleGPIO):
    def __init__ (self, mode, gpio_pin, polarity, pulseDelayOrTrainFreq, pulseDurationOrTrainDutyCycle, nPulsesOrTrainDuration, accuracy_level):
        if mode == PTSimpleGPIO.MODE_PULSES:
            self.task_ptr = ptSimpleGPIO.newDelayDur (gpio_pin, polarity, pulseDelayOrTrainFreq, pulseDurationOrTrainDutyCycle, nPulsesOrTrainDuration, accuracy_level)
        elif mode == PTSimpleGPIO.MODE_FREQ:
            self.task_ptr = ptSimpleGPIO.newFreqDuty (gpio_pin, polarity, pulseDelayOrTrainFreq, pulseDurationOrTrainDutyCycle, nPulsesOrTrainDuration, accuracy_level)
        self.respectTheGIL = False
	
    def do_train (self):
        return ptSimpleGPIO.doTask(self.task_ptr)
	
    def do_trains (self, n_trains):
        return ptSimpleGPIO.doTasks(self.task_ptr, n_trains)

    def cancel_trains (self):
        ptSimpleGPIO.unDoTasks (self.task_ptr)

    def set_train_pulses (self, new_pulse_number):
        return ptSimpleGPIO.modTrainLength (self.task_ptr, new_pulse_number)

    def set_train_duration (self, new_train_duration):
        return ptSimpleGPIO.modTrainDur(self.task_ptr, new_train_duration)

    def get_train_pulses (self):
        return ptSimpleGPIO.getPulseNumber (self.task_ptr)

    def get_train_duration (self):
        return ptSimpleGPIO.getTrainDuration (self.task_ptr)
    
  
"""
    Infinite_train is a class to output a continuous train of pulses on a GPIO pin. Number of pulses
    is not tracked. It starts when started and stops when stopped. An endFunc, when in effect,
    is called at end of ebery pulse. The duty cycle, frequency (or pulse delay and pulse duration)
    can be set while the train is running, using the functions from the base class. 
"""
class Infinite_train (PTSimpleGPIO):

    def __init__ (self, mode, gpio_pin, pulseDelayOrTrainFreq, pulseDurationOrTrainDutyCycle, accuracy_level):
        if mode == PTSimpleGPIO.MODE_PULSES:
            self.task_ptr = ptSimpleGPIO.newDelayDur (gpio_pin, 0, pulseDelayOrTrainFreq, pulseDurationOrTrainDutyCycle, 0, accuracy_level)
        elif mode == PTSimpleGPIO.MODE_FREQ:
            self.task_ptr = ptSimpleGPIO.newFreqDuty (gpio_pin, 0, pulseDelayOrTrainFreq, pulseDurationOrTrainDutyCycle, 0, accuracy_level)
        self.respectTheGIL = False
        
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
        raise NotImplementedError("Waiting on an infinite train is not supported")

"""
The Simple_Output class directly sets a gpio pin hi or low with no thread involvement. It is inited
with the pin number you want to use, and the level (0 =low, 1 =high) you want at start.
"""
class Simple_output (object):
    """Class does simple set hi/set low with no thread
    """
    def __init__(self, pin_number, level):
        self.task_ptr = ptSimpleGPIO.simpleOutput (pin_number, level)

    """
    Sets the level of the pin (0=low, 1 = high)
    """
    def  set_level (self, level):
        ptSimpleGPIO.setOutput (self.task_ptr, level)
    
