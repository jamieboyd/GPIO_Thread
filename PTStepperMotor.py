#! /usr/bin/python
#-*-coding: utf-8 -*-

import ptStepperMotor

from time import time

"""
    Class is a task to run a stepper motor in full step mode using 2 gpio pins and a TTL inverter, or in half step mode using 4 gpio pins
"""
class PTStepperMotor (PTSimpleGPIO):
    """
    :param:p_pinTuple is a tuple containing GPIO pin numbers, 2 for full, 4 for half
    :param:p_scaling is spatial scaling in steps/unit
    :param:p_speed is speed in units/sec
    :param:p_accuracy_level method to use for timing, as defined for PTSimpleGPIO
    """
    def __init__ (self, p_pinTuple, p_scaling, p_speed, p_accuracy_level):
        if len (p_pinTuple) == 2:
             self.task_ptr = ptStepperMotor.stepperMotorFull (p_pinTuple[0], p_pinTuple[1], p_scaling, p_speed, p_accuracy_level)
        elif len (p_pinTuple) == 4:
            self.task_ptr = ptStepperMotor.stepperMotorHalf (p_pinTuple[0], p_pinTuple[1], p_pinTuple[2], p_pinTuple[3], p_scaling, p_speed, p_accuracy_level)
        else:
            print ('Pins tuple must contain 2 pins for full step mode, or 4 pins for half step mode.')
        
    def set_scaling (self, p_scaling):
        ptStepperMotor.setScaling (self.task_ptr, p_scaling)
 
    def set_speed (self, p_speed):
        ptStepperMotor.setSpeed (self.task_ptr, p_speed)

    def set_zero (self):
        ptStepperMotor.setZero(self.task_ptr)
                                                       
    def move_rel (self, distance):
        ptStepperMotor.moveRel (self.task_ptr, distance)
                                                             
    def move_abs (self, position):
        ptStepperMotor.moveAbs (self.task_ptr, position)

    def get_scaling (self):
        return ptStepperMotor.getScaling (self.task_ptr)

    def get_speed (self):
        return ptStepperMotor.getSpeed (self.task_ptr)                                                         

    def get_position (self):
        return ptStepperMotor.getPosition (self.task_ptr)

    def is_busy (self):
        return ptStepperMotor.isBusy(self.task_ptr)

    def wait_on_busy (self, timeOutSecs):
        return ptStepperMotor.waitOnBusy (self.task_ptr, timeOutSecs)
                                                             
    def setHold (self, hold):
        ptStepperMotor.setHold(self.task_ptr, hold)
    
	
class XY_Stage (PTSimpleGPIO):
    """ Class is an XY stage or equivalent with 2 stepper motors, one for each axis
    """

    def __init__ (self, p_XpinTuple, p_YpinTuple, p_scaling, p_speed, p_accuracy_level):
        super().__init__()
        if len (p_XpinTuple) == 2 and len (p_YpinTuple) == 2:
            self.x_task_ptr = ptStepperMotor.stepperMotorFull (p_XpinTuple[0], p_XpinTuple[1], p_scaling, p_speed, p_accuracy_level)
            self.y_task_ptr = ptStepperMotor.stepperMotorFull (p_YpinTuplep_pinTuple[0], p_YpinTuple[1], p_scaling, p_speed, p_accuracy_level)
        elif len (p_XpinTuple) == 4 and len (p_YpinTuple) == 4:
            self.x_task_ptr = ptStepperMotor.stepperMotorHalf (p_XpinTuple[0], p_XpinTuple[1], p_XpinTuple[2], p_XpinTuple[3], p_scaling, p_speed, p_accuracy_level)
            self.y_task_ptr = ptStepperMotor.stepperMotorHalf (p_YpinTuplep_pinTuple[0], p_YpinTuple[1], p_YpinTuplep_pinTuple[2], p_YpinTuple[3], p_scaling, p_speed, p_accuracy_level)      
        else:
            print ('Pins tuples for X and Y must each contain 2 pins for full step mode, or 4 pins for half step mode.')

    def set_scaling (self, p_scaling):
        ptStepperMotor.setScaling (self.x_task_ptr, p_scaling)
        ptStepperMotor.setScaling (self.y_task_ptr, p_scaling)
 
    def set_speed (self, p_speed):
        ptStepperMotor.setSpeed (self.x_task_ptr, p_speed)
        ptStepperMotor.setSpeed (self.y_task_ptr, p_speed)

    def set_zero (self):
        ptStepperMotor.setZero(self.x_task_ptr)
        ptStepperMotor.setZero(self.y_task_ptr)
                                                       
    def move_rel (self, x_distance, y_distance):
        ptStepperMotor.moveRel (self.x_task_ptr, x_distance)
        ptStepperMotor.moveRel (self.y_task_ptr, y_distance)
                                                             
    def move_abs (self, x_position, y_position):
        ptStepperMotor.moveAbs (self.x_task_ptr, x_position)
        ptStepperMotor.moveAbs (self.y_task_ptr, y_position)

    def get_scaling (self):
        return ptStepperMotor.getScaling (self.x_task_ptr)

    def get_speed (self):
        return ptStepperMotor.getSpeed (self.x_task_ptr)                                                         

    def get_position (self):
        return (ptStepperMotor.getPosition (self.x_task_ptr), ptStepperMotor.getPosition (self.y_task_ptr))

    def is_busy (self):
        return (ptStepperMotor.isBusy(self.x_task_ptr) or ptStepperMotor.isBusy(self.y_task_ptr)) 

    def wait_on_busy (self, timeOutSecs):
        startTime = time ()
        if ptStepperMotor.waitOnBusy (self.x_task_ptr, timeOutSecs) : # timed out
            return 1
        else:
            adjusted_timeout = timeOutSecs - (time() - startTime)
            if adjusted_timeout < 0.01:
                return 1
            else:
                return ptStepperMotor.waitOnBusy (self.y_task_ptr, adjusted_timeout)
                                                             
    def setHold (self, hold):
        ptStepperMotor.setHold(self.x_task_ptr, hold)
        ptStepperMotor.setHold(self.y_task_ptr, hold)
      
    
if __name__ == '__main__':
    from PTStepperMotor import PTStepperMotor
    from time import sleep
    pin_tuple = (4,17)
    stepper = PTStepperMotor (pin_tuple, 10, 2, PTSimpleGPIO.ACC_MODE_SLEEPS)

    stepper.move_rel (2)
    stepper.set_speed(2)
 
    
