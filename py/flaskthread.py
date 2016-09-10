import threading
import atexit
import comtest
from flask import Flask

POOL_TIME = 0.0001 #Seconds

# variables that are accessible from anywhere
commonDataStruct = {}
# lock to control access to variable
dataLock = threading.Lock()
# thread handler
commsThread = threading.Thread()
counter = 0

def create_app():
    app = Flask(__name__)

    def interrupt():
        global commsThread
        commsThread.cancel()

    def doStuff():
        global commonDataStruct
        global commsThread
        global counter
        comtest.sendFrame(counter)
        with dataLock:
        # Do your stuff with commonDataStruct Here
            counter = counter + 1
        # Set the next thread to happen
            commsThread = threading.Timer(POOL_TIME, doStuff, ())
        commsThread.start()

    def doStuffStart():
        # Do initialisation stuff here
        global commsThread
        # Create your thread
        commsThread = threading.Timer(POOL_TIME, doStuff, ())
        commsThread.start()

    # Initiate
    doStuffStart()
    # When you kill Flask (SIGTERM), clear the trigger for the next thread
    atexit.register(interrupt)
    return app

app = create_app()