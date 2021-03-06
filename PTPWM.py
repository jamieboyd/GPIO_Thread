#! /usr/bin/python
#-*-coding: utf-8 -*-

import ptPWM
from array import array
from time import sleep

class PTPWM (object):
    TRAIN = 1
    INFINITE_TRAIN = 0
    PWM_MARK_SPACE = 1
    PWM_BALANCED = 0
    MODE_FREQ = 0
    MODE_PULSES = 1
    
    def __init__(self, mode, pwmFreq, pwmRange, useFIFO, pulseDurationOrTrainFreq, nPulsesOrTrainDuration, accuracyLevel):
        if mode == PTPWM.MODE_PULSES:
            self.task_ptr = ptPWM.newDelayDur (pwmFreq, pwmRange, useFIFO, pulseDurationOrTrainFreq, nPulsesOrTrainDuration, accuracyLevel)
        elif mode == PTPWM.MODE_FREQ:
            self.task_ptr = ptPWM.newFreqDuty (pwmFreq, pwmRange, useFIFO, pulseDurationOrTrainFreq, nPulsesOrTrainDuration, accuracyLevel)
        self.respectTheGIL = False
        self.PWM_channels = 0
        self.useFIFO = useFIFO
        self.respectTheGIL = False
        if nPulsesOrTrainDuration == PTPWM.INFINITE_TRAIN:
            self.train_type = PTPWM.INFINITE_TRAIN
        else:
            self.train_type = PTPWM.TRAIN

    def get_PWM_frequency (self):
        return ptPWM.getPWMFreq (self.task_ptr)

    def get_PWM_range (self):
        return ptPWM.getPWMRange (self.task_ptr)

    def get_PWM_channels (self):
        return ptPWM.getChannels (self.task_ptr)

    def add_channel (self, channel, audioOnly, mode, polarity, offState, dataArray):
        errVal = ptPWM.addChannel (self.task_ptr, channel, audioOnly, mode, polarity, offState, dataArray)
        if errVal == 0:
            self.PWM_channels &= channel
            if channel ==1:
                self.audioOnly1 = audioOnly
                self.mode1 = mode
                self.enable1 = 0
                self.polarity1 = polarity
                self.offState1 = offState
                self.dataArray1 = dataArray
            elif channel ==2:
                self.audioOnly2 = audioOnly
                self.mode2 = mode
                self.enable2 = 0
                self.polarity2 = polarity
                self.offState2 = offState
                self.dataArray2 = dataArray
        return errVal
            

    def start_train (self):
        if self.train_type == PTPWM.INFINITE_TRAIN:
            ptPWM.startTrain (self.task_ptr)
        elif self.train_type == PTPWM.TRAIN:
            ptPWM.doTask (self.task_ptr)

    def start_trains (self, num_trains):
        if self.train_type == PTPWM.INFINITE_TRAIN:
            ptPWM.startTrain (self.task_ptr)
        elif self.train_type == PTPWM.TRAIN:
            ptPWM.doTasks (self.task_ptr, num_trains)
    
    def stop_train (self):
        if self.train_type == PTPWM.INFINITE_TRAIN:
            ptPWM.stopTrain (self.task_ptr)
        elif self.train_type == PTPWM.TRAIN:
            ptPWM.unDoTasks (self.task_ptr)
    

    def set_PWM_enable (self, enable_state, channel, is_locking):
        errVal = ptPWM.setEnable(self.task_ptr, enable_state, channel, is_locking)
        if errVal == 0:
            if channel & 1:
                self.enable1 = enable_state
            if channel & 2:
                self.enable2 = enable_state
        return errVal
    

    def set_PWM_polarity (self, polarity, channel, is_locking):
        errVal = ptPWM.setPolarity(self.task_ptr, polarity, channel, is_locking)
        if errVal == 0:
            if channel & 1:
                self.polarity1 = polarity
            if channel & 2:
                self.polarity2 = polarity
        return errVal
    

    def set_PWM_off_state(self, offState, channel, is_locking):
        errVal = ptPWM.setOffState (self.task_ptr, offState, channel, is_locking)
        if errVal == 0:
            if channel & 1:
                self.offState1 = offState
            if channel & 2:
                self.offState1 = offState
        return errVal

    def set_array_pos (self, array_pos, channel, is_locking):
        errVal = ptPWM.setArrayPos (self.task_ptr, array_pos, channel, is_locking)
        return errVal

    def set_array_subrange (self, start_pos, stop_pos, channel, is_locking):
        errVal = ptPWM.setArraySubRange (self.task_ptr, start_pos, stop_pos, channel, is_locking)
        return errVal

    def set_new_array(self, data_array, channel, is_locking):
        errVal = ptPWM.setArray (self.task_ptr, data_array, channel, is_locking)
        if errVal == 0:
            if channel & 1:
                self.dataArray1 = data_array
            if channel & 2:
                self.dataArray2 = data_array
        return errVal
    
    def get_enable (self):
        return self.enable1 + 2 * self.enable2


    def wait_on_busy(self, waitSecs):
        return ptPWM.waitOnBusy(self.task_ptr, waitSecs)


class PTPWMsin (PTPWM):
    
    def __init__(self, chans):
        self.task_ptr = ptPWM.newSin (chans)
        self.PWM_channels = chans

    def set_sin_freq (self, new_frequency, channel, is_Locking):
        ptPWM.setSinFreq(self.task_ptr, new_frequency, channel, is_Locking)
        self.sin_freq = new_frequency
        

    def get_sin_freq (self, channel):
        return ptPWM.getSinFreq(self.task_ptr,channel)

    def start (self):
        ptPWM.setEnable(self.task_ptr, 1, self.PWM_channels, 0)
        ptPWM.startTrain(self.task_ptr)
 

    def stop (self):
        ptPWM.stopTrain(self.task_ptr)
        ptPWM.setEnable (self.task_ptr, 0, self.PWM_channels, 1)


class PTPWMsimp (object):

    def __init__(self, PWM_frequency, PWM_range):
         self.task_ptr = ptPWM.newThreadless(PWM_frequency, PWM_range)

    def add_channel (self, channel, mode, polarity, off_state):
        ptPWM.threadlessAddChan(self.task_ptr, channel, mode, polarity, off_state)

    def set_value (self, PWM_value, channels):
        ptPWM.threadlessSetValue(self.task_ptr, PWM_value, channels)

    def set_able(self, able_state, channels):
        ptPWM.threadlessSetAble(self.task_ptr,able_state, channels)

    def get_PWM_value (self, channel):
        return ptPWM.threadlessGetValue(self.task_ptr, channel)
        
    
