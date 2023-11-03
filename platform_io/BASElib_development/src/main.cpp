#include <Arduino.h>
#include <b64ArduinoSerial.h>

// typedef union {
//   bool signArray[6];
//   unsigned char uint8_value;
// } b64Char;

void signArrayPrint(bool arr[6]){
  Serial.println();
  for (int i = 0; i < 6; i++) {
    Serial.print(String(arr[i] ? "true" : "false"));
  }
  Serial.println();
}


void DEBUG() {
  // 26, 0, 27, 1, 52, 55, 62, 63 for b64
  // 011010, 000000, 011011, 000001, 110100, 110111, 111110, 111111
  // 97, 65, 98, 66, 48, 51, 43, 47 for ascii
  char test_b64_string[] = "aAbB03+/"; 
  BASE64 base64 = BASE64(test_b64_string, 8);
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
