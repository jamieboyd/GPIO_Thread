import leverThread
from array import array as array
posArray = array ('B', [0] * 200)
mylever = leverThread.new(posArray, 25, 23,0)
leverThread.startTrial(mylever)
leverThread.checkTrial (mylever)
