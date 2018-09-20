#include "GPIOlowlevel.h"

int main(int argc, char **argv){
	
	if (useGpioPeri() == nullptr){
		printf ("failed to map GPIO peripheral.\n");
		return 1;
	}
	//PWMperi
	if (usePWMPeri ()== nullptr){
		printf ("failed to map PWM peripheral.\n");
		return 2;
	}
	// PWM clock
	if (usePWMClockPeri ()== nullptr){
		printf ("failed to map PWM Clock peripheral.\n");
		return 3;
	}
	printf ("mapped all peripherals for PWM.\n");
	INP_GPIO(GPIOperi ->addr, 18);  // Set GPIO 18 to input to clear bits
	SET_GPIO_ALT(GPIOperi ->addr, 18, 5); // Set GPIO 18 to alt 5 function 
	// Turn off PWM.
	*(PWMperi->addr + PWM_CTL) = 0; 
	// Turn off enable flag.
	unsigned int clockSrc = CM_SRCPLL;
	unsigned int clockSrcRate = 500E06;
	unsigned int reqClockFreq =400000;
	unsigned int integerDivisor = clockSrcRate/reqClockFreq; // Divisor Value for clock, clock source freq/Divisor = PWM hz
	unsigned int fractionalDivisor = ((float)(clockSrcRate/reqClockFreq) - integerDivisor) * 4096;
	float actualClockRate = (clockSrcRate/(integerDivisor + (fractionalDivisor/4095)));
	
	unsigned int mash = CM_MASH2; // start with 2 stage MASH, minimum divisir is 3

	printf ("Calculated integer divisor is %d and fractional divisor is %d for a clock rate of %.3f.\n", integerDivisor, fractionalDivisor, actualClockRate);

	
	*(PWMClockperi->addr + CM_PWMCTL) = ((*PWMClockperi->addr + CM_PWMCTL) & CM_DISAB) | CM_PASSWD; ;
	while(*(PWMClockperi->addr + CM_PWMCTL) & CM_BUSY); // Wait for busy flag to turn off.
	printf ("busy flag turned off.\n");
	// Configure divider register
	*(PWMClockperi->addr + CM_PWMDIV) =(integerDivisor<<CM_DIVI)|(fractionalDivisor & 4095)|CM_PASSWD;
	// set Source and MASH in control register
	*(PWMClockperi->addr + CM_PWMCTL)= mash | clockSrc | CM_PASSWD;
	// set enable flag to enable clock
	*(PWMClockperi->addr + CM_PWMCTL) |= CM_ENAB | CM_PASSWD;
	while(!(*(PWMClockperi->addr + CM_PWMCTL) & CM_BUSY)); // Wait for busy flag to turn on.
	printf ("busy flag turn on again.\n");
	*(PWMperi->addr + PWM_RNG1)=100;
	*(PWMperi->addr + PWM_DAT1)=10;
	*(PWMperi->addr + PWM_CTL)=0x0081; // Channel 1 M/S mode, no FIFO, PWM mode, enabled.
	printf ("PWM enabled\n");
	usleep (1000000);
	*(PWMperi->addr + PWM_DAT1)=20;
	usleep (1000000);
	*(PWMperi->addr + PWM_DAT1)=30;
	usleep (1000000);
	*(PWMperi->addr + PWM_DAT1)=40;
	usleep (1000000);
	*(PWMperi->addr + PWM_DAT1)=50;
	usleep (1000000);
	*(PWMperi->addr + PWM_DAT1)=60;
	usleep (1000000);
	*(PWMperi->addr + PWM_DAT1)=70;
	usleep (1000000);
	*(PWMperi->addr + PWM_DAT1)=80;
	usleep (1000000);
	*(PWMperi->addr + PWM_DAT1)=90;
	usleep (1000000);
	*(PWMperi->addr + PWM_DAT1)=80;
	usleep (1000000);
	*(PWMperi->addr + PWM_DAT1)=70;
	usleep (1000000);
	*(PWMperi->addr + PWM_DAT1)=60;
	usleep (1000000);
	*(PWMperi->addr + PWM_DAT1)=50;
	usleep (1000000);
	*(PWMperi->addr + PWM_DAT1)=40;
	usleep (1000000);
	*(PWMperi->addr + PWM_DAT1)=30;
	usleep (1000000);
	*(PWMperi->addr + PWM_DAT1)=20;
	usleep (1000000);
	*(PWMperi->addr + PWM_DAT1)=10;
	usleep (1000000);
	// Turn off PWM.
	*(PWMperi->addr + PWM_CTL) = 0; 
	// Turn off enable flag.
	*(PWMClockperi->addr + CM_PWMCTL) = ((*PWMClockperi->addr + CM_PWMCTL) & ~0x10)|0x5a000000;; 
	return 0;
}

/*
g++ -O3 -std=gnu++11 -Wall GPIOlowlevel.cpp PWM_no_sleep.cpp -o PWMnoSleep

	// PWM clock and pWM control
	#define PWMCTL (*(volatile uint32_t *)0x2020c000)
	#define PWMSTA (*(volatile uint32_t *)0x2020c004)
	#define PWMDMAC (*(volatile uint32_t *)0x2020c008)
	#define PWMRNG1 (*(volatile uint32_t *)0x2020c010)
	#define PWMDAT1 (*(volatile uint32_t *)0x2020c014)
	#define PWMFIF1 (*(volatile uint32_t *)0x2020c018)
	#define PWMRNG2 (*(volatile uint32_t *)0x2020c020)
	#define PWMDAT2 (*(volatile uint32_t *)0x2020c024)

	#define CM_PWMCTL (*(volatile uint32_t *)0x201010a0)
	#define CM_PWMDIV (*(volatile uint32_t *)0x201010a4)

	PWMCTL=0; // Turn off PWM.

	CM_PWMCTL=(CM_PWMCTL&~0x10)|0x5a000000; // Turn off enable flag.
	while(CM_PWMCTL&0x80); // Wait for busy flag to turn off.
	CM_PWMDIV=0x5a000000|(5<<12); // Configure divider.
	CM_PWMCTL=0x5a000206; // Source=PLLD (500 MHz), 1-stage MASH.
	CM_PWMCTL=0x5a000216; // Enable clock.
	while(!(CM_PWMCTL&0x80)); // Wait for busy flag to turn on.
	
	PWMRNG1=100;
	PWMDAT1=10;

	PWMCTL=0x0081; // Channel 1 M/S mode, no FIFO, PWM mode, enabled.
	
	
	This configures the PWM for a 100 MHz clock, sets up a range of 100 to give an output frequency of 1 MHz and a duty cycle of 1/10. Unlike most of the other code I've seen, it does not use the kill flag on the clock, which is documented to be for debug purposes only and should not be used, and it does not use any usleeps(), but instead checks the busy flag in the proper way. Some code examples I have seen claim that this does not work, but it seems to work fine for me. The code is quite careful to not change things in the wrong order, according to the hints in the data sheet.

	This seems to me to be the correct way to configure the clock, but if anyone has any problems with it, do tell.
	
	
	
	
	
	
	
	#define CM_GP0CTL (*(volatile uint32_t *)0x20101070)
	#define CM_GP0DIV (*(volatile uint32_t *)0x20101074)

	CM_GP0CTL=CM_GP0CTL&~0x10; // Turn off enable flag.
	while(CM_GP0CTL&0x80); // Wait for busy flag to turn off.
	CM_GP0DIV=0x5a000000|(10<<12); // Configure divider.
	CM_GP0CTL=0x5a000206; // Source=PLLD (500 MHz), 1-stage MASH.
	CM_GP0CTL=0x5a000216; // Enable clock.
	while(!(CM_GP0CTL&0x80)); // Wait for busy flag to turn on.

	SetGPIOAlternateMode(4,0);
*/