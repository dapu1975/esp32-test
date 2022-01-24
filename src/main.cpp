#include <Arduino.h>

//GPIO4 - on board LED
//UART0 <-> comm with pc
//GPIO0 - LED red

int delayTime = 25;

void setup()
{
  // put your setup code here, to run once:
  pinMode(0, OUTPUT);
  pinMode(4, OUTPUT);
  Serial.begin(115200);
}

void loop()
{
  // put your main code here, to run repeatedly:
  digitalWrite(0, false);
  digitalWrite(4, true);
  delay(delayTime);
  digitalWrite(0, true);
  digitalWrite(4, false);
  delay(delayTime * 100);
  Serial.write('*');
}