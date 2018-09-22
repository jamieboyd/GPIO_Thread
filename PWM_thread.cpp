#include "PWM_thread.h"

/* ********************** non-class methods used from pulsed Thread *************************************

 ********************* PWM Initialization callback function ****************************************
Allocates task data for PWM thread.  Most data initialization is on a per channel basis, and
is done in the addChannel callback, so the initDataP is not used in this function.
last modified:
2018/09/18 by Jamie Boyd - channels and outputs modifications
2018/09/04 by Jamie Boyd - debugging
2018/08/07 by Jamie Boyd - modified for pulsedThread subclassing
2017/02/20 by Jamie Boyd - initial version  */
int ptPWM_Init (void * initDataP, void * &taskDataP){
#if beVerbose
		printf ("ptPWM_Init called.\n");
#endif
	// initDataP in this case is NULL, we don't use it
	// taskDataP is a pointer that needs to be allocated to taskCustomData
	ptPWMStructPtr taskData = new ptPWMStruct;
	taskDataP = taskData;
#if beVerbose
		printf ("ptPWM_Init: taskDataP initialized\n");
#endif
	// start with channels at 0
	taskData->channels = 0;
	// save ctl and data register addresses in task data for easy access
	taskData->ctlRegister = PWMperi->addr + PWM_CTL;
	taskData->statusRegister = PWMperi->addr + PWM_STA;
	taskData->FIFOregister = PWMperi->addr + PWM_FIF;
	taskData->dataRegister1 = PWMperi->addr + PWM_DAT1;
	taskData->dataRegister2 = PWMperi->addr + PWM_DAT2;
	return 0; 
}

/* ********************* PWM Channel Initialization callback function ****************************************
Configures a single channel (1 or 2) for a PWM thread.  Configures GPIO pins and writes configuration data to
PWM_CTL registers.  Sets fields in task Data.  Note that there is no harm in setting up a channel that is
already configured
last modified:
2019/09/20 by Jamie Boyd - added option to use FIFO
2018/09/19 by Jamie Boyd - initial version, moved from general initialization */
int ptPWM_addChannelCallback (void * modData, taskParams * theTask){
	
	ptPWMStructPtr taskData = (ptPWMStructPtr) theTask->taskData;
	ptPWMchanAddStructPtr chanAddPtr =(ptPWMchanAddStructPtr)modData;
	// some register address offsets and bits are channel specific, if we save them in variables
	// at the start, we can use same the code for either channel later on
	unsigned int dataRegisterOffset;
	unsigned int modeBit;
	unsigned int enableBit;
	unsigned int polarityBit;
	unsigned int offStateBit;
	unsigned int useFIFObit;
	unsigned int repeatFIFObit;
	if (chanAddPtr->channel == 1){
		// set channel 1 in taskData
		taskData->channels |= 1;
		// set array data
		taskData->arrayData1 = chanAddPtr->arrayData;
		taskData->nData1 = chanAddPtr->nData;
		taskData->startPos1 =0;
		taskData->stopPos1 = chanAddPtr->nData;
		taskData->useFIFO1=chanAddPtr->useFIFO;
		// set up GPIO
		if (chanAddPtr->audioOnly ){
			taskData->audioOnly1 =1;
		}else{
			INP_GPIO(GPIOperi ->addr, 18);           // Set GPIO 18 to input to clear bits
			SET_GPIO_ALT(GPIOperi ->addr, 18, 5);     // Set GPIO 18 to Alt5 function PWM0
			taskData->audioOnly1 =0;
		}
		// set bits and offsets appropriately for channel 1
		dataRegisterOffset = PWM_DAT1;
		modeBit = PWM_MSEN1;
		enableBit = PWM_PWEN1;
		polarityBit = PWM_POLA1;
		offStateBit = PWM_SBIT1;
		useFIFObit = PWM_USEF1;
		repeatFIFObit = PWM_RPTL1;
	}else{
		if (chanAddPtr->channel == 2){
			// set channel 2 in taskData
			taskData->channels |= 2;
			// set array data
			taskData->arrayData2 = chanAddPtr->arrayData;
			taskData->nData2 = chanAddPtr->nData;
			taskData->startPos1 =0;
			taskData->stopPos1 = chanAddPtr->nData;
			taskData->useFIFO2=chanAddPtr->useFIFO;
			// set up GPIO
			if (chanAddPtr->audioOnly){
				taskData->audioOnly2 = 1;
			}else{
				INP_GPIO(GPIOperi ->addr, 19);           // Set GPIO 19 to input to clear bits
				SET_GPIO_ALT(GPIOperi->addr, 19, 5);     // Set GPIO 19 to Alt5 function PWM1
				taskData->audioOnly2 =0;
			}
			// set bits and offsets appropriately for channel 2
			dataRegisterOffset = PWM_DAT2;
			modeBit = PWM_MSEN2;
			enableBit = PWM_PWEN2;
			polarityBit = PWM_POLA2;
			offStateBit = PWM_SBIT2;
			useFIFObit = PWM_USEF2;
			repeatFIFObit = PWM_RPTL2;
		}else{
			printf ("The PWM channel to be configured, %d, does not exist/n", chanAddPtr->channel);
			delete chanAddPtr;
			return 1;
		}
	}
	// instead of constantly writing to register, copy the register into a variable, then modify the variable
	// and write tot he register just one at the end
	unsigned int registerVal = *(PWMperi ->addr + PWM_CTL);
	// set up PWM channel 1 or 2 by writing to control register
	// set mode
	if (chanAddPtr->mode == PWM_MARK_SPACE){
		registerVal |= modeBit; // put PWM in MS Mode
	}else{
		registerVal &= ~(modeBit);  // clear MS mode bit for Balanced Mode
	}
	// set polarity
	if (chanAddPtr->polarity == 0){
		registerVal &= ~polarityBit;  // clear reverse polarity bit
	}else{
		registerVal |= polarityBit;  // set reverse polarity bit
	}
	// set off state, whether PWM output is hi or low when not transmitting data
	if (chanAddPtr->offState == 0){
		registerVal &= ~offStateBit; // clear OFFstate bit for low
	}else{
		registerVal |= offStateBit; // set OFFstate bit for hi
	}
	// set FIFO status
	if (chanAddPtr->useFIFO){
		registerVal |= useFIFObit;
		registerVal |= repeatFIFObit;
	}else{
		registerVal &= ~useFIFObit;
		registerVal &= ~repeatFIFObit;
	}
	// set initial enable state
	if (chanAddPtr->enable){
		// set initial PWM value first so we have something to put out
		*(PWMperi->addr + dataRegisterOffset) = chanAddPtr->arrayData[0]; 
		registerVal |= enableBit;
	}else{
		registerVal&= ~enableBit;
	}
	// finally, copy registerVal back to the register
	*(PWMperi ->addr + PWM_CTL)=registerVal;
#if beVerbose
	printf ("PWM Data register for channel %d contains %d.\n",chanAddPtr->channel , *(PWMperi->addr + dataRegisterOffset));
	printf ("PWM Control Register = 0x%x.\n", *(PWMperi ->addr + PWM_CTL));
#endif
	delete chanAddPtr;
	
	return 0;
}

/* *********************************** PWM Hi function (no low function) ******************************
gets the next value from the array to be output and writes it to the data register
Last Modified:
2018/09/21 by Jamie Boyd - adding FIFO
2018/09/18 by Jamie Boyd- updated for two channel 1 thread
2018/08/07 by Jamie Boyd -updated for pusledThread subclass */
void ptPWM_Hi (void * taskDataP){
	ptPWMStructPtr taskData = (ptPWMStructPtr)taskDataP;
	if (taskData->useFIFO1){
		while (!(*(taskData->statusRegister)  & PWM_FULL1)){
			if (taskData->channels & 1){
				*(taskData->FIFOregister) = taskData->arrayData1[taskData->arrayPos1];
				taskData->arrayPos1 += 1;
				if (taskData->arrayPos1 == taskData->stopPos1){
					taskData->arrayPos1 = taskData->startPos1;
				}
			}
			if (taskData->channels & 2){
				*(taskData->FIFOregister) = taskData->arrayData2[taskData->arrayPos2];
				taskData->arrayPos2 += 1;
				if (taskData->arrayPos2 == taskData->stopPos2){
					taskData->arrayPos2 = taskData->startPos2;
				}
			}
		}
	}else{
		if (taskData->channels & 1){
			*(taskData->dataRegister1) = taskData->arrayData1[taskData->arrayPos1];
			taskData->arrayPos1 += 1;
			if (taskData->arrayPos1 == taskData->stopPos1){
			taskData->arrayPos1 = taskData->startPos1;
			}
		}
		if (taskData->channels & 2){
			*(taskData->dataRegister2) = taskData->arrayData2[taskData->arrayPos2];
			taskData->arrayPos2 += 1;
			if (taskData->arrayPos2 == taskData->stopPos2){
			taskData->arrayPos2 = taskData->startPos2;
			}
		}
	}
}

/* ****************************** PWN Custom delete Function *****************************************************
deletes the custom data structure, that's it.  Thread does not "own" array data - whover made the thread deletes that array
Last Modified:
2018/08/07 by Jamie Boyd -updated for pusledThread subclass */
void ptPWM_delTask (void * taskData){
	ptPWMStructPtr pwmTaskPtr = (ptPWMStructPtr) taskData;
	delete pwmTaskPtr;
}

/* ****************** Callback to enable or disable PWM output on one or both channels ***********************
modData is a pointer to an int, bit 0 set for channel 1, bit 1 set for channel 2
bit 2 =4, set to enable output, unset to disable output
Last Modified:
2018/09/19 by Jamie Boyd - added channel info
2018/08/08 by Jamie Boyd - initial version */
int ptPWM_setEnableCallback (void * modData, taskParams * theTask){
	ptPWMStructPtr taskData = (ptPWMStructPtr) theTask->taskData;
	int * enablePtr =(int *)modData;
	if (*enablePtr & 4){ // we are enabling
		if (*enablePtr & 1){
			// set PWM value first so we are putting out current value
			*(taskData->dataRegister1) = taskData->arrayData1[taskData->arrayPos1];
			*(taskData -> ctlRegister) |= PWM_PWEN1; // set enable bit
			taskData->enable1 =1;
		}
		if (*enablePtr & 2){
			// set PWM value first so we are putting out current value
			*(taskData->dataRegister2) = taskData->arrayData2[taskData->arrayPos2];
			*(taskData -> ctlRegister) |= PWM_PWEN2; // set enable bit
			taskData->enable2 =1;
		}
	}else{ // we are un-enabling
		if (*enablePtr & 1){
			*(taskData -> ctlRegister) &= ~PWM_PWEN1; // set enable bit
			taskData->enable1 =0;
		}
		if (*enablePtr & 2){
			*(taskData -> ctlRegister) &= ~PWM_PWEN2; // set enable bit
			taskData->enable2 =0;
		}
	}
	delete enablePtr;
	return 0;
}

/* ***************** Callback to set polarity of PWM output ***********************
modData is a pointer to an int, bits 0 and 1 for channel
bit 2 =4, unset for normal polarity, set for reversed polarity
Last Modified:
2018/09/19 by Jamie Boyd - added channel info
2018/08/08 by Jamie Boyd - initial version */
int ptPWM_reversePolarityCallback (void * modData, taskParams * theTask){
	ptPWMStructPtr taskData = (ptPWMStructPtr) theTask->taskData;
	int * polarityPtr =(int *)modData;
	if (*polarityPtr & 4){
		if (*polarityPtr & 1){
			*(taskData -> ctlRegister) |= PWM_POLA1; // set reverse polarity
		}
		if (*polarityPtr & 2){
			*(taskData -> ctlRegister) |= PWM_POLA2; // set reverse polarity
		}
	}else{
		if (*polarityPtr & 1){
			*(taskData -> ctlRegister) &= ~PWM_POLA1; // un-set reverse polarity
		}
		if (*polarityPtr & 2){
			*(taskData -> ctlRegister) &= ~PWM_POLA2; // un-set reverse polarity
		}
	}
	delete polarityPtr;
	return 0;
}

/* ******************************** callback that Sets OFF state Of PWM output************************
modData is a pointer to an int, 0 for LOW when PWM is disabled, non-zero for HI 
Last Modified:
2018/09/19 by Jamie Boyd - added channel info
2018/08/08 by Jamie Boyd - initial version */
int ptPWM_setOffStateCallback (void * modData, taskParams * theTask){
	ptPWMStructPtr taskData = (ptPWMStructPtr) theTask->taskData;
	int * offStatePtr =(int *)modData;
	if (*offStatePtr & 4){
		if (*offStatePtr & 1){
			*(taskData -> ctlRegister ) |= PWM_SBIT1;
		}
		if (*offStatePtr & 2){
			*(taskData -> ctlRegister ) |= PWM_SBIT2;
		}
	} else{
		if (*offStatePtr & 1){
			*(taskData -> ctlRegister ) &= ~PWM_SBIT1;
		}
		if (*offStatePtr & 2){
			*(taskData -> ctlRegister ) &= ~PWM_SBIT2;
		}
	}
	delete (offStatePtr);
	return 0;
}

/* *************************** Sets Array start/end limits, current position, or changes array **********************************
modData is a ptPWMArrayModStruct structure of modBits, startPos, stopPos, arrayPos, and array data
last modified:
2018/08/20 by Jamie Boyd - cleaned up, fixed up
2018/08/08 by Jamie Boyd - initial version */
int ptPWM_ArrayModCalback  (void * modData, taskParams * theTask){
	ptPWMStructPtr taskData = (ptPWMStructPtr) theTask->taskData;
	ptPWMArrayModStructPtr modDataPtr = (ptPWMArrayModStructPtr) modData;
	// channel 1
	if ((modDataPtr->channel) & 1){
		// check if array data is being changed
		if ((modDataPtr->modBits) & 8){
			taskData->arrayData1 = modDataPtr->arrayData;
			taskData->nData1 = modDataPtr->nData;
		}
		// check for changing start pos or changing array data
		if ((modDataPtr->modBits) & 9){
			if (modDataPtr->startPos < taskData->nData1 -1){
				taskData ->startPos1 = modDataPtr->startPos;
			}else{
				taskData ->startPos1 = taskData->nData1-1;
			}
		}
		// check stop position or changing array data
		if ((modDataPtr->modBits) & 10){
			if (modDataPtr->stopPos > 0){
				if (modDataPtr->stopPos  < taskData->nData1){
					taskData -> stopPos1 = modDataPtr->stopPos;
				}else{
					taskData ->stopPos1 = taskData->nData1;
				}
			}else{
				taskData ->stopPos1 =1;
			}
		}
		// make sure start position <= stop position if data or positions changed
		if ((modDataPtr->modBits) & 11){
			if (taskData ->startPos1 > taskData->stopPos1){
				unsigned int temp = taskData ->startPos1;
				taskData ->startPos1 = taskData->stopPos1;
				taskData->stopPos1 = temp;
			}
		}
		// check current array position, anything we changed could affect it
		if ((modDataPtr->modBits) & 4){
			taskData-> arrayPos1 = modDataPtr-> arrayPos ;
		}
		if (taskData->arrayPos1 < taskData->startPos1){
			taskData->arrayPos1 = taskData->startPos1;
		}else{
			if (taskData->arrayPos1 >= taskData->stopPos1){
				taskData->arrayPos1 = taskData->stopPos1 -1;
			}
		}
		// update output
		*(taskData->dataRegister1) = taskData->arrayData1[taskData-> arrayPos1];
#if beVerbose
		printf ("start pos = %d, end pos = %d, nData = %d.\n",taskData->startPos1, taskData->stopPos1, taskData->nData1);
#endif
	}
	// channel 2
	if ((modDataPtr->channel) & 2){
		// check if array data is being changed
		if ((modDataPtr->modBits) & 8){
			taskData->arrayData2 = modDataPtr->arrayData;
			taskData->nData2= modDataPtr->nData;
		}
		// check for changing start pos or changing array data
		if ((modDataPtr->modBits) & 9){
			if (modDataPtr->startPos < taskData->nData2 -1){
				taskData ->startPos2 = modDataPtr->startPos;
			}else{
				taskData ->startPos2 = taskData->nData2-1;
			}
		}
		// check stop position
		if ((modDataPtr->modBits) & 10){
			if (modDataPtr->stopPos > 0){
				if (modDataPtr->stopPos  < taskData->nData2){
					taskData -> stopPos2 = modDataPtr->stopPos;
				}else{
					taskData ->stopPos2 = taskData->nData2;
				}
			}else{
				taskData ->stopPos2 =1;
			}
		}
		// make sure start position <= stop position
		if ((modDataPtr->modBits) & 11){
			if (taskData ->startPos2 > taskData->stopPos2){
				unsigned int temp = taskData ->startPos2;
				taskData ->startPos2 = taskData->stopPos2;
				taskData->stopPos2 = temp;
			}
		}
		// check current array position, anything we changed could affect it
		if ((modDataPtr->modBits) & 4){
			taskData-> arrayPos2 = modDataPtr-> arrayPos ;
		}
		if (taskData->arrayPos2 < taskData->startPos2){
			taskData->arrayPos2 = taskData->startPos2;
		}else{
			if (taskData->arrayPos2 >= taskData->stopPos2){
				taskData->arrayPos2 = taskData->stopPos2 -1;
			}
		}
		// update output
		*(taskData->dataRegister2) = taskData->arrayData2[taskData-> arrayPos2];
	}
	delete modDataPtr;
	return 0;
}

/* *****************************************************************************************************
********************************** PWM_thread Static Class Methods *************************************
Call these 2 methods before making a PWM_thread object. In this order:
1) map the peripherals
2) set the clock
3) make a PWM_thread
4) configure a PWM channel, 0 or 1 

****************************** Memory maps Peripherals for PWM *********PWM_thread.cpp:687:1:*********************************
 static function maps peripherals (GPIO, PWM, and PWM clock) needed for PWM. Does not set PWM clock. Does not set up any channels 
Last Modified: 2018/08/06 by Jamie Boyd - first version */
 int PWM_thread::mapPeripherals(){
	 // GPIOperi
	if (useGpioPeri() == nullptr){
#if beVerbose
		printf ("PWM_thread::mapPeripherals failed to map GPIO peripheral.\n");
#endif
		return 1;
	}
	//PWMperi
	if (usePWMPeri ()== nullptr){
#if beVerbose
		printf ("PWM_thread::mapPeripherals failed to map PWM peripheral.\n");
#endif
		return 2;
	}
	// PWM clock
	if (usePWMClockPeri ()== nullptr){
#if beVerbose
		printf ("PWM_thread::mapPeripherals failed to map PWM Clock peripheral.\n");
#endif
		return 3;
	}
	return 0;
}

/* ************************************sets PWM clock to give requested PWM frequency******************************************************
The PWM frequency is 1/(time taken to output a single value) which is determined by range and PWM clock frequency
The PWM range is the number of clock ticks needed to output a single value, i.e., the number of bits in the inputs
This PWM frequency is only accurate for the given PWMrange The two PWM channels could use different ranges, and thus have different PWM frequencies
even though they use the same PWM clock. We don't do that here; we always use the same range for both channels
Returns the actual PWM frequency, else returns -1 if the requested PWMFreq, PWMrange combination requires too high a clock speed
or -2 if the requested PWMFreq, PWMrange combination combination requires too low a clock speed 
Call this function before configuring any channels, because it zeros the PWM_CTL register, losing any channel configuration information
Last Modified: 
2018/09/19 by Jamie Boyd - set PWM range registers here, return calc freq to avoid static fields, use revamped lowlevel defs
2018/09/04 by Jamie Boyd - removed return value, freq saved in field
2018/08/06 by Jamie Boyd- modified for pulsedThread subclassing */
float PWM_thread::setClock (float PWMFreq, unsigned int PWMrange){
	// clock must be this fast in Hz to output PWMrange ticks in 1/PWMFreq seconds
	unsigned int clockFreq = PWMFreq * PWMrange;
#if beVerbose
	printf ("Requested PWM frequency = %.3f and range = %d, for a clock frequecy of %d.\n",  PWMFreq, PWMrange, clockFreq);
#endif
	// Choose the right clock source for the frequency, and divide it down. With MASH =2, mininum integer divider is 3
	unsigned int clockSrc =0; 
	unsigned int clockSrcRate =0;
	unsigned int mash = CM_MASH2; // start with 2 stage MASH, minimum divisir is 3
	// take it from the top, to use fastest source that we can
	if (clockFreq > (PLLD_CLOCK_RATE/2)){
		printf ("Requested PWM frequency %.3f and range %d require PWM clock rate of %.3f.\n", PWMFreq, PWMrange, (PWMFreq * PWMrange));
		printf ("Highest available PWM clock rate using PLL D with MASH=1 is %.3f.\n", (PLLD_CLOCK_RATE/2));
	}else{
		if (clockFreq > (PLLD_CLOCK_RATE/3)){
			mash = CM_MASH1;
			clockSrc = CM_SRCPLL;
			clockSrcRate = PLLD_CLOCK_RATE;
#if beVerbose
			printf ("PWM clock manager is using PWM clock source PLL D at 500 MHz with MASH =1.\n");
#endif
		}else{
			if (clockFreq > (PLLD_CLOCK_RATE/4095)){
				clockSrc = CM_SRCPLL;
				clockSrcRate = PLLD_CLOCK_RATE;
#if beVerbose
				printf ("PWM clock manager is using PWM clock source PLL D at 500 MHz with MASH =2.\n");
#endif
			}else{
				if (clockFreq > (HDMI_CLOCK_RATE/4095)){
					clockSrc = CM_SRCHDMI;
					clockSrcRate = HDMI_CLOCK_RATE;
#if beVerbose
					printf ("PWM clock manager is using HDMI Auxillary clock source at 216 MHz.\n");
#endif
				}else{
					if (clockFreq > (PI_CLOCK_RATE/4095)){
						clockSrc = CM_SRCOSC;
						clockSrcRate = PI_CLOCK_RATE;
#if beVerbose
						printf ("PWM clock manager is using PWM clock source oscillator at 19.2 MHz.\n");
#endif						
					}else{
						printf ("Calculated integer divisor, %d, is greater than 4095, the max divisor, you need to select a larger range or higher frequency\n", (int)(PI_CLOCK_RATE/clockFreq));
						return -2;
					}
				}					
			}
		}
	}
	// configure clock dividers
	unsigned int integerDivisor = clockSrcRate/clockFreq; // Divisor Value for clock, clock source freq/Divisor = PWM hz
	unsigned int fractionalDivisor = ((float)(clockSrcRate/clockFreq) - integerDivisor) * 4096;
	float actualClockRate = (clockSrcRate/(integerDivisor + (fractionalDivisor/4095)));
#if beVerbose
	printf ("Calculated integer divisor is %d and fractional divisor is %d for a clock rate of %.3f.\n", integerDivisor, fractionalDivisor, actualClockRate);
#endif	
	 // zero PWM_CTL register to turn off PWM (do this before configuring any channels, because you lose channel configuration information)
	*(PWMperi->addr + PWM_CTL) = 0; 
	// Turn off PWM clock enable flag and wait for clock busy flag to turn off
	*(PWMClockperi->addr + CM_PWMCTL) =  ((*PWMClockperi->addr + CM_PWMCTL) & CM_DISAB) | CM_PASSWD; 
	while(*(PWMClockperi->addr + CM_PWMCTL) & CM_BUSY);
#if beVerbose
	printf ("PWM Clock Busy flag turned off.\n");
#endif
	// configure clock divider REM writing to clock manager registers requires ORing with clock manager password
	*(PWMClockperi->addr + CM_PWMDIV) = (integerDivisor<<CM_DIVI)|(fractionalDivisor & 4095)|CM_PASSWD;
	 // configure PWM clock with selected src, with a 2-stage or 1-stage MASH as selected above
	*(PWMClockperi->addr + CM_PWMCTL) = mash | clockSrc | CM_PASSWD;
	// enable clock
	*(PWMClockperi->addr + CM_PWMCTL) = mash | clockSrc | CM_PASSWD | CM_ENAB ;
	while(!(*(PWMClockperi->addr + CM_PWMCTL) & CM_BUSY)); // Wait for clock busy flag to turn on.
#if beVerbose
	printf ("PWM Clock Busy flag turned on again.\n");
#endif
	// set range for both channels to the passed-in range, we always do PWM at same range for both channels
	*(PWMperi ->addr + PWM_RNG1) = PWMrange; // set range for channel 1
	*(PWMperi ->addr + PWM_RNG2) = PWMrange; // set range for channel 2
	// calculate and return actual PWM frequency
	return actualClockRate/PWMrange;
}

/* *************************************** Static ThreadMakers **************************************************************
Note that the timing of the thread that updates the value output by the PWM is independent of the PWM frequency. The update speed
of the thread should be no faster than the PWM frequency.
Last Modified:
2018/09/18 by Jamie Boyd - Revamping to just create thread and initialize clock, will add channels with different function
2018/08/06 by Jamie Boyd - Initial Version, copied and modified from GPIO thread maker */

// ***************** Integer pulse duration and number of pulses timing description inputs ************************************
PWM_thread * PWM_thread::PWM_threadMaker (float PWMFreq, unsigned int PWMrange, unsigned int durUsecs, unsigned int nPulses, int accuracyLevel) {
	
	// ensure peripherals for PWM controller are mapped
	int mapResult = PWM_thread::mapPeripherals ();
	if (mapResult){
		printf ("Could not map peripherals for PWM access with return code %d.\n", mapResult);
		return nullptr;
	}
	// set clock for PWM from input parmamaters
	float setFrequency = PWM_thread::setClock (PWMFreq, PWMrange);
	if (setFrequency < 0){
		printf ("Could not set clock for PWM with frequency = %.3f and range = %d.\n", PWMFreq, PWMrange);
		return nullptr;
	}
#if beVerbose
		printf ("Calculated PWM update frequency = %.3f\n", setFrequency);
#endif
	// call PWM_thread constructor with no init data, which calls pulsedThread constructor
	int errCode =0;
	PWM_thread * newPWM_thread = new PWM_thread (durUsecs, nPulses, accuracyLevel, errCode);
	if (errCode){
#if beVerbose
		printf ("PWM_threadMaker failed to make PWM_thread with errCode %d.\n", errCode);
#endif
		return nullptr;
	}
	// set custom task delete function
	newPWM_thread->setTaskDataDelFunc (&ptPWM_delTask);
	// set fields for freq, range, and channels
	newPWM_thread->PWMfreq = setFrequency;
	newPWM_thread->PWMrange = PWMrange;
	newPWM_thread->PWMchans = 0;
	// return new thread
	return newPWM_thread;
}

// *************** floating point frequency and trainDuration timing description inputs *****************************
PWM_thread * PWM_thread::PWM_threadMaker (float pwmFreq, unsigned int pwmRangeP,  float frequency, float trainDuration, int accuracyLevel){
	// ensure peripherals for PWM controller are mapped
	
	int mapResult = PWM_thread::mapPeripherals ();
	if (mapResult){
		printf ("Could not map peripherals for PWM access with return code %d.\n", mapResult);
		return nullptr;
	}
#if beVerbose
		printf ("All needed PWM peripherals were mapped\n");
#endif
	// set clock for PWM from input parmamaters
	float setFrequency = PWM_thread::setClock (pwmFreq, pwmRangeP);
	if (setFrequency < 0){
		printf ("Could not set clock for PWM with frequency = %.3f and range = %d.\n", pwmFreq, pwmRangeP);
		return nullptr;
	}
#if beVerbose
		printf ("PWM update frequency = %.3f\n", setFrequency);
#endif	
	// call PWM_thread constructor with no init data, which calls pulsedThread constructor
	int errCode =0;
	PWM_thread * newPWM_thread = new PWM_thread (frequency, trainDuration, accuracyLevel, errCode);
	if (errCode){
#if beVerbose
		printf ("PWM_threadMaker failed to make PWM_thread with errCode %d.\n", errCode);
#endif
		return nullptr;
	}
	// set custom task delete function
	newPWM_thread->setTaskDataDelFunc (&ptPWM_delTask);
	// set fields for freq, range, and channels
	newPWM_thread->PWMfreq = setFrequency;
	newPWM_thread->PWMrange = pwmRangeP;
	newPWM_thread->PWMchans = 0;
	// return new thread
	return newPWM_thread;
}

/* ************************************** PWM_thread Instance methods*******************************
****************** Destructor handles peripheral mapping and unsets alternate *************************
Thread data is destroyed by the pulsedThread destructor.  Array data is not "owned" by the class or the thread
All we need to do here is unset the alternate function for the GPIO pins and decrement the tally for the peripheral mappings
Last Modified:
2018/08/07 by Jamie Boyd - Initial Version */
PWM_thread::~PWM_thread (){
	if (PWMchans & 1){
		if (audioOnly1 == 0){
			INP_GPIO(GPIOperi->addr,18);
			OUT_GPIO(GPIOperi->addr,18);
			GPIO_CLR(GPIOperi->addr, (1 << 18));
		}
	}
	if (PWMchans & 2){
		if (audioOnly2 == 0){
			INP_GPIO(GPIOperi->addr,19);
			OUT_GPIO(GPIOperi->addr,19);
			GPIO_CLR(GPIOperi->addr, (1 << 19));
		}
	}
	unUseGPIOperi();
	unUsePWMperi();
	unUsePWMClockperi();
}

/* ********************************** Configures a PWM Channel ***************************************************
Makes and fills ptPWMchanStruct and calls ptPWM_addChannelCallback
Last Modified:
2018/09/20 by Jamie Boyd - added option to use a FIFO
2018/09/20 by Jamie Boyd - made call to modCustom non-locking, so best not add channels while thread is running
2018/09/19 by Jamie Boyd - initial version */
int PWM_thread::addChannel (int channel, int audioOnly, int useFIFO, int mode, int enable, int polarity, int offState, int * arrayData, unsigned int nData){
	
	ptPWMchanAddStructPtr addStructPtr = new ptPWMchanAddStruct;
	addStructPtr->channel = channel;
	addStructPtr->audioOnly = audioOnly;
	addStructPtr->useFIFO = useFIFO;
	addStructPtr->mode = mode;
	addStructPtr->enable = enable;
	addStructPtr->polarity = polarity;
	addStructPtr->offState = offState;
	addStructPtr->arrayData = arrayData;
	addStructPtr->nData = nData;
	int errVal = modCustom (&ptPWM_addChannelCallback, (void *)addStructPtr, 0); 
	if (errVal){
		return errVal;
	}
	if (channel == 1){
		PWMchans |= 1;
		audioOnly1 = audioOnly;
		mode1 = mode;
		enabled1 = enable;
		polarity1 = polarity;
		offState1 = offState;
	}else{
		if (channel == 2){
			PWMchans |= 2;
			audioOnly2 = audioOnly;
			mode2 = mode;
			enabled2 = enable;
			polarity2 = polarity;
			offState2 = offState;
		}
	}
	return 0;
}

/* ******************************* Setters of PWM channel Configuration and Data *********************************

******************************* sets Enable State **************************************************************
Last Modified:
2018/09/19 by Jamie Boyd - added channel parameter
2018/08/08 by Jamie Boyd - Initial Version  */
int PWM_thread::setEnable (int enableState, int channel, int isLocking){
	if (channel & 1){
		enabled1 = enableState;
	}
	if (channel & 2){
		enabled2 = enableState;
	}
	int * newEnableVal = new int;
	* newEnableVal =  (enableState << 2) + channel;
	int returnVal = modCustom (&ptPWM_setEnableCallback, (void *) newEnableVal, isLocking);
	return returnVal;
}

/* ****************************** sets Polarity ************************************
Last Modified:
2018/09/19 by Jamie Boyd - added channel parameter
2018/08/08 by Jamie Boyd - Initial Version  */
int PWM_thread::setPolarity (int polarityP, int channel, int isLocking){
	
	if (channel & 1){
		polarity1 = polarityP;
	}
	if (channel & 2){
		polarity2 = polarityP;
	}
	int * newPolarityVal = new int;
	* newPolarityVal =  (polarityP << 2) + channel;
	int returnVal = modCustom (&ptPWM_reversePolarityCallback, (void *) newPolarityVal, isLocking);
	return returnVal;
}

/* ****************************** sets OFF state ************************************
Last Modified:
2018/09/19 by Jamie Boyd - added channel parameter
2018/08/08 by Jamie Boyd - Initial Version  */
int PWM_thread::setOffState (int offStateP, int channel, int isLocking){
	
	if (channel & 1){
		offState1 = offStateP;
	}
	if (channel & 2){
		offState2 = offStateP;
	}
	int * newOffstateVal = new int;
	* newOffstateVal =  (offStateP << 2) + channel;
	int returnVal = modCustom (&ptPWM_setOffStateCallback, (void *) newOffstateVal, isLocking);
	return returnVal;
}

/* *************************************************** Sets array sub-range to use ********************************
Last Modified:
2018/09/20 by Jamie Boyd - initial version */
int PWM_thread::setArraySubrange (unsigned int startPos, unsigned int stopPos, int channel, int isLocking){

	ptPWMArrayModStructPtr arrayMod = new ptPWMArrayModStruct;
	arrayMod->startPos = startPos;
	arrayMod->stopPos = stopPos;
	arrayMod->channel = channel;
	arrayMod->modBits = 3; // 1 for start plus 2 for stop
	int returnVal = modCustom (&ptPWM_ArrayModCalback, (void *) arrayMod, isLocking);
	return returnVal;
}

/* **********************************Sets position in the array to be output next ************************
Last Modified:
2018/09/20 by Jamie Boyd - initial version */
int PWM_thread::setArrayPos (unsigned int arrayPos, int channel, int isLocking){

	ptPWMArrayModStructPtr arrayMod = new ptPWMArrayModStruct;
	arrayMod->arrayPos = arrayPos;
	arrayMod->modBits = 4;
	int returnVal = modCustom (&ptPWM_ArrayModCalback, (void *) arrayMod, isLocking);
	return returnVal;
}

/* **********************************Sets a new array of data to be output next ************************
Last Modified:
2018/09/20 by Jamie Boyd - initial version */
int PWM_thread::setNewArray (int * arrayData, unsigned int nData, int channel, int isLocking){
	ptPWMArrayModStructPtr arrayMod = new ptPWMArrayModStruct;
	arrayMod->arrayData = arrayData;
	arrayMod->nData = nData;
	arrayMod->modBits = 8;
	int returnVal = modCustom (&ptPWM_ArrayModCalback, (void *) arrayMod, isLocking);
	return returnVal;
}

/* ******************************* Getters of PWM Channel Configuration information *****************************

*********************************** Returns PWM Frequency ******************************************************
The rate in Hz at which the PWM value is refreshed from the data register or from the FIFO
Last Modified:
2018/09/21 by Jamie Boyd - initial version */
float PWM_thread::getPWMFreq (void){
	return PWMfreq;
}

/* ********************************** Returns PWM Range ******************************************************
The same PWM range is used by both channels
Last Modified:
2018/09/21 by Jamie Boyd - initial version */
unsigned int PWM_thread::PWM_thread::getPWMRange (void){
	return PWMrange;
}

int PWM_thread::getChannels (void){
	return PWMchans;
}

/* ************************* Returns a structure containing information about a channel **********************
returns a nullPtr if the channel is not 1 or 2, or if the channel has not been added
The calling function is reponsible for deleting the returned ptPWMchanInfoStructPtr, if not null
Last Modified:
2018/09/21 by Jamie Boyd - initial version */
ptPWMchanInfoStructPtr PWM_thread::getChannelInfo (int theChannel){
	ptPWMchanInfoStructPtr infoPtr = nullptr;
	// check if theChannel is 1 or 2
	if (!((theChannel ==1) || (theChannel ==2))){
#if beVerbose
		printf ("The requested channel, %d, does not existl; Channel must be either 1or 2.\n", theChannel);
#endif
		return infoPtr;
	}
	// check that channel has been configured
	if (!(PWMchans & theChannel)){
#if beVerbose
		printf ("The requested channel, %d, has not been configured.\n", theChannel);
#endif
		return infoPtr;
	}
	// now we know we have a channel we allocate some memory
	infoPtr = new ptPWMchanInfoStruct;
	if (theChannel == 1){
		infoPtr->audioOnly = audioOnly1;
		infoPtr->useFIFO = useFIFO1;
		infoPtr->mode = mode1;
		infoPtr->enable = enabled1;
		infoPtr->polarity = polarity1;
		infoPtr->offState = offState1;
	}else{
		infoPtr->audioOnly = audioOnly2;
		infoPtr->useFIFO = useFIFO2;
		infoPtr->mode = mode2;
		infoPtr->enable = enabled2;
		infoPtr->polarity = polarity2;
		infoPtr->offState = offState2;
	}
	return infoPtr;
}
