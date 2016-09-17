/**
* Reads commands coming from serial interface to drive an LED on/off
* Also prints led status back
*/

#define LEDPIN 13
#define CLEAR_EAT_JUNK_THRESHHOLD 2
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

int state = LOW;
char incomingByte = 0;
unsigned int frequency; 
byte frame[2];
byte zeroCounter = 0; // how many zeros in a row have we read?
byte readToArrayPointer = 0; // read to what structure? point 1 or wave 2.
int numberOfBytesRead = 0; // the number of bytes read in this read cycle
int numberOfBytesToRead = 0; // the number of bytes we expect in this frame
byte point[180][3];
byte wave[180];
byte waveRepeatSignal = 0;
byte pointReadIndex = 0;
int i;
String message;
int framePointIndex = 0;
int frameCount = 0;
int framesReceived = 0;
byte readBytePrev = 255;
byte readByte = 255;
boolean eatJunkMode = true; // swallow all input until we receive two consecutive zeros
byte clearJunkBytesEaten = 0;
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

  sendMessage("helloBlossom", "Arduino is on");
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

void setJunkModeOn(String message) {
  eatJunkMode = true;
  clearJunkBytesEaten = 0;
  readBytePrev = 255;
  sendMessage("eatJunkOn", message);
}

void setJunkModeOff() {
  eatJunkMode = false;
  sendMessage("eatJunk", "cleared");
}

void loop() {
  // if there's nothing to read, don't do anything
  if (Serial.available() >= (CLEAR_EAT_JUNK_THRESHHOLD + 3)) {
    // clear up any comms mess by eating all bytes until we get two consecutive zeros
    if (eatJunkMode) {
      readByte = Serial.read(); 
      sendMessage("got junk", String(readByte));
      if (readByte == 0) {
        clearJunkBytesEaten++;
      }

      if (clearJunkBytesEaten >= CLEAR_EAT_JUNK_THRESHHOLD) {
        setJunkModeOff();
      }
    } else {
      // do we have a target for our read or are we still waiting for a control?
      if ((Serial.available() >= 2) && (readToArrayPointer == 0)) {
        readByte = Serial.read(); // get the control byte after reading zeros
        sendMessage("checking control", String(readByte));
        if (readByte == 1) { // control message
            readToArrayPointer = 0;
            zeroCounter = 0;
            readBytePrev = Serial.read(); // unused 
            readByte = Serial.read();
            sendMessage("system state", String(readByte));
            if (readByte == 1) {
              systemOn = true;
              digitalWrite(13, HIGH);  
              setJunkModeOn("Set system ON, awaiting next command");
              sendMessage("systemOn", "true");
            } else if (readByte == 0) {
              systemOn = false;
              digitalWrite(13, LOW);  
              setJunkModeOn("Set system OFF, awaiting next command");
              sendMessage("systemOn", "false");
            } else {
              // we got an unexpected value so something went wrong. Time to eat junk.
              setJunkModeOn("Unknown comms state: " + String(readByte));
            }
        } else if ((readByte == 2) || (readByte == 3)) { // read to point or tone
            readToArrayPointer = readByte;
            sendMessage("readToArrayPointer", String(readToArrayPointer));
            zeroCounter = 0;
            readByte = Serial.read();
            if (readToArrayPointer == 2) {
              // there are three bytes for each point, so multiply by three
              numberOfBytesToRead = Serial.read() * 3;
            } else if (readToArrayPointer == 3) {
              waveRepeatSignal = readByte; 
              numberOfBytesToRead = Serial.read();
            } else {
              setJunkModeOn("Unknown point or wave reference");
            }
            sendMessage("frameReadInstruction", String(numberOfBytesToRead) + " bytes long");
            framePointIndex = 0;
            pointReadIndex = 0;
        } else {
          setJunkModeOn("Unknown control byte: " + String(readByte));
        }
      }

      // there's something to read and we know where it should go and we're not eating junk
      if ((Serial.available() >= 5) && (readToArrayPointer != 0) && !eatJunkMode) {
        while (Serial.available() && (numberOfBytesRead < numberOfBytesToRead)) {
          readByte = Serial.read(); // read a byte 
          if (readToArrayPointer == 2) {
            point[framePointIndex][pointReadIndex] = readByte;
            pointReadIndex++;
            if (pointReadIndex >= 2) {
              pointReadIndex = 0;
              framePointIndex++;
            }
          } else if (readToArrayPointer == 3) {
            wave[numberOfBytesRead] = readByte;
          }
          numberOfBytesRead++;
        }

        // are we done reading the specified number of bytes?
        if (numberOfBytesRead == numberOfBytesToRead) {
          if (readToArrayPointer == 2) {
            sendMessage("readFrame", "Read point frame " +  String(numberOfBytesRead / 3) + " points long");
          } else if (readToArrayPointer == 3) {
            sendMessage("readFrame", "Read wave frame " +  String(numberOfBytesRead) + " bytes long");
          }
          readToArrayPointer = 0;
        }
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



