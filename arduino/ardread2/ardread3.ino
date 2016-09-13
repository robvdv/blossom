/**
* Reads commands coming from serial interface to drive an LED on/off
* Also prints led status back
*/
#include "avr/pgmspace.h"

// table of 256 sine values / one sine period / stored in flash memory
const unsigned char sine256[] PROGMEM  = {
  127,130,133,136,139,143,146,149,152,155,158,161,164,167,170,173,176,178,181,184,187,190,192,195,198,200,203,205,208,210,212,215,217,219,221,223,225,227,229,231,233,234,236,238,239,240,
  242,243,244,245,247,248,249,249,250,251,252,252,253,253,253,254,254,254,254,254,254,254,253,253,253,252,252,251,250,249,249,248,247,245,244,243,242,240,239,238,236,234,233,231,229,227,225,223,
  221,219,217,215,212,210,208,205,203,200,198,195,192,190,187,184,181,178,176,173,170,167,164,161,158,155,152,149,146,143,139,136,133,130,127,124,121,118,115,111,108,105,102,99,96,93,90,87,84,81,78,
  76,73,70,67,64,62,59,56,54,51,49,46,44,42,39,37,35,33,31,29,27,25,23,21,20,18,16,15,14,12,11,10,9,7,6,5,5,4,3,2,2,1,1,1,0,0,0,0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,10,11,12,14,15,16,18,20,21,23,25,27,29,31,
  33,35,37,39,42,44,46,49,51,54,56,59,62,64,67,70,73,76,78,81,84,87,90,93,96,99,102,105,108,111,115,118,121,124

};

#define LEDPIN 13

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

int state = LOW;
char incomingByte = 0;
byte point[128][6];
byte pointReadIndex = 0;
byte framePointIndex = 0;
byte frameCount = 0;
byte framesReceived = 0;
byte readByte = 0;
long pointCount = 0;
int commCount = 0;
boolean ledHigh = false;
boolean reset = false;

volatile byte temp[10][6];


double dfreq;
double dfreqRead;
const double refclk=31376.6;      // measured

// variables used inside interrupt service declared as voilatile
volatile byte icnt1;             // var inside interrupt
volatile byte c4ms;              // counter incremented all 4ms
volatile byte frameOutputCounter;             // which frame we're playing
volatile byte frameDisplayCyclesCounter;             // which frame we're playing
volatile char ch;
volatile unsigned long phaccu;   // pahse accumulator

void setup() {
  Serial.begin(9600); 
  while(Serial.available())
  Serial.read();
  Serial.begin(38400); //9600, 14400, 19200, 28800, 38400, 57600, or 115200.
  while(Serial.available())
  Serial.read();
  pinMode(LEDPIN, OUTPUT);

  pinMode(6, OUTPUT);      // sets the digital pin as output
  pinMode(7, OUTPUT);      // sets the digital pin as output
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

void loop() {
    //Serial.print( "===" );
    //Serial.print( point[0][0] );

  // if there's something to read
  if (Serial.available() >= 13) {

    // start of point marker
    readByte = Serial.read();
    // swallow all non zero bytes until we get to a zero (start of point marker)
    if (readByte != 0) {
      while (readByte != 0) {
        readByte = Serial.read();
      }
      Serial.print( "Discarded corrupt point" );
    }

    readByte = Serial.read();
    // is this the start of a new frame (two zeros in a row)?
    if (readByte == 0) {
      //Serial.print( "New frame received. Last frame had points: " );
      //Serial.print( framePointIndex );

      frameCount = framePointIndex;
      framePointIndex = 0;
      framesReceived++;
      
      readByte = Serial.read();
    } 

    if (framePointIndex >= 128) {
      Serial.print( "Frame size exceeded" );
    } else {
      // write point data and read incrementally
      for(pointReadIndex=0; pointReadIndex < 5; pointReadIndex++) {
        point[framePointIndex][pointReadIndex] = readByte;
        readByte = Serial.read();
      }
      // but for the last point only write the point data
      point[framePointIndex][5] = readByte;
      
      
      framePointIndex++;
      pointCount++;
      commCount++;
      if (commCount == 1000) {
        commCount = 0;
        
        //Serial.print( "frameOutputCounter: " );
        //Serial.print( frameOutputCounter );
        //Serial.print( "frameDisplayCyclesCounter: " );
        //Serial.print( frameDisplayCyclesCounter );
        Serial.print( "Points received: " );
        Serial.print( pointCount );
        Serial.print( " frameCount: " );
        Serial.print( frameCount );
        
        /*
        Serial.print( "> " );
        for(pointReadIndex=0; pointReadIndex < 6; pointReadIndex++) {
          Serial.print( point[framePointIndex][pointReadIndex] );
          Serial.print( " " );
        }
        Serial.print( "#" );
        */
        //Serial.print( "framePointIndex: " );
        //Serial.print( framePointIndex );
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

  //if(frameDisplayCyclesCounter++ == 2) { 
    if(frameOutputCounter++ == frameCount) { 
      frameOutputCounter = 0;
     }   
     //frameDisplayCyclesCounter = 0;
  //}

 cbi(PORTD,7);            // reset PORTD,7
}




