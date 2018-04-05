# setup.py
from distutils.core import setup, Extension

setup(name='ptCountermandPulse',
      author = 'Jamie Boyd',
      author_email = 'jadobo@gmail.com',
      py_modules=['PTCountermandPulse'],
      ext_modules=[
        Extension('ptCountermandPulse',
                  ['GPIOlowlevel.cpp', 'SimpleGPIO_thread.cpp', 'CountermandPulse.cpp', 'CountermandPulse_Py.cpp'],
                  include_dirs = ['./','/usr/include'],
                  library_dirs = ['./','/usr/local/lib'],
		  extra_compile_args=["-O3", "-std=gnu++11"],
                  libraries = ['pulsedThread'],
                  )
        ]
)
