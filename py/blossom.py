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

from flask import Flask, render_template, session, request
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


print('############# Starting #######')

def background_thread():
    """Example of how to send server generated events to clients."""
    count = 0
    while True:
        socketio.sleep(10)
        count += 1
        socketio.emit('my response',
                      {'data': 'Server generated event', 'count': count},
                      namespace='/test')


@app.route('/')
def index():
    return render_template('index.html', async_mode=socketio.async_mode)

@socketio.on('circle size', namespace='/test')
def test_message(message):
    print('######>>>>>>>>>>##############')
    session['receive_count'] = session.get('receive_count', 0) + 1
    emit('circle size',
         {'data': message['data'], 'count': session['receive_count']})

@socketio.on('points', namespace='/test')
def test_message(message):
    data = message['data']
    print('data.length: ' + str(len(data)))
    #print('data' + str(data))

    session['receive_count'] = session.get('receive_count', 0) + 1
    emit('points received',
         {'data': message['data'], 'count': session['receive_count']})



@socketio.on('my ping', namespace='/test')
def ping_pong():
    emit('my pong')

if __name__ == '__main__':
    socketio.run(app, debug=True)
