#ifndef b64ArduinoSerial
#define b64ArduinoSerial

#include "Arduino.h"

class BASE64 {
  public:
    BASE64(const char ascii_string[], unsigned int b64_length );
    void extend(const char extender[], unsigned int b64_length);
    void extend_float(float number);
    void debug_byteArr(); // comment this section when production
    String b64_symbolize(); // comment this section when production
    void serial_exhaust_bytes();
  private:
    // initialize with the error signs.
    void _fault_control();
    // length of b64 
    int _len; 
    // length of byte array
    int _byteArrLen; 
    // the main byteArray
    volatile uint8_t *_byteArr;
    // last byte archive
    uint8_t _ending; 
    // ending byte position in _byteArr
    int _ending_position; 
    // meaningful bit length of the ending byte.
    // if _stored_length = 3, this meand that ending byte is like: xxxxx"010"
    int _stored_length;
};

class Hex8Float {
  public:
    // if the 8 hex char collected, return the float.
    // otherwise return nan.
    // must put the 8 hex(0~9, a, b, c, d, e, f) ascii char.
    float next(char hex);
  private:
    // temp storage for 2 hex binding
    bool _is_no_stored = true;
    char _hex_head;

    // union for getting the float value
    union float_byte { 
      uint8_t byte_arr[4];
      float number;
    } byte_float;
    uint8_t _pos = 0; // current storage line index
    
};


// ManageEEPROM first save the virtual id and its byte size by, 
// if empty then store 0, not empty then store its size on the key-size array on EEPROM.
// (sizes are each 1 byte, idx is 0~63)
class ManageEEPROM {
  public:
    // remove the all of datas from EEPROM.
    void clear();

    // save data to EEPROM by main, sub key (0~7 each), and data size.
    void assign_string(uint8_t main_key, uint8_t sub_key , const char string[], uint8_t length);
    
    // get the string data from its keys.
    void get_string(uint8_t main_key, uint8_t sub_key , char string[]);
  
    // save float data
    void assign_float(uint8_t main_key, uint8_t sub_key , float number);

    // get float data
    float get_float(uint8_t main_key, uint8_t sub_key);
  
  private:
    const uint8_t _core_offset = 64; 
};


class BGSerialCommunication_scottish {
  public:
    // must call this constructor ONLY ONCE in the setup() section.
    // select the main / sub id of which EEPROM manager could watching so that
    // user can user them as s, S command.
    // same index on sub / main arr is the same storage.
    // however, device_code, id, com_rule should be in the EEPROM too, but currently just fixed in code now.
    BGSerialCommunication_scottish(const char device_code[5], const char id[7], const char com_rule[5],
    const uint8_t main_id_arr[], const uint8_t sub_id_arr[], const uint8_t length);

    // just start the serial
    void setup();

    // just fire the private member _pint_string, which would be built during the constructor calling.
    void p();

    // fire the float array of sensor_value_array, only once.
    void r(const float sensor_value_array[], const int length);

    // get the float values from EEPROM manager by _main_id_arr and _sub_id_arr
    // then fire it through serial
    void s();

    // save float values to the EEPROM manager by _main_id_arr and _sub_id_arr.
    // saves the _pref_length amount only.
    void S(const float const_value_array[]);

    // detect the input command.
    // call it in the loop() section.
    // parameter is the sensor values which should be emitted when the 'r' or 'R' command called.
    void loop(const float sensor_value_array[], const int length);
  private:
    // special ping constant string
    char _ping_string[25] = "//";
    // S() temporary float storage. limit is 15, fixed.
    float _S_float_param[15] = {0};
    uint8_t _S_float_param_count = 0;

    // R, S fired check bool
    bool _is_R_fired = false;
    bool _is_S_fired = false;

    // EEPROM manager storage information
    const uint8_t *_main_id_arr;
    const uint8_t *_sub_id_arr;
    uint8_t _pref_length;


};


#endif