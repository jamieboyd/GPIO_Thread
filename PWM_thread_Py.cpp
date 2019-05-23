#include <pyPulsedThread.h>
#include "PWM_thread.h"
#include "PWM_sin_thread.h"

/* ************ Python C module using PWM_thread to do PWM output from Python ******************************
USes the PWM peripheral to output data from PWM channel 1 appearing on the audio Right channel, 
and optionally on GPIO 18, or from PWM channel 2, appearing on the audio Left channel and optionally on GPIO 19.
The date to be output is contained in a user-passed in aray, and is constantly fed to the PWM controller by the pulsedThread
Last Modifed:
2018/09/21 by Jamie Boyd - initial version

******************** Function called automatically when PyCapsule object is deleted in Python *****************************************
Last Modified:
2018/09/21 by Jamie Boyd - initial version */
static void ptPWM_del(PyObject * PyPtr){
	delete static_cast<PWM_thread *> (PyCapsule_GetPointer (PyPtr, "pulsedThread"));
}


/* ****************************************  Constructors ******************************************************
Sets up the PWM clock with the given frequency and range, and creates and configures a PWM_thread object to feed data to the PWM peripheral.
There is only 1 PWM controller, so do not call constructor more than once.  Make is_inited a class field in Python

***************************************** Pulse Description COnstructor***********************************
This function uses pulse duration and number of pulses to define the pulsedThread. 0 pulses means infinite train
Last Modified;
2019/09/21 by Jamie Boyd - initial version*/
 static PyObject* ptPWM_delayDur (PyObject *self, PyObject *args) {
	float pwmFreq;
	unsigned int pwmRange;
	int useFIFO;
	float durSecs;
	unsigned int nPulses;
	int accuracyLevel;
	if (!PyArg_ParseTuple(args,"fIifIi", &pwmFreq, &pwmRange, &useFIFO, &durSecs, &nPulses, &accuracyLevel)){
		PyErr_SetString (PyExc_RuntimeError, "Could not parse inputs for PWM frequency, PWM range, useFIFO,  pulse seconds, number of pulses, and thread accuracy.");
		return NULL;
	}
	PWM_thread * threadObj = PWM_thread::PWM_threadMaker (pwmFreq, pwmRange, useFIFO, (unsigned int) (1E06 * durSecs), nPulses, accuracyLevel);
	usleep (100000);
	if (threadObj == nullptr){
		PyErr_SetString (PyExc_RuntimeError, "PWM_threadMaker was not able to make a PWM object");
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(threadObj), "pulsedThread", ptPWM_del);
	}
}

/* **************************************** Train Frequency Constructor ***********************************
This function uses train frequency and train duration to define the pulsedThread. 0 duration means infinite train
Last Modified;
Last Modified;
2019/09/21 by Jamie Boyd - initial version*/
static PyObject* ptPWM_freqDuty (PyObject *self, PyObject *args) {
	float pwmFreq;
	unsigned int pwmRange;
	float trainFreq;
	int useFIFO;
	float trainDur;
	int accuracyLevel;
	if (!PyArg_ParseTuple(args,"fIiffi", &pwmFreq, &pwmRange, &useFIFO, &trainFreq, &trainDur, &accuracyLevel)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse inputs for PWM frequency, useFIFO, PWM range, train frequency, train duration, and thread accuracy.");
		return NULL;
	}
	PWM_thread * threadObj = PWM_thread::PWM_threadMaker (pwmFreq, pwmRange, useFIFO, trainFreq, trainDur, accuracyLevel);
	usleep (100000);
	if (threadObj == nullptr){
		PyErr_SetString (PyExc_RuntimeError, "PWM_threadMaker was not able to make a PWM object");
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(threadObj), "pulsedThread", ptPWM_del);
	}
}

/* ****************************************** Configures a channel to be output *****************************
There are only 2 channels, 1 and 2, both done on the same thread, so data can be fed to PWM peripheral channels in lockstep
Last Modified:
2018/09/21 by Jamie Boyd - initial verison */ 
static PyObject* ptPWM_addChannel (PyObject *self, PyObject *args) {
	
	PyObject *PyPtr;
	int channel;	// 1 or 2
	int audioOnly;	// 1 to NOT configure GPIO 18 or 19, the audio output will still play
	int mode;		// 0 = PWM_MARK_SPACE for things like servos that need duty cycle, 1 = PWM_BALANCED for analog output, like audio
	int polarity;	// 1 for reversed polarity of PWM output
	int offState;	// state, high or low, when PWM channel is NOT enabled.  Only set high after stopping PWM, set low before starting PWM again
	PyObject * bufferObj; // the unsigned int array of data to feed to PWM, made in Python
	
	if (!PyArg_ParseTuple(args,"OiiiiiO",  &PyPtr, &channel, &audioOnly, &mode, &polarity, &offState, &bufferObj)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object and channel confguration paramaters.");
		return NULL;
	}
	// parse buffer object
	unsigned int nData;
	int * arrayData;
	if (PyObject_CheckBuffer (bufferObj) == 0){
		PyErr_SetString (PyExc_RuntimeError, "Error getting bufferObj from Python array.");
		return NULL;
	}
	Py_buffer buffer;
	if (PyObject_GetBuffer (bufferObj, &buffer, PyBUF_FORMAT)==-1){
		PyErr_SetString (PyExc_RuntimeError,"Error getting C array from bufferObj from Python array");
		return NULL;
	}
#if beVerbose
	printf ("Buffer type is %s, length is %d bytes, and item size is %d.\n", buffer.format, buffer.len, buffer.itemsize);
#endif
	if (strcmp (buffer.format, "i") != 0){
		PyErr_SetString (PyExc_RuntimeError, "Error for bufferObj: data type of Python array is not integer");
		return NULL;
	}
	arrayData = static_cast <int *>(buffer.buf); // Now we have a pointer to the array from the passed in buffer
	nData = (unsigned int) buffer.len/buffer.itemsize;
	PyBuffer_Release (&buffer); // we don't need the buffer object as we have a pointer to the array start, which is all we care about
	// get pointer to PWM_thread
	PWM_thread * threadPtr = static_cast<PWM_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	// Call thread's addChannel function and retiurn the result
	return Py_BuildValue("i", threadPtr ->addChannel (channel, audioOnly, mode, polarity, offState, arrayData, nData));
}

/* ************************************** Sets enable state of PWM output of the thread **********************************
Does not start or stop the pulsedThread from writing data to the PWM peripheral - use start_train, stop_train, etc. for that
Last Modified:
2018/09/21 by Jamie Boyd */
static PyObject* ptPWM_setEnable(PyObject *self, PyObject *args) {
	
	PyObject *PyPtr;
	int enableState;
	int channel;
	int isLocking;
	if (!PyArg_ParseTuple(args,"Oiii",  &PyPtr, &enableState, &channel, &isLocking)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object, enable state, channel, and isLocking.");
		return NULL;
	}
	// get pointer to PWM_thread
	PWM_thread * threadPtr = static_cast<PWM_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	// Call thread's enableState function and return the result
	return Py_BuildValue("i", threadPtr ->setEnable(enableState, channel, isLocking));
}

/* ************************************** Sets polarity of PWM output of the thread **********************************
Last Modified:
2018/09/21 by Jamie Boyd */
static PyObject* ptPWM_setpolarity(PyObject *self, PyObject *args) {
	
	PyObject *PyPtr;
	int polarity;
	int channel;
	int isLocking;
	if (!PyArg_ParseTuple(args,"Oiii",  &PyPtr, &polarity, &channel, &isLocking)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object, polarity, channel, and isLocking.");
		return NULL;
	}
	// get pointer to PWM_thread
	PWM_thread * threadPtr = static_cast<PWM_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	// Call thread's setPolarity function and return the result
	return Py_BuildValue("i", threadPtr ->setPolarity(polarity, channel, isLocking));
}

/* ************************************** Sets offState of PWM output of the thread **********************************
Last Modified:
2018/09/21 by Jamie Boyd */
static PyObject* ptPWM_setOffState(PyObject *self, PyObject *args) {
	
	PyObject *PyPtr;
	int offState;
	int channel;
	int isLocking;
	if (!PyArg_ParseTuple(args,"Oiii",  &PyPtr, &offState, &channel, &isLocking)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object, offState, channel, and isLocking.");
		return NULL;
	}
	// get pointer to PWM_thread
	PWM_thread * threadPtr = static_cast<PWM_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	// Call thread's setoffState function and return the result
	return Py_BuildValue("i", threadPtr ->setOffState(offState, channel, isLocking));
}


/* ************************************** Sets arrayPos of PWM output of the thread **********************************
Last Modified:
2018/09/21 by Jamie Boyd */
static PyObject* ptPWM_setArrayPos(PyObject *self, PyObject *args) {
	
	PyObject *PyPtr;
	int arrayPos;
	int channel;
	int isLocking;
	if (!PyArg_ParseTuple(args,"Oiii",  &PyPtr, &arrayPos, &channel, &isLocking)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object, arrayPos, channel, and isLocking.");
		return NULL;
	}
	// get pointer to PWM_thread
	PWM_thread * threadPtr = static_cast<PWM_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	// Call thread's setarrayPos function and return the result
	return Py_BuildValue("i", threadPtr ->setArrayPos(arrayPos, channel, isLocking));
}

/* ************************ Sets start and stop positions in array of data the thread feeds to PWM **********************************
Last Modified:
2018/09/21 by Jamie Boyd */
static PyObject* ptPWM_setArraySubrange (PyObject *self, PyObject *args){
	PyObject *PyPtr;
	unsigned int startPos;
	unsigned int stopPos;
	int channel;
	int isLocking;
	if (!PyArg_ParseTuple(args,"OIIii", &PyPtr, &startPos, &stopPos, &channel, &isLocking)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object, start pos, stop pos, channel, and isLocking.");
		return NULL;
	}
	// get pointer to PWM_thread
	PWM_thread * threadPtr = static_cast<PWM_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	// Call thread's setAraySubRange function and return the result
	return Py_BuildValue("i", threadPtr ->setArraySubrange(startPos, stopPos, channel, isLocking));
}


/* **********************************Sets a new array of data to be output next ************************
Last Modified:
2018/09/21 by Jamie Boyd - initial version */
static PyObject* ptPWM_setArray(PyObject *self, PyObject *args){
	
	PyObject *PyPtr;
	PyObject * bufferObj; // the unsigned int array of data to feed to PWM, made in Python
	int channel;
	int isLocking;
	if (!PyArg_ParseTuple(args,"OOii",  &PyPtr, &bufferObj, &channel, &isLocking)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object, data array, channel, and isLocking.");
		return NULL;
	}
	// parse buffer object
	unsigned int nData;
	int * arrayData;
	if (PyObject_CheckBuffer (bufferObj) == 0){
		PyErr_SetString (PyExc_RuntimeError, "Error getting bufferObj from Python array.");
		return NULL;
	}
	Py_buffer buffer;
	if (PyObject_GetBuffer (bufferObj, &buffer, PyBUF_FORMAT)==-1){
		PyErr_SetString (PyExc_RuntimeError,"Error getting C array from bufferObj from Python array");
		return NULL;
	}
#if beVerbose
	printf ("Buffer type is %s, length is %d bytes, and item size is %d.\n", buffer.format, buffer.len, buffer.itemsize);
#endif
	if (strcmp (buffer.format, "i") != 0){
		PyErr_SetString (PyExc_RuntimeError, "Error for bufferObj: data type of Python array is not integer");
		return NULL;
	}
	arrayData = static_cast <int *>(buffer.buf); // Now we have a pointer to the array from the passed in buffer
	nData = (unsigned int) buffer.len/buffer.itemsize;
	PyBuffer_Release (&buffer); // we don't need the buffer object as we have a pointer to the array start, which is all we care about
	// get pointer to PWM_thread
	PWM_thread * threadPtr = static_cast<PWM_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	// Call thread's setNewArray function and return the result
	return Py_BuildValue("i", threadPtr ->setNewArray (arrayData, nData, channel, isLocking));
}


/* ************************************************** Gets PWM frequency of PWM peripheral **************************************
Last Modified:
2018/09/21 by Jamie Boyd - initial version */
static PyObject* ptPWM_getPWMFreq (PyObject *self, PyObject *PyPtr){
	PWM_thread * threadPtr = static_cast<PWM_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	return Py_BuildValue("f", threadPtr ->getPWMFreq());
}

/* ************************************************** Gets PWM range of PWM peripheral **************************************
Last Modified:
2018/09/21 by Jamie Boyd - initial version */
static PyObject* ptPWM_getPWMRange (PyObject *self, PyObject *PyPtr){
	PWM_thread * threadPtr = static_cast<PWM_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	return Py_BuildValue("I", threadPtr ->getPWMRange ());
}

/* ************************************************** Gets configured channels of PWM peripheral **************************************
Last Modified:
2018/09/21 by Jamie Boyd - initial version */
static PyObject* ptPWM_getChannels (PyObject *self, PyObject *PyPtr){
	PWM_thread * threadPtr = static_cast<PWM_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	return Py_BuildValue("i", threadPtr ->getChannels ());
}


/* ************************************************** Extra functions for PWM_sin_thread subclass ****************************************************

***************************************** destructor, called automatically when python PyCapcule object is deleted ***********************************
Last Modified:
2018/10/03 by Jamie Boyd - initial version */
static void ptPWM_sin_del(PyObject * PyPtr){
	delete static_cast<PWM_sin_thread *> (PyCapsule_GetPointer (PyPtr, "pulsedThread"));
}


/* ******************************* constructor for PWM_sin_thread *******************************************************
Last Modified:
2018/10/03 by Jamie Boyd - added usleep for no apparent benefit */
static PyObject* ptPWM_sin_NEW (PyObject *self, PyObject *args) {
	int chans;
	if (!PyArg_ParseTuple(args,"i", &chans)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for channel.");
		return NULL;
	}
	PWM_sin_thread * threadObj = PWM_sin_thread::PWM_sin_threadMaker (chans);
	usleep (100000);
	if (threadObj == nullptr){
		PyErr_SetString (PyExc_RuntimeError, "PWM_sin threadMaker was not able to make a PWM_sin object");
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(threadObj), "pulsedThread", ptPWM_sin_del);
	}
}

/* ******************************************** sets frequency of Sin wave to be output *******************************************
Last Modified:
2018/10/03 by Jamie Boyd */
static PyObject* ptPWM_setSinFreq (PyObject *self, PyObject *args){
	PyObject *PyPtr;
	unsigned int newFrequency;
	int channel;
	int isLocking;
	if (!(PyArg_ParseTuple(args,"OIIi", &PyPtr, &newFrequency, &channel, &isLocking))) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object, frequency, channel, and isLocking.");
		return NULL;
	}
	// get pointer to PWM_sin_thread
	PWM_sin_thread * threadPtr = static_cast<PWM_sin_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	// Call thread's setFreq function and return the result
	return Py_BuildValue("i", threadPtr ->setSinFrequency(newFrequency, channel, isLocking));
}

/* ************************************************** Gets the frequency of sine wave being output **************************************
Last Modified:
2018/09/25 by Jamie Boyd - initial version */
static PyObject* ptPWM_getSinFrequency (PyObject *self, PyObject *args){
	PyObject *PyPtr;
	int channel;
	if (!(PyArg_ParseTuple(args,"Oi", &PyPtr, &channel))) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object and channel.");
		return NULL;
	}
	// get pointer to PWM_sin_thread
	PWM_sin_thread * threadPtr = static_cast<PWM_sin_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	return Py_BuildValue("I", threadPtr ->getSinFrequency (channel));
}

/* ***********************************************************************************************************************
code for a simple PWM output without a thread "All PWM, No thread" so we can set pwm
values from Python, with much the same code used threaded version
**************************************************************************************************************************/

/* ******************** Custom Data Struct for  PWM no thread ***************************
last modified:
2019/01/ 09 by Jamie Boyd - initial version */
typedef struct pyPWM_no_thread_Struct{
	int channels; // 1 for channel 1, 2 for channel 2
	int mode1; //MARK_SPACE for servos or BALANCED for analog
	int enable1; // 1 if channel is enabled, 0 if not enabled
	int polarity1; // 1 for reversed output polarity, 0 for normal, default is 0
	int offState1; // 0 for low level when PWM is not enabled, 1 for high level when PWM is enabled
	int mode2; //MARK_SPACE for servos or BALANCED for analog
	int enable2; // 1 if channel is enabled, 0 if not enabled
	int polarity2; // 1 for reversed output polarity, 0 for normal, default is 0
	int offState2; // 0 for low level when PWM is not enabled, 1 for high level when PWM is enabled
	// calculated register addresses, used for customDataMod functions
	volatile unsigned int * ctlRegister; // address of PWM control register
	volatile unsigned int * dataRegister1; // address of register to write data to
	volatile unsigned int * dataRegister2; // address of register to write data to
	float setFrequency;
	unsigned int pwmRange;
	unsigned int currentValue1;
	unsigned int currentValue2;
} pyPWM_no_thread_Struct, *pyPWM_no_thread_StructPtr;


/* ********************* Custom delete function for PWM_no_thread ******************************************
last modified:
2019/01/09 by Jamie Boyd - initial version */
static void  pyPWM_no_thread_del(PyObject * PyPtr){
	pyPWM_no_thread_StructPtr taskData = static_cast <pyPWM_no_thread_StructPtr> (PyCapsule_GetPointer (PyPtr, "AllPWMNoThread"));
	if (taskData->channels & 1){
		INP_GPIO(GPIOperi->addr,18);
	}
	if (taskData->channels & 2){
		INP_GPIO(GPIOperi->addr,19);
	}
	//unUsePWMClockperi();
	//unUsePWMperi();
	//unUseGPIOperi();
}

/* *****************************initialization *************************************************************
makes the structure and returns pointer to the structure back to Python, but does not add any channels
maps peripherals and sets PWM clock frequency and PWM range
last modified:
2019/01/09 by Jamie Boyd - initial version */
static PyObject* pyPWM_no_thread_init (PyObject *self, PyObject *args) {
	float pwmFreq;
	unsigned int pwmRange;
	if (!(PyArg_ParseTuple(args,"fI", &pwmFreq, &pwmRange))) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for PWM frequency and Range.");
		return NULL;
	}
	
	int mapResult = PWM_thread::mapPeripherals ();
	if (mapResult){
		PyErr_SetString (PyExc_RuntimeError, "Could not map peripherals for PWM access.");
		return NULL;
	}
	// set clock for PWM from input parmamaters
	float setFrequency = PWM_thread::setClock (pwmFreq, pwmRange);
	if (setFrequency < 0){
		PyErr_SetString (PyExc_RuntimeError, "Could not set clock for PWM with requested frequency and range.");
		return NULL;
	}
	// make structure
	pyPWM_no_thread_StructPtr taskData = new  pyPWM_no_thread_Struct;
	taskData->setFrequency = setFrequency;
	taskData->pwmRange=pwmRange;
	// start with channels at 0,
	taskData->channels = 0;
	taskData->enable1=0;
	taskData->enable2 =0;
	// save ctl and data register addresses in task data for easy access
	taskData->ctlRegister = PWMperi->addr + PWM_CTL;
	taskData->dataRegister1 = PWMperi->addr + PWM_DAT1;
	taskData->dataRegister2 = PWMperi->addr + PWM_DAT2;
	// wrap this struct in a pyCapsule and return it to python
	return PyCapsule_New (static_cast <void *>(taskData), "AllPWMNoThread", pyPWM_no_thread_del);
}

/* ***************************** adds a channel *************************************************************
adds and configures a channel, either 1 on GPIO 18  or 2 on GPIO 19 with requested mode, polarity, and offstate
last modified:
2019/01/09 by Jamie Boyd - initial version */
static PyObject* pyPWM_no_thread_addChan (PyObject *self, PyObject *args) {
	PyObject *PyPtr;
	int channel;
	int mode;
	int polarity;
	int offState;
	if (!(PyArg_ParseTuple(args,"Oiiii", &PyPtr, &channel, &mode, &polarity, &offState))) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for PWM_no_thread pointer, channel, mode, polarity, and off-state.");
		return NULL;
	}
	// get pointer to the task data structure
	pyPWM_no_thread_StructPtr taskData = static_cast <pyPWM_no_thread_StructPtr> (PyCapsule_GetPointer (PyPtr, "AllPWMNoThread"));
	// some register address offsets and bits are channel specific, if we save them in variables
	// at the start, we can use same the code for either channel later on
	unsigned int modeBit;
	unsigned int polarityBit;
	unsigned int offStateBit;
	unsigned int enableBit;
	// instead of constantly writing to register, copy control register into a variable 
	// then set the control register with the variable at the end
	unsigned int registerVal = *(taskData->ctlRegister);
	registerVal &= ~(PWM_MODE1 | PWM_MODE2);
	*(taskData->ctlRegister) =0;
	if (channel == 1){
		// set channel 1 in taskData
		taskData->channels |= 1;
		// set up GPIO
		INP_GPIO(GPIOperi ->addr, 18);           // Set GPIO 18 to input to clear bits
		SET_GPIO_ALT(GPIOperi ->addr, 18, 5);     // Set GPIO 18 to Alt5 function PWM0
		// set bits and offsets appropriately for channel 1
		modeBit = PWM_MSEN1;
		enableBit = PWM_PWEN1;
		polarityBit = PWM_POLA1;
		offStateBit = PWM_SBIT1;
	}else{
		if (channel == 2){
			// set channel 2 in taskData
			taskData->channels |= 2;
			// set up GPIO
			INP_GPIO(GPIOperi ->addr, 19);           // Set GPIO 19 to input to clear bits
			SET_GPIO_ALT(GPIOperi->addr, 19, 5);     // Set GPIO 19 to Alt5 function PWM1
			// set bits and offsets appropriately for channel 2
			modeBit = PWM_MSEN2;
			enableBit = PWM_PWEN2;
			polarityBit = PWM_POLA2;
			offStateBit = PWM_SBIT2;
		}else{
			PyErr_SetString (PyExc_RuntimeError, "Channel requested for configuration was not 1 or 2.");
			return NULL;
		}
	}
	// set up PWM channel 1 or 2 by writing to control register
	// set mode
	if (mode == PWM_MARK_SPACE){
		registerVal |= modeBit; // put PWM in MS Mode
	}else{
		registerVal &= ~(modeBit);  // clear MS mode bit for Balanced Mode
	}
	// set polarity
	if (polarity == 0){
		registerVal &= ~polarityBit;  // clear reverse polarity bit
	}else{
		registerVal |= polarityBit;  // set reverse polarity bit
	}
	// set off state, whether PWM output is hi or low when not transmitting data
	if (offState == 0){
		registerVal &= ~offStateBit; // clear OFFstate bit for low
	}else{
		registerVal |= offStateBit; // set OFFstate bit for hi
	}
	// start in unenabled state
	registerVal &= ~enableBit;
	// set the control register with registerVal
	*(taskData->ctlRegister) = registerVal;
	Py_RETURN_NONE;
}


/* ***************************** sets a PWM value for a channel *************************************************************
last modified:
2019/01/09 by Jamie Boyd - initial version */
static PyObject* pyPWM_no_thread_setValue (PyObject *self, PyObject *args){
	PyObject *PyPtr;
	int pwmValue;
	int pwmChans;
	if (!PyArg_ParseTuple(args,"Oii", &PyPtr, &pwmValue, &pwmChans)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for PWM_no_thread pointer, PWM value, and channel.");
		return NULL;
	}
	pyPWM_no_thread_StructPtr taskData = static_cast<pyPWM_no_thread_StructPtr > (PyCapsule_GetPointer(PyPtr, "AllPWMNoThread"));
	if ((pwmValue < 0) || (pwmValue >= taskData -> pwmRange)){
		PyErr_SetString (PyExc_RuntimeError, "Value requested for output was greater than PWM range.");
		return NULL;
	}
	if (pwmChans & 1){
		*(taskData->dataRegister1) = pwmValue;
		taskData ->currentValue1 = pwmValue;
	}
	if (pwmChans & 2){
		*(taskData->dataRegister2) = pwmValue;
		taskData ->currentValue2 = pwmValue;
	}
	Py_RETURN_NONE;
}

/* ***************************** sets able state (enabled or disabled) for a channel or both channels *************************************************************
last modified:
2019/01/09 by Jamie Boyd - initial version */
static PyObject* pyPWM_no_thread_setAble (PyObject *self, PyObject *args){
	PyObject *PyPtr;
	int ableState;
	int pwmChans;
	if (!PyArg_ParseTuple(args,"Oii", &PyPtr, &ableState, &pwmChans)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for PWM_no_thread pointer, able state and PWM channel.");
		return NULL;
	}
	pyPWM_no_thread_StructPtr taskData = static_cast<pyPWM_no_thread_StructPtr > (PyCapsule_GetPointer(PyPtr, "AllPWMNoThread"));
	if (ableState){
		if (pwmChans & 1){
			*(taskData->ctlRegister)  |= PWM_PWEN1;
		}
		if (pwmChans & 2){
			*(taskData->ctlRegister)  |= PWM_PWEN2;
		}
	}else{
		if (pwmChans & 1){
			*(taskData->ctlRegister) &= ~PWM_PWEN1;
		}
		if (pwmChans & 2){
			*(taskData->ctlRegister) &= ~PWM_PWEN2;
		}
	}
	Py_RETURN_NONE;
}

/* ***************************** returns value currently being output by a PWM channel *************************************************************
last modified:
2019/01/09 by Jamie Boyd - initial version */
static PyObject* pyPWM_NoThread_getVal (PyObject *self, PyObject *args){
	PyObject *PyPtr;
	int pwmChan;
	if (!PyArg_ParseTuple(args,"Oi", &PyPtr, &pwmChan)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for PWM_no_thread pointer, and PWM channel.");
		return NULL;
	}
	pyPWM_no_thread_StructPtr taskData = static_cast<pyPWM_no_thread_StructPtr > (PyCapsule_GetPointer(PyPtr, "AllPWMNoThread"));
	if (pwmChan == 1){
		return Py_BuildValue ("I", taskData->currentValue1 );
	}else{
		if (pwmChan ==2){
			return Py_BuildValue ("I", taskData->currentValue2);
		}else{
			PyErr_SetString (PyExc_RuntimeError, "Requested PWM channel was not 1 or 2");
			return NULL;
		}
	}
}


/* Module method table - those starting with ptPWM are defined here, those starting with pulsedThread are defined in pyPulsedThread.h*/
static PyMethodDef ptPWMMethods[] = {
	{"isBusy", pulsedThread_isBusy, METH_O, "(PyCapsule) returns number of tasks a thread has left to do, 0 means finished all tasks"},
	{"waitOnBusy", pulsedThread_waitOnBusy, METH_VARARGS, " (PyCapsule, timeOutSecs) Returns when a thread is no longer busy, or after timeOutSecs"},
	{"doTask", pulsedThread_doTask, METH_O, "(PyCapsule) Tells the pulsedThread object to do whatever task it was configured for"},
	{"doTasks", pulsedThread_doTasks, METH_VARARGS, "(PyCapsule, nTasks) Tells the pulsedThread object to do whatever task it was configured for nTasks times"},
	{"unDoTasks", pulsedThread_unDoTasks, METH_O, "(PyCapsule) Tells the pulsedThread object to stop doing however many task it was asked to do"},
	{"startTrain", pulsedThread_startTrain, METH_O, "(PyCapsule) Tells a pulsedThread object configured as an infinite train to start"},
	{"stopTrain", pulsedThread_stopTrain, METH_O, "(PyCapsule) Tells a pulsedThread object configured as an infinite train to stop"},
	//{"modDelay", pulsedThread_modDelay, METH_VARARGS, "(PyCapsule, newDelaySecs) changes the delay period of a pulse or LOW period of a train"},
	{"modDur", pulsedThread_modDur, METH_VARARGS, "(PyCapsule, newDurationSecs) changes the delay period of a pulse or HIGH period of a train"},
	{"modTrainLength", pulsedThread_modTrainLength, METH_VARARGS, "(PyCapsule, newTrainLength) changes the number of pulses of a train"},
	{"modTrainDur", pulsedThread_modTrainDur, METH_VARARGS, "(PyCapsule, newTrainDurSecs) changes the total duration of a train"},
	{"modTrainFreq", pulsedThread_modFreq, METH_VARARGS, "(PyCapsule, newTrainFequency) changes the frequency of a train"},
	//{"modTrainDuty", pulsedThread_modDutyCycle, METH_VARARGS, "(PyCapsule, newTrainDutyCycle) changes the duty cycle of a train"},
	//{"getPulseDelay", pulsedThread_getPulseDelay, METH_O, "(PyCapsule) returns pulse delay, in seconds"},
	{"getPulseDuration", pulsedThread_getPulseDuration, METH_O, "(PyCapsule) returns pulse duration, in seconds"},
	{"getPulseNumber", pulsedThread_getPulseNumber, METH_O, "(PyCapsule) returns number of pulses in a train, 1 for a single pulse, or 0 for an infinite train"},
	{"getTrainDuration", pulsedThread_getTrainDuration, METH_O, "(PyCapsule) returns duration of a train, in seconds"},
	{"getTrainFrequency", pulsedThread_getTrainFrequency, METH_O, "(PyCapsule) returns frequency of a train, in Hz"},
	//{"getTrainDutyCycle", pulsedThread_getTrainDutyCycle, METH_O, "(PyCapsule) returns duty cycle of a train, between 0 and 1"},
	{"unsetEndFunc", pulsedThread_UnSetEndFunc, METH_O, "(PyCapsule) un-sets any end function set for this pulsed thread"},
	{"hasEndFunc", pulsedThread_hasEndFunc, METH_O, "(PyCapsule) Returns the endFunc status (installed or not installed) for this pulsed thread"},
	{"setEndFuncObj", pulsedThread_SetPythonEndFuncObj, METH_VARARGS, "(PyCapsule, PythonObj, int mode) sets a Python object to provide endFunction for pulsedThread"},
	{"setTaskFuncObj", pulsedThread_SetPythonTaskObj, METH_VARARGS, "(PyCapsule, PythonObj) sets a Python object to provide LoFunc and HiFunc for pulsedThread"},
	{"setArrayEndFunc", pulsedThread_setArrayFunc, METH_VARARGS, "(PyCapsule, Python float array, endFuncType, isLocking) sets pulsedThread endFunc to set frequency (type 0) or duty cycle (type 1) from a Python float array"},
	{"cosDutyCycleArray", pulsedThread_cosineDutyCycleArray, METH_VARARGS, "(Python float array, pointsPerCycle, offset, scaling) fills passed-in array with cosine values of given period, with applied scaling and offset expected to range between 0 and 1"},
	{"getModFuncStatus", pulsedThread_modCustomStatus, METH_O, "(PyCapsule) Returns 1 if the pulsedThread object is waiting for the thread to call a modFunction, else 0"},

	{"newDelayDur", ptPWM_delayDur, METH_VARARGS, "(pwmFreq, pwmRange, useFIFO, durUsecs, nPulses, accuracyLevel) Creates and configures new PWM task"},
	{"newFreqDuty", ptPWM_freqDuty, METH_VARARGS, "(pwmFreq, pwmRange, useFIFO, trainFreq , trainDuration, accuracyLevel) Creates and configures new PWM task"},
	{"newSin", ptPWM_sin_NEW, METH_VARARGS, "(channels) creates a new PWM_sin task for the given channel."},
	{"addChannel", ptPWM_addChannel,METH_VARARGS, "(PyCapsule,channel, audioOnly, mode, polarity, offState, dataArray)"},
	{"setEnable", ptPWM_setEnable, METH_VARARGS, "(PyCapsule, enableState, channel, isLocking) Enables or disables the PWM channel"},
	{"setPolarity", ptPWM_setpolarity, METH_VARARGS, "(PyCapsule, polarity, channel, isLocking) Sets polarity of the PWM channel"},
	{"setOffState", ptPWM_setOffState, METH_VARARGS, "(PyCapsule, offState, channel, isLocking) Sets offState of the PWM channel"},
	{"setArrayPos", ptPWM_setArrayPos, METH_VARARGS, "(PyCapsule, arrayPos, channel, isLocking) Sets next position in array of data"},
	{"setArraySubRange", ptPWM_setArraySubrange, METH_VARARGS, "(PyCapsule, startPos, stopPos, channel, isLocking) Selects subset of data"},
	{"setArray", ptPWM_setArray, METH_VARARGS, "(PyCapsule,  array, channel, isLocking) Sets the array of data for pulsedThread to feed to PWM"},
	{"setSinFreq", ptPWM_setSinFreq, METH_VARARGS, "(PyCapsule,  sin Frequency, channel, isLocking) Sets the frequency of sine wave that a PWM_sin outputs "},
	{"getSinFreq", ptPWM_getSinFrequency, METH_VARARGS, "(PyCapsule, channel) returns the frequency of sine wave that a PWM_sin is outputting "},
	{"getPWMFreq", ptPWM_getPWMFreq, METH_O, "(PyCapsule) returns the PWM update frequency"},
	{"getPWMRange", ptPWM_getPWMRange, METH_O, "(PyCapsule) returns the PWM range"},
	{"getChannels", ptPWM_getChannels, METH_O, "(PyCapsule) returns a bit-wise number indicating which PWM channels are configured"},
	
	{"newThreadless", pyPWM_no_thread_init, METH_VARARGS, "(pwmFreq, pwmRange) configures a PWM task with no associated thread"},
	{"threadlessAddChan", pyPWM_no_thread_addChan, METH_VARARGS, "(PyCapsule, channel, mode, polarity, offstate) configures a channel for threadless PWM"},
	{"threadlessSetValue", pyPWM_no_thread_setValue, METH_VARARGS, "(PyCapsule, value, channels) sets PWM value to output for selected channels"},
	{"threadlessSetAble", pyPWM_no_thread_setAble, METH_VARARGS, "(PyCapule, ableState, channels) enables or disables seletced PWM channels"},
	{"threadlessGetValue", pyPWM_NoThread_getVal, METH_VARARGS, "(PyCapule, channel) returns PWM being output by selected channel"},
	{ NULL, NULL, 0, NULL}
  };

  /* Module structure */
  static struct PyModuleDef ptPWMmodule = {
    PyModuleDef_HEAD_INIT,
    "ptPWM",           /* name of module */
    "An external module using pulsedThread to feed data to the PWM peripheral on a Raspberry Pi",  /* Doc string (may be NULL) */
    -1,                 /* Size of per-interpreter state or -1 */
    ptPWMMethods       /* Method table */
  };

  /* Module initialization function */
  PyMODINIT_FUNC
  PyInit_ptPWM(void) {
    return PyModule_Create(&ptPWMmodule);
  }  
