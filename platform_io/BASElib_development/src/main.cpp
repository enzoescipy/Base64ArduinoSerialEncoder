#include <Arduino.h>
#include <b64ArduinoSerial.h>
#include <string.h>

void DEBUG() {
  BASE64 base64 = BASE64("BBBBB",5);
  // base64.debug_byteArr();
  // Serial.println(base64.b64_symbolize());

  // base64.extend("1234567890",10);
  // Serial.println(base64.b64_symbolize());
  // base64.debug_byteArr();

  base64.serial_exhaust_bytes();

  // Serial.println("hello, world!");


}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // Serial.println("hello, fuck you!");
  DEBUG();

}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
