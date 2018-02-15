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