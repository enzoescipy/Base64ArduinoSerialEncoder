#include <Arduino.h>
#include <b64ArduinoSerial.h>
#include <string.h>
#include <EEPROM.h>


const uint8_t main_id_arr[] = {0};
const uint8_t sub_id_arr[] = {0};
const float tester[] = {1.11, 2.22, 3.33};
BGSerialCommunication_scottish BGSerial = BGSerialCommunication_scottish(
  "BH01", "random", "BGst", main_id_arr, sub_id_arr, 1);

// clear EEPROM before testment please!!
void clear_EEPROM() {
  Serial.begin(9600);
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  Serial.println("clear done!");
}

void setup() {
  // clear_EEPROM();
  BGSerial.setup();
}

void loop() {
  BGSerial.loop(tester, 3);
}
