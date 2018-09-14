#include <pyPulsedThread.h>
#include "PWM_sin_thread.h"

/* ************ PWM sine wave output from Python using PWM_sin_thread ******************************
Last Modifed:
2018/09/13 by Jamie Boyd - initial version

******************** Function called automatically when PyCapsule object is deleted in Python *****************************************
Last Modified:
2018/09/13 by Jamie Boyd - initial version */
static void  ptPWMsin_del(PyObject * PyPtr){
    delete static_cast<PWM_sin_thread *> (PyCapsule_GetPointer (PyPtr, "pulsedThread"));
}


/* ****************************************  Constructors ******************************************************
Creates and configures a PWM_sin_thread object to output a sin wave from PWM channel 0 GPIO 18 or from GPIO 40
appearing on the on audio Right channel , or from PWM channel 1, appearing on GPIO 19 or GPIO 41 appearing on
the audio Left channel
channel:  0 or 1 for PWM channel, plus 2 if outputting channel to audio instead of GPIO
enable: 1 to start the PWM channel running when thread is initialized, 0 to not initially enable the PWM output
initial freq: the frequency to start with
Last Modified;
2019/09/13 by Jamie Boyd - initial version*/
 static PyObject* ptSimpleGPIO_FreqDuty (PyObject *self, PyObject *args) {
	int channel;
	int enable;
	unsigned int initialFreq;
	if (!PyArg_ParseTuple(args,"iiI", &channel, &enable, &initialFreq)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for channel, enable, and initial frequency.");
		return NULL;
	}
	PWM_sin_thread * threadObj = PWM_sin_thread::PWM_sin_threadMaker (channel, enable, initialFreq);
	if (threadObj == nullptr){
		PyErr_SetString (PyExc_RuntimeError, "PWM_sin_threadMaker was not able to make a PWM_sin object");
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(threadObj), "pulsedThread", ptPWMsin_del);
	}
  }