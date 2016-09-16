/*
      SDK-85 Interface
      Version 1.0
  The SDK-85 teletype interface sends data LSB first. The data is inverted because of the teletype interface,
  which was a current loop. The SDK-85 sends nine bits (start and eight bits of data), and then pauses to fake
  parity and three stop bits. The Interface responds with non-inverted data, a start bit and eight data bits.
  The data line is left high, and the stop bits would be ignored by the SDK-85 anyway.
  
  To use, connect as shown in the "SDK-85 System Design Kit User's Manual" section 5-7 "Converter Circuit For RS232C Serial Port"
  The bottom pad of the removed R6 connects to Arduino pin 2, the center contact of S25 connects to Arduino pin 3, and the 
  Arduino should be grounded to the SDK-85.
  I use Linux. The following instructions assume you use some *nix.
  You need "screen" or some other terminal emulator that sends every character (the SDK uses the "escape" key in the insert command). 
  Load this code into the Arduino.
  Start your terminal emulator. Ex. "screen /dev/ttyACM0" or whatever your Arduino shows up as in the /dev folder.
  Press "Reset" on the SDK-85. The screen should show "SDK-85    VER 2.1" (unless you have an earlier version of firmware) and then a line
  that contains only a "." prompt. There may be some junk characters above the first line.
  Type (capital) 'X' then press "enter". If the screen shows your SDK registers and values, you have two way communcation.
  
  Uncomment "#define ECHO" below to make the Arduino echo back the character you sent it AFTER it sends it to the SDK. This would be
  useful for throttling if you are writing a program to send files to the SDK.
  Uncomment "#define DEBUG" if you like a chatty program. If you disable BLOCK_ILLEGAL_CHARS this only tells you if data was dropped 
  due to collisions between SDK receive and PC receive. Otherwise it also tells you the hex code of any blocked characters.
  Comment out "#define BLOCK_ILLEGAL_CHARS" to allow the Arduino to send characters that the SDK is just going to reject. I understand that
  my if statement is really horrible, but I don't know a better way to filter the characters, and even if it has to check every case,
  it's still a lot quicker than sending a 110 baud byte to the SDK and getting back an error character ("*").
  
  With "DEBUG", "ECHO" and "BLOCK_ILLEGAL_CHARS" not defined, there is no feedback from the Arduino.  All data is coming from the SDK.
  
  With only "BLOCK_ILLEGAL_CHARS" defined the only feedback that comes from the Arduino is
  '[B]'?
  where B is a blocked character. If an undisplayable character is received, the error is
  ''?
  All other data is coming from the SDK.
 */
//#define ECHO
//#define DEBUG
#define BLOCK_ILLEGAL_CHARS
const int SDK_BITS = 8;                                  
const int SDK_BIT_HALF = 4500;                           // half bit width in uS, to delay to middle of bit
const int SDK_BIT_WIDTH = 9090;                          // full bit width in uS, to skip start and stop bits
const float VER = 0.1;                                   // version number of this program
int sdkRxPin = 2;                                        // digital pin 2 is recieve from SDK-85
int sdkTxPin = 3;                                        // digital pin 3 is transmit to SDK-85
int sdkLedPin = 13;                                      // the builtin led on the Arduino Uno
unsigned long sdkBitTimer;                               // compare current time to this for bit widths
int sdkBitsLeft = SDK_BITS;                              // to send or receive
boolean gettingSDK_Byte = false;
boolean sendingSDK_Byte = false;
int sdkByte = 0;                                         // to send or receive
#ifdef ECHO
int pcRxByte = 0;                                        // pcRxByte exists only to echo data back to pc
#endif
int multiplier = 1;                                      // multiplier for input bit to position it

// the setup routine runs once when you press reset:
void setup()
{
  Serial.begin(9600);                                     // initialize serial communication at 9600 bits per second
  pinMode(sdkRxPin, INPUT);
  pinMode(sdkTxPin, OUTPUT);
  digitalWrite(sdkTxPin, HIGH);                           // start bit is low, which continually interrupts SDK-85
  pinMode(sdkLedPin,OUTPUT);                              // this led will be on when data is rxing from SDK-85
  attachInterrupt(0, sdkRxISR, RISING);                   // interrupt on rising edge to start rxing from SDK-85
  flushSerial();                                          // trying to get rid of random char on SDK-85 restart
}

void loop()
{
  if(gettingSDK_Byte)
  {
    if (micros() >= sdkBitTimer)
    {
      sdkBitTimer = micros() + SDK_BIT_WIDTH;             // reset elapsed time
      sdkByte |= !digitalRead(sdkRxPin) * multiplier;     // this flips the input (data is sent LSB first)
      multiplier = multiplier << 1;                       // set for next bit position
      if(!sdkBitsLeft)
      {        
        sdkByte &= 0x7F;
        Serial.print((char)sdkByte);                      // send data to the PC
        digitalWrite(sdkLedPin, LOW);                     // SDK activity light off
        gettingSDK_Byte = false;                          // clear flag to allow pc rx and tx
        flushSerial();                                    // drops messages recieved from PC while recieving from SDK        
      }
      else
      {
        sdkBitsLeft--;                                    // next
      }
    }
  }
  else                                                    // not gettingSDK_Byte
  {
    if(!sendingSDK_Byte && Serial.available())            // skip if already rxing or no data available
    {
      sdkByte = Serial.read();                            // this blocks, but already checked for data above
#ifdef BLOCK_ILLEGAL_CHARS
      if(isDigit(sdkByte) || sdkByte == 'X' || sdkByte == 'I' || sdkByte == 'G' || sdkByte == 'M' || sdkByte == 'S' || sdkByte == 13 || sdkByte == 27 || sdkByte == 32 || ( sdkByte > 64 && sdkByte < 71))
      {
#endif  // BLOCK_ILLEGAL_CHARS
#ifdef ECHO      
          pcRxByte = sdkByte;                              // pcRxByte exists only to echo data back to the PC
#endif // ECHO
          sendingSDK_Byte = true;                          // start sending data to the SDK
          sdkBitsLeft = SDK_BITS;                          // set for next transmit or receive
          sdkByte = sdkByte << 1;                          // make start bit
          sdkTxBit();                                      // and send it
#ifdef BLOCK_ILLEGAL_CHARS
    }
    else
    {
      Serial.print("'");                                  // comment these lines out to make the Arduino silently block illegal characters
      Serial.print((char)sdkByte);                        // this does not print unprintable characters. Change to "Serial.print(sdkByte,HEX);" to show hex codes
      Serial.print("'?");
#ifdef DEBUG
      Serial.print(" invalid input [0x");
      Serial.print(sdkByte,HEX);                          // shows hex value so you can see control chars, etc
      Serial.print("].");
#endif // DEBUG
    }
#endif // BLOCK_ILLEGAL_CHARS
    }
    if (sendingSDK_Byte)
    {
      if(micros() >= sdkBitTimer)                          // if a bit width has passed
      {
        sdkTxBit();                                        // send the next bit
      }
    }
  }
}

void sdkTxBit()
{
  int temp = sdkByte & 1;                                 // check LSB
  if(temp)                                                // if 1
  {
    digitalWrite(sdkTxPin, HIGH);                         // send 1
  }
  else                                                    // if 0
  {
    digitalWrite(sdkTxPin, LOW);                          // send 0
  }
  if(sdkBitsLeft)
  {
    sdkBitTimer = micros() + SDK_BIT_WIDTH;               // set timer for the end of this bit
    sdkByte = sdkByte >> 1;                               // get rid of the bit just sent
    sdkBitsLeft--;
  }
  else                                                    // no more to send
  {
    digitalWrite(sdkTxPin, HIGH);                         // leave Tx high, its normal state
    delayMicroseconds(SDK_BIT_WIDTH * 10);                // wait for stop bit
    sendingSDK_Byte = false;
#ifdef ECHO
    Serial.print((char)pcRxByte);                         // echo back data to the PC that was sent to SDK
#endif // ECHO
  }
}

void flushSerial()
{
#ifdef DEBUG
  int temp = 0;
#endif  // DEBUG
  if(Serial.available())
  {
    while(Serial.read() != -1)                            // if there was data in the buffer
    {
#ifdef DEBUG
      temp++;                                             // flush it
#endif  // DEBUG
    }
  }
#ifdef DEBUG
  if(temp)
    {
      Serial.print(" rejected input ");
      Serial.print(temp);
      Serial.print(" times ");
    }
#endif  // DEBUG
}

// attached to rxDataPin, runs on each rising edge
void sdkRxISR()
{
   if(!gettingSDK_Byte)
   {
     digitalWrite(sdkLedPin, HIGH);                       // Turn on SDK Rx lamp
     sdkBitTimer = micros() + SDK_BIT_HALF + SDK_BIT_WIDTH;// Go to center of first data bit (skip start bit)
     gettingSDK_Byte = true;                              // flag to avoid running this ISR again
     sendingSDK_Byte = false;
     sdkBitsLeft = SDK_BITS;                           // setup for next time
     multiplier = 1;                                   // setup for next time
     sdkByte = 0;       
   }
}
