#include <Arduino.h>
#include <Preferences.h>
#include <stdlib.h>
#include <string.h>
#include <Wire.h>

#include "getCommandLineFromSerialPort.h"
#include "HX711.h"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

Preferences preferences;
unsigned int StartCounter;

/* OLED circuit wiring */
#define I2C_ADDRESS 0x3C // 0X3C+SA0 - 0x3C or 0x3D
#define RST_PIN -1       // Define proper RST_PIN if required.
SSD1306AsciiWire oled;

#define BUZZER 34 // Buzzer
#define BUTTON 25 // Button

/* HX711 circuit wiring */
const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 27;
long LOADCELL_OFFSET;
long LOADCELL_DIVIDER;
bool cleared;
HX711 loadcell;

/* CommandLineInterface */
String inputString = "";                     // a String to hold incoming data
bool stringComplete = false;                 // whether the string is complete
char CommandLine[COMMAND_BUFFER_LENGTH + 1]; // Read commands into this buffer
                                             // from Serial.  +1 in length
                                             // for a termination char
String result;

bool filling = false;
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
  // initialize beeper
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  delay(250);
  digitalWrite(BUZZER, HIGH);
  // initialize button
  pinMode(BUTTON, INPUT_PULLUP);
  // initialize load cell
  preferences.begin("pust", false);
  LOADCELL_OFFSET = preferences.getLong("LC_OFFSET", 50682624);
  LOADCELL_DIVIDER = preferences.getLong("LC_DIVIDER", 5895655);
  preferences.end();
  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  loadcell.set_offset(LOADCELL_OFFSET);
  loadcell.set_scale(LOADCELL_DIVIDER);
  Serial.println("\n\n*************************************");
  Serial.println("*** (c) 2022 D. Pust - Waage V0.1 ***");
  Serial.println("*************************************");
  // increment StartCounter
  preferences.begin("pust", false);
  StartCounter = preferences.getUInt("Counter", 0);
  StartCounter++;
  preferences.putUInt("Counter", StartCounter);
  preferences.end();
  delay(500);
}
/*---------------------------------------------------------------------------*/
void loop()
{
  bool received = getCommandLineFromSerialPort(CommandLine);
  if (received)
  {
    result = "Error - wrong syntax";
    if (strcmp(CommandLine, "$LIST") == 0)
    {
      Serial.print("StartCounter = ");
      Serial.println(StartCounter);
      Serial.print("LC_DIVIDER = ");
      Serial.println(LOADCELL_DIVIDER);
      Serial.print("LC_OFFSET = ");
      Serial.println(LOADCELL_OFFSET);
      Serial.print("cell cleared = ");
      Serial.println(cleared);
      result = "OK";
    }
    if (strcmp(CommandLine, "$FORMAT") == 0)
    {
      result = "";
      preferences.begin("pust", false);
      preferences.clear();
      preferences.end();
      ESP.restart();
    }
    else if (strcmp(CommandLine, "$REBOOT") == 0)
    {
      ESP.restart();
    }
    else if (strcmp(CommandLine, "$TARE") == 0)
    {
      loadcell.tare();
      LOADCELL_OFFSET = loadcell.get_offset();
      preferences.begin("pust", false);
      preferences.putLong("LC_OFFSET", LOADCELL_OFFSET);
      preferences.end();
      result = "OK";
    }
    else if (strcmp(CommandLine, "$CLEAR") == 0)
    {
      loadcell.set_scale();
      loadcell.tare();
      cleared = true;
      oled.clear();
      result = "OK";
    }
    else if (strcmp(CommandLine, "$CALI") == 0)
    {
      if (cleared)
      {
        LOADCELL_DIVIDER = long(loadcell.get_units(10) / 67.9);
        loadcell.set_scale(LOADCELL_DIVIDER);
        preferences.begin("pust", false);
        preferences.putLong("LC_DIVIDER", LOADCELL_DIVIDER);
        preferences.end();
        cleared = false;
        result = "OK";
      }
      else
      {
        result = "ERROR - not cleared before calibrate";
      }
    }
    else if (strcmp(CommandLine, "$START") == 0)
    {
      filling = true;
      result = "OK";
    }
    else if (strcmp(CommandLine, "$STOP") == 0)
    {
      filling = false;
      result = "OK";
    }
    Serial.println(result);
  }

  if (loadcell.wait_ready_timeout(100))
  {
    if (cleared == false)
    {
      long channel_value = loadcell.get_units(20); // channel value
      oled.home();
      oled.println("HX711:       ");
      oled.print(channel_value);
      oled.println(" g     ");
    }
  }
  else
  {
    // TODO count errors stop when >= 3 seconds
    Serial.println("HX711 not found.");
  }
}
/*---------------------------------------------------------------------------*/
