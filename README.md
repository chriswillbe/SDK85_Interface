# SDK85_Interface

SDK85_Interface is an Arduino program that allows you to control an Intel SDK-85 using a PC.

SDK85_Interface is an Arduino program that transmits and receives data from a PC at 9600 baud, and sends it to the SDK-85 teletype interface at 110 baud. You will need to modify the SDK-85 as shown in the "SDK-85 System Design Kit User's Manual" section 5-7 "Converter Circuit For RS232C Serial Port". You will also need a terminal emulator program like "screen" in order to enter commands and receive data.

The following is a comment taken from the program:

    SDK-85 Interface
  The SDK-85 teletype interface sends data LSB first. The data is inverted because of the teletype interface,
  which was a current loop. The SDK-85 sends nine bits (start and eight bits of data), and then pauses to fake
  parity and three stop bits. The Interface responds with non-inverted data, a start bit and eight data bits.
  The data line is left high, and the stop bits would be ignored by the SDK-85 anyway.
  
  To use, connect as shown in the "SDK-85 System Design Kit User's Manual" section 5-7 "Converter Circuit For RS232C Serial Port"
  The bottom pad of the removed R6 connects to Arduino pin 2, the center contact of S25 connects to Arduino pin 3, and the 
  Arduino should be grounded to the SDK-85.
  
  The following instructions assume you use some *nix.
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
