#! /usr/bin/python
#-*-coding: utf-8 -*-

import ptPWM
import PTSimpleGPIO

class PTPWM (PTSimpleGPIO, metaclass =SingletonForGPIO):
    PTPWM_TRAIN =1
    PTPWM_INFINITE_TRAIN =0
    
    def __init__(self, mode, pwmFreq, pwmRange, pulseDurationOrTrainFreq, nPulsesOrTrainDuration, accuracyLevel):
        if mode == PTSimpleGPIO.MODE_PULSES:
            self.task_ptr = ptPWM.newDelayDur (pwmFreq, pwmRange, pulseDurationOrTrainFreq, nPulsesOrTrainDuration, accuracyLevel)
        elif mode == PTSimpleGPIO.MODE_FREQ:
            self.task_ptr = ptPWM.newFreqDuty (pwmFreq, pwmRange, pulseDurationOrTrainFreq, nPulsesOrTrainDuration, accuracyLevel)
        self.PWM_channels = 0
        self.respectTheGIL = False
        if nPulsesOrTrainDuration == PTPWM_INFINITE_TRAIN:
            self.train_type = PTPWM_INFINITE_TRAIN
        else:
            self.train_type = PTPWM_TRAIN


    def get_PWM_frequency (self):
        return ptPWM.getPWMFreq (self.task_ptr):

    def get_PWM_range (self):
        return ptPWM.getPWMRange (self.task_ptr)

    def get_PWM_channels (self):
        return ptPWM.getChannels (self.task_ptr)

    def add_channel (channel, audioOnly, useFIFO, mode, enable, polarity, offState, dataArray):
        errVal = ptPWM_addChannel (channel)
        if errVal == 0:
            self.PWM_channels &= channel
            if channel ==1:
                self.audioOnly1 = audioOnly
                self.useFIFO1=useFIFO
                self.mode1 = mode
                self.enable1 = enable
                self.polarity1 = polarity
                self.offState1 = offState
                self.dataArray1 = dataArray
            elif channel ==2:
                self.audioOnly2 = audioOnly
                self.useFIFO2=useFIFO
                self.mode2 = mode
                self.enable2 = enable
                self.polarity2 = polarity
                self.offState2 = offState
                self.dataArray2 = dataArray
        return errVal
            

    def start_train ():
        if self.train_type == PTPWM_INFINITE_TRAIN:
            ptPWM.startTrain (self.task_ptr)
        elif self.train_type == PTPWM_TRAIN:
            ptPWM.doTask (self.task_ptr)
        return errVal

    def start_trains (num_trains):
        if self.train_type == PTPWM_INFINITE_TRAIN:
            ptPWM.startTrain (self.task_ptr)
        elif self.train_type == PTPWM_TRAIN:
            ptPWM.doTasks (self.task_ptr, num_trains)
        return errVal
    

    def stop_train ():
        if self.train_type == PTPWM_INFINITE_TRAIN:
            ptPWM.stopTrain (self.task_ptr)
        elif self.train_type == PTPWM_TRAIN:
            ptPWM.unDoTasks (self.task_ptr)
        return errVal
    

    def set_PWM_enable (self, enable_state, channel is_locking):
        errVal = ptPWM.setEnable(self.task_ptr, enable_state, channel, is_locking)
        if errVal == 0:
            if channel ==1:
                self.enable1 = enable_state
            else:
                self.enable2 = enable_state
        return errVal
    

    def set_PWM_polarity (self, polarity, channel is_locking):
        errVal = ptPWM.setPolarity(self.task_ptr, enable_state, channel, is_locking)
        if errVal == 0:
            if channel ==1:
                self.polarity1 = polarity
            else:
                self.polarity2 = enable_polarity
        return errVal
    

    def set_PWM_off_state(self, offState, channel, is_locking):
        errVal = ptPWM.setOffState (self.task_ptr, offState, channel, is_locking)
        if errVal == 0:
            if channel ==1:
                self.offState1 = offState
            else:
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
            if channel == 1:
                self.dataArray1 = data_array
            elif channel == 2:
                self.dataArray2 = data_array
        return errVal
    
    def get_enable (self):
        return self.enabled

    def get_frequency (self, frequency, is_locking):
        return self.frequency
