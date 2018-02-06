# setup.py
from distutils.core import setup, Extension

setup(name='ptSimpleGPIO',
      author = 'Jamie Boyd',
      author_email = 'jadobo@gmail.com',
      py_modules=['SimpleGPIO'],
      ext_modules=[
        Extension('ptSimpleGPIO',
                  ['SimpleGPIO_thread.cpp', 'SimpleGPIO_Py.cpp'],
                  include_dirs = ['./','/usr/include'],
                  library_dirs = ['./','/usr/local/lib'],
		  extra_compile_args=["-O3", "-std=gnu++11"],
                  libraries = ['pulsedThread'],
                  )
        ]
)
