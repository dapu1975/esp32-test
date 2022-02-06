#include <Arduino.h>
#include <stdlib.h>
#include <string.h>
#include <Wire.h>

#include "getCommandLineFromSerialPort.h"
#include "SimpleCLI.h"
#include "HX711.h"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

/* OLED circuit wiring */
#define I2C_ADDRESS 0x3C // 0X3C+SA0 - 0x3C or 0x3D
#define RST_PIN -1       // Define proper RST_PIN if required.
SSD1306AsciiWire oled;

/* HX711 circuit wiring */
const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 27;
HX711 scale;

/* CommandLineInterface */
String inputString = "";                     // a String to hold incoming data
bool stringComplete = false;                 // whether the string is complete
char CommandLine[COMMAND_BUFFER_LENGTH + 1]; // Read commands into this buffer from Serial.  +1 in length for a termination char
String result;
const char *delimiters = ", \n";             // commands can be separated by return, space or comma

//  CLI - command line interface (uart)
//  $1        -> request parameter $1
//  $1 = 100  -> set parameter $1 to 100
//  $$        -> show all parameter
//  $TARA     -> tara set current weight to zero
//  $CALI     -> calibrate
//  $REBOOT   -> restart
//  $RESET    -> reset all to default
//  $RUN      -> start filling
//  $STOP     -> filling
//  $WEIGH    -> weigh with tara and scale

// 1st must be "$"
// When there is more than A-Za-z0-9= then reject command
// when there is an "=" in cmd then split into operator and operand
// do command
// return "ok" or "error"
// "break#<errorcode>" stops all communication and 
/*---------------------------------------------------------------------------*/
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
  // beep
  pinMode(32, OUTPUT);
  digitalWrite(32, LOW);
  delay(250);
  digitalWrite(32, HIGH);
  // initialize scale
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN, 128);
  Serial.println("\n\n*************************************");
  Serial.println("*** (c) 2022 D. Pust - Waage V1.0 ***");
  Serial.println("*************************************");
}
/*---------------------------------------------------------------------------*/
void loop()
{
  bool received = getCommandLineFromSerialPort(CommandLine);
  if (received)
  {
    result = "Error - wrong syntax";
    if (strcmp(CommandLine, "$RESET") == 0) {
      result = "x";
      ESP.restart(); 
    } else
    if (strcmp(CommandLine, "$TARA") == 0) {
      result = "NOK";
    } else
    if (strcmp(CommandLine, "$CALI") == 0) {
      result = "NOK";
    }
    Serial.println(result);
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
/*---------------------------------------------------------------------------*/
