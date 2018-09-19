#include "GPIOlowlevel.h"

int map_peripheral(bcm_peripheralPtr p, int memInterface){
	if (memInterface == IFACE_DEV_GPIOMEM){
		// Open newfangled dev/gpiomem instead of /dev/mem for access without sudo
		if ((p->mem_fd = open("/dev/gpiomem", O_RDWR|O_SYNC) ) < 0) {
			perror("Failed to open /dev/gpiomem");
			return 1;
		}
	}else{
		if ((p->mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
			perror("Failed to open /dev/mem. Did you forget to sudo");
			return 1;
		}
	}	
	/* mmap IO */
	p->map = mmap(
		NULL,							//Any address in our space will do
		BLOCK_SIZE,						//Map length
		PROT_READ|PROT_WRITE|PROT_EXEC,	// Enable reading & writing to mapped memory
		MAP_SHARED| MAP_LOCKED,			//Shared with other processes
		p->mem_fd,						//File to map
		p->addr_p						//Offset to base address
	);
	//p->map = mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, p->mem_fd, p->addr_p);
	if (p->map == MAP_FAILED) {
		perror("mmap error");
		close (p->mem_fd);
		return 1;
	}
	p ->addr = (volatile unsigned int *)p->map;
	// close file descriptor
	if (close(p -> mem_fd) < 0){
		perror("couldn't close memory file descriptor");
		return 1;
	}
	return 0;
}

/* ******************** Un-Map a Peripheral *******************************************/
void unmap_peripheral(bcm_peripheralPtr p) {
	munmap(p->map, BLOCK_SIZE);
}


/* ****** Utility functions for peripheral mapping ***********************
have to initialize these somewhere */
bcm_peripheralPtr GPIOperi = nullptr;
int GPIOperi_users=0;

volatile unsigned int * useGpioPeri (void){
	// map GPIO peripheral, if needed
	int errCode =0;
 	if (GPIOperi ==nullptr) {
  		GPIOperi = new bcm_peripheral {GPIO_BASE};
  		errCode = map_peripheral (GPIOperi, IFACE_DEV_GPIOMEM);
 		if (errCode){
			GPIOperi  = nullptr;
			GPIOperi_users =0;
			return nullptr;
  		}
 	}
	GPIOperi_users +=1;
	return GPIOperi->addr;
 }
  
 void unUseGPIOperi (void){
	  if (GPIOperi_users > 0){ // no-one would call unUseGPIOperi before useGpioPeri, would they?
		GPIOperi_users -=1;
		if (GPIOperi_users ==0){
			unmap_peripheral (GPIOperi);
			delete (GPIOperi);
		}
	} 
 }
 
bcm_peripheralPtr PWMperi = nullptr;
int PWMperi_users=0;

volatile unsigned int * usePWMPeri (void){
	// map PWM peripheral, if needed
	int errCode =0;
 	if (PWMperi ==nullptr) {
  		PWMperi = new bcm_peripheral {PWM_BASE};
  		errCode = map_peripheral (PWMperi, IFACE_DEV_MEM);
  		if (errCode){
			PWMperi = nullptr;
			PWMperi_users = 0;
			return nullptr;
  		}
	}
	PWMperi_users +=1;
	return PWMperi->addr;
 }
  
 void unUsePWMperi (void){
	  if (PWMperi_users > 0){
		PWMperi_users -= 1;
		if (PWMperi_users == 0){
			unmap_peripheral (PWMperi);
			delete (PWMperi);
		}
	} 
 }

bcm_peripheralPtr PWMClockperi = nullptr;
int PWMClockperi_users=0;

volatile unsigned int * usePWMClockPeri (void){
	// map PWM clock peripheral, if needed
	int errCode =0;
 	if (PWMClockperi ==nullptr) {
  		PWMClockperi = new bcm_peripheral {CM_PWMBASE};
  		errCode = map_peripheral (PWMClockperi, IFACE_DEV_MEM);
  		if (errCode){
			PWMClockperi = nullptr;
			PWMClockperi_users = 0;
			return nullptr;
  		}
	}
	PWMClockperi_users +=1;
	return PWMClockperi->addr;
 }

void unUsePWMClockperi (void){
	if (PWMClockperi_users > 0){
		PWMClockperi_users -= 1;
		if (PWMClockperi_users == 0){
			*(PWMClockperi->addr  + PWMCLK_CNTL) =(*(PWMClockperi->addr  + PWMCLK_CNTL) &~0x10)|BCM_PASSWORD; // Turn off clock enable flag.
			unmap_peripheral (PWMClockperi);
			delete (PWMClockperi);
		}
	} 
}
