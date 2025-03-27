/*
   For more detailed information, see the LoRaLib Wiki
   https://github.com/jgromes/LoRaLib/wiki

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/LoRaLib/
*/

// include the library
#include "Arduino.h"
#include <LoRaLib.h>

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

// Device adress
#define LOCAL_ADRESS 0x11
#define PROB_SCALE 10000
#define MSG_LEN 9
#define ERR_VALUE 65535

// Structs
struct encodedInTwoBytes{
  byte upper_byte;
  byte lower_byte;
};

// Functions declarations
void setFlag(void);
byte encodeLowerByte(int num);
byte encodeUpperByte(int num);
bool encodeNumber(int num, encodedInTwoBytes *encoded);
int decodeUpperByte(byte upper_byte);
int decodeNumber(byte lower_byte, byte upper_byte);

// save transmission state between loops
int transmissionState = ERR_NONE;

// flag to indicate that a packet was sent
volatile bool transmittedFlag = true;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

void setup() {
  Serial.begin(9600);

  // initialize SX1278 with default settings
  Serial.print(F("Initializing ... "));
  // carrier frequency:                   434.0 MHz
  // bandwidth:                           125.0 kHz
  // spreading factor:                    9
  // coding rate:                         7
  // sync word:                           0x12
  // output power:                        17 dBm
  // current limit:                       100 mA
  // preamble length:                     8 symbols
  // amplifier gain:                      0 (automatic gain control)
  int state = lora.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // set the function that will be called when packet transmission is finished
  lora.setDio0Action(setFlag);

  // setup finished
  Serial.print(F("Setup finished."));
}

void loop() {
  int smoke = 273;
  int flame = 514;
  int gass = 4021;
  float fire_prob = 0.053;
  int scaled_probability = (int)(fire_prob*PROB_SCALE);

  encodedInTwoBytes encoded_smoke;
  encodedInTwoBytes encoded_flame;
  encodedInTwoBytes encoded_gass;
  encodedInTwoBytes encoded_prob;

  bool res_smoke = encodeNumber(smoke, &encoded_smoke);
  bool res_flame = encodeNumber(flame, &encoded_flame);
  bool res_gass = encodeNumber(gass, &encoded_gass);
  bool res_prob = encodeNumber(scaled_probability, &encoded_prob);

  // check if the previous transmission finished
  if(transmittedFlag) {
    // disable the interrupt service routine while
    // processing the data
    enableInterrupt = false;

    // reset flag
    transmittedFlag = false;

    if (transmissionState == ERR_NONE) {
      // packet was successfully sent
      Serial.println(F("transmission finished!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(transmissionState);
    }

    // wait 2 seconds before transmitting again
    delay(2000);

    // send another one
    Serial.print(F("Sending another packet ... "));

    // you can transmit byte array up to 256 bytes long
    // Packet = {Local adress, Smoke measurement, Flame measurement, Gass measurement, Fire probability * 10000}
    byte byteArr[] = {LOCAL_ADRESS, encoded_smoke.upper_byte, encoded_smoke.lower_byte, encoded_flame.upper_byte, encoded_flame.lower_byte, 
                      encoded_gass.upper_byte, encoded_gass.lower_byte, encoded_prob.upper_byte, encoded_prob.lower_byte};
    int transmissionState = lora.startTransmit(byteArr, MSG_LEN);
    
    // we're ready to send more packets, enable interrupt service routine
    enableInterrupt = true;
  }
}

// this function is called when a complete packet is transmitted by the module
// IMPORTANT: this function MUST be 'void' type and MUST NOT have any arguments!
void setFlag(void) {
  // check if the interrupt is enabled
  if(!enableInterrupt) {
    return;
  }

  // we sent a packet, set the flag
  transmittedFlag = true;
}

byte encodeLowerByte(int num){
  int mask = 255; // bin = '1111 1111'
  int lowerResult = mask & num;
  byte LowerByte = (byte)lowerResult;
  return LowerByte;
}

byte encodeUpperByte(int num){
  int mask = 255; // bin = '1111 1111'
  int shifted_num = num >> 8;
  int lowerResult = mask & shifted_num;
  byte LowerByte = (byte)lowerResult;
  return LowerByte;
}

bool encodeNumber(int num, encodedInTwoBytes *encoded) {
  if (num < 0 || num >= ERR_VALUE) { // It is not possible to decode int in two bytes
    // Error value
    encoded->lower_byte = 0xFF; 
    encoded->upper_byte = 0xFF;
    return false;
  }
  encoded->lower_byte = encodeLowerByte(num);
  encoded->upper_byte = encodeUpperByte(num);
  return true;
}

int decodeUpperByte(byte upper_byte){
  int shifted = (int)upper_byte << 8;
  return shifted;
} 

int decodeNumber(byte lower_byte, byte upper_byte) {
  int decoded_number = (int)lower_byte | decodeUpperByte(upper_byte); // Bitovy OR
  return decoded_number;
}
