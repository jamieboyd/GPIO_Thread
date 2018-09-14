#! /usr/bin/python
#-*-coding: utf-8 -*-

import ptPWMsin
import PTSimpleGPIO

class PWM_Sin (PTSimpleGPIO.Infinite_train):

    def __init__(self, channel, enable, initial_frequency):
        self.task_ptr = ptPWMsin.new(channel, enable, initial frequency)
	self.enabled = enable
	self.frequency = initial_frequency
        self.respectTheGIL = False

    def set_enable (self, enable_state, is_locking):
	    self.enabled = enable_state
        return ptPWMsin.setEnable(self.task_ptr, enable_state, is_locking)
    
    def set_frequency (self, frequency, is_locking):
	    self.frequency = frequency
        return ptPWMsin.setFrequency(self.task_ptr, frequency, is_locking)
	
	
	def get_enable (self):
	    return self.enabled
    
    def get_frequency (self, frequency, is_locking):
	    return self.frequency