#! /usr/bin/python
#-*-coding: utf-8 -*-

import ptLeverThread
import PTSimpleGPIO
import array

class PTLeverThread (object):
    """
    PTLeverThread controls a lever used for the AutoHeadFix program
    """
     def __init__ (self, gpio_pin, polarity, delay, duration, accuracy_level):
    
    
