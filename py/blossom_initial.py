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
import comms

from flask import Flask, render_template, session, request, send_from_directory
from flask_socketio import SocketIO, emit, join_room, leave_room, \
    close_room, rooms, disconnect

# Set this variable to "threading", "eventlet" or "gevent" to test the
# different async modes, or leave it set to None for the application to choose
# the best option based on installed packages.
async_mode = None

pointsData = 0

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
socketio = SocketIO(app, async_mode=async_mode)
thread = None
lastMessageSentTime = datetime.datetime.now()

print('Starting Blossom...')

def background_thread():
    while True:
        time.sleep(1)
        messages = comms.getMessages()
        print('looking for message')
        if messages:
            print('Got message')
            emit('message')

@app.route('/')
def index():
    return render_template('index.html', async_mode=socketio.async_mode)

@app.route('/js/<path:path>')
def send_js(path):
    return send_from_directory('js', path)

@app.route('/css/<path:path>')
def send_css(path):
    return send_from_directory('css', path)

@socketio.on('points', namespace='/test')
def socket_points(message):
    global pointsData
    global lastMessageSentTime
    pointsData = message['data']
    timeDiff = datetime.datetime.now() - lastMessageSentTime
    if timeDiff.microseconds >= 20000:
        print('data.length: ' + str(len(pointsData)))
        lastMessageSentTime = datetime.datetime.now()
        comms.sendFrame(pointsData)
        comms.getMessages()

@socketio.on('getMessages', namespace='/test')
def socket_getMessages():
    messages = comms.getMessages()
    emit('message', messages)


@socketio.on('my ping', namespace='/test')
def ping_pong():
    emit('my pong')

if __name__ == '__main__':
    socketio.run(app, debug=True)
