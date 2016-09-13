/**
* Reads commands coming from serial interface to drive an LED on/off
* Also prints led status back
*/

#define LEDPIN 13
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

int state = LOW;
char incomingByte = 0;
unsigned int frequency; 
byte frame[2];
byte point[360][3];
byte pointReadIndex = 0;
int i;
String message;
int framePointIndex = 0;
int frameCount = 0;
int framesReceived = 0;
byte readByte = 0;
long pointCount = 0;
int commCount = 0;
boolean ledHigh = false;
boolean reset = false;

volatile byte temp[10][3];


double dfreq;
double dfreqRead;
const double refclk=31376.6;      // measured

// variables used inside interrupt service declared as voilatile
volatile byte icnt1;             // var inside interrupt
volatile byte c4ms;              // counter incremented all 4ms
volatile int frameOutputCounter;             // which frame we're playing
volatile char ch;
volatile unsigned long phaccu;   // pahse accumulator

void setup() {
  Serial.begin(9600); 
  while(Serial.available())
  Serial.read();
  Serial.begin(115200); //9600, 14400, 19200, 28800, 38400, 57600, or 115200.
  while(Serial.available())
  Serial.read();
  pinMode(LEDPIN, OUTPUT);

  pinMode(8, OUTPUT);      // Blue
  pinMode(9, OUTPUT);      // Green
  pinMode(10, OUTPUT);      // Red
  pinMode(11, OUTPUT);     // pin11= PWM  output / frequency output
  pinMode(3, OUTPUT);     // pin3= PWM  output / frequency output
  
  Setup_timer2();

  // disable interrupts to avoid timing distortion
  cbi (TIMSK0,TOIE0);              // disable Timer0 !!! delay() is now not available
  sbi (TIMSK2,TOIE2);              // enable Timer2 Interrupt

  temp[0][0] = 11;
  temp[0][1] = 12;
  temp[0][2] = 13;

}

// accepts a two dimensional array containing key/value pairs
// and sends to Serial
void sendMessage(String key, String value) {
  message = "{\"";
  message += key;
  message += "\":\"";
  message += value;
  message += "\"}";
  Serial.print("message:");
  Serial.print(message);
}

void loop() {
    //Serial.print( "===" );
    //Serial.print( point[0][0] );

  // if there's something to read
  if (Serial.available() >= 20) {

    // start of point marker
    readByte = Serial.read();
    // swallow all non zero bytes until we get to a zero (start of point marker)
    if (readByte != 0) {
      Serial.print( "Discarding corrupt point:" );
      Serial.print( readByte );
      while (readByte != 0) {
        readByte = Serial.read();
        Serial.print( "_" );
        Serial.print( readByte );
      }
      Serial.print( "<" );
    }

    readByte = Serial.read();
    // is this the start of a new frame (two zeros in a row)?
    if (readByte == 0) {
      //Serial.print( "New frame received. Last frame had points: " );
      //Serial.print( framePointIndex );
      sendMessage("frame received", String(framePointIndex));

      //Serial.print( "New frame: " );
      // read the frame config
      for(pointReadIndex=0; pointReadIndex < 2; pointReadIndex++) {
        frame[pointReadIndex] = Serial.read();
        //Serial.print( pointReadIndex );
        //Serial.print(":" );
        //Serial.print( frame[pointReadIndex] );
        //Serial.print( " " );
      }
      frequency = frame[0] * 256 + frame[1];
      //Serial.print( frequency );
      //Serial.print( "#" );

      frameCount = framePointIndex;
      framePointIndex = 0;
      framesReceived++;
      
      readByte = Serial.read(); // discard the leading zero
      readByte = Serial.read(); // and load the next meaningful byte
      //Serial.print("Next byte after frame: " );
      //Serial.print(readByte);
    } 

    if (framePointIndex >= 360) {
      Serial.print( "Frame size exceeded" );
    } else {
      // write point data and read incrementally
      //Serial.print("Got point:");
      for(pointReadIndex=0; pointReadIndex < 2; pointReadIndex++) {
        //Serial.print(readByte);
        point[framePointIndex][pointReadIndex] = readByte;
        readByte = Serial.read();
      }
      // but for the last point only write the point data
      //Serial.print(readByte);
      point[framePointIndex][2] = readByte;
      
      
      framePointIndex++;
      pointCount++;
      commCount++;
      if (commCount == 1000) {
        commCount = 0;

        sendMessage("pointCount", String(pointCount));
        sendMessage("frameCount", String(frameCount));
        /*
        Serial.print( "> " );
        for(pointReadIndex=0; pointReadIndex < 3; pointReadIndex++) {
          Serial.print( point[framePointIndex - 1][pointReadIndex] );
          Serial.print( " " );
        }
        Serial.print( "#" );
        */
      }
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

  sbi(PORTD,7);          // Test / set PORTD,7 high to observe timing with a oscope

  OCR2A = point[frameOutputCounter][0];
  OCR2B = point[frameOutputCounter][1];
  
  digitalWrite(10, bitRead(point[frameOutputCounter][2], 0));
  digitalWrite(9, bitRead(point[frameOutputCounter][2], 1));
  digitalWrite(8, bitRead(point[frameOutputCounter][2], 2));
  
  frameOutputCounter++;
  if(frameOutputCounter == frameCount) { 
    frameOutputCounter = 0;
  }

 cbi(PORTD,7);            // reset PORTD,7
}



