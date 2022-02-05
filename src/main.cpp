#include <Arduino.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <SimpleCLI.h>
#include "HX711.h"
#include "string.h"
#include "stdlib.h"

// OLED circuit wiring
#define I2C_ADDRESS 0x3C // 0X3C+SA0 - 0x3C or 0x3D
#define RST_PIN -1       // Define proper RST_PIN if required.
SSD1306AsciiWire oled;

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 27;
HX711 scale;

// TODO : receive commands from uart
//  console interface uart
//  $1 = request parameter $1
//  $1 = 100 set parameter $1 to 100
//  $$ = show all parameter
//  $T = tara set current weight to zero
//  $C = calibrate
//  $R = restart
//  $RST=RST = reset all to default

String inputString = "";     // a String to hold incoming data
bool stringComplete = false; // whether the string is complete

#define CR '\r'
#define LF '\n'
#define BS '\b'
#define NULLCHAR '\0'
#define SPACE ' '

#define COMMAND_BUFFER_LENGTH 25             // length of serial buffer for incoming commands
char CommandLine[COMMAND_BUFFER_LENGTH + 1]; // Read commands into this buffer from Serial.  +1 in length for a termination char

const char *delimiters = ", \n"; // commands can be separated by return, space or comma

// prototypes
bool getCommandLineFromSerialPort(char *commandLine);

//-----------------------------------------------------------------------------
void setup()
{
  // reserve 200 bytes for the inputString:
  inputString.reserve(200);
  // initialize console
  Serial.begin(115200);
  // initialize display
  Wire.begin();
  Wire.setClock(400000L);
  oled.begin(&Adafruit128x32, I2C_ADDRESS);
  oled.setFont(Adafruit5x7);
  oled.set2X();
  // initialize scale
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN, 128);
}
//-----------------------------------------------------------------------------
void loop()
{

  bool received = getCommandLineFromSerialPort(CommandLine); // global CommandLine is defined in CommandLine.h
  if (received)
  {
  }

  if (scale.wait_ready_timeout(1000))
  {
    long reading = scale.read_average(20);
    oled.home();
    oled.println("HX711: ");
    oled.println(reading);
  }
  else
  {
    // TODO count errors stop when >= 3 seconds
    Serial.println("HX711 not found.");
  }
}
//-----------------------------------------------------------------------------
bool getCommandLineFromSerialPort(char *commandLine)
{
  static uint8_t charsRead = 0; // note: COMAND_BUFFER_LENGTH must be less than 255 chars long
  // read asynchronously until full command input
  while (Serial.available())
  {
    char c = Serial.read();
    switch (c)
    {
    case CR: // likely have full command in buffer now, commands are terminated by CR and/or LS
    case LF:
      commandLine[charsRead] = NULLCHAR; // null terminate our command char array
      if (charsRead > 0)
      {
        charsRead = 0; // charsRead is static, so have to reset
        Serial.println(commandLine);
        return true;
      }
      break;
    case BS: // handle backspace in input: put a space in last char
      if (charsRead > 0)
      { // and adjust commandLine and charsRead
        commandLine[--charsRead] = NULLCHAR;
        Serial << byte(BS) << byte(SPACE) << byte(BS); // no idea how this works, found it on the Internet
      }
      break;
    default:
      // c = tolower(c);
      if (charsRead < COMMAND_BUFFER_LENGTH)
      {
        commandLine[charsRead++] = c;
      }
      commandLine[charsRead] = NULLCHAR; // just in case
      break;
    }
  }
  return false;
}