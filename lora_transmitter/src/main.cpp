/*  */

// include the library
#include "Arduino.h"
#include <LoRaLib.h>
#include <SPI.h>
#include "SD.h"
#include "model_real_data.h"
#include <EloquentTinyML.h>

// Set mode. In SD card mode are data transmitted and saved to SD card, 
// in normal mode are transmitted only. Comment out following # define 
// for non-SD mode.
#define SDCARD_MODE

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

// FIRE NET
// Define the number of inputs and outputs for the model
#define NUMBER_OF_NET_INPUTS  3
#define NUMBER_OF_NET_OUTPUTS 1

// Define the size of the tensor arena (used for memory allocation during inference)
// For small models, 2–4 KB is usually enough
#define TENSOR_ARENA_SIZE (2 * 1024)

// Create an instance of the TfLite interpreter for the given model
Eloquent::TinyML::TfLite<NUMBER_OF_NET_INPUTS, NUMBER_OF_NET_OUTPUTS, TENSOR_ARENA_SIZE> ml;


// Device global constatnts
#define LOCAL_ADRESS 0x11
#define PROB_SCALE 10000
#define VOLTAGE_SCALE 1000
#define MSG_LEN 11
#define ERR_VALUE 65535
#define TIME_SPAN 6000
// set to proper value!!! (0.013526 for 4M7/1M7 divider, 3.3V FS, 10 bit ADC)
// for 12,6 V FS and voltage scale 10000 will overflow the 2B value!!!
#define DIVIDER_RATIO 0.01679785627705628 // 4.7M/1.47M resistor divider, 3.3V FS, 10 bit ADC

// Pins sensors
#define FlameSensorPin A3
#define SmokeSensorPin A1
#define GasSensorPin A2
#define BatteryPin A5
// other pins
#define SD_CSPIN 9
#define FireSwitch 8

// Structs
struct encodedInTwoBytes {
  byte upper_byte;
  byte lower_byte;
};

struct averageSensorVals {
  int smoke;
  int flame;
  int gas; 
} average_values;

// Functions declarations
void setFlag(void);
byte encodeLowerByte(int num);
byte encodeUpperByte(int num);
bool encodeNumber(int num, encodedInTwoBytes *encoded);
int decodeUpperByte(byte upper_byte);
int decodeNumber(byte lower_byte, byte upper_byte);
void countMovingAverage(int x, float *avg, float a);
void measureAverageValues(averageSensorVals *average);
void measureBattery(float *vbat);
float getFireProbability();

#ifdef SDCARD_MODE

// SD card related function declaration
int getNumFiles(File dir);
bool writeData(File mf, averageSensorVals *average, int is_fire, float fire_prob);
bool writeHeader(File mf, String header);

// file object for SD card data and string for file name
File myFile;
String full_filename;

#endif

// Global variable
long timeOfLastMeasurement = millis();

// save transmission state between loops
int transmissionState = ERR_NONE;

// flag to indicate that a packet was sent
volatile bool transmittedFlag = true;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;


int freeRAM() {
  // Estimate free RAM memory
  // +---------------------+  <- high addresses
  // |      STACK ↓        |  ← address of stack_dummy
  // |                     |
  // |     Free RAM        |
  // |                     |
  // |      HEAP ↑         |  ← result of malloc(4)
  // +---------------------+  <- low addresses

  // free RAM ≈ address_of_stack - address_of_heap
  char stack_dummy = 0;
  return &stack_dummy - (char*)malloc(4);
}


void setup() {
  Serial.begin(9600);
  
  Serial.print("Estimated free RAM: ");
  Serial.println(freeRAM());
  Serial.print("Model size: ");
  Serial.println(model_tflite_len);

  // Initialize the model from the included model data array
  Serial.println("Starting model init...");
  ml.begin(model_tflite);
  Serial.println("Model is ready!");

  #ifdef SDCARD_MODE
  String filename = "tabor"; // default file name for saving data on SD, can be changed

  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CSPIN)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  
  File root = SD.open("/");

  int num_files = getNumFiles(root);

  full_filename = filename + String(num_files) + String(".txt") ;

  myFile = SD.open(full_filename, FILE_WRITE);

  // if the file opened okay, write header to it:
  if (myFile) {
    // write header to file
    writeHeader(myFile, "smoke,flame,gas,label, prob");
    // close the file:
    myFile.close();

  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening file");
  }
  myFile.close();

  #endif // end of SD card mode

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

  // // Initialize the model from the included model data array
  // Serial.println("Starting model init...");
  // ml.begin(model_tflite);
  // Serial.println("Model is ready!");

  // set pins modes
  pinMode(FlameSensorPin, INPUT_ANALOG);
  pinMode(SmokeSensorPin, INPUT_ANALOG);
  pinMode(GasSensorPin, INPUT_ANALOG);
  pinMode(BatteryPin, INPUT_ANALOG);

  pinMode(FireSwitch, INPUT_PULLUP);

  // setup finished
  Serial.println(F("Setup finished."));
}

void loop() {
  
  if (transmittedFlag and (millis() - timeOfLastMeasurement) >= TIME_SPAN) {
    // Measure current values and count average
    measureAverageValues(&average_values);
    
    // Predict probability of flame
    float fire_prob = getFireProbability();
    // float fire_prob = 0.5;
    int scaled_probability = (int)(fire_prob*PROB_SCALE);
    
    #ifdef SDCARD_MODE // save to sd card only if control variable SDCARD_MODE defined 
      myFile = SD.open(full_filename, FILE_WRITE);

      // if the file opened okay, write to it:
      if (myFile) {
      
        int is_fire = !digitalRead(FireSwitch);
        writeData(myFile, &average_values, is_fire, fire_prob); // write data to SD card
        myFile.close(); // close the file
        Serial.println("Golden label, switch is:");
        Serial.println(is_fire);

      } else {
        // if the file didn't open, print an error:
        Serial.println("error opening file");
      }
      //myFile.close();
      #endif // end of SD card mode

    float v_bat;
    measureBattery(&v_bat); // #TODO: maybe we dont need to measure battery voltage every 6 seconds
    //Serial.print("Battery voltage: ");
    Serial.println(v_bat);

    int scaled_vbat = (int)(v_bat*VOLTAGE_SCALE);


    Serial.print("Input: ");
    Serial.print(average_values.smoke); Serial.print(", ");
    Serial.print(average_values.flame); Serial.print(", ");
    Serial.print(average_values.gas);
    Serial.print("  =>  Fire probability: ");
    Serial.println(fire_prob);

    // Encode int into 2 bytes
    encodedInTwoBytes encoded_smoke;
    encodedInTwoBytes encoded_flame;
    encodedInTwoBytes encoded_gas;
    encodedInTwoBytes encoded_prob;
    encodedInTwoBytes encoded_vbat;
    bool res_smoke = encodeNumber(average_values.smoke, &encoded_smoke);
    bool res_flame = encodeNumber(average_values.flame, &encoded_flame);
    bool res_gas = encodeNumber(average_values.gas, &encoded_gas);
    bool res_prob = encodeNumber(scaled_probability, &encoded_prob);
    bool res_vbat = encodeNumber(scaled_vbat, &encoded_vbat);

    // disable the interrupt service routine while processing the data
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

    // send another one
    Serial.print(F("Sending another packet ... "));

    // you can transmit byte array up to 256 bytes long
    // Packet = {Local adress, Smoke measurement, Flame measurement, Gas measurement, Fire probability * 10000, battery voltage*10000}
    byte byteArr[] = {LOCAL_ADRESS, encoded_smoke.upper_byte, encoded_smoke.lower_byte, encoded_flame.upper_byte, encoded_flame.lower_byte, 
                      encoded_gas.upper_byte, encoded_gas.lower_byte, encoded_prob.upper_byte, encoded_prob.lower_byte, encoded_vbat.upper_byte,
                    encoded_vbat.lower_byte};
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
  lora.sleep();
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

void countMovingAverage(int x, float *avg, float a=0.8) {
  *avg = a*x + (1-a)*(*avg);
}

void measureAverageValues(averageSensorVals *average) {
  float avg_smoke = 0;
  float avg_flame = 0;
  float avg_gas = 0;

  int i = 0;
  int num_iter = 10;
  while (i < num_iter) {
    avg_smoke += analogRead(SmokeSensorPin);
    avg_flame += analogRead(FlameSensorPin);
    avg_gas += analogRead(GasSensorPin);
    i += 1;
    delay(10);
  }
  
  average->smoke = (int)(avg_smoke/num_iter);
  average->flame = (int)(avg_flame/num_iter);
  average->gas = (int)(avg_gas/num_iter);

  timeOfLastMeasurement = millis();
}

void measureBattery(float *vbat){
  const int num_iter = 4;
  int sum_vbat = 0;
  for (int i = 0; i < num_iter; i++){
    sum_vbat += analogRead(BatteryPin);
    delay(5);
  }
  *vbat = ((float)sum_vbat/(float)num_iter);
  *vbat = *vbat*DIVIDER_RATIO;
  //Serial.println(*vbat);
}

float getFireProbability(){
  // Store input values in an array as expected by EloquentTinyML
  float input[NUMBER_OF_NET_INPUTS] = { average_values.smoke, average_values.flame, average_values.gas };

  // Predict output
  float predicted = ml.predict(input);

  return predicted;
}

// SD card related functions: ---------------------
#ifdef SDCARD_MODE

int getNumFiles(File dir) {
  int cntr = 0;
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      break;
    }
    cntr++;
    entry.close();
  }
  return cntr;

}

bool writeData(File mf, averageSensorVals *average, int is_fire, float fire_prob){
  mf.print(average->smoke);
  mf.print(',');
  mf.print(average->flame);
  mf.print(',');
  mf.print(average->gas);
  mf.print(',');
  mf.print(is_fire);
  mf.print(',');
  mf.print(fire_prob);
  mf.print('\n');
  return true;
}

bool writeHeader(File mf, String header){
  mf.println(header);
  return true;
}

#endif