
from array import array as array
posArray = array ('B', [0] * 300)
import leverThread
mylever = leverThread.new(posArray, 25, 23,0)

leverThread.startTrial(mylever)
leverThread.checkTrial (mylever)
