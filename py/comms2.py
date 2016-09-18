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
time.sleep(0.5)

ser = serial.Serial('/dev/ttyUSB0', 115200)   #9600, 14400, 19200, 28800, 38400, 57600, or 115200.

# reset the arduino
ser.setDTR(level=False)
time.sleep(1)
# ensure there is no stale data in the buffer
ser.flushInput()
ser.setDTR()
time.sleep(0.5)
ser.flushInput()

readyToSend = True
lastSentCommand = 0
lastSentLength = 0

def sendData(data):
    global readyToSend
    global lastSentCommand
    global lastSentLength

    # check to see if there's data to receive first
    if readyToSend == False:
        inp = ser.read(ser.inWaiting())
        if inp:
            print("got data: " + inp)
            if (int(inp[0]) == lastSentCommand) and (int(inp[1]) == lastSentLength):
                readyToSend = True
            else:
                print("bad comms ")
                print(inp[0])
                print(inp[1])
                print(lastSentCommand)
                print(lastSentLength)
                readyToSend = True

    # are we ready to send the data?
    if readyToSend == True:
        sendData = bytearray()
        lastSentCommand = data[0]
        lastSentLength = data[1]
        for val in data:
            sendData.append(val)

        ser.write(sendData)
        # set ready to false because we need to wait for the ack
        readyToSend = False



