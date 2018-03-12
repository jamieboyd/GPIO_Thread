# setup.py
from distutils.core import setup, Extension

setup(name='HX711',
	author = 'Jamie Boyd',
	author_email = 'jadobo@gmail.com',
	py_modules=['Scale'],
	ext_modules=[
	Extension('HX711',
	['HX711.cpp', 'GPIOlowlevel.cpp', 'HX711_Py.cpp'],
		include_dirs = ['./', '/usr/include'],
		library_dirs = ['./', '/usr/local/lib'],
		extra_compile_args=["-O3", "-std=gnu++11"],
		libraries = ['pulsedThread'],
		)
	]
)