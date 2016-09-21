#!/usr/bin/env python
import serial
import struct
import time
import io
import sys
import re
import datetime
import math
import pickle
import serial
import struct
import commsModern1 as comms
import time

from flask import Flask, render_template, session, request, send_from_directory
from flask_socketio import SocketIO, emit, join_room, leave_room, \
    close_room, rooms, disconnect

# Set this variable to "threading", "eventlet" or "gevent" to test the
# different async modes, or leave it set to None for the application to choose
# the best option based on installed packages.
async_mode = None

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
socketio = SocketIO(app, async_mode=async_mode)
thread = None
messageLastSentTime = time.time()
bytesSent = 0
print('Starting ComTest...')

@app.route('/')
def index():
    return render_template('indexModern1.html', async_mode=socketio.async_mode)

@app.route('/js/<path:path>')
def send_js(path):
    return send_from_directory('js', path)

@app.route('/css/<path:path>')
def send_css(path):
    return send_from_directory('css', path)

@socketio.on('sendData', namespace='/test')
def socket_points(message):
    global messageLastSentTime
    global bytesSent
    data = message['data']
    message = comms.sendData(data)
    if message:
        emit('message', message)

        #bytesSent = bytesSent + len(data)

        #timeDiff = time.time() - messageLastSentTime
        #if (timeDiff > 1):
        #    emit('message', "Average throughput: " +str(bytesSent / timeDiff) )
        #    messageLastSentTime = time.time()
        #    bytesSent = 0

@socketio.on('receiveData', namespace='/test')
def socket_getMessages():
    messages = comms.receiveData()
    emit('message', messages)

if __name__ == '__main__':
    socketio.run(app, debug=True)
