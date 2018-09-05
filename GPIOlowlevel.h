#ifndef GPIOLOWLEVEL_H
#define GPIOLOWLEVEL_H

/* ********************** Low Level access to Raspberry Pi GPIO Peripherals ************************ 
last modified:
2018/02/01 by Jamie Boyd - renaming some files for gitHub
2017/10/11 by Jamie Boyd - reorganizing by peripheral, and working on I2C
2017/05/02 by Jamie Boyd - added I2C
2017/02/17 by Jamie Boyd - initial version for GPIO, plus PWM */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

/***********************************************Base Settings *************************************
The CPU of Raspberry Pi 1 is Broadcom BCM2835, and the peripheral addresses start
from 0x20000000. The CPU of Raspberry Pi 2 is Broadcom BCM2836, and the peripheral 
addresses starts from 0x3F000000. The Raspberry Pi 3 is Broadcom BCM2837 but has
the same peripheral address start as the Raspberry Pi 2. We first define which
Raspberry Pi board to use. Define RPI2 if using a Raspberry Pi 3
Take care to have defined only one at time. So comment out exactly one of the following 2 lines */
//#define RPI
#define RPI2

/*We define a constant BCM_PERI_BASE that contains the base of all peripherals'
physical address. Different peripherals are defined by an offset to the base physical address. */
#ifdef RPI
#define BCM_PERI_BASE	0x20000000
#endif
#ifdef RPI2
#define BCM_PERI_BASE	0x3F000000
#endif

/* ********************** Structure for access to low level memory for BCM Peripherals*************************/
typedef struct bcm_peripheral {
    unsigned long addr_p;
    int mem_fd;
    void *map;
    volatile unsigned int *addr;
} bcm_peripheral, *bcm_peripheralPtr;


/* ****************** define the page and block size in the memory *********************************/
#define PAGE_SIZE 	4096
#define BLOCK_SIZE 	4096


/* ******************************* Define memory interface Method *************************************
Low level memory for GPIO can be accessed through the new /dev/gpiomem interface
which does not require root access. Unfortunately, the PWM and clock hardware are
NOT accessible through /dev/gpiomem, and must be accessed through /dev/mem,
which requires running your programs with sudo or gksudo to get root access.
Trying to access PWM registers through /dev/gpiomem WILL CRASH YOUR PI */
#define IFACE_DEV_GPIOMEM	1	// use this only for GPIO
#define IFACE_DEV_MEM		0	// use this for GPIO or PWM and/or clock access

/* ************************ Map BCM Peripherals **********************************************
This function takes a pointer to a bcm_peripheral struct, p, whose addr_p field should be set
to the base addresss of the peripheral you wish to control. It maps the low level memory
of the peripheral and fills out the rest of the fields in the bcm_peripheral struct.
The memInterface paramater determines which method to use. */
int map_peripheral(bcm_peripheralPtr p, int memInterface);
void unmap_peripheral(bcm_peripheralPtr p);
/**************************************************GPIO Peripheral************************************************
GPIO_BASE is the base address of GPIO peripherals, and its offset from the physical address is 0x200000. */
#define GPIO_BASE       	(BCM_PERI_BASE + 0x200000)	// GPIO controller

/*macros that can be used for setting and unsetting bits in registers relative to gpio memory mapping */
#define INP_GPIO(gpio,g)   *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3)) 	// sets a GPIO pin as input, clearing bits, so useful as a "cleanser"
#define OUT_GPIO(gpio,g)  *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))		// sets a GPIO pin as output
#define SET_GPIO_ALT(gpio,g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))
#define GPIO_SET(gpio, g)  *(gpio + 7)= g  // sets bits which are 1 ignores bits which are 0
#define GPIO_CLR(gpio, g)  *(gpio + 10) = g // clears bits which are 1 ignores bits which are 0
#define GPIO_READ(gpio, g)  *(gpio + 13) &= g //returns 0 if g is not set, else returns g

extern bcm_peripheralPtr GPIOperi ;
extern int GPIOperi_users;
volatile unsigned int * useGpioPeri (void);
void unUseGPIOperi (void);


/* **************************************************PWM Periperal**********************************************************
PWM_BASE is defined by 0x20C000 offset from the base peripheral addresss */
#define PWM_BASE			(BCM_PERI_BASE + 0x20C000)
/* PWM control registers addresses are defined by an offset to PWM_BASE. */
#define PWM_CTL		0             // PWM Control
#define PWM_STATUS  	1		// PWM Status
#define PWM0_RNG		4            // PWM Channel 0 Range
#define PWM0_DAT		5            // PWM Channel 0 Data
#define PWM1_RNG		8            // PWM Channel 1 Range
#define PWM1_DAT		9            // PWM Channel 1 Data
#define PWMCLK_CNTL 	40        // PWM Clock Control
#define PWMCLK_DIV	41         // PWM Clock Divisor
/* *****************defined bits for PWM  control registers ****************************
*******************************channel 0******************************************/
#define	PWM0_MS_MODE    0x0080  // Run in MS mode
#define	PWM0_USEFIFO    0x0020  // Data from FIFO
#define	PWM0_REVPOLAR   0x0010  // Reverse polarity
#define	PWM0_OFFSTATE   0x0008  // Ouput Off state
#define	PWM0_REPEATFF   0x0004  // Repeat last value if FIFO empty
#define	PWM0_SERIAL     0x0002  // Run in serial mode
#define	PWM0_ENABLE     0x0001  // Channel Enable
/* ******************channel 1 ******************************************************/
#define	PWM1_MS_MODE    0x8000  // Run in MS mode
#define	PWM1_USEFIFO    0x2000  // Data from FIFO
#define	PWM1_REVPOLAR   0x1000  // Reverse polarity
#define	PWM1_OFFSTATE   0x0800  // Ouput Off state
#define	PWM1_REPEATFF   0x0400  // Repeat last value if FIFO empty
#define	PWM1_SERIAL       0x0200  // Run in serial mo
#define	PWM1_ENABLE     0x0100  // Channel Enable


#define PWM_MARK_SPACE 0
#define PWM_BALANCED 1

extern bcm_peripheralPtr PWMperi ;
extern int PWMperi_users;
volatile unsigned int * usePWMPeri (void);
void unUsePWMperi (void);

/* ************************************************* PWM Clock Control*********************************************************
Values for setting some registers need to be ORed with this magic number, the clock manager password */
#define	BCM_PASSWORD 0x5A000000
/* PWM_CLOCK_BASE is defined by 0x101000 offset from the base peripheral addresss */
#define PWM_CLOCK_BASE (BCM_PERI_BASE + 0x101000)
/*Frequency of oscillators that we use as source for things like PWM clock*/
#define PI_CLOCK_RATE 19.2e6	//19.2 Mhz
#define PLLD_CLOCK_RATE 500e6	// 500 MHz phase locked loop D 

extern bcm_peripheralPtr PWMClockperi;
extern int PWMClockperi_users;
extern float bcm_PWM_Clockfreq ;
volatile unsigned int * usePWMClockPeri (void);
void unUsePWMClockperi (void);


/*********************************************** GPU CLock ******************************************************/
#define GPU_CLOCK1		(0x7E801000 - 0x7e000000 + BCM_PERI_BASE)
#define GPU_CLOCK2		0x3F801000
#define GPU_CLOCK3		0x3F003000

/************************************************* BSC peripheral, for I2C******************************************************************
I2C  Control, done with one of the Broadcom Serial Controllers, BSC0 for Pi 1, BSC1 for Pi 2 */
#ifdef RPI
#define BSC_BASE	(BCM_PERI_BASE + 0x205000)
#endif
#ifdef RPI2
#define BSC_BASE	(BCM_PERI_BASE + 0x804000)
#endif

/* *******************************BSC Register Addresses **************************************/
#define BSC_C		0x00		// Control Register
#define BSC_S		0x01		// Status Register
#define BSC_DLEN 	0x02		// Data Length Register
#define BSC_A  	0x03		// Slave Address Register (the Pi must be the master)
#define BSC_FIFO	0x04		// Data FIFO Register (First In, First Out buffer)

/* ***************************** Defined bits for Control Register ****************************/
#define BSC_C_I2CEN    	(1 << 15)			// Enable - set this bit to enable controller
#define BSC_C_INTR    	(1 << 10)			// Interrupt in- set to enable interrupts when Read FIFO is full
#define BSC_C_INTT    	(1 << 9)				// Interrupt out - set to enable interrupts when write FIFO is empty
#define BSC_C_INTD    	(1 << 8)				// set to generate interrupys when a transfer is done
#define BSC_C_ST    	(1 << 7)				// set to start a new transfer
#define BSC_C_CLEAR    	(3 << 4)				// set to clear FIFO (this requires two bits, 5 and 4)
#define BSC_C_READ    	1					// when 1, we are reading, when 0 we are writing

/* ************************ defined bits for status register *************************************/
#define BSC_S_CLKT		(1 << 9)				// clock stretch timeout  - will be set when slave holds the SCL signal high for too long
#define BSC_S_ERR    	(1 << 8)				// will be set when slave fails to acknowledge either its adress or a data byte written to it
#define BSC_S_RXF    	(1 << 7)				// will be set when receiving data if FIFO is full.  No more input will be read
#define BSC_S_TXE    	(1 << 6)				// will be set when transmitting data and FIFO is empty. No data will be transmitted 
#define BSC_S_RXD    	(1 << 5)				// will be set when FIFO contains at least one byte of data to read
#define BSC_S_TXD    	(1 << 4)				// will be set when the FIFO has space for at least one more byte of data to
#define BSC_S_RXR    	(1 << 3)				// will be set during a read transfer if FIFO is more than 1/2 full and needs reading
#define BSC_S_TXW    	(1 << 2)				// will be set during a write transfer when the FIFO is less than 1/2 full and needs writing
#define BSC_S_DONE   	(1 << 1)				// will be set when transfer is complete. reset by writing a 1
#define BSC_S_TA    	1						// will be set when transfer is active

/* **********************Useful bit combinations *********************************************/
#define CLEAR_STATUS    BSC_S_CLKT|BSC_S_ERR|BSC_S_DONE
#define START_READ    	BSC_C_I2CEN|BSC_C_ST|BSC_C_CLEAR|BSC_C_READ
#define START_WRITE   	BSC_C_I2CEN|BSC_C_ST


inline unsigned int dump_bsc_status(bcm_peripheralPtr I2Cperi){
	
	unsigned int s = *((I2Cperi->addr) + BSC_S); 

	printf("BSC_S: ERR=%d  RXF=%d  TXE=%d  RXD=%d  TXD=%d  RXR=%d  TXW=%d  DONE=%d  TA=%d\n",
        (s & BSC_S_ERR) != 0,
        (s & BSC_S_RXF) != 0,
        (s & BSC_S_TXE) != 0,
        (s & BSC_S_RXD) != 0,
        (s & BSC_S_TXD) != 0,
        (s & BSC_S_RXR) != 0,
        (s & BSC_S_TXW) != 0,
        (s & BSC_S_DONE) != 0,
        (s & BSC_S_TA) != 0 );
	return s;
	
	

}

inline void wait_i2c_done(bcm_peripheralPtr I2Cperi){
        //Wait till done, let's use a timeout just in case
        int timeout = 100;
        while(!((*((I2Cperi->addr) + BSC_S) & BSC_S_DONE)) && --timeout) {
            usleep(1000);
        }
        if(timeout == 0)
            printf("wait_i2c_done() timeout. Something went wrong.\n");
}

inline void i2c_init(bcm_peripheralPtr GPIOPeri){
	
	int SDApin, SCLpin;
#ifdef RPI
	SDApin = 0;
	SCLpin = 1;
#endif
#ifdef RPI2
	SDApin = 2;
	SCLpin = 3;
#endif
	INP_GPIO(GPIOPeri->addr, SDApin);
	SET_GPIO_ALT(GPIOPeri->addr, SDApin, 0);
	INP_GPIO(GPIOPeri->addr, SCLpin);
	SET_GPIO_ALT(GPIOPeri->addr, SCLpin, 0);
}
#endif
