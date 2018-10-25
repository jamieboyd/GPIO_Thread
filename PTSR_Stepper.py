#! /usr/bin/python
#-*-coding: utf-8 -*-

import ptSR_stepper
from array import array

class PTSR_Stepper (object):
    def __init__(self, data_pin, shift_reg_pin, storage_reg_pin, num_motors, steps_per_sec, accuracyLevel):
        self.data_pin = data_pin
        self.shift_reg_pin = shift_reg_pin
        self.storage_reg_pin = storage_reg_pin
        self.num_motors = num_motors
        self.task_ptr = ptSR_stepper.new (data_pin, shift_reg_pin, storage_reg_pin, num_motors, steps_per_sec, accuracyLevel)
    
    def move (self, moves_tuple):
        moves_array =array ('i', (el for el in (moves_tuple)))
        return ptSR_stepper.move (self.task_ptr, moves_array)

    def free (self, free_tuple):
        free_array = array ('i', (el for el in (free_tuple)))
        return ptSR_stepper.free (self.task_ptr, free_array)

    def hold (self, hold_tuple):
        hold_tuple = array ('i', (el for el in (hold_tuple)))
        return ptSR_stepper.hold (self.task_ptr, hold_array)

    def emerg_stop (self):
        return ptSR_stepper.stop (self.task_ptr)

    def get_speed (self):
        return ptSR_stepper.getSpeed (self.task_ptr)

    def set_speed (self, steps_per_sec):
        return ptSR_stepper.setSpeed (self.task_ptr, steps_per_sec)

    def is_moving (self):
        return ptSR_stepper.isBusy (self.task_ptr)

    def wait_while_moving (self, time_out_secs):
        return ptSR_stepper.waitOnBusy (self.task_ptr, time_out_secs)

    
