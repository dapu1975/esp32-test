#include <Arduino.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <SimpleCLI.h>
#include "HX711.h"

// console interface uart
// $1 = request parameter $1
// $1 = 100 set parameter $1 to 100
// $$ = show all parameter
// $T = tara set current weight to zero
// $C = calibrate
// $R = restart
// $RST=RST = reset all to default

// OLED circuit wiring
#define I2C_ADDRESS 0x3C // 0X3C+SA0 - 0x3C or 0x3D
#define RST_PIN -1       // Define proper RST_PIN if required.
SSD1306AsciiWire oled;

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 27;
HX711 scale;

//-----------------------------------------------------------------------------
void setup()
{
  // initialize console
  Serial.begin(115200);
  // initialize display
  Wire.begin();
  Wire.setClock(400000L);
  oled.begin(&Adafruit128x32, I2C_ADDRESS);
  oled.setFont(Adafruit5x7);
}
//-----------------------------------------------------------------------------
void loop()
{
  oled.clear();
  oled.set2X();
  oled.println("HX711 V0.1");
  delay(500);
}
//-----------------------------------------------------------------------------
