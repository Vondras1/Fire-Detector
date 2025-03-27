/*
   LoRaLib Receive with Interrupts Example

   This example listens for LoRa transmissions and tries to
   receive them. Once a packet is received, an interrupt is
   triggered. To successfully receive data, the following
   settings have to be the same on both transmitter
   and receiver:
    - carrier frequency
    - bandwidth
    - spreading factor
    - coding rate
    - sync word
    - preamble length

   For more detailed information, see the LoRaLib Wiki
   https://github.com/jgromes/LoRaLib/wiki

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/LoRaLib/
*/

// include the library
#include "Arduino.h"
#include <LoRaLib.h>

#define NODE_LOCAL_ADRESS 0x11
#define PROB_SCALE 10000
#define MSG_LEN 9
#define ERR_VAL 65535 // max value of received integer, is treated as error value

// create instance of LoRa class using SX1278 module
// this pinout corresponds to RadioShield
// https://github.com/jgromes/RadioShield
// NSS pin:   10 (4 on ESP32/ESP8266 boards)
// DIO0 pin:  2
// DIO1 pin:  3
// IMPORTANT: because this example uses external interrupts,
//            DIO0 MUST be connected to Arduino pin 2 or 3.
//            DIO1 MAY be connected to any free pin
//            or left floating.
SX1272 lora = new LoRa;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

// Functions declarations
int decodeUpperByte(byte upper_byte);
int decodeNum(byte lower_byte, byte upper_byte);
bool readMsg(byte arr[MSG_LEN], byte *transm_id, int *flame, int *gas, int *smoke, int *prob);
void printMsg(byte transm_id, int flame, int gas, int smoke, int prob);

void setFlag(void) {
  // check if the interrupt is enabled
  if(!enableInterrupt) {
    Serial.println(F("Seflag terminated, Interrupt not enabled"));
    return;
  }

  // we got a packet, set the flag
  Serial.println(F("recflag set to true"));
  receivedFlag = true;
}

void setup() {
  Serial.begin(9600);

  // initialize SX1278 with default settings
  Serial.print(F("Initializing ... "));
  // carrier frequency:           434.0 MHz
  // bandwidth:                   125.0 kHz
  // spreading factor:            9
  // coding rate:                 7
  // sync word:                   0x12
  // output power:                17 dBm
  // current limit:               100 mA
  // preamble length:             8 symbols
  // amplifier gain:              0 (automatic gain control)
  int state = lora.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // set the function that will be called
  // when new packet is received
  lora.setDio0Action(setFlag);

  // start listening for LoRa packets
  // NOTE: for spreading factor 6, the packet length
  //       must be known in advance, and provided to both
  //       startReceive() and readData() methods!
  Serial.print(F("Starting to listen ... "));
  state = lora.startReceive();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // NOTE: 'listen' mode will be disabled
  // automatically by calling any of the
  // following methods:
  //
  // lora.standby()
  // lora.sleep()
  // lora.transmit()
  // lora.receive()
  // lora.scanChannel()
  //
  // LoRa module will not receive any new
  // packets until 'listen' mode is re-enabled
  // by calling lora.startReceive()
}


void loop() {
  //variables for received sensor readings, ID, etc.
  byte transm_id;
  int flame;
  int gas;
  int smoke;
  int scaled_probability;

  // check if the flag is set

  //Serial.print(F("received flag: "));
  //Serial.println(receivedFlag);
  if(receivedFlag) {
    // disable the interrupt service routine while
    // processing the data
    enableInterrupt = false;

    // reset flag
    receivedFlag = false;
    
    // buffer - byte array for received data
    byte byteArr[MSG_LEN];
    int state = lora.readData(byteArr, MSG_LEN);
    

    if (state == ERR_NONE) {
      // packet was successfully received
      Serial.println(F("Received packet!"));

      // print data of the packet
      //Serial.print(F("Data:\t\t\t"));
      //Serial.println(str);

      Serial.println("Bytearray: ");
      for(auto i: byteArr){
        Serial.print(i);
        Serial.print(", ");
      }
      Serial.print("\n");
      
      bool success = readMsg(byteArr, &transm_id, &flame, &gas, &smoke, &scaled_probability);

      printMsg(transm_id, flame, gas, smoke, scaled_probability);

      Serial.print("received values valid: ");
      Serial.println(success);


    } else if (state == ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      Serial.println(F("CRC error!"));

    }

    // we're ready to receive more packets,
    // enable interrupt service routine
    enableInterrupt = true;
    state = lora.startReceive();
  }

}

int decodeUpperByte(byte upper_byte){
  int shifted = (int)upper_byte << 8;
  return shifted;
} 

int decodeNum(byte lower_byte, byte upper_byte) {
  int decoded_number = (int)lower_byte | decodeUpperByte(upper_byte);
  return decoded_number;
}

bool readMsg(byte arr[MSG_LEN], byte *transm_id, int *flame, int *gas, int *smoke, int *prob){
  *transm_id = arr[0];
  *flame = decodeNum(arr[2], arr[1]);
  *gas = decodeNum(arr[4], arr[3]);
  *smoke = decodeNum(arr[6], arr[5]);
  *prob = decodeNum(arr[8], arr[7]);
  if(*flame >= ERR_VAL || *gas >= ERR_VAL||*smoke >= ERR_VAL||*prob >= ERR_VAL){
    return false;
  }
  return true; 
}

void printMsg(byte transm_id, int flame, int gas, int smoke, int prob){
  Serial.print(F("transmitter ID: "));
  Serial.print(transm_id);
  Serial.print(F(", flame: "));
  Serial.print(flame);
  Serial.print(F(", gas: "));
  Serial.print(gas);
  Serial.print(F(", smoke: "));
  Serial.print(smoke);
  Serial.print(F(", prob: "));
  Serial.print(prob);
  Serial.println();
}