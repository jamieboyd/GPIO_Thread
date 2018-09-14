# setup.py
from distutils.core import setup, Extension

setup(name='ptPWM_sin',
      author = 'Jamie Boyd',
      author_email = 'jadobo@gmail.com',
      py_modules=['PTPWMsin'],
      ext_modules=[
        Extension('ptPWMsin',
                  ['GPIOlowlevel.cpp', 'PWM_thread.cpp', 'PWM_sin_thread.cpp', 'PWM_sin_Py.cpp'],
                  include_dirs = ['./','/usr/include'],
                  library_dirs = ['./','/usr/local/lib'],
		  extra_compile_args=["-O3", "-std=gnu++11"],
                  libraries = ['pulsedThread'],
                  )
        ]
)
