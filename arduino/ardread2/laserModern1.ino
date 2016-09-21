/**
* Reads commands coming from serial interface to drive an LED on/off
* Also prints led status back
*/

#define LEDPIN 13
#define CONTROL_MESSAGE_SIZE 3

byte command = 0;
byte dataLength = 0;
byte dataId = 0;
byte data[255]; // receive data buffer
byte dataPosition = 0;
long dataCount = 0;

void setup() {
  Serial.begin(9600); 
  while(Serial.available())
  Serial.read();
  Serial.begin(115200); //9600, 14400, 19200, 28800, 38400, 57600, or 115200. 230400
  while(Serial.available())
  Serial.read();
  digitalWrite(LEDPIN, LOW);
}

void sendAck(String message) {
  Serial.print(command); 
  Serial.print(",");
  Serial.print(dataLength);
  Serial.print(",");
  Serial.print(dataId);
  Serial.print(",");
  Serial.print(message);
  command = 0;
}

void loop() {
  // if we have enough bytes to receive a control message and there's no command yet set
  if ((Serial.available() >= CONTROL_MESSAGE_SIZE) && (command == 0)) {
    command = int(Serial.read()); 
    dataLength = int(Serial.read()); 
    dataId = int(Serial.read()); 
    dataPosition = 0; // reset the data read position
    if (command == 1) {
      digitalWrite(LEDPIN, HIGH);
      sendAck("Set LED ON");
    } else if (command == 2) {
      digitalWrite(LEDPIN, LOW);
      sendAck("Set LED OFF");
    } else if (command == 3) {  // get status/poll
      sendAck("tot:" + String(dataCount));
      dataCount = 0;
    }
  }

  // is there something to do?
  if (command != 0) {
    // gobble as much data as possible
    while (Serial.available() && (dataPosition < dataLength)) {
      data[dataPosition] = Serial.read(); 
      dataPosition++;
    }

    // have we read the full length of data? Clear the command
    if (dataPosition == dataLength) {
      dataCount += dataLength;
      sendAck("ok");
    }
  }
  
}
