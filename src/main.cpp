#include <Arduino.h>
#include <Preferences.h>
#include <stdlib.h>
#include <string.h>
#include <Wire.h>
#include <esp_task_wdt.h>

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

/* other peripherials */
#define BUTTON 25 // Button
#define LED_GN 32 // LED green
#define LED_RD 33 // LED red
#define BUZZER 5  // Buzzer

/* HX711 circuit wiring */
const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 27;
long LOADCELL_OFFSET;
long LOADCELL_DIVIDER;
bool cleared;
HX711 loadcell;

// 3 seconds WDT
#define WDT_TIMEOUT 3 /* seconds watchdog timer */

/* CommandLineInterface */
String inputString = "";                     // a String to hold incoming data
bool stringComplete = false;                 // whether the string is complete
char CommandLine[COMMAND_BUFFER_LENGTH + 1]; // Read commands into this buffer
                                             // from Serial.  +1 in length
                                             // for a termination char
String result;

/* error codes etc. */
int ErrorCount = 0; /* count times reading error */
long last_ms;       /* remember last millis for blinking */
long ms;            /* remember millis for blinking */

int error_code = 0x00;

long weight;      /* current weight from loadcell */
long last_weight; /* last weight to prevent leaking*/
long high_weight; /* switch to slow filling */
long max_weight;  /* stop filling */

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
  // initialize beeper + leds
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_GN, OUTPUT);
  pinMode(LED_RD, OUTPUT);
  digitalWrite(BUZZER, LOW);
  digitalWrite(LED_RD, HIGH);
  digitalWrite(LED_GN, HIGH);

  esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);               // add current thread to WDT watch

  delay(250);
  digitalWrite(BUZZER, HIGH);
  digitalWrite(LED_RD, LOW);
  digitalWrite(LED_GN, LOW);
  // initialize button
  pinMode(BUTTON, INPUT_PULLUP);
  // initialize load cell
  preferences.begin("pust", false);
  LOADCELL_OFFSET = preferences.getLong("LC_OFFSET", -101200);
  LOADCELL_DIVIDER = preferences.getLong("LC_DIVIDER", 389

  );
  preferences.end();
  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  loadcell.set_scale(LOADCELL_DIVIDER);
  loadcell.set_offset(LOADCELL_OFFSET);
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
      Serial.print("error_code = ");
      Serial.println(error_code);
      result = "OK";
    }
    else if (strcmp(CommandLine, "$RESET") == 0)
    {
      error_code = 0;
      result = "OK";
    }
    else if (strcmp(CommandLine, "$FORMAT") == 0)
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
        LOADCELL_OFFSET = loadcell.get_offset();
        preferences.begin("pust", false);
        preferences.putLong("LC_DIVIDER", LOADCELL_DIVIDER);
        preferences.putLong("LC_OFFSET", LOADCELL_OFFSET);
        preferences.end();
        cleared = false;
        result = "OK";
      }
      else
      {
        result = "ERROR - not cleared before calibrate";
        error_code |= 0x0000000000000001;
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
      weight = loadcell.get_units(20); // channel value
      oled.home();
      oled.println("HX711:       ");
      oled.print(weight);
      oled.println(" g     ");
    }
    ErrorCount = 0;
  }
  else
  {
    ErrorCount++;
    if (ErrorCount >= 5)
    {
      error_code |= 0x0000000000000100;
      filling = false;
    }
  }

  /* filling */
  digitalWrite(LED_GN, filling);
  if (filling == true)
  {
    if ((weight > 0) and (weight < (last_weight * 0.9)))
    {
      error_code |= 0x0000000000000010;
      filling = false;
    }
    else
    {
      last_weight = weight;
    }
  }

  /* Fault indication lamp */
  if (error_code > 0)
  {
    ms = millis();
    if (ms >= last_ms + 400)
    {
      digitalWrite(LED_RD, !digitalRead(LED_RD));
      Serial.print("ERROR - ");
      Serial.println(error_code);
      last_ms = ms;
    }
  }
  else
  {
    digitalWrite(LED_RD, false);
  }
  esp_task_wdt_reset();
}
/*---------------------------------------------------------------------------*/
