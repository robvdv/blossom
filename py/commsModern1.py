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
lastSentCommand = -1
lastSentLength = 0
lastSentId = 1

def sendData(data):
    global readyToSend
    global lastSentCommand
    global lastSentLength
    global lastSentId
    message = False
    now = time.time()

    # check to see if there's data to receive first
    if readyToSend == False:
        inp = ser.read(ser.inWaiting())
        if inp:

            ack = inp.strip().split(",")
            receivedCommand = int(ack[0])
            receivedLength = int(ack[1])
            receivedId = int(ack[2])

            if receivedCommand == 3:
                print(str(now) + " got data: " + inp)

            if (len(ack) >= 4) and (ack[3] != "ok"):
                message = ack[3]

            if lastSentId != receivedId:
                message = "Error: dropped " + str(lastSentId - receivedId) + " frame(s) lastSentId: " \
                          + str(lastSentId) + " receivedId: " + str(receivedId)
                print(str(now) + " " + message)

            # lastSentCommand = -1 when it's never been set before
            if ((lastSentCommand == -1) or (receivedCommand == lastSentCommand) and (receivedLength == lastSentLength)):
                readyToSend = True
            else:
                logMsg = "Error: bad comms receivedId: "
                logMsg += str(receivedId)
                logMsg += " receivedCommand: "
                logMsg += str(receivedCommand)
                logMsg += " lastSentCommand: "
                logMsg += str(lastSentCommand)

                logMsg += " receivedLength: "
                logMsg += str(receivedLength)
                logMsg += " lastSentLength: "
                logMsg += str(lastSentLength)

                print(logMsg)
                readyToSend = True

            lastSentCommand = data[0]
            lastSentLength = data[1]
            lastSentId = data[2]


    # are we ready to send the data?
    if readyToSend == True:
        sendData = bytearray()

        for val in data:
            sendData.append(val)

        ser.write(sendData)
        # set ready to false because we need to wait for the ack
        readyToSend = False

    return message

