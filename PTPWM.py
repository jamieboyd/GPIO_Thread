#! /usr/bin/python
#-*-coding: utf-8 -*-

import ptPWM
from PTSimpleGPIO import PTSimpleGPIO,SingletonForGPIO
from array import array
from time import sleep

class PTPWM (metaclass = SingletonForGPIO):
    TRAIN =1
    INFINITE_TRAIN = 0
    PWM_MARK_SPACE =0
    PWM_BALANCED =1
    
    def __init__(self, mode, pwmFreq, pwmRange, useFIFO, pulseDurationOrTrainFreq, nPulsesOrTrainDuration, accuracyLevel):
        if mode == PTSimpleGPIO.MODE_PULSES:
            self.task_ptr = ptPWM.newDelayDur (pwmFreq, pwmRange, useFIFO, pulseDurationOrTrainFreq, nPulsesOrTrainDuration, accuracyLevel)
        elif mode == PTSimpleGPIO.MODE_FREQ:
            self.task_ptr = ptPWM.newFreqDuty (pwmFreq, pwmRange, useFIFO, pulseDurationOrTrainFreq, nPulsesOrTrainDuration, accuracyLevel)
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

    def add_channel (self, channel, audioOnly, mode, enable, polarity, offState, dataArray):
        errVal = ptPWM.addChannel (self.task_ptr, channel, audioOnly, mode, enable, polarity, offState, dataArray)
        if errVal == 0:
            self.PWM_channels &= channel
            if channel ==1:
                self.audioOnly1 = audioOnly
                self.mode1 = mode
                self.enable1 = enable
                self.polarity1 = polarity
                self.offState1 = offState
                self.dataArray1 = dataArray
            elif channel ==2:
                self.audioOnly2 = audioOnly
                self.mode2 = mode
                self.enable2 = enable
                self.polarity2 = polarity
                self.offState2 = offState
                self.dataArray2 = dataArray
        return errVal
            

    def start_train (self):
        if self.train_type == PTPWM.INFINITE_TRAIN:
            errVal = ptPWM.startTrain (self.task_ptr)
        elif self.train_type == PTPWM.TRAIN:
            errVal = ptPWM.doTask (self.task_ptr)
        return errVal

    def start_trains (num_trains):
        if self.train_type == PTPWM.INFINITE_TRAIN:
            errVal = ptPWM.startTrain (self.task_ptr)
        elif self.train_type == PTPWM.TRAIN:
            errVal =ptPWM.doTasks (self.task_ptr, num_trains)
        return errVal
    

    def stop_train (self):
        if self.train_type == PTPWM.INFINITE_TRAIN:
            errVal =ptPWM.stopTrain (self.task_ptr)
        elif self.train_type == PTPWM.TRAIN:
            errVal =ptPWM.unDoTasks (self.task_ptr)
        return errVal
    

    def set_PWM_enable (self, enable_state, channel, is_locking):
        errVal = ptPWM.setEnable(self.task_ptr, enable_state, channel, is_locking)
        if errVal == 0:
            if channel ==1:
                self.enable1 = enable_state
            else:
                self.enable2 = enable_state
        return errVal
    

    def set_PWM_polarity (self, polarity, channel, is_locking):
        errVal = ptPWM.setPolarity(self.task_ptr, polarity, channel, is_locking)
        if errVal == 0:
            if channel ==1:
                self.polarity1 = polarity
            else:
                self.polarity2 = polarity
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


class PTPWMsin (PTPWM):
    
    def __init__(self, chans):
        self.task_ptr = ptPWM.newSin (chans)
        self.PWM_channels = chans
        self.useFIFO = 1
        self.respectTheGIL = False
        self.train_type = PTPWM.INFINITE_TRAIN


    def set_sin_freq (self, new_frequency, channel, is_Locking):
        ptPWM.setSinFreq(self.task_ptr, new_frequency, channel, is_Locking)
        self.sin_freq = new_frequency
        

    def get_sin_freq (self, channel):
        return ptPWM.getSinFreq(self.task_ptr,channel)

    def start (self):
        self.start_train ()
        self.set_PWM_enable (1,self.PWM_channels,0)

    def stop (self):
        self.stop_train ()
        self.set_PWM_enable (0,self.PWM_channels,0)

if __name__ == '__main__':
    import PTPWM
    from PTPWM import PTPWM, PTPWMsin
    wavy = PTPWM (PTSimpleGPIO.MODE_FREQ, 1E03, 1000, 0, 2, 0, 1)
    dataArray = array('i', (i for i in range (0, 1000)))
    wavy.add_channel (1, 0, PTPWM.PWM_BALANCED, 0, 0, 0, dataArray)
    wavy.set_PWM_enable (1, 1, 0)
    wavy.start_train()
    sleep (10)
    wavy.set_PWM_enable (0, 1, 0)
    wavy.stop_train()
    del wavy
    siner = PTPWMsin (1)
    siner.set_sin_freq(1000, 1, 1)
    siner.start()
    sleep (5)
    siner.set_sin_freq(2000, 1, 1)
    sleep (5)
    siner.set_sin_freq(7000, 1, 1)
    sleep (5)
    siner.stop()
    del siner

