#include "PWM_thread.h"

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
	*(PWMClockperi->addr  + PWMCLK_CNTL) =(*(PWMClockperi->addr  + PWMCLK_CNTL) &~0x10)|BCM_PASSWORD; // Turn off clock enable flag.
	unmap_peripheral(PWMClockperi);
	delete PWMClockperi;
	PWMClockperi = nullptr;
}

/* Maps peripherals for PWM. Does not set PWM clock. Does not set up any channels */
int ptPWM_mapPeripherals (bcm_peripheralPtr &GPIOperi, bcm_peripheralPtr &PWMperi, bcm_peripheralPtr &PWMClockperi){
	int errCode;
	if (GPIOperi == nullptr){
		GPIOperi = new bcm_peripheral {GPIO_BASE};
		errCode = map_peripheral(GPIOperi, IFACE_DEV_GPIOMEM);
		if (errCode){
			return 1;
		}
	}
	if (PWMperi == nullptr){
		PWMperi = new bcm_peripheral {PWM_BASE};
		errCode = map_peripheral(PWMperi, IFACE_DEV_MEM);
		if (errCode){
			return 2;
		}
	}
	if (PWMClockperi == nullptr){
		PWMClockperi = new bcm_peripheral {CLOCK_BASE};
		errCode = map_peripheral(PWMClockperi, IFACE_DEV_MEM);
		if (errCode){
			return 3;
		}
	}
	return 0;
}

/*sets PWM clock to give newPWMFreq PWM frequency. This PWM frequency is only accurate for the given PWMrange
The two PWM channels could use different ranges, and thus have different PWM frequencies
even though they use the same PWM clock.
Returns new clock frequency if the requested PWM frequency, PWMrange combination caused the clock 
divisor to exceed the maximum settable value of 4095 */
float ptPWM_SetClock (float newPWMFreq, int PWMrange, bcm_peripheralPtr PWMperi, bcm_peripheralPtr PWMClockperi){
	
	float clockFreq = newPWMFreq * PWMrange;
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
		return 1;
	}
	int fractionalDivisor = ((clockRate/clockFreq) - integerDivisor) * 4096;
	unsigned int PWM_CTLstate = *(PWMperi->addr + PWM_CTL); // save state of PWM_CTL register
	*(PWMperi->addr + PWM_CTL) = 0 ;  // Turn off PWM.
	*(PWMClockperi->addr  + PWMCLK_CNTL) =(*(PWMClockperi->addr  + PWMCLK_CNTL) &~0x10)|BCM_PASSWORD; // Turn off PWM clock enable flag.
	while(*(PWMClockperi->addr  + PWMCLK_CNTL)&0x80); // Wait for clock busy flag to turn off.
	//printf ("PWM Clock Busy flag turned off.\n");
	*(PWMClockperi->addr  + PWMCLK_DIV) =(integerDivisor  << 12)|(fractionalDivisor&4095)|BCM_PASSWORD; // Configure divider. 
	*(PWMClockperi->addr  + PWMCLK_CNTL) = 0x400 | clockSrc | BCM_PASSWORD; // start PWM clock with Source=Oscillator @19.2 MHz, 2-stage MASH
	*(PWMClockperi->addr  + PWMCLK_CNTL) = 0x410| clockSrc | BCM_PASSWORD; // Source (=Oscillator 19.2 MHz , 6 = 500MHZ PLL d) 2-stage MASH, plus enable
	while(!(*(PWMClockperi->addr  + PWMCLK_CNTL) &0x80)); // Wait for busy flag to turn on.
	//printf ("PWM Clock Busy flag turned on again.\n");
	*(PWMperi->addr + PWM_CTL) = PWM_CTLstate; // restore saved PWM_CTL register state
	return clockRate/(integerDivisor + (fractionalDivisor/4095));
}

/* ***********************************************************************************
Initialization callback function. Initializes a single channel, either 0 or 1.
last modified:
2017/02/20 by Jamie Boyd - initial version 
*/
int ptPWM_Init (void * initDataP, void *  volatile &taskDataP){
	// initData is a pointer to our init structure
	PWMInitStructPtr initData = (PWMInitStructPtr)initDataP;
	// task data is a pointer to taskCustomData that needs to be initialized from our custom init structure
	PWMStructPtr taskData = new PWMStruct;
	taskDataP = taskData;
	//PWMStructPtr taskData = new PWMStruct;
	//taskDataP = malloc (sizeof(PWMStruct));
	//PWMStructPtr taskData = (PWMStructPtr)taskDataP;
	// copy fields
	taskData->channel = initData->channel;
	taskData->mode = initData->mode;
	taskData->range = initData->range;
	taskData->GPIOaddr = initData->GPIOaddr;
	taskData->PWMaddr = initData->PWMaddr;
	taskData->dataPos = initData->dataPos;
	taskData->nData = initData->nData;
	taskData->pwmData= initData->pwmData;
	// set up PWM channel 0 or 1
	if (taskData->channel == 0){
		INP_GPIO(taskData->GPIOaddr,18);           // Set GPIO 18 to input to clear bits
		SET_GPIO_ALT(taskData->GPIOaddr,18,5);     // Set GPIO 18 to Alt5 function PWM0
		taskData->rangeRegisterOffset = PWM_RNG0;
		taskData->dataRegisterOffset = PWM_DAT0;
		taskData->modeBit = 0x80;
		taskData->enableBit = 0x1;

	}else{
		INP_GPIO(taskData->GPIOaddr,19);           	// Set GPIO 19 to input to clear bits
		SET_GPIO_ALT(taskData->GPIOaddr,19,5);     // Set GPIO 19 to Alt5 function PWM1
		taskData->rangeRegisterOffset = PWM_RNG1;
		taskData->dataRegisterOffset = PWM_DAT1;
		taskData->modeBit = 0x8000;
		taskData->enableBit = 0x100;
	}
	*(taskData->PWMaddr  + taskData->rangeRegisterOffset) = taskData->range; // set range
	if (taskData->mode ==PWM_MARK_SPACE){
		*(taskData->PWMaddr  + PWM_CTL) |= (taskData->modeBit) ; // put PWM in MS Mode
	}else{
		*(taskData->PWMaddr  + PWM_CTL) &= ~(taskData->modeBit);  // clear MS mode bit for balanced mode
	}
	// if enable, set enable bit to start PWM running right away
	if (!(initData->waitForEnable)){
		// set initial PWM value first so we have something to put out
		*(taskData->PWMaddr  + taskData->dataRegisterOffset) = taskData->pwmData[taskData->dataPos];
		*(taskData->PWMaddr  + PWM_CTL) |= taskData->enableBit;
	}
	return 0; // 
}

/*Sets the next value in the array to be output*/
void ptPWM_Hi (void * volatile taskDataP){
	PWMStructPtr taskData = (PWMStructPtr)taskDataP;
	*(taskData->PWMaddr  + taskData->dataRegisterOffset) = taskData->pwmData [(taskData->dataPos % taskData->nData)];
	taskData->dataPos +=1;
}
		
/*There is need for Lo function different from hi function for PWM */	
void ptPWM_Lo (void * volatile taskDataP){}
	

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
