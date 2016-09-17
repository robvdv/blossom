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

ser = serial.Serial('/dev/ttyUSB0', 115200)   #9600, 14400, 19200, 28800, 38400, 57600, or 115200.

# reset the arduino
ser.setDTR(level=False)
time.sleep(1)
# ensure there is no stale data in the buffer
ser.flushInput()
ser.setDTR()
time.sleep(1)

ser.flushInput()
pointsPerSecond = 1000
secondDelay = 1.0 / pointsPerSecond
startTime = datetime.datetime.now()


print "## Sending ###"
print str(pointsPerSecond) + " points per second."
print str(pointsPerSecond * 7) + " bytes per second."
print str(pointsPerSecond * 7 * 8) + " bits per second."
print str(secondDelay) + " ms delay."

def sendFrame(pointsData):
    frames = bytearray()

    for row in pointsData:
        for cell in row:
            frames.append(cell)

    ser.write(frames)

def getMessages():
    inp = ser.read(ser.inWaiting())
    if inp:
        print("got message: " + inp)
        return inp


