#! /usr/bin/python
#-*-coding: utf-8 -*

import leverThread
from array import array

from PTSimpleGPIO import PTSimpleGPIO, Infinite_train, Train
from AHF_Stimulator import AHF_Stimulator
from AHF_Rewarder import AHF_Rewarder
from AHF_LickDetector import AHF_LickDetector
from AHF_Mouse import Mouse
import time
import json
import os
from time import time, sleep
from datetime import datetime
from random import random


class AHF_Stimulator_Lever (AHF_Stimulator):
    cueMode_def = 
    


    def __init__ (self, configDict, rewarder, lickDetector, textfp):

        
posArray = array ('B', [0] * 300)

mylever = leverThread.new(posArray, 25, 23,0)

leverThread.startTrial(mylever)
leverThread.checkTrial (mylever)
