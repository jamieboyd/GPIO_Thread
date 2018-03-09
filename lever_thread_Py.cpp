/* *** Makes a new lever thread object. 
*/
static PyObject* ptLever_New (PyObject *self, PyObject *args) {
	
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