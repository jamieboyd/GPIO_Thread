#include "PWM_thread.h"


/* ***********************************************************************************
Initialization callback function. Initializes a single channel, either 0 or 1.
last modified:
2018/08/07 by Jamie Boyd - modified for pulsedThread subclassing
2017/02/20 by Jamie Boyd - initial version  */
int ptPWM_Init (void * initDataP, void *  volatile &taskDataP){
	// initData is a pointer to our init structure
	ptPWMInitStructPtr initData = (ptPWMInitStructPtr)initDataP;
	// task data is a pointer to taskCustomData that needs to be initialized from our custom init structure
	ptPWMStructPtr taskData = new ptPWMStruct;
	taskDataP = taskData;
	// copy PWM settings, or set defaults
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
	// set up PWM by writing to control registers
	// register addresses that vary by chanel
	unsigned int dataRegisterOffset;
	unsigned int rangeRegisterOffset;
	unsigned int modeBit;
	unsigned int enableBit;
	unsigned int polarityBit;
	unsigned int offStateBit;
	// set up PWM channel 0 or 1
	if (initData->channel == 0){
		INP_GPIO(initData->GPIOaddr,18);           // Set GPIO 18 to input to clear bits
		SET_GPIO_ALT(initData->GPIOaddr,18,5);     // Set GPIO 18 to Alt5 function PWM0
		rangeRegisterOffset = PWM0_RNG;
		dataRegisterOffset = PWM0_DAT;
		modeBit = PWM0_MS_MODE;
		enableBit = PWM0_ENABLE;
		polarityBit = PWM0_REVPOLAR;
		offStateBit = PWM0_OFFSTATE;

	}else{
		INP_GPIO(initData->GPIOaddr,19);           	// Set GPIO 19 to input to clear bits
		SET_GPIO_ALT(initData->GPIOaddr,19,5);     // Set GPIO 19 to Alt5 function PWM1
		rangeRegisterOffset = PWM1_RNG;
		dataRegisterOffset = PWM1_DAT;
		modeBit = PWM1_MS_MODE;
		enableBit = PWM1_ENABLE;
		polarityBit = PWM1_REVPOLAR;
		offStateBit = PWM1_OFFSTATE;
	}
	// save control and data register address for easy access from thread hiFunc and customMod functions
	taskData -> ctlRegister = initData->PWMaddr  + PWM_CTL;
	taskData->dataRegister = initData->PWMaddr  + dataRegisterOffset ;
	// set range
	*(initData->PWMaddr  + rangeRegisterOffset) = initData->range; 
	// set mode
	if (initData->mode ==PWM_MARK_SPACE){
		*(taskData -> ctlRegister ) |= modeBit ; // put PWM in MS Mode
	}else{
		*(taskData -> ctlRegister ) &= ~modeBit;  // clear MS mode bit for balanced mode
	}
	// set polarity to non-reversed
	*(taskData -> ctlRegister ) &= ~polarityBit;  // clear reverse polarity bit
	// set off state to low by unsetting offstate bit
	*(taskData -> ctlRegister ) &= ~offStateBit;
	// if wait-for-enable, clear enable bit, else set the bit to start PWM running right away
	if (initData->waitForEnable){
		*(taskData -> ctlRegister) &= ~enableBit;
	}else{
		// set initial PWM value first so we have something to put out
		*(taskData->dataRegister) = taskData->arrayData[taskData->0];
		*(taskData -> ctlRegister) |= enableBit;
	}
	return 0; // 
}

/* *************************************** Sets the next value in the array to be output********************************************
Last Modified:
2018/08/07 by Jamie Boyd -updated for pusledThread subclass*/
void ptPWM_Hi (void * volatile taskDataP){
	ptPWMStructPtr taskData = (ptPWMStructPtr)taskDataP;
	*(taskData->dataRegister) = taskData->arrayData [taskData->arrayPos];
	if (taskData->arrayPos == taskData->endPos ){
		taskData->arrayPos = taskData->startPos;
	}else{
		taskData->arrayPos +=1;
	}
}

/* ****************************** Custom delete Function *****************************************************/
void ptPWM_delTask (void * taskData){
	ptPWMStructPtr pwmTaskPtr = (ptPWMStructPtr) taskData;
	delete (ptPWMStructPtr);
}

/*
	// set polarity
	if (initData->polarity){
		*(initData->PWMaddr  + PWM_CTL) |= polarityBit ; // set reverse polarity
	}else{
		*(initData->PWMaddr  + PWM_CTL) &= ~polarityBit;  // clear reverse polarity bit
	}
*/




/* ****************************** Destructor handles peripheral mapping and unsets alternate *************************
Thread data is destroyed by the pulsedThread destructor. All we need to do here is unset the alternate function for the GPIO pin
and decrement the tally for the peripheral mappings
and 
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
	unUseGPIOperi();
	unUsePWMperi();
	unUsePWMclockPeri();
}

/* frees a single PWM channel, so it can be used for standard GPIO agian */
void ptPWM_delChan (char bcm_PWM_chan){
	if (bcm_PWM_chan == 0){
		INP_GPIO(GPIOperi->addr,18);
		OUT_GPIO(GPIOperi->addr,18);
		GPIO_CLR(GPIOperi->addr, (1 << 18)) ;
	}else{
		if (bcm_PWM_chan == 1){
			INP_GPIO(GPIOperi->addr,19);
			OUT_GPIO(GPIOperi->addr,19);
			GPIO_CLR(GPIOperi->addr, (1 << 19));
		}
	}
}

/*Frees up all PWM resources.  Shuts down PWM and PWM clock */
void ptPWM_cleanUp(bcm_peripheralPtr &GPIOperi, bcm_peripheralPtr &PWMperi, bcm_peripheralPtr &PWMClockperi){
	// GPIO
	unmap_peripheral(GPIOperi);
	delete GPIOperi;
	GPIOperi = nullptr;
	// PWM
	*(PWMperi->addr + PWM_CTL) = 0 ;  // Turn off PWM.
	unmap_peripheral(PWMperi);
	delete PWMperi;
	PWMperi = nullptr;
	// pwm clock
	unmap_peripheral(PWMClockperi);
	delete PWMClockperi;
	PWMClockperi = nullptr;
}





	

/* *********************************************
Custom data mod callback that sets enable state. modData is a pointer to an int for the enable state
Use with pulsedThread::modCustom
last modified:
2017/04/03 by Jamie Boyd - initial version
*/
int ptPWM_enableCallback (void * modData, taskParams * theTask){
	int * enableState = (int *)modData;
	PWMStructPtr taskData = (PWMStructPtr)theTask->taskCustomData;
	if (* enableState == 0){
		*(taskData->PWMaddr  + PWM_CTL) &= ~taskData->enableBit;
	}else{
		*(taskData->PWMaddr  + PWM_CTL) |= taskData->enableBit;
	}
	return 0;
} 
 

/* *********************************************
Custom data mod callback that sets arrayLength, or at least how much of the array we are using. modData is a pointer to an int for aray length
Use with pulsedThread::modCustom
last modified:
2017/04/04 by Jamie Boyd - initial version
*/
int ptPWM_setNumDataCallback (void * modData, taskParams * theTask){
	int * arrayLength = (int *)modData;
	PWMStructPtr taskData = (PWMStructPtr)theTask->taskCustomData;
	taskData ->nData = *arrayLength;
	return 0;
}


/* *********************************************
Custom data mod callback that sets array Position, the value that will be next output
Use with pulsedThread::modCustom
last modified:
2017/04/04 by Jamie Boyd - initial version
*/
int ptPWM_setArrayPosCallback (void * modData, taskParams * theTask){
	int * newArrayPos= (int *)modData;
	PWMStructPtr taskData = (PWMStructPtr)theTask->taskCustomData;
	taskData ->dataPos = *newArrayPos;
	return 0;
}

/* *********************************************
Custom data mod callback that sets a new array of values to output
Use with pulsedThread::modCustom
last modified:
2017/04/03 by Jamie Boyd - initial version
*/
int ptPWM_setArrayCallback (void * modData, taskParams * theTask){
	PWMInitStructPtr initStructPtr = (PWMInitStructPtr) modData;
	PWMStructPtr taskData = (PWMStructPtr)theTask->taskCustomData;
	taskData ->nData = initStructPtr->nData;
	taskData->pwmData = initStructPtr->pwmData;

	return 0;
}


 /* ******************************** PWM_thread Class Methods *******************************************************
 static function maps peripherals for PWM. Does not set PWM clock. Does not set up any channels 
Last Modified: 2018/08/06 by Jamie Boyd - first version */
 int PWM_thread::mapPeripherals(){
	 // GPIOperi
	if (useGpioPeri() == nullptr){
#if beVerbose
		printf ("mapPeripherals failed to map GPIO peripheral.\n");
#endif
		return 1;
	}
	//PWMperi
	if (usePWMPeri ()== nullptr){
#if beVerbose
		printf ("mapPeripherals failed to map PWM peripheral.\n");
#endif
		return 2;
	}
	// PWM clock
	if (usePWMClockPeri ()== nullptr){
#if beVerbose
		printf ("mapPeripherals failed to map PWM Clock peripheral.\n");
#endif
		return 3;
	}
	return 0;
}


/* ************************************sets PWM clock to give newPWMFreq PWM frequency******************************************************
This PWM frequency is only accurate for the given PWMrange The two PWM channels could use different ranges, and thus have different PWM frequencies
even though they use the same PWM clock. Don't do that; use the same range for both channels, saved in a class variable
Returns new clock frequency, else returns -1 if the requested PWM frequency, PWMrange combination caused the clock 
divisor to exceed the maximum settable value of 4095 
Last Modified: 2018/08/06 by Jamie Boyd- modified for pulsedThread subclassing*/
float PWM_thread::setClock (float PWMFreq, int PWMrange){
	
	float clockFreq = PWMFreq * PWMrange;
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
		return -1;
	}
	int fractionalDivisor = ((clockRate/clockFreq) - integerDivisor) * 4096;
	unsigned int PWM_CTLstate = *(PWMperi->addr + PWM_CTL); // save state of PWM_CTL register
	*(PWMperi->addr + PWM_CTL) = 0 ;  // Turn off PWM.
	*(PWMClockperi->addr  + PWMCLK_CNTL) =(*(PWMClockperi->addr  + PWMCLK_CNTL) &~0x10)|BCM_PASSWORD; // Turn off PWM clock enable flag.
	while(*(PWMClockperi->addr  + PWMCLK_CNTL)&0x80); // Wait for clock busy flag to turn off.
	//printf ("PWM Clock Busy flag turned off.\n");
	*(PWMClockperi->addr  + PWMCLK_DIV) =(integerDivisor  << 12)|(fractionalDivisor&4095)|BCM_PASSWORD; // Configure divider. 
	*(PWMClockperi->addr  + PWMCLK_CNTL) = 0x400 | clockSrc | BCM_PASSWORD; // start PWM clock with Source=Oscillator @19.2 MHz, 2-stage MASH
	*(PWMClockperi->addr  + PWMCLK_CNTL) = 0x410| clockSrc | BCM_PASSWORD; // Source (1=Oscillator 19.2 MHz , 6 = 500MHZ PLL d) 2-stage MASH, plus enable
	while(!(*(PWMClockperi->addr  + PWMCLK_CNTL) &0x80)); // Wait for busy flag to turn on.
	//printf ("PWM Clock Busy flag turned on again.\n");
	*(PWMperi->addr + PWM_CTL) = PWM_CTLstate; // restore saved PWM_CTL register state
	// calculate and save final frequency, save range as well
	PWM_thread::PWMfreq = clockRate/(integerDivisor + (fractionalDivisor/4095));
	PWM_thread::PWMrange =PWMrange;
	return PWM_thread::PWMfreq;
}

/* ******************** ThreadMaker with Integer pulse duration, delay, and number of pulses timing description inputs ********************
 Last Modified:
2018/08/06 by Jamie Boyd - Initial Version, copied and modified from GPIO thread maker */
PWM_thread * PWM_thread::PWM_threadMaker (int channel, int mode, int polarity,  int enable, int * arrayData, unsigned int nData, unsigned int  durUsecs, unsigned int nPulses, int accuracyLevel) {
	// make and fill an init struct
	ptPWMinitStructPtr  initStruct = new ptPWMinitStruct;
	initStruct->channel = channel;
	initStruct->polarity = polarity;
	initStruct->mode = mode;
	// Set address for GPIO peri
	initStruct->GPIOperiAddr = GPIOperi->addr ;
	// set addess for PWM peri
	initStruct->PWMperiAddr = PWMperi->addr;
	// set address for  clock for PWM 
	initStruct->PWMClockPeriAddr=PWMClockperi->addr;
	// call PWM_thread constructor, which calls pulsedThread constructor
	int errCode =0;
	PWM_thread * newPWM_thread = new PWM_thread (durUsecs, nPulses, (void *) initStruct, accuracyLevel, errCode) 
	if (errCode){
#if beVerbose
		printf ("PWM_threadMaker failed to make PWM_thread.\n");
#endif
		return nullptr;
	}
	// set custom task delete function
	newPWM_thread->setTaskDataDelFunc (&PWM_delTask);
	return newPWM_thread;
}
