#ifndef GPIOLOWLEVEL_H
#define GPIOLOWLEVEL_H

/* ********************** Low Level access to Raspberry Pi GPIO Peripherals ************************ 
last modified:
2018/09/18 by Jamie Boyd - PWM constants improved
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
/* *************************************************GPIO Peripheral************************************************
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
/* ********************PWM control registers addresses defined by an offset to PWM_BASE. *************/
#define PWM_CTL		0x0          // PWM Control
#define PWM_STA		0x4		// PWM status
#define PWM_DMAC		0x8		// PWM DMA configuration
#define PWM_RNG1		0x10        // PWM Channel 1 Range
#define PWM_DAT1		0x14	// PWM Channel 1 Data
#define PWM_FIF		0x18	//PWM FIFO input, for both channels
#define PWM_RNG2		0x20	// PWM Channel 2 Range
#define PWM_DAT2		0x24	// PWM Channel 2 Data

/* ******************************* Defined Bits for Control Register (PWM_CTL) *******************************************/
#define PWM_MSEN2	0x8000	//  bit 15, when set, run channel 2 in Mark/Space mode, when cleared run in balanced mode
#define PWM_USEF2	0x2000	// bit 13, when set, use FIFO for input for channel 2, when cleared, use data register
#define PWM_POLA2	0x1000	// bit 12, when set, output of channel 2 is reversed
#define PWM_SBIT2		0x800	// bit 11, when set, channel 2 output is high when PWM is not transmitting, when cleared, output is low when not transmitting
#define PWM_RPTL2		0x400	// bit 10, when set, transmission interrupts when FIFO is empty, when cleared, last data is transmitted
#define PWM_MODE2	0x200	// bit 9, when set, channel 2 uses serializer mode. when clear, channel 2 uses PWM mode
#define PWM_PWEN2	0x100	// bit 8, when set, channel 2 is enabled, when cleared, channel 2 is not transmitting
#define PWM_MSEN1	0x80	// bit 7, when set, run channel 1 in Mark/Space mode, when cleared run in balanced mode
#define PWM_CLRF1		0x40	// bit 6, when set, clears the FIFO used by both channels, one shot, when cleared, has no effect
#define PWM_USEF1	0x20	// bit 5, use FIFO for input for channel 1, when cleared, use data register
#define PWM_POLA1	0x10	// bit 4, when set, output of channel 1 is reversed
#define PWM_SBIT1		0x8		// bit 3, when set, channel 1 output is high when PWM is not transmitting, when cleared, output is low when not transmitting
#define PWM_RPTL1		0x4		// bit 2 when set, transmission interrupts when FIFO is empty, when cleared, last data is transmitted
#define PWM_MODE1	0x2		// bit 1, when set, channel 1 uses serializer mode. when clear, channel 1 uses PWM mode
#define PWM_PWEN1	0x1		// bit 0, when set, channel 1 is enabled, when cleared, channel 1 is not transmitting

/* ******************************* Defined Bits for Status Register (PWM_STA) *******************************************/
#define PWM_STA4		0x1000	// bit 12, channel 4 state, if set, channel is transmitting, if clear, channel is not transmitting. There is no channel 4?
#define PWM_STA3		0x800	// bit 11, channel 3 state, if set, channel is transmitting, if clear, channel is not transmitting. There is no channel 3?
#define PWM_STA2		0x400	// bit 10, channel 2 state, if set, channel is transmitting, if clear, channel is not transmitting. 
#define PWM_STA1		0x200	// bit 9, channel 1 state, if set, channel is transmitting, if clear, channel is not transmitting. 
#define PWM_BERR		0x100	// bit 8, set if error has occurred while writing to registers
#define PWM_GAPO4	0x80	// bit 7, flag set when gap occurred writing to FIFO on channel 4.There is no channel 4?
#define PWM_GAPO3	0x40	// bit 6, flag set when gap occurred writing to FIFO on channel 3.There is no channel 3?
#define PWM_GAPO2	0x20	// bit 5, flag set when gap occurred writing to FIFO on channel 2.
#define PWM_GAPO1	0x10	// bit 4, flag set when gap occurred writing to FIFO on channel 1.
#define PWM_RERR1	0x8		// bit 3, FIFO read error flag
#define PWM_WERR1	0x4		// bit 2, FIFO write error flag
#define PWM_EMPT1	0x2		// bit 1, FIFO empty flag
#define PWM_FULL1		0x1		// bit 0, FIFO full flag

#define PWM_MARK_SPACE 0
#define PWM_BALANCED 1

extern bcm_peripheralPtr PWMperi ;
extern int PWMperi_users;
volatile unsigned int * usePWMPeri (void);
void unUsePWMperi (void);

/* ************************************************* Clock Manager - Audio Clock (PWM) control *********************************************************
/* CM_PWMCTL is defined by 0x101000 offset from the base peripheral addresss */
#define CM_PWMBASE	(BCM_PERI_BASE + 0x101000)
/* ********************CLock manager Divisor register address defined by an offset to CM_PWMCTL *************/
#define CM_PWMCNTL		0x0		// PWM clock control
#define CM_PWMDIV		0x4		// PWM Divisor

/* ******************************* Defined Clock Manager Control Register (CM_PWMCTL) Bits *******************************************/
#define CM_PASSWD	0x5A000000 	// bits 31-24, some values need to be ORed with this magic number, the clock manager password */
#define CM_MASH0		0x0			// bits 9 and 10 control MASH divider, 0 means integer division
#define CM_MASH1		0x200		// bit 9, 1-stage MASH
#define CM_MASH2		0x400		// bit 10, 2-stage mash
#define CM_MASH3		0x600		// bits 9 and 10 set, 3 stage mash
#define CM_FLIP		0x100		// bit 8, if set, inverts clock generator output
#define CM_BUSY		0x80		// bit 7, indicates if the clock generator is running
#define CM_KILL		0x20		//  bit 5, when set, stops and resets the clock generator
#define CM_ENAB		0x10		// bit 4, setting with OR requests the clock to start, BUSY flag will go high when final cycle is completed
#define CM_DISAB	0xffffffef	// bit 4, unsetting with AND requests the clock to stop, BUSY flag will go low when final cycle is completed
#define CM_SRCOSC		0x1			// use Pi's on board oscillator as input for clock source at 19.2 Mhz
#define CM_SRCPLL		0x6			// use Pi's Phase Locked Loop D as  input for clock source at 500 MHz 
#define CM_SRCHDMI	0x7			// use HDMI auxillary clock as nput for clock source at 216 Mhz

/* ******************************* Defined Divisor Register (CM_PWMDIV) Bits *******************************************/
#define CM_DIVI		12			// integer part of divisor, from 23-12 bits, need to shift left 12 bits. Fractional from 0 to 11

/*Frequency of oscillators that we use as source for things like PWM clock*/
#define PI_CLOCK_RATE 19.2e6	//19.2 Mhz
#define HDMI_CLOCK_RATE 216e06 // 216 Mhz HDMI auxillary
#define PLLD_CLOCK_RATE 500e6	// 500 MHz phase locked loop D 

extern bcm_peripheralPtr PWMClockperi;
extern int PWMClockperi_users;
volatile unsigned int * usePWMClockPeri (void);
void unUsePWMClockperi (void);


/* ********************************************** GPU CLock ******************************************************/
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
