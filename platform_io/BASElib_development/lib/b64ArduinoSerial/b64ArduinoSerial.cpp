#include "Arduino.h"
#include "b64ArduinoSerial.h"
#include "string.h"
#include "math.h"
#include "EEPROM.h"

/// debuging functions region

BASE64 B64HANDLE = BASE64("",0);

ManageEEPROM EEPROMHANDLE;

Hex8Float HEX8FLOATHANDLE;


void byteArrSimplePrint(uint8_t byteArr[], int len ) {
  for (int i = 0; i < len; i++) {
  Serial.print(byteArr[i], BIN);
  Serial.print("  ");
  }
}

void debugConsole(String description, String variable) {
  Serial.println(description + "::" + variable);
}

void debugLineBreak() {
  Serial.println();
  Serial.println();
}

/// end of region


/// convinent functions region


// convert the ascii_char to the corresponding b64 symbol index.
// Return:
//  0 <= result < 64 : base64 index
//  result = -1 : unidentified ascii-char. (wrong input)
int8_t asciiCharToB64Index(char ascii_char) {
  int8_t result = ascii_char;
  // Serial.println(String(result) + "asciiOriginal");
  if (64 < ascii_char && ascii_char < 91) {
    result -= 65;
  } else if (96 < ascii_char && ascii_char < 123) {
    result -= 71;
  } else if (47 < ascii_char && ascii_char < 58) {
    result += 4;
  } else if (43 == ascii_char) {
    result = 62;
  } else if (47 == ascii_char) {
    result = 63;
  } else {
    result = -1;
  }

  return result;
}



// convert the b64 symbol to the corresponding ascii_char symbol index.
// Return:
//  0 <= result < 127 : ascii char
//  result = -1 : unidentified b64. (wrong input)
char b64ByteToAsciiSymbol(uint8_t b64_char) {
  char result = b64_char;
  // Serial.println(String(result) + "asciiOriginal");
  if (0 <= b64_char && b64_char < 26) {
    result += 65;
  } else if (26 <= b64_char && b64_char < 52) {
    result += 71;
  } else if (52 <= b64_char && b64_char < 62) {
    result -= 4;
  } else if (62 == b64_char) {
    result = 43;
  } else if (63 == b64_char) {
    result = 47;
  } else {
    result = -1;
  }

  return result;
}





/// end of region


// initialize the BASE64 object with the error signs.
void BASE64::_fault_control() {
  // exception handles
  _len = -1;
  _byteArrLen = -1;
  free((void*) _byteArr);
  _byteArr = NULL;
  _ending = -1;
  _ending_position = -10;
  _stored_length = -1;
}

// get ascii string and convert it to the base64-byte encoded object.
// assigned _byteArr will generated by stacking the bits from b64 symbol, by
// FIFO order. (first char of b64 will be placed at final index, and then pops out very first when interpret happens.)
// Param:
//   char ascii_string[] : string that contains each ascii char representing the base64 symbol.
//                         eg ) "aAbB12/+"
//   unsigned int length : string length provided. function will not check the ascii_string's length directly.
BASE64::BASE64(const char ascii_string[], unsigned int b64_length) {
  _len = b64_length;
  // assign the value to the instance fields
  _byteArrLen = (_len + 4 - (_len % 4)) * 3 / 4;

  // Serial.println(_byteArrLen);
  _byteArr = (uint8_t *)malloc(sizeof(uint8_t) * _byteArrLen);
  
  // convert the ascii symbol to the base64 symbol.
  // at the same time, save the byte representative of the b64 symbol.
  // if inappropriate ascii symbol has found, raise exception.
  uint8_t stored_bits = 0x00;
  int stored_length = 0;
  int byteArr_current_focus = _byteArrLen - 1;
  for (unsigned int i = 0; i < b64_length; i++) {
    const char target_char = ascii_string[i];
    const int8_t ascii_to_b64 = asciiCharToB64Index(target_char);
    // Serial.println(ascii_to_b64);
    if (ascii_to_b64 < 0) {
      // exception handling
      _fault_control();
      return;
    } 
    
    const uint8_t b64_index = ascii_to_b64;
    // convert b64_index to the bit form.
    // however, b64 is 6bit while byte is 8bit.
    // so, stack b64 from last index of array.

    // first, put the b64 symbol bits into the stored_bits.
    const uint8_t b64_combined = stored_bits | (b64_index << stored_length);

    stored_length += 6;
    if (stored_length < 8) {
      stored_bits = b64_combined;
    } else {
      // check if << left shift operator overflow then the digits vanish or not?
      // -> yes, vanish.
      stored_length -= 8;
      _byteArr[byteArr_current_focus] = b64_combined;
      stored_bits = b64_index >> (6 - stored_length);
      byteArr_current_focus--;
    }
  
  }

  if (stored_length > 0) {
    _byteArr[byteArr_current_focus] = stored_bits;
    _ending_position = byteArr_current_focus;
    byteArr_current_focus--;
  } else {
    _ending_position = byteArr_current_focus;
  }
  _stored_length = stored_length;
  _ending = stored_bits;

}


 
void BASE64::extend(const char extender[], unsigned int extender_length) {
  // re-calculate the byteArrLen, and move pointer to the temporary variable
  _len += extender_length;
  int byteArrLenBefore = _byteArrLen;
  volatile uint8_t *byteArrBefore =_byteArr;

  const int base64_byte_div = _len * 6 / 8;
  if ((_len * 6 % 8) != 0) {
    _byteArrLen = base64_byte_div + 1;
  } else {
    _byteArrLen = base64_byte_div;
  }

  // re-assign the pointer
  // Serial.println(_byteArrLen);
  _byteArr = (uint8_t *)malloc(sizeof(uint8_t) * _byteArrLen);

  // put the byteArrBefore to the _byteArr. however, the ending byte will not applied.
  int byteArr_past_focus = byteArrLenBefore - 1;
  int byteArr_current_focus = _byteArrLen - 1;

  for (int i = 0; i < byteArrLenBefore - _ending_position - 1; i++) {
    _byteArr[byteArr_current_focus] = byteArrBefore[byteArr_past_focus];
    byteArr_current_focus--;
    byteArr_past_focus--;
  }

  // debugConsole("_ending_position",String(_ending_position));
  // debugConsole("byteArr_current_focus",String(byteArr_current_focus));
  // debugConsole("byteArr_past_focus",String(byteArr_past_focus));

  // re-assign the stored_bits and its stored_length
  uint8_t stored_bits = byteArrBefore[byteArr_past_focus];
  // byteArr_current_focus--;


  // free the array before
  free((void*) byteArrBefore);

  int stored_length = _stored_length;
  for (unsigned int i = 0; i < extender_length; i++) {
    const char target_char = extender[i];
    const int8_t ascii_to_b64 = asciiCharToB64Index(target_char);

    if (ascii_to_b64 < 0) {
      // exception handling
      _fault_control();
      return;
    }


    // debugConsole("i",String(i));
    // debugConsole("ascii_to_b64",String(ascii_to_b64, DEC));
    const uint8_t b64_index = ascii_to_b64;
    // convert b64_index to the bit form.
    // however, b64 is 6bit while byte is 8bit.
    // so, stack b64 from last index of array.

    // first, put the b64 symbol bits into the stored_bits.
    const uint8_t b64_combined = stored_bits | (b64_index << stored_length);

    stored_length += 6;
    if (stored_length < 8) {
      stored_bits = b64_combined;
    } else {
      // check if << left shift operator overflow then the digits vanish or not?
      // -> yes, vanish.
      stored_length -= 8;
      _byteArr[byteArr_current_focus] = b64_combined;
      stored_bits = b64_index >> (6 - stored_length);
      byteArr_current_focus--;
    }
  }

  if (stored_length > 0) {
    _byteArr[byteArr_current_focus] = stored_bits;
    _ending_position = byteArr_current_focus;
    byteArr_current_focus--;
  } else {
    _ending_position = byteArr_current_focus;
  }
  _stored_length = stored_length;
  _ending = stored_bits;

  // exhaust the left byteArr_current_focus until zero, filing 0x00 for each iteration.
  while (byteArr_current_focus >= 0) {
    _byteArr[byteArr_current_focus] = 0x00;
    byteArr_current_focus--;
  }
}

void BASE64::extend_float(float number) {
  union float_b64 { // warning : byte ordering is little-endian!
    uint8_t byte_arr[4];
    float number;
  } b64_float;


  b64_float.number = number;
  // //debug
  // for (int i = 0; i < 4; i++) {
  //   Serial.println(b64_float.byte_arr[i], BIN);
  // }
  // //debug

  // re-calculate the byteArrLen, and move pointer to the temporary variable
  _len += 6;
  int byteArrLenBefore = _byteArrLen;
  volatile uint8_t *byteArrBefore =_byteArr;

  const int base64_byte_div = _len * 6 / 8;
  if ((_len * 6 % 8) != 0) {
    _byteArrLen = base64_byte_div + 1;
  } else {
    _byteArrLen = base64_byte_div;
  }

  // re-assign the pointer
  _byteArr = (uint8_t *)malloc(sizeof(uint8_t) * _byteArrLen);

  // put the byteArrBefore to the _byteArr. however, the ending byte will not applied.
  int byteArr_past_focus = byteArrLenBefore - 1;
  int byteArr_current_focus = _byteArrLen - 1;
  for (int i = 0; i < byteArrLenBefore - _ending_position - 1; i++) {
    _byteArr[byteArr_current_focus] = byteArrBefore[byteArr_past_focus];
    byteArr_current_focus--;
    byteArr_past_focus--;
  }

  // free the byteArrBefore.
  // append the float bytes.
  uint8_t stored_bits = byteArrBefore[byteArr_past_focus];
  free((void*) byteArrBefore);

  for (unsigned int i = 0; i < 4; i++) {
    const uint8_t target_byte = b64_float.byte_arr[i];

    // first, put the b64 symbol bits into the stored_bits.
    const uint8_t b64_combined = stored_bits | (target_byte << _stored_length);

    // check if << left shift operator overflow then the digits vanish or not?
    // -> yes, vanish.
    _byteArr[byteArr_current_focus] = b64_combined;
    stored_bits = target_byte >> (8 - _stored_length);
    byteArr_current_focus--;
  }

  // we need to add the 4 zero bit to the end because of the base64 convention.
  const uint8_t target_byte = 0x00;
  const uint8_t b64_combined = stored_bits | (target_byte << _stored_length);

  _stored_length += 4;
  if (_stored_length < 8) {
    stored_bits = b64_combined;
  } else {
    // check if << left shift operator overflow then the digits vanish or not?
    // -> yes, vanish.
    _stored_length -= 8;
    _byteArr[byteArr_current_focus] = b64_combined;
    stored_bits = 0x00;
    byteArr_current_focus--;
  }


  if (_stored_length > 0) {
    _byteArr[byteArr_current_focus] = stored_bits;
    _ending_position = byteArr_current_focus;
    byteArr_current_focus--;
  } else {
    _ending_position = byteArr_current_focus;
  }
  _ending = stored_bits;

  // exhaust the left byteArr_current_focus until zero, filing 0x00 for each iteration.
  while (byteArr_current_focus >= 0) {
    _byteArr[byteArr_current_focus] = 0x00;
    byteArr_current_focus--;
  }
}

void flipArray(volatile uint8_t arr[], int length) {
  uint8_t byte_storage = 0x00;
  int reverse_index = length - 1;
  int index = 0;
  while (index < reverse_index) {
    byte_storage = arr[reverse_index];
    arr[reverse_index] = arr[index];
    arr[index] = byte_storage;
    reverse_index--;
    index++;
  }
}

// write _byteArr then deactivate this BASE64 object.
void BASE64::serial_exhaust_bytes() {
  flipArray(_byteArr, _byteArrLen);
  Serial.write((char*) _byteArr, _byteArrLen) ;
  _fault_control();
}

// Serial.println the _byteArr to the bit 0-1 string
void BASE64::debug_byteArr() {
  Serial.println(b64_symbolize());
  Serial.println();
  Serial.println("||DEBUG||");
  debugConsole("_len",String(_len));
  debugConsole("_byteArrLen",String(_byteArrLen));
  byteArrSimplePrint((uint8_t*) _byteArr, _byteArrLen);
  Serial.println("||spec||");
  debugConsole("_ending",String(_ending, BIN));
  debugConsole("_ending_position",String(_ending_position));
  debugConsole("_stored_length",String(_stored_length));
  Serial.println();
}


String BASE64::b64_symbolize() {
  int len = _len;
  int byteArrLen = _byteArrLen;
  int byteArr_current_focus = byteArrLen - 1;
  String result = "";

  uint8_t stored_bits = 0x00;
  int stored_length = 0;
  const uint8_t b64_mask = 0x3F;
  for (int i = 0; i < len; i ++) {
    const uint8_t target_char = _byteArr[byteArr_current_focus];
    if (stored_length < 6) {
      const char masked = ((target_char << stored_length) | stored_bits) & b64_mask ;
      result += String(b64ByteToAsciiSymbol( masked ));
      stored_length = 2 + stored_length;
      stored_bits = target_char >> (8 - stored_length);
      byteArr_current_focus--;
    } else {
      const char masked = stored_bits & b64_mask ;
      result += String(b64ByteToAsciiSymbol(masked));
      stored_length = stored_length - 6;
      stored_bits = stored_bits >> 6;
    }
  }
  return result;
}

uint8_t hexCharToInt(char hex) {
  const char table[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd' ,'e' ,'f'};
  for (int i = 0; i < 16; i++) {
    if (hex == table[i]) {
      return i;
    }
  }
  return 16;
}

float Hex8Float::next(char hex) {
  if (_pos > 3) {
    _pos = 0;
    return byte_float.number;
  }

  if (is_no_stored) {
    _hex_head = hex;
  } else {
    is_no_stored = false;
    const uint8_t head_4bit = hexCharToInt(_hex_head);
    const uint8_t tail_4bit = hexCharToInt(hex);
    byte_float.byte_arr[_pos] = head_4bit * 16 + tail_4bit;
    _pos++;
  }

  return 0.0/0.0;
}

void ManageEEPROM::clear() {
  for (int i = 0 ; i < 64 ; i++) {
    EEPROM.write(i, 0);
  }
}


void ManageEEPROM::assign_string(uint8_t main_key, uint8_t sub_key , const char string[], uint8_t length) {
  
  // store the size of the data in the first 0~63 location
  const int key = sub_key + main_key * 8;
  EEPROM.write(key, length);
  
  // count the accumulated byte length
  int key_sum = 0;
  for (int i = 0; i < key; i++) {
    key_sum += EEPROM.read(i);
  }

  for (int i = 0; i < length; i++) {
    const int idx = _core_offset + key_sum + i;
    EEPROM.write(idx, string[i]);
  }

}

void ManageEEPROM::get_string(uint8_t main_key, uint8_t sub_key , char string[]) {
  // get the stored value size
  const int key = sub_key + main_key * 8;
  const uint8_t size = EEPROM.read(key);
  if (size == 0) {
    return;
  }

  // count the accumulated byte length
  int key_sum = 0;
  for (int i = 0; i < key; i++) {
    key_sum += EEPROM.read(i);
  }

  // retrieve the each byte then store in the string array.
  for (int i = 0; i < size; i++) {
    const int idx = _core_offset + key_sum + i ;
    string[i] = EEPROM.read(idx);
  }
}

void ManageEEPROM::assign_float(uint8_t main_key, uint8_t sub_key , float number) {
  // store the size of the data in the first 0~63 location
  const int key = sub_key + main_key * 8;
  EEPROM.write(key, 4);
  
  // count the accumulated byte length
  int key_sum = 0;
  for (int i = 0; i < key; i++) {
    key_sum += EEPROM.read(i);
  }

  // get byte from union
  union float_b64 { // warning : byte ordering is little-endian!
    uint8_t byte_arr[4];
    float number;
  } b64_float;
  b64_float.number = number;


  for (int i = 0; i < 4; i++) {
    const int idx = _core_offset + key_sum + i;
    EEPROM.write(idx, b64_float.byte_arr[i]);
  }
}

float ManageEEPROM::get_float(uint8_t main_key, uint8_t sub_key) {
  // get the stored value size
  const int key = sub_key + main_key * 8;
  const uint8_t size = EEPROM.read(key);
  if (size == 0) {
    return 0.0/0.0;
  }

  // count the accumulated byte length
  int key_sum = 0;
  for (int i = 0; i < key; i++) {
    key_sum += EEPROM.read(i);
  }

  // retrieve the each byte then store in the union array.
  union float_b64 { // warning : byte ordering is little-endian!
    uint8_t byte_arr[4];
    float number;
  } b64_float;

  for (int i = 0; i < 4; i++) {
    const int idx = _core_offset + key_sum + i ;
    b64_float.byte_arr[i] = EEPROM.read(idx);
  }

  return b64_float.number;
}



// start the serial, and then set the constants.
// note : you should put the device_code, com_rule 's length = 4.
BGSerialCommunication_scottish::BGSerialCommunication_scottish(const char device_code[5], const char id[7], const char com_rule[5],
    const uint8_t main_id_arr[], const uint8_t sub_id_arr[], const uint8_t length) {
  // build the complete ping code
  strncat(_ping_string, device_code, 4);
  strncat(_ping_string, "++", 2);
  strncat(_ping_string, id, 6);
  strncat(_ping_string, "++", 2);
  strncat(_ping_string, com_rule, 4);
  strncat(_ping_string, "++//", 4);

  _main_id_arr = main_id_arr;
  _sub_id_arr = sub_id_arr;
  _pref_length = length;
}

void BGSerialCommunication_scottish::setup() {
  Serial.begin(9600);
}


// send the ping sign through the serial.
void BGSerialCommunication_scottish::p() {
  // build the base64, burn it, remove it.
  B64HANDLE = BASE64(_ping_string,24);
  B64HANDLE.serial_exhaust_bytes();
}

// send the float array values through the serial.
void BGSerialCommunication_scottish::r(const float sensor_value_array[], const int length) {
  // determine if the length is odd or even
  bool is_not_div_by_4;
  const int count = 8 * (length + 1) - 2;
  if (count % 4 == 0) {
    // you have to use "...++++//"
    is_not_div_by_4 = true;
  } else {
    // you have to use "...++//"
    is_not_div_by_4 = false;
  }

  B64HANDLE = BASE64("//",2);
  
  for (int i = 0; i < length; i++) {
    const float target_value = sensor_value_array[i];
    B64HANDLE.extend_float(target_value);
    B64HANDLE.extend("++",2);
  }

  if (is_not_div_by_4) {
    B64HANDLE.extend("++//", 4);
  } else {
    B64HANDLE.extend("//",2);
  }
  // build the base64, burn it, remove it.
  B64HANDLE.serial_exhaust_bytes();
  // B64HANDLE.debug_byteArr();
}

void BGSerialCommunication_scottish::s() {
  // load the data from EEPROM
  float* stored_float_arr = (float *)malloc(sizeof(float) * _pref_length);
  for (int i = 0; i < _pref_length; i++) {
    stored_float_arr[i] = EEPROMHANDLE.get_float(_main_id_arr[i], _sub_id_arr[i]);
  }

  r(stored_float_arr, _pref_length);
}

void BGSerialCommunication_scottish::S(const float const_value_array[]) {
  for (int i = 0; i < _pref_length; i++) {
    EEPROMHANDLE.assign_float(_main_id_arr[i], _sub_id_arr[i], const_value_array[i]);
  }

  B64HANDLE = BASE64("//SS++//", 8);
  B64HANDLE.serial_exhaust_bytes(); 
}

void BGSerialCommunication_scottish::loop(const float sensor_value_array[], const int length) {
  if (Serial.available() == 0) {
    if (_is_R_fired == false && _is_S_fired == false) {
      return;
    } else if (_is_R_fired) {
      // work the fired R 
      r(sensor_value_array, length);
      return;
    }
  }

  const char command_char = Serial.read();

  if (_is_R_fired == false && _is_S_fired == false) {
    // simple command (p, r, s) case + R, S initial detection
    if (command_char == 'p') {
      p();
    } else if (command_char == 'r') {
      r(sensor_value_array, length);
    } else if (command_char == 's') {
      s();
    } else if (command_char == 'R') {
      _is_R_fired = true;
      return;
    } else if (command_char == 'S') {
      _is_S_fired = true;
      return;
    }
  } else if (_is_R_fired) {
    // R fired case
    // check if command_char is 'X'
    if (command_char == 'X') {
      _is_R_fired = false;
      return;
    }
    // work the fired R 
    r(sensor_value_array, length);
    return;
  } else if (_is_S_fired) {
    // check if command char is ']'
    if (command_char == ']') {
      S(_S_float_param);
      _S_float_param_count = 0;
      _is_S_fired = false;
      return;
    } else if (command_char != '[') {
      const float result = HEX8FLOATHANDLE.next(command_char);
      if (isnan(result) != 0) {
        _S_float_param[_S_float_param_count] = result;
        _S_float_param_count++;
      }
    }
  }
}
