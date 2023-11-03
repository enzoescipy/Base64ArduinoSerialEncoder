#ifndef b64ArduinoSerial
#define b64ArduinoSerial

#include "Arduino.h"

class BASE64 {
  public:
    BASE64(const char ascii_string[], unsigned int b64_length );
    void extend(const char extender[], unsigned int b64_length);
    void debug_byteArr(); // comment this section when production
    String b64_symbolize(); // comment this section when production
  private:
    // initialize with the error signs.
    void _fault_control();
    // length of b64 
    int _len; 
    // length of byte array
    int _byteArrLen; 
    // the main byteArray
    uint8_t *_byteArr;
    // last byte archive
    uint8_t _ending; 
    // ending byte position in _byteArr
    int _ending_position; 
    // meaningful bit length of the ending byte.
    // if _stored_length = 3, this meand that ending byte is like: xxxxx"010"
    int _stored_length;
};





#endif