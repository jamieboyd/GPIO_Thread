from array import array
import HX711
"""HX711 is a Python/C++ module from GPIO_Thread that does the GPIO clocking and data reading from
the HX711 load cell on a separate thread at C++ speed using the pulsedThread library
"""

class Scale:
    """
    Class to operate a scale based on an HX711 load cell amplifier
    """

    def __init__ (self,  dataPin, clockPin, gmPerUnit, arraySizeP):
        """  
        Initializes a Python scale object, containing a pointer to a C++
        object that does threaded and non-threaded reading from HX711
        load cell amplifier
        :param dataPin:pin connected to the DAT pin on the HX711 breakout.
        :param ClockPin:pin connected to the SCK pin on the HX711 breakout
        :param gmPerUnit:scaling in grams/ 24-bit A/D unit
        """
        self.threadArray = array ('f', [0] * arraySizeP)
        self.arraySize = arraySizeP
        self.hx711ptr = HX711.new (dataPin, clockPin, gmPerUnit, self.threadArray)
    
    def tare(self, nAvg, printVal):
        """
        Records the tare value, which will be subtracted from subsequent 
        scale reads. The tare value is stored in the C++ object
        :param nAvg:number of readings to average together to get tare
        :param printVal:if set, will print the tare value as well as save it
        :returns:the tare value, in A/D units, not scaled into grams
        """
        tareValue = HX711.tare (self.hx711ptr, nAvg)
        if printVal == True:
            for i in range (0, nAvg):
                print ('Tare {} value is {} A/D units'.format (i, self.threadArray [i]))
            print ('New Tare value is {:.5} A/D units'.format (tareValue))
        return tareValue

        
    def weigh (self, nAvg):
        """
        Gets an averaged weight value in grams with tare subtracted
        and scaling applied
        :param nAvg:the number of weight reading to average to get returned value
        :returns:weight in grams
        """
        return HX711.weigh (self.hx711ptr, nAvg)

    def weighOnce (self):
        """
        Gets weight from a single reading, in grams with tare and scaling applied
        :returns:weight in grams
        """
        return HX711.weigh (self.hx711ptr, 1)
    
        
    def threadStart (self, size):
        """
        Tells the C++ thread to start reading from the HX711 and placing results 
        in the array. When this function returns, the thread continues reading 
        until stopped or size weights have been obtained
        :param size:number of weights for the thread to place in the array. The array will be
        resized if needed
        """
        if size > self.arraySize:
            size = self.arraySize
        HX711.weighThreadStart(self.hx711ptr, size)

    def threadStop (self):
        """
        Tells the C++ thread to stop reading weights and filling the array
        :returns:the number of weights the thread has placed in the array
        """
        return HX711.weighThreadStop (self.hx711ptr)

    def threadCheck (self):
        """
        Gets the number of weights the thread has already placed in the array, but
        does not twell the thread to stop reading weights and filling the array
        """
        return HX711.weighThreadCheck (self.hx711ptr)
            
    def setScaling (self, newScaling):
        """
        Sets scaling in grams/24-bit A/D units for the load cell amplifier to the passed-in
        value. The scaling is stored in the C++ object
        :param newScaling:the scaling, in grams/unit, to apply to load cell data
        """
        HX711.setScaling (self.hx711ptr, newScaling)

    def setScalingFromStandard (self, standardWt):
        """
        Calculates scaling by measuring a standard weight, in grams
        """
        HX711.scalingFromStd (self.hx711ptr, stdWt, 10)

    def getScaling (self):
        """
        Gets the scaling in grams/24-bit A/D unit from the C++ object
        :returns:the scaling used for the load cell data in grams/24-bit A/D unit
        """
        return HX711.getScaling (self.hx711ptr)

    def getTareVal (self):
        """
        Gets the tare value in raw 24-bit A/D units from the C++ object
        :returns:tare value in raw 24-bit A/D units
        """
        return HX711.getTareValue (self.hx711ptr)

    def getClockPin (self):
        """
        Gets number of the GPIO pin to which the Pi exports the generated clock signal (SCK)
        :returns:GPIO pin number for clock signal
        """
        return HX711.getClockPin (self.hx711ptr)

    def getDataPin (self):
        """
        gets number of the GPIO pin from which the Pi reads the data (DAT)
        :return:number of the GPIO pin for data
        """
        return HX711.getDataPin (self.hx711ptr)

    def turnOn (self):
        """
        Makes sure the HX711 is ready to start weighing, waking it from the low power state if sleeping
        """
        HX711.turnOn (self.hx711ptr)

    def turnOff (self):
        """
        Puts the HX711 into a low power state
        """
        HX711.turnOff (self.hx711ptr)

    def scaleRunner (self, extraOptions):
        """
        Runs a loop that lets the user easily use the main functions of the scale
        plus user can select other options from calling code and get the user selection to handle themselves
        """
        inputStr = '------------------------------------Scale Runner------------------------------------\n'
        inputStr += '-1:\tQuit Scale Runner\n'
        inputStr += '0:\tTare the scale with average of 10 readings\n'
        inputStr += '1:\tPrint current Tare value\n'
        inputStr += '2:\tSet new scaling factor in grams per A/D unit\n'
        inputStr += '3:\tCalculate scaling from a standard weight\n'
        inputStr += '4:\tPrint current scaling factor\n'
        inputStr += '5:\tWeigh something with a single reading\n'
        inputStr += '6:\tWeigh something with an average of 10 readings\n'
        inputStr += '7:\tStart a threaded read\n'
        inputStr += '8:\tSet scale to low power mode\n'
        inputStr += '9:\tWake scale from low power mode\n'
        inputStr += extraOptions + ':'
        while True:
            event = int (input (inputStr))
            if event == -1:
                break
            if event == 0:
                self.tare(10, True)
            elif event ==1:
               print ('Curent Tare Value = {:.5} A/D units'.format(HX711.getTareValue (self.hx711ptr)))
            elif event == 2:
                try:
                    newScaling = float (input ("Enter new scaling factor in grams per A/D unit:"))
                except ValueError:
                    print ('Value Error: Next time, enter a number for the scaling')
                    continue
                HX711.setScaling (self.hx711ptr, newScaling)
            elif event == 3:
                try:
                    stdWt = float (input ("Put standard on scale and enter its weight in grams:"))
                except ValueError:
                    print ('Value Error: Next time, enter a number for the standard weight.')
                    continue
                print ('Calculated Scaling ={:.5} grams per A/D unit'.format (HX711.scalingFromStd (self.hx711ptr, stdWt, 10))) 
            elif event == 4:
                print ('Curent Scaling Factor = {:.5} grams per A/D unit'.format(HX711.getScaling (self.hx711ptr)))
            elif event == 5:
                print ('Weight = {:.5} grams'.format(HX711.weigh (self.hx711ptr, 1)))
            elif event == 6:
                print ('Average of 10 weighings = {:.5} grams'.format(HX711.weigh (self.hx711ptr, 10)))
            elif event == 7:
                self.threadStart (self.arraySize)
                nReads = self.threadCheck() 
                while nReads < self.arraySize:
                    print ("Thread has read {} weights, last reading was {:.5} grams".format (nReads, self.threadArray [nReads-1]))
                    nReads = self.threadCheck()
            elif event == 8:
                self.turnOff()
            elif event == 9:
                self.turnOn()
            else:
                break
        return event

