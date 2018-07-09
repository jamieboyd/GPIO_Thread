#include "GPIOlowlevel.h"

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
  		PWMClockperi = new bcm_peripheral {PWM_CLOCK_BASE};
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
			unmap_peripheral (PWMClockperi);
			delete (PWMClockperi);
		}
	} 
}
