/**
* Reads commands coming from serial interface to drive an LED on/off
* Also prints led status back
*/

#define LEDPIN 13
#define CONTROL_MESSAGE_SIZE 2

byte command = 0;
byte dataLength = 0;
byte data[126]; // receive data buffer
byte dataPosition = 0;

void setup() {
  Serial.begin(9600); 
  while(Serial.available())
  Serial.read();
  Serial.begin(115200); //9600, 14400, 19200, 28800, 38400, 57600, or 115200.
  while(Serial.available())
  Serial.read();
  digitalWrite(LEDPIN, LOW);
}

void loop() {
  // if we have enough bytes to receive a control message and there's no command yet set
  if (Serial.available() >= CONTROL_MESSAGE_SIZE) {
    command = Serial.read(); 
    dataLength = Serial.read(); 
    dataPosition = 0; // reset the data read position
  }

  if (command != 0) {
    if (command == 1) {
      digitalWrite(LEDPIN, HIGH);
    } else if (command == 2) {
      digitalWrite(LEDPIN, LOW);
    } else {
      if ((Serial.available() >= dataLength)) {
        while (dataPosition < dataLength) {
          data[dataPosition] = Serial.read(); 
          dataPosition++;
        }
      }
    }
    
    Serial.print(command); // print the ack
    Serial.print(dataLength);
    command = 0;
  }
  
}
