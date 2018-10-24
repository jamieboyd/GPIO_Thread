# SR_stepper_setup.py
from distutils.core import setup, Extension

setup(name='ptSR_stepper',
	author = 'Jamie Boyd',
	author_email = 'jadobo@gmail.com',
	py_modules=['PTSR_stepper'],
	ext_modules=[
	Extension('ptSR_stepper',
	['SR_stepper_thread.cpp', 'GPIOlowlevel.cpp', 'SR_stepper_thread_Py.cpp'],
		include_dirs = ['./', '/usr/include'],
		library_dirs = ['./', '/usr/local/lib'],
		extra_compile_args=["-O3", "-std=gnu++11"],
		libraries = ['pulsedThread'],
		)
	]
)