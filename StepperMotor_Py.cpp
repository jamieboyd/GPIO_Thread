#include "Python.h"
#include "StepperMotor_thread.h"

/***********************************************************************************************************************
-------------Full Step and HalfStep biphasic Stepper motors WITHOUT using wiringPi library for Raspberry Pi --------------------------------------
************************************************************************************************************************/

static PyObject* ptStepperMotor_cleanUp (PyObject *self, PyObject *noargs){
	delete GPIOperi;
	GPIOperi = nullptr;
	Py_RETURN_NONE;
}

/*Function called automatically when PyCapsule object is deleted in Python */
static void  ptStepperMotor_del(PyObject * PyPtr){
    delete static_cast<ptStepperMotor *> (PyCapsule_GetPointer (PyPtr, "ptStepperMotor"));
}

/*Creates and configures a ptStepperMotor object to run a stepper motor, either Full step or half step modes

 accLevel: accuracy level for timing, 0 relies solely on sleeping for timing, which may not be accurate beyond the ms scale
	1 sleeps for first part of a ticks, but wakes early and does stricter timing with clocks at the end of the tick,
	checking each time against expected time. more processor intensive, more accurate.
*/

static PyObject * ptStepperMotorFull (PyObject *self, PyObject *args){
	int pinA;
	int pinB;
	float scaling;
	float speed;
	int accLevel;
	if (!PyArg_ParseTuple(args,"iiffi", &pinA, &pinB, &scaling, &speed, &accLevel)) {
		return NULL;
	}
	int errCode;
	if (GPIOperi == nullptr){
		GPIOperi = new bcm_peripheral {GPIO_BASE};
		errCode = map_peripheral(GPIOperi, IFACE_DEV_GPIOMEM);
		if (errCode){
			return NULL;
		}
	}
	ptStepperMotor* stepper = new ptStepperMotor(pinA, pinB, 0,0, kSTEPPERMOTORFULL, scaling, speed, accLevel);
	if (stepper == nullptr){
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(stepper), "ptStepperMotor", ptStepperMotor_del);
	}
}

static PyObject * ptStepperMotorHalf (PyObject *self, PyObject *args){
	int pinA;
	int pinB;
	int pinAbar;
	int pinBbar;
	float scaling;
	float speed;
	int accLevel;
	if (!PyArg_ParseTuple(args,"iiiiffi", &pinA, &pinB, &pinAbar, &pinBbar, &scaling, &speed, &accLevel)) {
		return NULL;
	}
	//ptStepperMotorHalf * stepper = new ptStepperMotorHalf (pinA, pinB, scaling, speed, accLevel);
	ptStepperMotor * stepper  = new ptStepperMotor(pinA, pinB, pinAbar, pinBbar, kSTEPPERMOTORHALF, scaling, speed, accLevel);
	if (stepper == nullptr){
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(stepper), "ptStepperMotor", ptStepperMotor_del);
	}
}

static PyObject* ptStepperMotor_setScaling (PyObject *self, PyObject *args){
	PyObject *PyPtr;
	float newScaling;
  	if (!PyArg_ParseTuple(args,"Of", &PyPtr, &newScaling)) {
		return NULL;
	}
	ptStepperMotor * stepper = static_cast<ptStepperMotor * > (PyCapsule_GetPointer(PyPtr, "ptStepperMotor"));
	stepper->setScaling (newScaling);
	Py_RETURN_NONE;
}

static PyObject* ptStepperMotor_setSpeed (PyObject *self, PyObject *args){
	PyObject *PyPtr;
	float newSpeed;
  	if (!PyArg_ParseTuple(args,"Of", &PyPtr, &newSpeed)) {
		return NULL;
	}
	ptStepperMotor * stepper = static_cast<ptStepperMotor * > (PyCapsule_GetPointer(PyPtr, "ptStepperMotor"));
	stepper->setSpeed (newSpeed);
	Py_RETURN_NONE;
}


static PyObject* ptStepperMotor_setZero (PyObject *self, PyObject *PyPtr) {
    ptStepperMotor * stepper = static_cast<ptStepperMotor * > (PyCapsule_GetPointer(PyPtr, "ptStepperMotor"));
    stepper->setZero();
    Py_RETURN_NONE;
}

static PyObject* ptStepperMotor_moveRel (PyObject *self, PyObject *args){
	PyObject *PyPtr;
	float distance;
  	if (!PyArg_ParseTuple(args,"Of", &PyPtr, &distance)) {
		return NULL;
	}
	ptStepperMotor * stepper = static_cast<ptStepperMotor * > (PyCapsule_GetPointer(PyPtr, "ptStepperMotor"));
	stepper->moveRel (distance);
	Py_RETURN_NONE;
}

static PyObject* ptStepperMotor_moveAbs (PyObject *self, PyObject *args){
	PyObject *PyPtr;
	float position;
  	if (!PyArg_ParseTuple(args,"Of", &PyPtr, &position)) {
		return NULL;
	}
	ptStepperMotor * stepper = static_cast<ptStepperMotor * > (PyCapsule_GetPointer(PyPtr, "ptStepperMotor"));
	stepper->moveAbs (position);
	Py_RETURN_NONE;
}

static PyObject* ptStepperMotor_getScaling (PyObject *self, PyObject *PyPtr) {
    ptStepperMotor * stepper = static_cast<ptStepperMotor * > (PyCapsule_GetPointer(PyPtr, "ptStepperMotor"));
    return Py_BuildValue("f", stepper->getScaling());
}

static PyObject* ptStepperMotor_getSpeed (PyObject *self, PyObject *PyPtr) {
    ptStepperMotor * stepper = static_cast<ptStepperMotor * > (PyCapsule_GetPointer(PyPtr, "ptStepperMotor"));
    return Py_BuildValue("f", stepper->getSpeed());
}

static PyObject* ptStepperMotor_getPosition (PyObject *self, PyObject *PyPtr) {
    ptStepperMotor * stepper = static_cast<ptStepperMotor * > (PyCapsule_GetPointer(PyPtr, "ptStepperMotor"));
    return Py_BuildValue("f", stepper->getPosition());
}

static PyObject* ptStepperMotor_isBusy (PyObject *self, PyObject *PyPtr) {
    ptStepperMotor * stepper = static_cast<ptStepperMotor * > (PyCapsule_GetPointer(PyPtr, "ptStepperMotor"));
    return Py_BuildValue("i", stepper->isBusy());
}

static PyObject* ptStepperMotor_waitOnBusy (PyObject *self, PyObject *args){
	PyObject *PyPtr;
	float timeOut;
  	if (!PyArg_ParseTuple(args,"Of", &PyPtr, &timeOut)) {
		return NULL;
	}
	ptStepperMotor * stepper = static_cast<ptStepperMotor * > (PyCapsule_GetPointer(PyPtr, "ptStepperMotor"));
	return Py_BuildValue ("i", stepper->waitOnBusy (timeOut));
}

/*This function only works for half-step stepper motors - don't call it for a full-step stepper motor*/
static PyObject* ptStepperMotor_setHold (PyObject *self, PyObject *args){
	PyObject *PyPtr;
	int hold;
  	if (!PyArg_ParseTuple(args,"Oi", &PyPtr, &hold)) {
		return NULL;
	}
	ptStepperMotor * stepper = static_cast<ptStepperMotor* > (PyCapsule_GetPointer(PyPtr, "ptStepperMotor"));
	stepper->setHold (hold);
	Py_RETURN_NONE;
}

/* Module method table */
static PyMethodDef ptStepperMotorMethods[] = {
	{"stepperMotorFull", ptStepperMotorFull, METH_VARARGS, "Creates and configures new ptStepperMotor in full-step mode"},
	{"stepperMotorHalf", ptStepperMotorHalf, METH_VARARGS, "Creates and configures new ptStepperMotor for half-step mode"},
	{"setScaling", ptStepperMotor_setScaling, METH_VARARGS, "Sets steper scaling in steps/unit (calibrated for whatever your unit is in, e.g., metres, degrees"},
	{"setSpeed", ptStepperMotor_setSpeed, METH_VARARGS, "Sets stepper speed in units/sec (calibrated for whatever your unit is in, e.g., metres, degrees"},
	{"setZero", ptStepperMotor_setZero, METH_O, "Sets stepper's current position to be new zero point"},
	{"moveRel", ptStepperMotor_moveRel,METH_VARARGS, "Moves the stepper motor the given distance in calibrated units."},
	{"moveAbs", ptStepperMotor_moveAbs, METH_VARARGS, "Moves the stepper motor to the given absolute position, based on the current zero position."},
	{"getScaling", ptStepperMotor_getScaling, METH_O, "Gets stepper's current speed setting, in steps/unit, for whatever units are being used"},
	{"getSpeed", ptStepperMotor_getSpeed, METH_O, "Gets stepper's current speed setting, in calibrated units/sec."},
	{"getPosition", ptStepperMotor_getPosition, METH_O, "Gets stepper's current position, based on last zeroed position"},
	{"isBusy", ptStepperMotor_isBusy, METH_O, "returns 1 if stepper motor is currently moving, else 0"},
	{"waitOnBusy", ptStepperMotor_waitOnBusy, METH_VARARGS, "Returns when a stepper motor is no longer moving, or after timeOut secs"},
	{"setHold", ptStepperMotor_setHold, METH_VARARGS, "For a half-step stepper motor, 1 holds the stepper motor steady, 0 makes the motor free to move"},
	{ NULL, NULL, 0, NULL}
  };
  
  /* Module structure */
  static struct PyModuleDef ptStepperMotormodule = {
    PyModuleDef_HEAD_INIT,
    "ptStepperMotor",           /* name of module */
    "An external module using pulsedThreads and wiringPi to control full-step or half-step stepper motors",  /* Doc string (may be NULL) */
    -1,                 /* Size of per-interpreter state or -1 */
    ptStepperMotorMethods       /* Method table */
  };

  /* Module initialization function */
  PyMODINIT_FUNC
  PyInit_ptStepperMotor(void) {
    return PyModule_Create(&ptStepperMotormodule);
  }  