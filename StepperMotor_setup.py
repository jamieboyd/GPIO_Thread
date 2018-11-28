# setup.py
from distutils.core import setup, Extension

setup(name='ptStepperMotor',
      author = 'Jamie Boyd',
      author_email = 'jadobo@gmail.com',
      ext_modules=[
        Extension('ptStepperMotor',
                  ['pulsedThread.cpp', 'pulsedThread_StepperMotor.cpp', 'pyPulsedThread_StepperMotor.cpp'],
                  include_dirs = ['./'],
                  library_dirs = ['./','/usr/local/lib'],
		  extra_compile_args=["-O3", "-std=gnu++11"],
                  libraries = ['pthread', './libwiringPi.a'],
		  extra_objects= ['./libwiringPi.a']
                  )
        ]
)
