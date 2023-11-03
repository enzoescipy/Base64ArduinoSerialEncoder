#include "Arduino.h"
#include "string.h"
#include "base64.hpp"
#include "HX711.h"
#include <EEPROM.h>

#define BUFFER_ENCODE 50 // inspect this amount of buffer string to find <//> command starter.
#define COMMAND_LEN 5 // current master's command variation number.

#define QUEUE_SIZE 100
#define BYTEARR_REVERSE_ON false // false then disable the byteArrReverse function 

//Hx711 objects
HX711 scale0;
HX711 scale1;
HX711 scale2;

// HX711 circuit wiring
const short SCK_PIN_0 = 2;
const short DOUT_PIN_0 = 3;
const short SCK_PIN_1 = 4;
const short DOUT_PIN_1 = 5;
const short SCK_PIN_2 = 6;
const short DOUT_PIN_2 = 7;


// check if E code fired
bool is_E_fired = false;
bool is_S_fired = false;

// sensors linear parameters. each represent l0, l1, l2 param.
float loadcell_divide_0 = 1.0f;
float loadcell_divide_1 = 1.0f;
float loadcell_divide_2 = 1.0f;
float loadcell_offset_0 = 0.0f;
float loadcell_offset_1 = 0.0f;
float loadcell_offset_2 = 0.0f;

// // queue define
typedef struct{
    unsigned short front;
    unsigned short rear;
    char data[QUEUE_SIZE];
} Queue;

void init_queue(Queue *q) {
    //// (out) <- front _ _ _ _ _ rear <- (in) ////
    // data comes from rear, pop out from front. FIFO.
    q->front = 0;
    q->rear = 0;
}

bool is_queue_full(Queue *q) {
    return (q->rear + 1) % QUEUE_SIZE == q->front;
}

bool is_queue_empty(Queue *q) {
    return q->rear == q->front;
}

void debug_queue_stats(Queue *q) {
    Serial.print(q->front);
    Serial.print(" ,");
    Serial.print(q->rear);
    Serial.println();
}

unsigned short length_queue(Queue *q) {
    short length = q->front - q->rear;
    if (length < 0) {
        return -length;
    } else {
        return length;
    }
}


bool push_queue(Queue *q, char c) {
    if (is_queue_full(q)) {
        return false;
    }
    q->data[q->rear] = c;
    q->rear = (q->rear + 1) % QUEUE_SIZE;
    return true;
} 

char pop_queue(Queue *q) {
    if (is_queue_empty(q)) {
        return -1;
    }
    char temp =  q->data[q->front];
    q->front = (q->front + 1) % QUEUE_SIZE;
    return temp;
}

char peek_queue(Queue *q, unsigned short index) {
    if (is_queue_empty(q)) {
        return -1;
    }
    char ind = (index + q->front) % QUEUE_SIZE;
    return q->data[ind];
}



// // 



// // // EEPROM
// void saveEEPROM() {
//     const char size = 8;
//     short divideId;
//     short offsetId;
//     short isSavedId;
//     char* divideByteArray;
//     char* offsetByteArray;

//     float loadcell_divide[3] = {loadcell_divide_0,loa_______;
//       float floatVal;        
//     } b64_float ;

//     //get the 4-bit float data, then reverse them. (endian change)
//     b64_float.floatVal = floatNum;
//     unsigned char Flipped_arr[4] = {"\0"};
//     for (char i=0; i<4; i++)
//     {
//         Flipped_arr[3 - i] = b64_float.intVal[i];
//     }

//     //convert byte format to base64 format.
//     encode_base64(Flipped_arr, 4, base64);

// }

// base64DecodeFloat
// change base64 encoding value to equivalent float. 
// causion : this function get value without changing arduino's basic string format. so, 
// one letter will occupy just same as before, 2-byte.
// setter base64[7] length=6 because of null terminator.
float base64DecodeFloat(unsigned char base64[7]) {
    char decoded[5] = {'/0'};
    decode_base64(base64, decoded);


    union float_base64
    {
      unsigned char intVal[4];
      float floatVal;        
    } b64_float ;

    //set the 4-bit char data, with reversed. (endian change)
    for (char i=0; i<4; i++)
    {
        b64_float.intVal[3 - i] = decoded[i];
    }

    return b64_float.floatVal;
}





// get base64ToByte() parameter byteArr's proper length.
unsigned int base64ToByte_getlength(int base64Len) {
    return (base64Len + 4 - (base64Len % 4)) * 3 / 4;
}

//change base64 encoded ascii string to serial sendable byte list form
// returns the decoded_length.
// byteArr's length should be base64ToByte_getlength() 's suggested length.
void base64ToByte(unsigned char base64[], short base64_length, unsigned char byteArr[]) {
    // add 'A's which means zero in base64, to match bits in conversion.
    unsigned short compensate = 4 - (base64_length % 4);
    unsigned char base64_compensate[base64_length + compensate + 1];
    for (short i=0; i<base64_length; i++){
        base64_compensate[i] = base64[i];
    }
    for (char i=0; i<compensate; i++) {
        base64_compensate[base64_length + i] = 'A';
    }
    base64[base64_length + compensate] = '\0';
    
    //convert compensated base64 to byte form
    decode_base64(base64_compensate, byteArr);
}


// Serial buffer that saves hardware buffered char s.

// // ping reply ready example
// unsigned char reply[] = "BH+/8/";
// unsigned int reply_bytelength = base64ToByte_getlength(reply);
// unsigned char *reply_byteArr = (unsigned char *)malloc(sizeof(char) * reply_bytelength);

Queue serialBufferQueue;


const unsigned char p_reply[] = "//BH++++//";
const unsigned int p_reply_bytelength = base64ToByte_getlength(strlen(p_reply));
const unsigned char *p_reply_byteArr = (unsigned char *)malloc(sizeof(char) * p_reply_bytelength);
// action_ping
// check if encoded buffer's 0th ~ elements is ping command
// then do the reply through serial 
void action_ping() {
    // check 2th element is 'p'
    // without checking 0, 1th elements are '//'
    char p_peeker = peek_queue(&serialBufferQueue, 2);
    if (p_peeker != 'p') {
        return;
    } 

    // send reply
    Serial.write(p_reply_byteArr, p_reply_bytelength);


    // clear buffer
    Serial.flush();
    init_queue(&serialBufferQueue);
}

// action_emit
// check if encoded buffer's 0th ~ elements is emit command
// then do the reply through serial 
void action_emit() {

    // check 2th element is 'p'
    // without checking 0, 1th elements are '//'
    char e_peeker = peek_queue(&serialBufferQueue, 2);
    if (e_peeker != 'e') {
        return;
    } 

    // sensor values update
    float l0 = scale0.get_units();
    float l1 = scale1.get_units();
    float l2 = scale2.get_units();
    
    // reply make
    unsigned char e_reply[42] = {"\0"};
    strncat(e_reply, "//l0++l1++l2++", 14);
    unsigned char getter0[7] = {"\0"} ;
    base64IncodeFloat(l0, getter0);
    strncat(e_reply, getter0, 6);
    strncat(e_reply, "++", 2);

    unsigned char getter1[7] = {"\0"} ;
    base64IncodeFloat(l1, getter1);
    strncat(e_reply, getter1, 6);
    strncat(e_reply, "++", 2);

    unsigned char getter2[7] = {"\0"} ;
    base64IncodeFloat(l2, getter2);
    strncat(e_reply, getter2, 6);
    strncat(e_reply, "++//", 4);


    unsigned int e_reply_bytelength = base64ToByte_getlength(41);
    unsigned char *e_reply_byteArr = (unsigned char *)malloc(sizeof(char) * e_reply_bytelength);
    
    // reply ready
    base64ToByte(e_reply, 40, e_reply_byteArr);
    byteArrReverse(e_reply_byteArr, e_reply_bytelength);

    // send reply
    Serial.write(e_reply_byteArr, e_reply_bytelength);

    // clear buffer
    Serial.flush();
    init_queue(&serialBufferQueue);

}

const unsigned char SE_reply[] = "//++//";
const unsigned int SE_reply_bytelength = base64ToByte_getlength(strlen(SE_reply));
const unsigned char *SE_reply_byteArr = (unsigned char *)malloc(sizeof(char) * SE_reply_bytelength);
// action_Emit
// check if encoded buffer's 0th ~ elements is E command
// then do the reply through serial repeatedely
void action_Emit() {
    // check 2th element is 'p'
    // without checking 0, 1th elements are '//'
    char E_peeker = peek_queue(&serialBufferQueue, 2);
    if (E_peeker != 'E') {
        return;
    } 

    is_E_fired = true;

    // clear buffer
    Serial.flush();
    init_queue(&serialBufferQueue);
    
}
void action_Emit_rep() {
    delay(100);
    // check 2th element is 'X'
    // without checking 0, 1th elements are '//'
    char e_peeker0 = peek_queue(&serialBufferQueue, 0);
    char e_peeker1 = peek_queue(&serialBufferQueue, 1);
    char e_peeker2 = peek_queue(&serialBufferQueue, 2);
    if (e_peeker0 == '/' && e_peeker1 == '/' && e_peeker2 != 'X') {
        Serial.flush();
        init_queue(&serialBufferQueue);
    } else if (e_peeker0 == '/' && e_peeker1 == '/' && e_peeker2 == 'X') {
        is_E_fired = false;
        Serial.flush();
        init_queue(&serialBufferQueue);
        return;
    }

    // sensor values update
    float l0 = scale0.get_units();
    float l1 = scale1.get_units();
    float l2 = scale2.get_units();

    // reply make
    unsigned char e_reply[30] = {"\0"};
    strncat(e_reply, "//", 2);
    unsigned char getter0[7] = {"\0"} ;
    base64IncodeFloat(l0, getter0);
    strncat(e_reply, getter0, 6);
    strncat(e_reply, "++", 2);

    unsigned char getter1[7] = {"\0"} ;
    base64IncodeFloat(l1, getter1);
    strncat(e_reply, getter1, 6);
    strncat(e_reply, "++", 2);

    unsigned char getter2[7] = {"\0"} ;
    base64IncodeFloat(l2, getter2);
    strncat(e_reply, getter2, 6);
    strncat(e_reply, "++//", 4);


    unsigned int e_reply_bytelength = base64ToByte_getlength(29);
    unsigned char *e_reply_byteArr = (unsigned char *)malloc(sizeof(char) * e_reply_bytelength);
    
    // reply ready
    base64ToByte(e_reply, 28, e_reply_byteArr);
    byteArrReverse(e_reply_byteArr, e_reply_bytelength);

    // send reply
    Serial.write(e_reply_byteArr, e_reply_bytelength);
}


// action_save
// check if encoded buffer's 0th ~ elements is S command
// then do the save EEPROM.
// order is, 0_divide -> 0_offset -> 1_divide -> ...
void action_Save() {
    // check 2th element is 'S'
    // without checking 0, 1th elements are '//'
    char S_peeker = peek_queue(&serialBufferQueue, 2);
    if (S_peeker != 'S') {
        return;
    } 

    is_S_fired = true;
}

void action_Save_rep() {
    // validate if command is proper form.
    if (length_queue(&serialBufferQueue) < 55) {
        return;
    }
    const char slash[3] = "//";
    const char plus[3] = "++";
    char check[3] = {'\0'};
    check[0] = peek_queue(&serialBufferQueue, 0);
    check[1] = peek_queue(&serialBufferQueue, 1);
    if (strcmp(slash, check) != 0) { return; }
    check[0] = peek_queue(&serialBufferQueue, 53);
    check[1] = peek_queue(&serialBufferQueue, 54);
    if (strcmp(slash, check) != 0) { return; }
    for (short i=3; i < 55; i = i + 8) {
        check[0] = peek_queue(&serialBufferQueue, i);
        check[1] = peek_queue(&serialBufferQueue, i+1);
        if (strcmp(plus, check) != 0) { return; }
    }

    // get the values from buffer
    char value[7] = {'\0'};
    float decoded_list[6] = {0.0f};
    short index = 0;
    for (short i=5; i < 55; i = i + 8) {
        for (short j=0; j < 6; j++){
            value[j] = peek_queue(&serialBufferQueue, i+j);
        }
        decoded_list[index] = base64DecodeFloat(value);
        index++;
    } 


    loadcell_divide_0 = decoded_list[0];
    loadcell_offset_0 = decoded_list[1];
    loadcell_divide_1 = decoded_list[2];
    loadcell_offset_1 = decoded_list[3];
    loadcell_divide_2 = decoded_list[4];
    loadcell_offset_2 = decoded_list[5];

    // EEPROM save
    saveEEPROM();

    // sensor apply
    scale0.set_scale(loadcell_divide_0);
    scale0.set_offset(loadcell_offset_0);
    scale1.set_scale(loadcell_divide_1);
    scale1.set_offset(loadcell_offset_1);
    scale2.set_scale(loadcell_divide_2);
    scale2.set_offset(loadcell_offset_2);


    // clear buffer
    Serial.flush();
    init_queue(&serialBufferQueue);
    is_S_fired = false;
}


// action_save_check
// check if encoded buffer's 0th ~ elements is s command
// then do the print EEPROM.
void action_save_check() {
    // check 2th element is 's'
    // without checking 0, 1th elements are '//'
    char s_peeker = peek_queue(&serialBufferQueue, 2);
    if (s_peeker != 's') {
        return;
    }
    // EEPROM load
    loadEEPROM();

    // reply make
    unsigned char s_reply[54] = {"\0"};
    strncat(s_reply, "//", 2);

    unsigned char s_getter[7] = {"\0"} ;
    base64IncodeFloat(loadcell_divide_0, s_getter);
    strncat(s_reply, s_getter, 6);
    strncat(s_reply, "++", 2);
    base64IncodeFloat(loadcell_offset_0, s_getter);
    strncat(s_reply, s_getter, 6);
    strncat(s_reply, "++", 2);

    base64IncodeFloat(loadcell_divide_1, s_getter);
    strncat(s_reply, s_getter, 6);
    strncat(s_reply, "++", 2);
    base64IncodeFloat(loadcell_offset_1, s_getter);
    strncat(s_reply, s_getter, 6);
    strncat(s_reply, "++", 2);

    base64IncodeFloat(loadcell_divide_2, s_getter);
    strncat(s_reply, s_getter, 6);
    strncat(s_reply, "++", 2);
    base64IncodeFloat(loadcell_offset_2, s_getter);
    strncat(s_reply, s_getter, 6);
    strncat(s_reply, "++//", 4);


    unsigned int s_reply_bytelength = base64ToByte_getlength(53);
    unsigned char *s_reply_byteArr = (unsigned char *)malloc(sizeof(char) * s_reply_bytelength);
    
    // reply ready
    base64ToByte(s_reply, 52, s_reply_byteArr);
    byteArrReverse(s_reply_byteArr, s_reply_bytelength);

    // send reply
    Serial.write(s_reply_byteArr, s_reply_bytelength);

    //serial cleaning
    Serial.flush();
    init_queue(&serialBufferQueue);
    
}



// sign list prepare
void (*signFuncArr[COMMAND_LEN]) () = {action_ping, action_emit, action_Emit, action_Save, action_save_check};

void moveOutSerial()
{
 while (1)
 {
  if (Serial.read() == -1)
  {
   break;
  }
 }
}





void setup() {
    // serial start
    Serial.begin(9600);
    moveOutSerial();

    // Hx711 start
    scale0.begin(DOUT_PIN_0, SCK_PIN_0);
    scale1.begin(DOUT_PIN_1, SCK_PIN_1);
    scale2.begin(DOUT_PIN_2, SCK_PIN_2);

    //Hx711 initiate
    loadEEPROM();
    scale0.set_scale(loadcell_divide_0);
    scale0.set_offset(loadcell_offset_0);
    scale1.set_scale(loadcell_divide_1);
    scale1.set_offset(loadcell_offset_1);
    scale2.set_scale(loadcell_divide_2);
    scale2.set_offset(loadcell_offset_2);

    // ping
    base64ToByte(p_reply, 10, p_reply_byteArr);
    byteArrReverse(p_reply_byteArr, p_reply_bytelength);

    //init queue
    init_queue(&serialBufferQueue);

    //bool init
    is_E_fired = false;
    is_S_fired = false;

    // divide, offset reset
    loadcell_divide_0 = 0.0f;
    loadcell_divide_1 = 0.0f;
    loadcell_divide_2 = 0.0f;
    loadcell_offset_0 = 0.0f;
    loadcell_offset_1 = 0.0f;
    loadcell_offset_2 = 0.0f;
}

void loop() {
    delay(100);
    //// put the serial buffer-ed char.s in the queue.
    unsigned short Serialcount = Serial.available();
    if (Serialcount) {
        for (short i=0; i<Serialcount; i++) {
            char buffer_get = Serial.read();
            push_queue(&serialBufferQueue, buffer_get);
        }
    }

    //// drop char before "//" in the encoded buffer.
    // only inspect BUFFER_ENCODE amount.


    char peeker0;
    char peeker1;

    const unsigned int count = length_queue(&serialBufferQueue);
    int inspect_length = (count - 1 > BUFFER_ENCODE) ? BUFFER_ENCODE : count - 1;
    for (short i=0; i < inspect_length; i++) {
        peeker0 = peek_queue(&serialBufferQueue, 0);
        peeker1 = peek_queue(&serialBufferQueue, 1);
        if (peeker0 == '/' && peeker1 == '/') {
            break;
        }
        pop_queue(&serialBufferQueue);
    }


    // special command recognition. 
    // only if is E or S fired
    if (is_E_fired) {
        action_Emit_rep();
        return;
    } else if (is_S_fired) {
        action_Save_rep();
        return;
    }

    
    // command recognition. recognite matching command then call proper function
    // only if "//" detected
    char peeker2;
    peeker0 = peek_queue(&serialBufferQueue, 0);
    peeker1 = peek_queue(&serialBufferQueue, 1);
    peeker2 = peek_queue(&serialBufferQueue, 2);
    if (peeker0 != '/' || peeker1 != '/') {
        return;
    }
    if (peeker2 == 'p') {(*signFuncArr[0]) ();}
    else if (peeker2 == 'e') {(*signFuncArr[1]) ();}
    else if (peeker2 == 'E') {(*signFuncArr[2]) ();}
    else if (peeker2 == 'S') {(*signFuncArr[3]) ();}
    else if (peeker2 == 's') {(*signFuncArr[4]) ();}

}
