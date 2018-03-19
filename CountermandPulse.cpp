




/* ***************** countermandable Hi Callback ******************************
Task to do on High tick, sets GPIO line high or ow depending on polarity
unless countermanded
last modified:
2018/03/16 by Jamie Boyd - initial version */
void Countermand_Hi (void *  taskData){
	SimpleGPIOStructPtr gpioTaskPtr = (SimpleGPIOStructPtr) taskData;
	if (gpioTaskPtr->countermand == true){
		gpioTaskPtr->countermand = false;
	}else{
		*(gpioTaskPtr->GPIOperiHi) =gpioTaskPtr->pinBit;
	}
}




/* ******************* ThreadMaker for countermandable pulse ********************
Last Modified:
2018/03/16 by Jamie Boyd  - initial version */


/* ****************************** Custom delete Function *****************************************************/
void CountermandPulse_delTask (void * taskData){
	CountermandPulseStructPtr gpioTaskPtr = (CountermandPulseStructPtr) taskData;
	delete (CountermandPulseStructPtr);
}

CountermandPulse * CountermandPulse::CountermandPulse_threadMaker (int pin, int polarity, unsigned int delayUsecs, unsigned int durUsecs, int accuracyLevel){
	// make and fill an init struct
	SimpleGPIOInitStructPtr initStruct = new SimpleGPIOInitStruct;
	initStruct->thePin = pin;
	initStruct->thePolarity = polarity;
	initStruct->GPIOperiAddr =  useGpioPeri ();
	if (initStruct->GPIOperiAddr == nullptr){
#if beVerbose
		printf ("CountermandPulse_threadMaker failed to map GPIO peripheral.\n");
#endif
		return nullptr;
	}
	int errCode =0;
	// call CountermandPulse constructor, which calls SimpleGPIO_thread contructor
	CountermandPulse * newCountermandPulse = new CountermandPulse (pin, polarity, delayUsecs, durUsecs, (void *) &initStruct, accuracyLevel, errCode);
	if (errCode){
#if beVerbose
		printf ("CountermandPulse_threadMaker Failed to make pulsed thread.\n");
#endif
		return nullptr;
	}
	// set countermanable high function in place of regular hi function
	newCountermandPulse->setHighFunc (&Countermand_Hi);
	// set custom delete function for task data
	newCountermandPulse->setTaskDataDelFunc (&CountermandPulse_delTask);
	// make a task pointer for easy direct access to thread task data for countermanding without benefit of mutex
	newCountermandPulse->taskStructPtr = (CountermandPulseStructPtr)newCountermandPulse->getTaskData ();
	return newCountermandPulse;
}
