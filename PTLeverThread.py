#! /usr/bin/python
#-*-coding: utf-8 -*-

import ptLeverThread
import array
from time import sleep
import RPi.GPIO as GPIO
class PTLeverThread (object):
    """
    PTLeverThread controls a lever used for the AutoHeadFix program
    """
    MOTOR_ENABLE = 20  # GPIO pin to enable motor
    GOAL_CUER = 22     # GPIO pin to turn on when lever is in goal position
    FORCE_IS_SYMMETRIC = True
    """
    If True, 16 bit force output (0 - 4095) goes from full counterclockwise force to full clockwise force
    with midpoint (2048) being no force on lever. If False, force, 0-4095 maps from no force to full foprce, and
    a GPIO pin controls the direction of the force, clockwise or counter-clockwise
    """
    def __init__ (self, posBufSizeP, isCuedP, nCircOrToGoalP, isReversedP, goalCuerPinP, cuerFreqP, motorEnable):
        self.posBuffer = array.array('h', [0]*posBufSizeP)
        self.leverThread = ptLeverThread.new (self.posBuffer, isCuedP, nCircOrToGoalP, isReversedP, goalCuerPinP, cuerFreqP)
        self.posBufSize = posBufSizeP
        if isCuedP:
            self.nToGoal = nCircOrToGoalP
        else:
            self.nCirc = nCircOrToGoalP
        self.goalCuerPin= goalCuerPinP
        self.cuerFreq = cuerFreqP
        self.motorEnablePin = motorEnable
        GPIO.setmode (GPIO.BCM)
        GPIO.setup(self.motorEnablePin, GPIO.OUT)

    def setMotorEnable (self, motorState):
        if motorState == 0:
            GPIO.output (self.motorEnablePin, GPIO.LOW)
        else:
            GPIO.output (self.motorEnablePin, GPIO.HIGH)
        
    def setConstForce (self,newForce):
        ptLeverThread.setConstForce(self.leverThread, newForce)

    
    def getConstForce (self):
        return ptLeverThread.getConstForce(self.leverThread)


    def applyForce (self, theForce):
        ptLeverThread.applyForce(self.leverThread, theForce)

    def applyConstForce(self):
        ptLeverThread.applyConstForce (self.leverThread)

    def zeroLever (self, zeroMode, isLocking):
        return ptLeverThread.zeroLever(self.leverThread, zeroMode, isLocking)

    def setPerturbForce (self, perturbForce):
        ptLeverThread.setPerturbForce(self.leverThread, perturbForce)

    def setPerturbStartPos (self, startPos):
        ptLeverThread.setPerturbStartPos(self.leverThread, startPos)

    def startTrial (self):
        self.trialComplete = False
        ptLeverThread.startTrial(self.leverThread)
        while not self.trialComplete:
            print (self.checkTrial())
            sleep (0.1)

    def checkTrial (self):
        resultTuple = ptLeverThread.checkTrial(self.leverThread)
        self.trialComplete = resultTuple [0]
        self.trialPos = resultTuple [1]
        self.inGoal = resultTuple [2]
        return resultTuple
    
    def turnOnGoalCue (self):
        ptLeverThread.doGoalCue(self.leverThread, 1)
    
    def turnOffGoalCue (self):
        ptLeverThread.doGoalCue( self.leverThread, 0)


    def setHoldParams(self, goalBottom, goalTop, nHoldTicks):
        ptLeverThread.setHoldParams (self.leverThread, goalBottom, goalTop, nHoldTicks)

    def getLeverPos (self):
        return ptLeverThread.getLeverPos(self.leverThread)

    def abortUnCuedTrial(self):
        ptLeverThread.abortUncuedTrial(self.leverThread)

    def isCued (self):
        return ptLeverThread.isCued (self.leverThread)

    def setCued (self, isCued):
        return ptLeverThread.setCued (self.leverThread, isCued)
        
    def setTicksToGoal (self, ticksToGoal):
        ptLeverThread.setTicksToGoal (self.leverThread, ticksToGoal)
        
    
if __name__ == '__main__':
    lever = PTLeverThread (1000, True, 125, True, PTLeverThread.GOAL_CUER, 0, PTLeverThread.MOTOR_ENABLE)
    lever.setConstForce (300)
    
    
    
