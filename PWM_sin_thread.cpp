	
void ptPWM_sin_func (void * taskDataP){
	ptPWMStructPtr taskData = (ptPWMStructPtr)taskDataP;
	*(taskData->dataRegister) = taskData-> arrayData[(taskData->arrayPos % (taskData->endPos -1))];	
	taskData->arrayPos += taskDataP->freq;
}
	
	PWM_sin_thread * PWM_sin_thread::PWM_sin_threadMaker (int channel, int mode, int enable){
	/* 	make sine wave array data for a 1 Hz sine wave running at PWM_SIN_UPDATE_FREQ
		The method is to make a cosine wave at lowest ever needed frequency, in this case 1 Hz, and instead of changing the wave when we change frequencies,
		we change the step by which we jump. We can easily do any multiple of the base frequency.  With 1Hz cosine wave, we add 1 each time for 1 Hz,
		add 2 each time for 2 Hz, etc, using the modulo operator % so we don't get tripped up at wrap-around. 
	*/
	int * dataArray = new int [PWM_SIN_UPDATE_FREQ];
	double arraySize = (double)PWM_SIN_UPDATE_FREQ;
	double offset = PWM_SIN_RANGE/2;
	for (double ii=0; ii< arraySize; ii +=1){
		dataArray [ii] = (unsigned int) round (offset - offset * cos (PHI *(ii/ arraySize)));
	}
	
}


