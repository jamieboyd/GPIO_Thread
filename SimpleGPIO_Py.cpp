#include <pyPulsedThread.h>
#include "SimpleGPIO_thread.h"

/* ************Simple GPIO Pulses and Trains from Python using SimpleGPIO_thread ******************************
Last Modifed:
2018/02/05 by Jamie Boyd - updates for pulsedThread class/SimpleGPIO_thread subclass

************************* Constructors for Pulses and Trains *************************************************

************************ Makes a Pulse ****************************************************************
Last Modified:
2018/02/05 by Jamie Boyd - updates for pulsedThread class/SimpleGPIO_thread subclass */
static PyObject* ptSimpleGPIO_pulse (PyObject *self, PyObject *args) {
	float delay;
	float dur;
	int polarity;
	int pin;
	int accLevel;
	if (!PyArg_ParseTuple(args,"ffiii",  &delay, &dur, &polarity, &pin, &accLevel)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for delay, duration, polarity, pin, and accuracy level.");
		return NULL;
	}
	SimpleGPIO_thread *  threadObj= SimpleGPIO_thread::SimpleGPIO_threadMaker (pin, polarity, (unsigned int) round (1e06 *delay), (unsigned int) round (1e06 *dur), (unsigned int) 1, accLevel);
	if (threadObj == nullptr){
		PyErr_SetString (PyExc_RuntimeError, "SimpleGPIO_threadMaker was not able to make a GPIO thread object");
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(threadObj), "pulsedThread", pulsedThread_del);
	}
  }
  
  /* Creates and configures a SimpleGPIO_thread object to output a train of pulses on a GPIO pin
 Two functions, one taking paramaters of pulse timing and number, (delay in seconds, duration in seconds, and number of puses)
 the other taking paramaters of train frequency,  train duration , and train duty cycle. 0 for either nPulses or trainDuration
 requests an infinite train.
 pin: number of the gpio pin used for the task. 
 accLevel: accuracy level for timing, 0 relies solely on sleeping for timing, which may not be accurate beyond the ms scale
 1 sleeps for first part of a ticks, but wakes early and does stricter timing with clocks at the end of the tick,
 checking each time against expected time. more processor intensive, more accurate.
  Note all trains are normally low; i.e.,  at start and end of train, line is set low. Could easily add an option for normally high */

  static PyObject* ptSimpleGPIO_trainDelayDur (PyObject *self, PyObject *args) {
	float delay;
	float duration;
	unsigned int nPulses;
	int pin;
	int accLevel;
	if (!PyArg_ParseTuple(args,"ffiii", &delay, &duration, &nPulses, &pin, &accLevel)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for delay, duration, number of pulses, pin, and accuracy level.");
		return NULL;
	}
	SimpleGPIO_thread *  threadObj= SimpleGPIO_thread::SimpleGPIO_threadMaker (pin, 1, (unsigned int) round (1e06 *delay), (unsigned int) round (1e06 *duration), (unsigned int) nPulses, accLevel);
	if (threadObj == nullptr){
		PyErr_SetString (PyExc_RuntimeError, "SimpleGPIO_threadMaker was not able to make a GPIO thread object");
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(threadObj), "pulsedThread", pulsedThread_del);
	}
  }
  
  static PyObject* ptSimpleGPIO_trainFreqDuty (PyObject *self, PyObject *args) {
	float trainFrequency;
	float trainDuration;
	float trainDutyCycle;
	int pin;
	int accLevel;
	if (!PyArg_ParseTuple(args,"fffii", &trainFrequency, &trainDutyCycle, &trainDuration, &pin, &accLevel)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for frequency, duty cycle, train duration, pin, and accuracy level.");
		return NULL;
	}
	SimpleGPIO_thread *  threadObj= SimpleGPIO_thread::SimpleGPIO_threadMaker (pin, 1, (float) trainFrequency, (float) trainDutyCycle, (float) trainDuration, accLevel); 
	if (threadObj == nullptr){
		PyErr_SetString (PyExc_RuntimeError, "SimpleGPIO_threadMaker was not able to make a GPIO thread object");
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(threadObj), "pulsedThread", pulsedThread_del);
	}
  }
  
 static PyObject* ptSimpleGPIO_setPin (PyObject *self, PyObject *args){
	  PyObject *PyPtr;
	  int newPin;
	  int isLocking;
	  if (!PyArg_ParseTuple(args,"Oii", &PyPtr, &newPin, &isLocking)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object, newPin, and isLocking");
		return NULL;
	}
	SimpleGPIO_thread * threadPtr = static_cast<SimpleGPIO_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	int returnVal =threadPtr->setPin (newPin, isLocking);
	return  Py_BuildValue("i", returnVal);
}

 static PyObject* ptSimpleGPIO_setLevel (PyObject *self, PyObject *args){
	  PyObject *PyPtr;
	  int theLevel;
	  int isLocking;
	  if (!PyArg_ParseTuple(args,"Oii", &PyPtr, &theLevel, &isLocking)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object, level, and isLocking");
		return NULL;
	}
	SimpleGPIO_thread * threadPtr = static_cast<SimpleGPIO_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	int returnVal =threadPtr->setLevel (theLevel, isLocking);
	return  Py_BuildValue("i", returnVal);
}

#define includeThis 0
#if includeThis

/*Sets a Python Object as the endFunction data for a SimpleGPIO thread, and sets the endFunction that
call that Python object's endFunc method, which it better have..... */
static int ptSimpleGPIO_setEndFuncObjCallBack (void * modData, taskParams * theTask){
	SimpleGPIOStructPtr endFuncData = (SimpleGPIOStructPtr)theTask->endFuncData;
	taskData->endFuncData = modData;
	return 0;
}

static void ptSimpleGPIO_RunPythonEndFunc (taskParams * theTask){
	PyObject *PyObjPtr = (PyObject *) theTask->endFuncData;
	PyGILState_STATE state=PyGILState_Ensure();
	PyObject *result = PyObject_CallMethod (PyObjPtr, "endFunc", "(fffiii)",theTask->trainFrequency, theTask->trainDutyCycle, theTask->trainDuration, theTask->pulseDelayUsecs, theTask->pulseDurUsecs, theTask->nPulses);
	Py_DECREF (result);
	//PyRun_SimpleString ("print ('The end function ran with no error')");
	PyGILState_Release(state);
}


static PyObject* ptSimpleGPIO_setEndFuncObj (PyObject *self, PyObject *args){
	PyObject *PyPtr;
	PyObject *PyObjPtr;
	if (!PyArg_ParseTuple(args,"OO", &PyPtr, &PyObjPtr)) {
		PyRun_SimpleString ("print ('SetEndFuncObj error')");
		return NULL;
	}

	if (!PyEval_ThreadsInitialized()){
		PyEval_InitThreads();
	}
	SimpleGPIO_thread * threadPtr = static_cast<SimpleGPIO_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	threadPtr->modCustom (&ptSimpleGPIO_setEndFuncObjCallBack, (void *) PyObjPtr, 1);
	threadPtr->setEndFunc (&ptSimpleGPIO_RunPythonEndFunc);
	Py_RETURN_NONE;
}


static PyObject* ptSimpleGPIO_unSetEndFunc (PyObject *self, PyObject *PyPtr) {
	SimpleGPIOStructPtr * threadPtr = static_cast<SimpleGPIOStructPtr * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	SimpleGPIOStructPtr->unSetEndFunc();
	Py_RETURN_NONE;
}

static PyObject* ptSimpleGPIO_hasEndFunc (PyObject *self, PyObject *PyPtr) {
	SimpleGPIOStructPtr * threadPtr = static_cast<pulsedThread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	return  Py_BuildValue("i", threadPtr->hasEndFunc());
}


/*Function for passing an array to a train or pulse, and selecting one of the C++ endFunc that sets dutyCycle or Frequency from the array */
static PyObject* ptSimpleGPIO_setArrayFunc (PyObject *self, PyObject *args) {
	PyObject * PyPtr;	// pulsed thread object, either a train or a pulse
	PyObject * bufferObj; // the floating point array of dutyCycles or Frequencies
	int endFuncType;  // 0 for frequency, 1 for dutyCycle
	int isLocking; 
	
	if (!PyArg_ParseTuple(args,"OOii", &PyPtr, &bufferObj, &endFuncType, &isLocking)) {
		return NULL;
	}
	if (PyObject_CheckBuffer (bufferObj) == 0){
		PyRun_SimpleString ("print ('Error getting bufferObj from Python array')");
		return NULL;
	}
	Py_buffer buffer;
	if (PyObject_GetBuffer (bufferObj, &buffer, PyBUF_FORMAT)==-1){
		PyRun_SimpleString ("print ('Error getting C array from bufferObj from Python array')");
		return NULL;
	}
	//printf ("Buffer type is %s, length is %d bytes, and item size is %d.\n", buffer.format, buffer.len, buffer.itemsize);
	if (strcmp (buffer.format, "f") != 0){
		PyRun_SimpleString ("print ('Error for bufferObj: data type of Python array is not float')");
		return NULL;
	}
	
	float* arrayStart = static_cast <float *>(buffer.buf); // Now we have a pointer to the array from the passed in buffer
	pulsedThread * threadPtr = static_cast<pulsedThread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread")); // get pointer to pulsedThread
	int errVar = SimpleGPIO_setUpArray (threadPtr, arrayStart, (unsigned int) buffer.len/buffer.itemsize, isLocking); // adds a pointer to the array to the endFunc data
	if (errVar){
		PyRun_SimpleString ("print (Failed to set up the array for the endFunction.')");
	}
	// install the endFunction that updates the duty cycle or frequency after every train of pulses
	if (endFuncType ==0){
		threadPtr->setEndFunc (&SimpleGPIO_DutyCycleFromArrayEndFunc);
	}else{
		threadPtr->setEndFunc (&SimpleGPIO_FreqFromArrayEndFunc);
	}
	// install custom delete function
	threadPtr->setCustomDataDelFunc (&SimpleGPIO_ArrayStructCustomDel);
	PyBuffer_Release (&buffer); // we don't need the buffer object as we have a pointer to the array start, which is all we care about
	Py_RETURN_NONE;
}


/* Sets level hi or lo manually */
static PyObject* ptSimpleGPIO_setLevel (PyObject *self, PyObject *args){
	PyObject *PyPtr;
	int level; // 0 for low, 1 for hi
	int isLocking;
	if (!PyArg_ParseTuple(args,"Oii", &PyPtr, &level, &isLocking)) {
		return NULL;
	}
	pulsedThread * threadPtr = static_cast<pulsedThread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	int * newLevelPtr = new int;
	* newLevelPtr = level;
	int returnVal =threadPtr -> modCustom (&SimpleGPIO_setLevelCallBack, (void *) newLevelPtr, isLocking);
	return  Py_BuildValue("i", returnVal);
}


/* code for a simple output without a thread "All Pulse, No thread" so we can set pin levels
high and low from Python,with same code, avoiding possible conflicts with RPi.GPIO */
static void  AllPulseNoThread_del(PyObject * PyPtr){
	 delete static_cast<SimpleGPIOStructPtr *> (PyCapsule_GetPointer (PyPtr, "AllPulseNoThread"));
}


static PyObject* simpleGPIO_output (PyObject *self, PyObject *args) {
	int pin;
	int level; // initial level, high or low
	if (!PyArg_ParseTuple(args,"ii", &pin, &level)) {
		return NULL;
	}
	// all we need is a SimpleGPIOStruct, no thread, so we will init the struct here
	SimpleGPIOStructPtr structPtr = new SimpleGPIOStruct;
	structPtr -> pin =pin;
	//structPtr -> GPIOaddr = GPIOperi->addr;
	structPtr->pinBit =  1 << structPtr->pin;
	// initialize pin for output and set it low or high
	*(GPIOperi->addr+ ((structPtr->pin) /10)) &= ~(7<<(((structPtr->pin) %10)*3));
	*(GPIOperi->addr  + ((structPtr->pin)/10)) |=  (1<<(((structPtr->pin)%10)*3));
	structPtr->GPIOperiHi = (unsigned int *) GPIOperi->addr + 7;
	structPtr->GPIOperiLo = (unsigned int *) GPIOperi->addr + 10;
	if (level >= 1){
		*(GPIOperi->addr+ 7) =structPtr->pinBit;
	}else{
		*(GPIOperi->addr + 10) =structPtr->pinBit;
	}
	return PyCapsule_New (static_cast <void *>(structPtr), "AllPulseNoThread", AllPulseNoThread_del);
}


  static PyObject* simpleGPIO_setOutput (PyObject *self, PyObject *args){
	  PyObject *PyPtr;
	  int level;
	  if (!PyArg_ParseTuple(args,"Oi", &PyPtr, &level)) {
		return NULL;
	}
	SimpleGPIOStructPtr structPtr = static_cast<SimpleGPIOStructPtr > (PyCapsule_GetPointer(PyPtr, "AllPulseNoThread"));
	if (level >= 1){ // set high
		*(structPtr->GPIOperiHi) =structPtr->pinBit;
	}else{
		*(structPtr->GPIOperiLo)=structPtr->pinBit;
	}
	Py_RETURN_NONE;
}
#endif
	
/* Module method table - those starting with ptSimpleGPIO are defined here, those starting with pulsedThread are defined in pyPulsedThread.h*/
static PyMethodDef ptSimpleGPIOMethods[] = {
	{"isBusy", pulsedThread_isBusy, METH_O, "returns number of tasks a thread has left to do, 0 means finished all tasks"},
	{"waitOnBusy", pulsedThread_waitOnBusy, METH_VARARGS, "Returns when a thread is no longer busy, or after timeOut secs"},
	{"doTask", pulsedThread_doTask, METH_O, "Tells the pulsedThread object to do whatever task it was configured for"},
	{"doTasks", pulsedThread_doTasks, METH_VARARGS, "Tells the pulsedThread object to do whatever task it was configured for multiple times"},
	{"unDoTasks", pulsedThread_unDoTasks, METH_O, "Tells the pulsedThread object to stop doing however many task it was asked to do"},
	{"startTrain", pulsedThread_startTrain, METH_O, "Tells a pulsedThread object configured as an infinite train to start"},
	{"stopTrain", pulsedThread_stopTrain, METH_O, "Tells a pulsedThread object configured as an infinite train to stop"},
	{"modDelay", pulsedThread_modDelay, METH_VARARGS, "changes the delay period of a pulse or LOW period of a train"},
	{"modDur", pulsedThread_modDur, METH_VARARGS, "changes the delay period of a pulse or HIGH period of a train"},
	{"modTrainLength", pulsedThread_modTrainLength, METH_VARARGS, "changes the number of pulses of a train"},
	{"modTrainDur", pulsedThread_modTrainDur, METH_VARARGS, "changes the total duration of a train"},
	{"modTrainFreq", pulsedThread_modFreq, METH_VARARGS, "changes the frequency of a train"},
	{"modTrainDuty", pulsedThread_modDutyCycle, METH_VARARGS, "changes the duty cycle of a train"},
	{"getPulseDelay", pulsedThread_getPulseDelay, METH_O, "returns pulse delay, in seconds"},
	{"getPulseDuration", pulsedThread_getPulseDuration, METH_O, "returns pulse duration, in seconds"},
	{"getPulseNumber", pulsedThread_getPulseNumber, METH_O, "returns number of pulses in a train, 1 for a single pulse, or 0 for an infinite train"},
	{"getTrainDuration", pulsedThread_getTrainDuration, METH_O, "returns duration of a train, in seconds"},
	{"getTrainFrequency", pulsedThread_getTrainFrequency, METH_O, "returns frequency of a train, in Hz"},
	{"getTrainDutyCycle", pulsedThread_getTrainDutyCycle, METH_O, "returns duty of a train, between 0 and 1"},
	{"setEndFuncObj",pulsedThread_SetPythonEndFuncObj,METH_VARARGS,"sets a Python object to provide the EndFunction for a pulsedThread"},
	{"setTaskObject", pulsedThread_SetPythonTaskObj, METH_VARARGS, "sets a Python Object to provide HI and LO functtions for a pulsedThread"},
	
	{"pulse", ptSimpleGPIO_pulse, METH_VARARGS, "Creates and configures new  pulsedThread object to do a pulse"},
	{"trainDelayDur", ptSimpleGPIO_trainDelayDur, METH_VARARGS, "Creates and configures new pulsedThread object to do a train of pulses with requested pulse timing"},
	{"trainFreqDuty", ptSimpleGPIO_trainFreqDuty, METH_VARARGS, "Creates and configures new  pulsedThread object to do a train of pulses with requested train characteristics"},
	{"setLevel", ptSimpleGPIO_setLevel, METH_VARARGS, "Sets the output level of a ptSimpleGPIO task to lo or hi when thread is not busy"},
	{"setPin", ptSimpleGPIO_setPin, METH_VARARGS, "Sets the GPIO pin used by the SimpleGPIO task"},
#if includeThis
	{"output", simpleGPIO_output, METH_VARARGS, "Creates a simple output on a single pin, with no associated thread"},
	{"setOutput", simpleGPIO_setOutput, METH_VARARGS, "sets level of a simple output with no associated thread"},
	{"setEndFunc", ptSimpleGPIO_setEndFuncObj, METH_VARARGS, "sets an object whose endFunc will be run at end of each train or pulse"},
	{"unSetEndFunc", ptSimpleGPIO_unSetEndFunc,  METH_O, "turns off the endFunc from running at the end of each train or pulse"},
	{"hasEndFunc", ptSimpleGPIO_hasEndFunc, METH_O, "tells if you have an endFunc installed"},
	{"setArrayEndFunc", ptSimpleGPIO_setArrayFunc, METH_VARARGS, "sets a C++ endFunction to set dutyCycle or Frequency from a passed-in array"}, 
#endif 
	{ NULL, NULL, 0, NULL}
  };
  
  /* Module structure */
  static struct PyModuleDef ptSimpleGPIOmodule = {
    PyModuleDef_HEAD_INIT,
    "ptSimpleGPIO",           /* name of module */
    "An external module for precise timing of Raspberry Pi GPIO pulses and trains with pulsedThreads",  /* Doc string (may be NULL) */
    -1,                 /* Size of per-interpreter state or -1 */
    ptSimpleGPIOMethods       /* Method table */
  };

  /* Module initialization function */
  PyMODINIT_FUNC
  PyInit_ptSimpleGPIO(void) {
    return PyModule_Create(&ptSimpleGPIOmodule);
  }  
