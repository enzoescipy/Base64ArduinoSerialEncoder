#ifndef b64ArduinoSerial
#define b64ArduinoSerial

#include "Arduino.h"

class BASE64 {
  public:
    BASE64(char ascii_string[], unsigned int b64_length );
    void debug_byteArr(); // comment this section when production
  private:
    int _len; // length of b64 
    int _byteArrLen; // length of byte array
    uint8_t *_byteArr;
};





#endif