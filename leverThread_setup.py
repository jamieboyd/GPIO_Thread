# lever_thread_setup.py
from distutils.core import setup, Extension

setup(name='leverThread',
	author = 'Jamie Boyd',
	author_email = 'jadobo@gmail.com',
	py_modules=['PTLeverThread'],
	ext_modules=[
	Extension('ptLeverThread',
	['leverThread.cpp', 'GPIOlowlevel.cpp', 'SimpleGPIO_thread.cpp', 'PWM_thread.cpp', 'leverThread_Py.cpp'],
		include_dirs = ['./', '/usr/include'],
		library_dirs = ['./', '/usr/local/lib'],
		extra_compile_args=["-O3", "-std=gnu++11"],
		libraries = ['pulsedThread', 'wiringPi'],
		)
	]
)
