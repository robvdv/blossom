import serial  
import struct
import time
import io
import sys
import re
import datetime
import math

ser = serial.Serial('/dev/ttyUSB0', 9600)

# reset the arduino
ser.setDTR(level=False)  
time.sleep(1)  
# ensure there is no stale data in the buffer
ser.flushInput()  
ser.setDTR()  
time.sleep(1)

ser = serial.Serial('/dev/ttyUSB0', 38400)

# reset the arduino
ser.setDTR(level=False)  
time.sleep(1)  
# ensure there is no stale data in the buffer
ser.flushInput()  
ser.setDTR()  
time.sleep(1)

print "Sending data"

ser.flushInput()  
count = 1;
countTotal = 0;
pointsPerSecond = 1000;
secondDelay = 1.0 / pointsPerSecond;
startTime = datetime.datetime.now()
framePointIndex = 0;

print "## Sending ###"
print str(pointsPerSecond) + " points per second."
print str(pointsPerSecond * 7) + " bytes per second."
print str(pointsPerSecond * 7 * 8) + " bits per second."
print str(secondDelay) + " ms delay."

while True:
	x = math.sin(math.radians(framePointIndex * 3))
	y = math.cos(math.radians(framePointIndex * 3))

	smallX = math.sin(math.radians(framePointIndex * 27))
	smallY = math.cos(math.radians(framePointIndex * 27))
	
	x = x * 0.7 + smallX * 0.3
	y = y * 0.7 + smallY * 0.3
	#x = smallX * 0.3
	#y = smallY * 0.3

	x = (x + 1) * 127
	y = (y + 1) * 127

	x = int(x) + 1 # remember 0 is end of point/frame marker
	y = int(y) + 1


	#bar = "#" * (x / 3)
	#print bar
	#bar = "." * (y / 3)
	#print bar
	
	frame = bytearray([0, 68, 69, 6])
	ser.write(frame)

	count = count + 1
	countTotal = countTotal + 1
	if count > 5:
		count = 1

	framePointIndex = framePointIndex + 1;
	if framePointIndex >= 120:
		framePointIndex = 0
		ser.write(bytearray([0, 0, 0, 69]))
		#print("Sent New Frame")

	#time.sleep(0.025)
	inp = ser.read(ser.inWaiting())
	if inp:
		num = re.findall(r'\d+', inp)
		currentTime = datetime.datetime.now()
		timeDiff = currentTime - startTime
		secondsElapsed = timeDiff.seconds
		if ((len(num) > 0) & (secondsElapsed > 0)): 
			pps = int(num[0]) / secondsElapsed
			print(inp + " : " + str(pps) + " pps")
		else: 
			print(inp)
		
	inp = False
	



