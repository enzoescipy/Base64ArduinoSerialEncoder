#include <Arduino.h>
#include <b64ArduinoSerial.h>
#include <string.h>

void DEBUG() {
  // 26, 0, 27, 1, 52, 55, 62, 63 for b64
  // 011010, 000000, 011011, 000001, 110100, 110111, 111110, 111111
  // 97, 65, 98, 66, 48, 51, 43, 47 for ascii
  // char test_b64_string[] = "aAbB+1234567890"; 
  // for (int i = 1; i < 16; i++) {
  //   BASE64 base64 = BASE64(test_b64_string,i);
  //   base64.debug_byteArr();
  // }

  BASE64 base64 = BASE64("aAbB+",5);
  base64.extend("1234567890",10);
  Serial.println(base64.b64_symbolize());
  base64.debug_byteArr();

}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("hello, fuck you!");
  DEBUG();

}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
