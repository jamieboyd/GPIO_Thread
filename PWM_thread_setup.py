# setup.py
from distutils.core import setup, Extension

setup(name='ptPWM',
      author = 'Jamie Boyd',
      author_email = 'jadobo@gmail.com',
      py_modules=['PTPWM'],
      ext_modules=[
        Extension('ptPWM',
                  ['GPIOlowlevel.cpp', 'PWM_thread.cpp', 'PWM_thread_Py.cpp'],
                  include_dirs = ['./','/usr/include'],
                  library_dirs = ['./','/usr/local/lib'],
		  extra_compile_args=["-O3", "-std=gnu++11"],
                  libraries = ['pulsedThread'],
                  )
        ]
)
