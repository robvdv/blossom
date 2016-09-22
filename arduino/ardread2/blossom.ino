/**
* Reads commands coming from serial interface to drive an LED on/off
* Also prints led status back
*/

#define LEDPIN 13
#define CONTROL_MESSAGE_SIZE 4
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

byte command = 0;
byte subCommand = 0;
int dataLength = 0;
byte dataId = 0;
byte point[120][3];
byte pointPosition = 0; // the position in the points array we're reading to
byte pointPositionData = 0; // the position in the points subarray we're reading to
byte pointCount = 0; // the total number of points
byte pointIndex = 0; // the current point we're displaying

byte soundWave[120];
byte soundWaveRepeatCycle = 1;
byte soundWaveRepeatCount = 0; // how many times we've played this byte
byte soundWaveCount = 0;
byte soundWaveIndex = 0; // the current wave sample we're displaying
byte soundWaveByteIndex = 0; // the current wave sample we're displaying

int dataPosition = 0;
long dataCount = 0;

volatile int frameOutputCounter;             // which frame we're playing
volatile byte colours;
void setup() {
  Serial.begin(9600); 
  while(Serial.available())
  Serial.read();
  Serial.begin(115200); //9600, 14400, 19200, 28800, 38400, 57600, or 115200. 230400
  while(Serial.available())
  Serial.read();

  pinMode(LEDPIN, OUTPUT);
  pinMode(7, OUTPUT);      // Sound
  pinMode(8, OUTPUT);      // Blue
  pinMode(9, OUTPUT);      // Green
  pinMode(10, OUTPUT);      // Red
  pinMode(11, OUTPUT);     // pin11= PWM  output / frequency output
  pinMode(3, OUTPUT);     // pin3= PWM  output / frequency output
  
  digitalWrite(LEDPIN, LOW);
 
  Setup_timer2();

  // disable interrupts to avoid timing distortion
  cbi (TIMSK0,TOIE0);              // disable Timer0 !!! delay() is now not available
  sbi (TIMSK2,TOIE2);              // enable Timer2 Interrupt

}

void sendAck(String message) {
  Serial.print(command); 
  Serial.print(",");
  Serial.print(dataLength / 256);  //bitshift!
  Serial.print(",");
  Serial.print(dataLength % 256);  //bitshift!
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
    subCommand = int(Serial.read()); 
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
    } else if (command == 4) {  // get points
      dataLength = (subCommand * 256) + dataLength; // subcommand is used to define the length here
      pointCount = dataLength / 3;
      pointPosition = 0;
      pointPositionData = 0;
    } else if (command == 5) {  // get wave
      soundWaveCount = dataLength;
      soundWaveRepeatCycle = subCommand;
    }
  }

  // is there something to do?
  if (command != 0) {
    // gobble as much data as possible
    if (command == 4) { // points
      while (Serial.available() && (dataPosition < dataLength)) {
        point[pointPosition][pointPositionData] = Serial.read(); 
        pointPositionData++;
        if (pointPositionData == 3) {
          pointPosition++;
          pointPositionData = 0;
        }
        dataPosition++;
      }
    } else if (command == 5) { // wave
      while (Serial.available() && (dataPosition < dataLength)) {
        soundWave[dataPosition] = Serial.read(); 
        dataPosition++;
      }
    }

    // have we read the full length of data? Clear the command
    if (dataPosition == dataLength) {
      dataCount += dataLength;
      sendAck("ok" + String(pointCount));
    }
  }
  
}

//******************************************************************
// timer2 setup
// set prscaler to 1, PWM mode to phase correct PWM,  16000000/510 = 31372.55 Hz clock

void Setup_timer2() {

// Timer2 Clock Prescaler to : 1
  sbi (TCCR2B, CS20);
  cbi (TCCR2B, CS21);
  cbi (TCCR2B, CS22);

  // Timer2 PWM Mode set to Phase Correct PWM
  cbi (TCCR2A, COM2A0);  // clear Compare Match
  sbi (TCCR2A, COM2A1);

  cbi (TCCR2A, COM2B0);  // clear Compare Match
  sbi (TCCR2A, COM2B1);

  sbi (TCCR2A, WGM20);  // Mode 1  / Phase Correct PWM
  cbi (TCCR2A, WGM21);
  cbi (TCCR2B, WGM22);
}

//******************************************************************
// Timer2 Interrupt Service at 31372,550 KHz = 32uSec
// this is the timebase REFCLOCK for the DDS generator
// FOUT = (M (REFCLK)) / (2 exp 32)
// runtime : 8 microseconds ( inclusive push and pop)
ISR(TIMER2_OVF_vect) {
  //sbi(PORTD,7);          // Test / set PORTD,7 high to observe timing with a oscope

  OCR2A = point[pointIndex][0];
  OCR2B = point[pointIndex][1];

  colours = point[pointIndex][2];
  //digitalWrite(10, bitRead(colours, 0));
  //digitalWrite(9, bitRead(colours, 1));
  //digitalWrite(8, bitRead(colours, 2));
  //digitalWrite(10, 1);
  //digitalWrite(9, 1);
  //digitalWrite(8, 1);
  bitWrite(PORTB,2, bitRead(colours, 0)); //pin 8
  bitWrite(PORTB,1, bitRead(colours, 1)); //pin 9
  bitWrite(PORTB,0, bitRead(colours, 2)); //pin 10

  
  digitalWrite(7, bitRead(soundWave[soundWaveIndex], soundWaveByteIndex));

  // cycle through the points
  pointIndex++;
  if(pointIndex >= pointCount) { 
    pointIndex = 0;
  }
/*
  soundWaveByteIndex++;
  if(soundWaveByteIndex >= 8) {  // first cycle through each bit in the byte
    soundWaveByteIndex = 0;
    soundWaveIndex++;
    if (soundWaveIndex >= soundWaveCount) { // finally if we've exceeded all samples
      soundWaveIndex = 0;
    }
  }
*/

  // cycle through the sound
  soundWaveByteIndex++;
  if(soundWaveByteIndex >= 8) {  // first cycle through each bit in the byte
    soundWaveByteIndex = 0;
    soundWaveRepeatCount++;
    if (soundWaveRepeatCount >= soundWaveRepeatCycle) { // then repeat it the required number of times
      soundWaveRepeatCount = 0;
      soundWaveIndex++;
      if (soundWaveIndex >= soundWaveCount) { // finally if we've exceeded all samples
        soundWaveIndex = 0;
      }
    }
  }
  

  //cbi(PORTD,7);            // reset PORTD,7
}
