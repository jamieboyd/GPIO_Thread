from array import array
from PTPWM import PTPWM
from PTSimpleGPIO import PTSimpleGPIO
a1 = array('i', (0 for i in range (0, 1000)))
for i in range(1, 301):
    a1 [i] = 250

w1 = PTPWM(PTSimpleGPIO.MODE_PULSES, 1000, 1000, 0, (1/1000), 1000, 2)
w1.add_channel(1,0,PTPWM.PWM_MARK_SPACE,0,0,a1)
w1.set_PWM_enable (1,1,0)
w1.start_train()
