#include "PWM_thread.h"
	
	
/* ********************* PWM Initialization callback function ****************************************
Initializes a single channel, either 0 or 1 
last modified:
2018/09/04 by Jamie Boyd - debugging
2018/08/07 by Jamie Boyd - modified for pulsedThread subclassing
2017/02/20 by Jamie Boyd - initial version  */
int ptPWM_Init (void * initDataP, void *  &taskDataP){
#if beVerbose
		printf ("ptPWM_Init called.\n");
#endif
	// initData is a pointer to our init structure
	ptPWMinitStructPtr initData = (ptPWMinitStructPtr)initDataP;
	// taskDataP is a pointer to taskCustomData that needs to be initialized from our custom init structure
	ptPWMStructPtr taskData = new ptPWMStruct;
	taskDataP = taskData;
#if beVerbose
		printf ("ptPWM_Init: taskDataP initialized\n");
#endif
	// copy PWM settings, and set defaults
	taskData->channel = initData->channel;
	taskData->mode = initData->mode;
	taskData->enable = initData->enable;
	taskData->polarity =0;
	taskData->offState =0;
	// copy pointer to data, and set start, end, and current position to full range
	taskData->arrayData= initData->arrayData;
	taskData->nData = initData->nData;
	taskData->startPos =0;
	taskData->arrayPos =0;
	taskData->endPos =taskData->nData - 1;
	// set up PWM channel 0 or 1 by writing to control register and range register
	// some register address offsets and bits vary by chanel
	unsigned int dataRegisterOffset;
	unsigned int rangeRegisterOffset;
	unsigned int modeBit;
	unsigned int enableBit;
	unsigned int polarityBit;
	unsigned int offStateBit;
	// channel = 0 done on GPIO 18 or 1 done on GPIO 19, 2 for channel 0 through audio on pin 40, 3 for channel 1 on audio through pin 41
	// that is, bit 0 is for the channel and bit 1 is for the output
	if (initData->channel & 1){ // bit 1 is set for channel 1 (on GPIO 18 or 40 for audio), unset for channel 0 (on GPIO 19 or 41 for audio)
		if (initData->channel & 2){ // send channel 1 to audio output
			INP_GPIO(GPIOperi ->addr,41);           	// Set GPIO 41 to input to clear bits
			SET_GPIO_ALT(GPIOperi ->addr,41,0);     // Set GPIO 41 to Alt0 function PWM1
		}else{
			INP_GPIO(GPIOperi ->addr,19);           	// Set GPIO 19 to input to clear bits
			SET_GPIO_ALT(GPIOperi ->addr,19,5);     // Set GPIO 19 to Alt5 function PWM1
		}
		rangeRegisterOffset = PWM1_RNG;
		dataRegisterOffset = PWM1_DAT;
		modeBit = PWM1_MS_MODE;
		enableBit = PWM1_ENABLE;
		polarityBit = PWM1_REVPOLAR;
		offStateBit = PWM1_OFFSTATE;
	}else{
		if (initData->channel & 2){ // send channel 0 to audio output
			INP_GPIO(GPIOperi ->addr, 40);           // Set GPIO 40to input to clear bits
			SET_GPIO_ALT(GPIOperi ->addr, 40, 0);     // Set GPIO 40 to Alt0 function PWM0
		}else{
			INP_GPIO(GPIOperi ->addr, 18);           // Set GPIO 18 to input to clear bits
			SET_GPIO_ALT(GPIOperi ->addr, 18, 0);     // Set GPIO 18 to Alt5 function PWM0
		}
		rangeRegisterOffset = PWM0_RNG;
		dataRegisterOffset = PWM0_DAT;
		modeBit = PWM0_MS_MODE;
		enableBit = PWM0_ENABLE;
		polarityBit = PWM0_REVPOLAR;
		offStateBit = PWM0_OFFSTATE;

	}
	// set range
	*(PWMperi ->addr  + rangeRegisterOffset) = initData->range; // set range
	// set mode
	if (initData->mode ==PWM_MARK_SPACE){
		*(PWMperi ->addr + PWM_CTL) |= modeBit; // put PWM in MS Mode
	}else{
		*(PWMperi ->addr  + PWM_CTL) &= ~(modeBit);  // clear MS mode bit for balanced mode
	}
	// set polarity to non-reversed
	*(PWMperi ->addr  + PWM_CTL) &= ~polarityBit;  // clear reverse polarity bit
	// set off state to low 
	*(PWMperi ->addr  + PWM_CTL) &= ~offStateBit; // clear OFFstate bit
	// set enable state
	if (initData->enable){
		// set initial PWM value first so we have something to put out
		*(PWMperi ->addr  + dataRegisterOffset) = taskData->arrayData[0]; 
		*(PWMperi ->addr  + PWM_CTL) |= enableBit;
	}else{
		*(PWMperi ->addr  + PWM_CTL) &= ~enableBit;
	}
	// save ctl and data register addresses in task data for easy access
	taskData -> ctlRegister = PWMperi->addr + PWM_CTL;
	taskData->dataRegister = PWMperi->addr + dataRegisterOffset;
	delete initData;
	return 0; 
}

/* *********************************** PWM Hi function (no low function) ******************************
gets the next value from the array to be output and writes it to the data register
Last Modified:
2018/08/07 by Jamie Boyd -updated for pusledThread subclass */
void ptPWM_Hi (void * taskDataP){
	ptPWMStructPtr taskData = (ptPWMStructPtr)taskDataP;
	if (taskData->arrayPos < taskData->endPos){
		taskData->arrayPos += 1;
	}else{
		taskData->arrayPos = taskData->startPos;
	}
	*(taskData->dataRegister) = taskData->arrayData[taskData->arrayPos];
}

/* ****************************** PWN Custom delete Function *****************************************************
deletes the custom data structure, that's it
Last Modified:
2018/08/07 by Jamie Boyd -updated for pusledThread subclass */
void ptPWM_delTask (void * taskData){
	ptPWMStructPtr pwmTaskPtr = (ptPWMStructPtr) taskData;
	delete (pwmTaskPtr);
}


/* **************** PWM Custom data mod callbacks *****************************

******************* Enables or disables PWM output ***********************
modData is a pointer to an int, 0 to disable output, non-zero to enable output 
Last Modified:
2018/08/08 by Jamie Boyd - initial version */
int ptPWM_setEnableCallback (void * modData, taskParams * theTask){
	ptPWMStructPtr taskData = (ptPWMStructPtr) theTask->taskData;
	int * enablePtr =(int *)modData;
	unsigned int enableBit;
	if (taskData->channel ==0){
		enableBit = PWM0_ENABLE;
	}else{
		enableBit = PWM1_ENABLE;
	}
	if (*enablePtr == 0){
		*(taskData -> ctlRegister) &= ~enableBit; // clear enable bit
	}else{
		// set PWM value first so we are putting out current value
		*(taskData->dataRegister) = taskData->arrayData[taskData->arrayPos];
		*(taskData -> ctlRegister) |= enableBit; // set enable bit
	}
	delete (enablePtr);
	return 0;
}

/* ***************** Sets polarity of PWM output ***********************
modData is a pointer to an int, 0 for normal polarity, 1 for reversed polarity
Last Modified:
2018/08/08 by Jamie Boyd - initial version */
int ptPWM_reversePolarityCallback (void * modData, taskParams * theTask){
	ptPWMStructPtr taskData = (ptPWMStructPtr) theTask->taskData;
	int * polarityPtr =(int *)modData;
	unsigned int polarityBit;
	if (taskData->channel ==0){
		polarityBit = PWM0_REVPOLAR;;
	}else{
		polarityBit = PWM1_REVPOLAR;
	}
	if (*polarityPtr == 0){ // normal polarity
		*(taskData -> ctlRegister)  &= ~polarityBit;  // clear reverse polarity bit
	}else{
		*(taskData -> ctlRegister) = polarityBit ; // set reverse polarity
	}
	delete (polarityPtr);
	return 0;
}

/* ******************************** Sets OFF state Of PWM output************************
modData is a pointer to an int, 0 for LOW when PWM is disabled, non-zero for HI 
Last Modified:
2018/08/08 by Jamie Boyd - initial version */
int ptPWM_setOffStateCallback (void * modData, taskParams * theTask){
	ptPWMStructPtr taskData = (ptPWMStructPtr) theTask->taskData;
	int * offStatePtr =(int *)modData;
	unsigned int offStateBit;
	if (taskData->channel ==0){
		offStateBit = PWM0_OFFSTATE;
	}else{
		offStateBit = PWM1_OFFSTATE;
	}
	if (*offStatePtr ==0){ // low
		*(taskData -> ctlRegister ) &= ~offStateBit; // clear OFFstate bit
	}else{
		*(taskData -> ctlRegister ) |= offStateBit; // set OFFstate bit
	}
	delete (offStatePtr);
	return 0;
}

/* *************************** Sets Array position in the array of data to output ***************
modData is a pointer to an unsigned int, the array Position, to be next output, and outputs it
last modified:
2018/08/08 by Jamie Boyd - updated for pulsedThread subclass
2017/04/04 by Jamie Boyd - initial version   */
int ptPWM_setArrayPosCallback (void * modData, taskParams * theTask){
	unsigned int * newArrayPos= (unsigned int *)modData;
	ptPWMStructPtr taskData = (ptPWMStructPtr) theTask->taskData;
	taskData ->arrayPos = *newArrayPos;
	*(taskData->dataRegister) = taskData->arrayData[taskData ->arrayPos];
	delete newArrayPos;
	return 0;
}

/* *************************** Sets Array start/end limits, current position, or changes array **********************************
modData is a ptPWMArrayModStruct structure of modBits, startPos, endPos, arrayPos, and array data
last modified:
2018/08/08 by Jamie Boyd - initial version */
int ptPWM_ArrayModCalback  (void * modData, taskParams * theTask){
	ptPWMStructPtr taskData = (ptPWMStructPtr) theTask->taskData;
	ptPWMArrayModStructPtr modDataPtr = (ptPWMArrayModStructPtr) modData;
	if ((modDataPtr->modBits) & 1){ // start position
		taskData ->startPos = modDataPtr->startPos;
	}
	if ((modDataPtr->modBits) & 2){
		taskData -> endPos = modDataPtr->endPos;
	}
	if ((modDataPtr->modBits) & 4){
		taskData -> arrayPos = modDataPtr-> arrayPos;
		*(taskData->dataRegister) = taskData->arrayData[taskData ->arrayPos];
	}
	if ((modDataPtr->modBits) & 8){
		taskData -> arrayData = modDataPtr->arrayData;
	}
	delete modDataPtr;
	return 0;
}


/* ********************************** PWM_thread Static Class Methods ****************************************************
Call these before making a PWM_thread object


****************************** Memory maps Peripherals for PWM ******************************************
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
#if beVerbose
	printf ("GPIO address = %u PWM address = %u PWM clock address = %u \n", GPIOperi ->addr, PWMperi->addr, PWMClockperi->addr);
#endif
	return 0;
}


/* ************************************sets PWM clock to give new PWM frequency******************************************************
The PWM frequency is 1/(time taken to output a single value) which is determined by range and clock frequency
This PWM frequency is only accurate for the given PWMrange The two PWM channels could use different ranges, and thus have different PWM frequencies
even though they use the same PWM clock. We don't do that here; we use the same range for both channels, saved in a class variable
Returns new clock frequency, else returns -1 if the requested PWM frequency, PWMrange combination caused the clock 
divisor to exceed the maximum settable value of 4095 
Last Modified: 
2018/09/04 by Jamie Boyd - removed return value, freq saved in static field
2018/08/06 by Jamie Boyd- modified for pulsedThread subclassing*/
void PWM_thread::setClock (float PWMFreq){
	
	 // clock must be this fast in Hz to output PWM_thread::PWMrange ticks in 1/PWMFreq seconds
	float clockFreq = PWMFreq * PWM_thread::PWMrange;
	unsigned int clockRate ;
	int clockSrc ;
	if (clockFreq < (PI_CLOCK_RATE/4)){
		clockRate = PI_CLOCK_RATE;
		clockSrc = 1;
		printf ("Using PWM clock source oscillator at 19.2 MHz.\n");
	}else{
		clockRate = PLLD_CLOCK_RATE;
		clockSrc = 6;
		printf ("Using PWM clock source PLL D at 500 MHz.\n");
	}
	int integerDivisor = clockRate/clockFreq; // Divisor Value for clock, clock source freq/Divisor = PWM hz
	if (integerDivisor > 4095){ // max divisor is 4095 - need to select larger range or higher frequency
		printf ("Calculated integer divisor, %d, is greater than 4095, the max divisor, you need to select a larger range or higher frequency\n", integerDivisor);
		return ;
	}
#if beVerbose
	printf ("Calculated integer divisor is %d.\n", integerDivisor);
#endif
	int fractionalDivisor = ((clockRate/clockFreq) - integerDivisor) * 4096;
	unsigned int PWM_CTLstate = *(PWMperi->addr + PWM_CTL); // save state of PWM_CTL register
	*(PWMperi->addr + PWM_CTL) = 0;  // Turn off PWM.
	*(PWMClockperi->addr + PWMCLK_CNTL) =(*(PWMClockperi->addr  + PWMCLK_CNTL) &~0x10)|BCM_PASSWORD; // Turn off PWM clock enable flag.
	while(*(PWMClockperi->addr + PWMCLK_CNTL)&0x80); // Wait for clock busy flag to turn off.
#if beVerbose
	printf ("PWM Clock Busy flag turned off.\n");
#endif
	*(PWMClockperi->addr + PWMCLK_DIV) =(integerDivisor  << 12)|(fractionalDivisor & 4095)|BCM_PASSWORD; // Configure divider. 
	*(PWMClockperi->addr + PWMCLK_CNTL) = 0x400 | clockSrc | BCM_PASSWORD; // start PWM clock with Source=Oscillator @19.2 MHz, 2-stage MASH
	*(PWMClockperi->addr + PWMCLK_CNTL) = 0x410| clockSrc | BCM_PASSWORD; // Source (1=Oscillator 19.2 MHz , 6 = 500MHZ PLL d) 2-stage MASH, plus enable
	while(!(*(PWMClockperi->addr + PWMCLK_CNTL) &0x80)); // Wait for busy flag to turn on.
#if beVerbose
	printf ("PWM Clock Busy flag turned on again.\n");
#endif
	*(PWMperi->addr + PWM_CTL) = PWM_CTLstate; // restore saved PWM_CTL register state
	// calculate and save final PWM frequency
	PWM_thread::PWMfreq = (clockRate/(integerDivisor + (fractionalDivisor/4095)))/PWM_thread::PWMrange;
}

/* ******************** ThreadMaker with Integer pulse duration, delay, and number of pulses timing description inputs ********************
 Last Modified:
2018/08/06 by Jamie Boyd - Initial Version, copied and modified from GPIO thread maker */
PWM_thread * PWM_thread::PWM_threadMaker (int channel, int mode, int enable, int * arrayData, unsigned int nData, unsigned int  durUsecs, unsigned int nPulses, int accuracyLevel) {
	unsigned int chanBit = (1<<channel);
	if (PWM_thread::PWMchans & chanBit){
#if beVerbose
		printf ("PWM channel %d is already in use.\n", channel);
#endif
		return nullptr;
	}
	PWM_thread::PWMchans |= chanBit;
	// make and fill an init struct
	ptPWMinitStructPtr initStruct = new ptPWMinitStruct;
	initStruct->channel = channel;
	initStruct->mode = mode;
	initStruct -> enable = enable;
	initStruct->arrayData = arrayData;
	initStruct->nData = nData;
	initStruct ->range =  PWM_thread::PWMrange;
	// Set address for GPIO peri
	//initStruct->GPIOperiAddr = GPIOperi->addr ;
	// set addess for PWM peri
	//initStruct->PWMperiAddr = PWMperi->addr;
	// call PWM_thread constructor, which calls pulsedThread constructor
	int errCode =0;
	PWM_thread * newPWM_thread = new PWM_thread (durUsecs, nPulses, (void *) initStruct, accuracyLevel, errCode) ;
	if (errCode){
#if beVerbose
		printf ("PWM_threadMaker failed to make PWM_thread.\n");
#endif
		return nullptr;
	}
	// set custom task delete function
	newPWM_thread->setTaskDataDelFunc (&ptPWM_delTask);
	// set object variables 
	newPWM_thread ->PWM_chan = channel;
	newPWM_thread ->offState =0; // 0 for low when not enabled, 1 for high when enabled
	newPWM_thread->polarity = 0; // 0 for normal polarity, 1 for reversed
	newPWM_thread ->enabled=enable; // 0 for not enabled, 1 for enabled
	PWM_thread::PWMchans |= ~chanBit; // set channel bit
	return newPWM_thread;
}

/* ******************** ThreadMaker with floating point frequency and duration timing description inputs ********************
 Last Modified:
2018/08/06 by Jamie Boyd - Initial Version, copied and modified from GPIO thread maker */
PWM_thread * PWM_thread::PWM_threadMaker (int channel, int mode,  int enable, int * arrayData, unsigned int nData, float frequency, float trainDuration, int accuracyLevel) {
	unsigned int chanBit = (1<<channel);
	if (PWM_thread::PWMchans & chanBit){
#if beVerbose
		printf ("PWM channel %d is already in use.\n", channel);
#endif
		return nullptr;
	}
	PWM_thread::PWMchans |= chanBit;
	// make and fill an init struct
	ptPWMinitStructPtr  initStruct = new ptPWMinitStruct;
	initStruct->channel = channel;
	initStruct->mode = mode;
	initStruct -> enable = enable;
	initStruct->arrayData = arrayData;
	initStruct->nData = nData;
	initStruct ->range =  PWM_thread::PWMrange;
	// call PWM_thread constructor, which calls pulsedThread constructor
	int errCode =0;
	PWM_thread * newPWM_thread = new PWM_thread (frequency, trainDuration, (void *) initStruct, accuracyLevel, errCode);
	if (errCode){
#if beVerbose
		printf ("PWM_threadMaker failed to make PWM_thread.\n");
#endif
		PWM_thread::PWMchans &= (~chanBit); // unset channel bit
		return nullptr;
	}
	// set custom task delete function
	newPWM_thread->setTaskDataDelFunc (&ptPWM_delTask);
	// set object variables 
	newPWM_thread ->PWM_chan = channel;
	newPWM_thread ->offState =0; // 0 for low when not enabled, 1 for high when enabled
	newPWM_thread->polarity = 0; // 0 for normal polarity, 1 for reversed
	newPWM_thread ->enabled=enable; // 0 for not enabled, 1 for enabled
	PWM_thread::PWMchans |= ~chanBit; // set static channel bit
	return newPWM_thread;
}

 /* ****************************** Destructor handles peripheral mapping and unsets alternate *************************
Thread data is destroyed by the pulsedThread destructor. All we need to do here is unset the alternate function for the GPIO pin
and decrement the tally for the peripheral mappings
Last Modified:
2018/08/07 by Jamie Boyd - Initial Version */
PWM_thread::~PWM_thread (){
	if (PWM_chan == 0){
		INP_GPIO(GPIOperi->addr,18);
		OUT_GPIO(GPIOperi->addr,18);
		GPIO_CLR(GPIOperi->addr, (1 << 18)) ;
	}else{
		if (PWM_chan == 1){
			INP_GPIO(GPIOperi->addr,19);
			OUT_GPIO(GPIOperi->addr,19);
			GPIO_CLR(GPIOperi->addr, (1 << 19));
		}
	}
	unsigned int chanBit = (1<<PWM_chan);
	PWM_thread::PWMchans &= (~chanBit); // unset channel bit
	unUseGPIOperi();
	unUsePWMperi();
	unUsePWMClockperi();
}

/* ****************************** utility functions - setters and getters *************************

******************************* sets Enable State ************************************
Last Modified:
2018/08/08 by Jamie Boyd - Initial Version  */
int PWM_thread::setEnable (int enableStateP, int isLocking){
	enabled = enableStateP;
	int * newEnableVal = new int;
	* newEnableVal =  enabled;
	int returnVal = modCustom (&ptPWM_setEnableCallback, (void *) newEnableVal, isLocking);
	return returnVal;
}


/* ****************************** sets Polarity ************************************
Last Modified:
2018/08/08 by Jamie Boyd - Initial Version  */
int PWM_thread::setPolarity (int polarityP, int isLocking){
	
	polarity = polarityP;
	int * newPolarityVal = new int;
	* newPolarityVal =  polarity;
	int returnVal = modCustom (&ptPWM_reversePolarityCallback, (void *) newPolarityVal, isLocking);
	return returnVal;
}

/* ****************************** sets OFF state ************************************
Last Modified:
2018/08/08 by Jamie Boyd - Initial Version  */
int PWM_thread::setOffState (int offStateP, int isLocking){
	
	offState = offStateP;
	int * newOffstateVal = new int;
	* newOffstateVal =  offState;
	int returnVal = modCustom (&ptPWM_setOffStateCallback, (void *) newOffstateVal, isLocking);
	return returnVal;
}

