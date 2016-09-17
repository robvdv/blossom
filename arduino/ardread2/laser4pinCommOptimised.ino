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
byte zeroCounter = 0; // how many zeros in a row have we read?
byte readToArrayPointer = 0; // read to what structure? point 1 or wave 2.
byte readBuffer[20];  // holds a buffer of recently read bytes
byte readBufferPointer = 0; // points to the current position in the read buffer
int numberOfBytesRead = 0; // the number of bytes read in this read cycle
int numberOfBytesToRead = 0; // the number of bytes we expect in this frame
byte point[360][3];
byte wave[360];
byte pointReadIndex = 0;
int i;
String message;
int framePointIndex = 0;
int frameCount = 0;
int framesReceived = 0;
byte readBytePrev = 0;
byte readByte = 0;
boolean systemOn = false; // turn on speakers and lasers
long pointCount = 0;
int commCount = 0;
boolean ledHigh = false;
boolean reset = false;

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

  pinMode(7, OUTPUT);      // Sound
  pinMode(8, OUTPUT);      // Blue
  pinMode(9, OUTPUT);      // Green
  pinMode(10, OUTPUT);      // Red
  pinMode(11, OUTPUT);     // pin11= PWM  output / frequency output
  pinMode(3, OUTPUT);     // pin3= PWM  output / frequency output
  
  Setup_timer2();

  // disable interrupts to avoid timing distortion
  cbi (TIMSK0,TOIE0);              // disable Timer0 !!! delay() is now not available
  sbi (TIMSK2,TOIE2);              // enable Timer2 Interrupt

}

// accepts a two dimensional array containing key/value pairs
// and sends to Serial
void sendMessage(String key, String value) {
  message = "{\"";
  message += key;
  message += "\":\"";
  message += value;
  message += "\"}";
  Serial.print(message);
}

void loop() {
  if (Serial.available() >=5) {
      // we haven't yet set up where we're trying to read to
      if (readToArrayPointer == 0) {
        readByte = Serial.read(); // read a byte 
        if (readByte == 0) { // if we read a zero then keep reading
            readToArrayPointer = 0;
            zeroCounter++;
        } else if (readByte == 1) { // control message
            readToArrayPointer = 0;
            zeroCounter = 0;
            readBytePrev = Serial.read();
            readByte = Serial.read();
            if (readByte == 1) {
                systemOn = true;
                digitalWrite(13, HIGH);  
                sendMessage("systemOn", "true");
            } else if (readByte == 0) {
                systemOn = false;
                digitalWrite(13, LOW);  
                sendMessage("systemOn", "false");
            }
        } else if ((readByte == 1) || (readByte == 2)) { // read to point or tone
            readToArrayPointer = readByte;
            zeroCounter = 0;
            readBytePrev = Serial.read();
            readByte = Serial.read();
            numberOfBytesToRead = (readBytePrev * 256) + readByte;
            framePointIndex = 0;
            pointReadIndex = 0;
        }
      } else {
        while (Serial.available() && (numberOfBytesRead < numberOfBytesToRead)) {
          readByte = Serial.read(); // read a byte 
          if (readToArrayPointer == 1) {
            point[framePointIndex][pointReadIndex] = readByte;
            pointReadIndex++;
            if (pointReadIndex >= 2) {
              pointReadIndex = 0;
              framePointIndex++;
            }
          } else if (readToArrayPointer == 2) {
            wave[numberOfBytesRead] = readByte;
          }
          numberOfBytesRead++;
        }
        if (numberOfBytesRead == numberOfBytesToRead) {
          if (readToArrayPointer == 1) {
            sendMessage("readFrame", "Read point frame " +  String(numberOfBytesRead / 3) + " points long");
          } else {
            sendMessage("readFrame", "Read wave frame " +  String(numberOfBytesRead) + " bytes long");
          }
          readToArrayPointer = 0;
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



