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

ser = serial.Serial('/dev/ttyUSB0', 115200)   #9600, 14400, 19200, 28800, 38400, 57600, or 115200. 230400

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
lastSentId = 0

def sendData(data):
    global readyToSend
    global lastSentCommand
    global lastSentLength
    global lastSentId
    message = False
    now = time.time()

    lastSentCommand = data[0]
    lastSentLength = data[1]
    lastSentId = data[2]

    # check to see if there's data to receive first
    if readyToSend == False:
        inp = ser.read(ser.inWaiting())
        if inp:
            print(str(now) + " got data: " + inp)
            ack = inp.strip().split(",")
            receivedCommand = int(ack[0])
            receivedLength = int(ack[1])
            receivedId = int(ack[2])
            if len(ack) >= 4:
                message = ack[3]
                #print("got message: " + message)

            #print("receivedId " + str(receivedId))
            #print("lastSentId " + str(lastSentId))
            if lastSentId != (receivedId + 1):
                message = "dropped " + str(lastSentId - receivedId) + " frame(s)"

            if (receivedCommand == lastSentCommand) and (receivedLength == lastSentLength):
                readyToSend = True
            else:
                print("bad comms ")
                print(receivedCommand)
                print(receivedLength)
                print(lastSentCommand)
                print(lastSentLength)
                readyToSend = True

    # are we ready to send the data?
    if readyToSend == True:
        sendData = bytearray()

        for val in data:
            sendData.append(val)

        ser.write(sendData)
        # set ready to false because we need to wait for the ack
        readyToSend = False

    return message

