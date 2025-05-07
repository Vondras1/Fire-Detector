/*
   For more detailed information, see the LoRaLib Wiki
   https://github.com/jgromes/LoRaLib/wiki

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/LoRaLib/
*/

// include the library
#include "Arduino.h"
#include <LoRaLib.h>
#include <String.h>
#include <SoftwareSerial.h>

#define NODE_LOCAL_ADRESS 0x11
#define PROB_SCALE 10000
#define VOLTAGE_SCALE 1000
#define MSG_LEN 11
#define ERR_VAL 65535 // max value of received integer, is treated as error value

// pin define
# define GSM_POWERKEY_PIN 8
# define GSM_STATUS_PIN 9

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

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

//structs
struct receivedMsg{
  byte transm_id;
  int flame;
  int gas;
  int smoke;
  int prob;
  int vbat;
};

// creating instance of receivedMsg struct, global
receivedMsg msg;

// Functions declarations
int decodeUpperByte(byte upper_byte);
int decodeNum(byte lower_byte, byte upper_byte);
bool readMsg(byte arr[MSG_LEN], receivedMsg *msg);
void printMsg(receivedMsg msg);
// GSM
void gsm_module_init();
bool WaitForResponse();
bool SendCommand(String command);
bool SendCommandCustomResp(String command, String resp);
bool WaitForCustomResponse(String resp);

//SoftwareSerial gprsSerial(2, 3); // RX, TX
SoftwareSerial gprsSerial(6, 7); // RX, TX

bool newData = false;

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
  gprsSerial.begin(9600); // the GPRS baud rate

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
  pinMode(GSM_STATUS_PIN, INPUT);
  pinMode(GSM_POWERKEY_PIN, OUTPUT);
  gsm_module_init();

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
  if(receivedFlag) {
    // disable the interrupt service routine while processing the data
    enableInterrupt = false;

    // reset flag
    receivedFlag = false;
    newData = true;
    
    // buffer - byte array for received data
    byte byteArr[MSG_LEN];
    int state = lora.readData(byteArr, MSG_LEN);
    
    if (state == ERR_NONE) {
      // packet was successfully received
      Serial.println(F("Received packet!"));

      // print data of the packet
      Serial.println("Bytearray: ");
      for(auto i: byteArr){
        Serial.print(i);
        Serial.print(", ");
      }
      Serial.print("\n");
      
      // decode the incoming message form byte array to receivedMsg struct
      bool success = readMsg(byteArr, &msg);

      // print message data to Serial monitor
      printMsg(msg);

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

  if (newData){
    newData = false;

    // GSM
    float v_bat = float(msg.vbat)/VOLTAGE_SCALE;
    float prob = float(msg.prob)/PROB_SCALE;

    Serial.print("Flame sensor = ");
    Serial.print(msg.flame);
    Serial.println(" °C");
    Serial.print("Probability = ");
    Serial.print(prob);
    Serial.println(" %");

    if (gprsSerial.available())
      Serial.write(gprsSerial.read());

    SendCommand("AT");

    // SendCommand("AT+CPIN?"); // SIM pin
    // SendCommand("AT+CREG?"); 
    // SendCommand("AT+CSQ"); // signal quality
    
    SendCommand("AT+CGATT?");
    
    // start gprs connection
    SendCommand("AT+QICSGP=1,\"internet.t-mobile.cz\",\"gprs\",\"gprs\""); 
    
    // set next QIOPEN command to access through domain name (not IP)
    SendCommand("AT+QIDNSIP=1");                                           
    
    // connect to server
    SendCommand("AT+QIOPEN=\"TCP\",\"api.thingspeak.com\",\"80\"");        
    
    // wait for response with connection info
    WaitForCustomResponse("CONNECT");                                      
    
    // begin http GET request to remote server
    SendCommandCustomResp("AT+QISEND", ">");    

    String str = "GET https://api.thingspeak.com/update?api_key=S8QL4UGSKU4E3NUH&field1=" + String(msg.flame) + "&field2=" + String(msg.smoke) + "&field3=" + String(msg.gas) + "&field4=" + String(prob) + "&field5=" + String(v_bat);
    gprsSerial.println(str);
    gprsSerial.println((char)26); // end of message
    WaitForCustomResponse("SEND");
    WaitForCustomResponse("CLOSED"); // wait until http connection close
    SendCommandCustomResp("AT+QIDEACT", "DEACT"); // deactivate gprs context
    // delay(10000);
  }

  delay(50);
}

int decodeUpperByte(byte upper_byte){
  int shifted = (int)upper_byte << 8;
  return shifted;
} 

int decodeNum(byte lower_byte, byte upper_byte) {
  int decoded_number = (int)lower_byte | decodeUpperByte(upper_byte);
  return decoded_number;
}

bool readMsg(byte arr[MSG_LEN], receivedMsg *msg){
  msg->transm_id = arr[0];
  msg->smoke = decodeNum(arr[2], arr[1]);
  msg->flame = decodeNum(arr[4], arr[3]);
  msg->gas = decodeNum(arr[6], arr[5]);
  msg->prob = decodeNum(arr[8], arr[7]);
  msg->vbat = decodeNum(arr[10], arr[9]);
  if(msg->flame >= ERR_VAL || msg->gas >= ERR_VAL||msg->smoke >= ERR_VAL||msg->prob >= ERR_VAL){
    return false;
  }
  return true; 
}

void printMsg(receivedMsg msg){
  Serial.print(F("transmitter ID: "));
  Serial.print(msg.transm_id);
  Serial.print(F(", smoke: "));
  Serial.print(msg.smoke);
  Serial.print(F(", flame: "));
  Serial.print(msg.flame);
  Serial.print(F(", gas: "));
  Serial.print(msg.gas);
  Serial.print(F(", prob: "));
  Serial.print(msg.prob);
  Serial.print(F(", vbat: "));
  Serial.print(msg.vbat);
  Serial.println();
}

void gsm_module_init(){
  
  
  if(!digitalRead(GSM_STATUS_PIN)){
    digitalWrite(GSM_POWERKEY_PIN, HIGH);
    delay(2000);
    digitalWrite(GSM_POWERKEY_PIN, LOW);
  }
}


bool WaitForResponse()
{
  unsigned long startTime = millis();
  while (millis() - startTime < 5000)
  {
    String reply = gprsSerial.readStringUntil('\n');
    if (reply.length() > 0)
    {
      Serial.print("Received: \"");
      Serial.print(reply);
      Serial.println("\"");

      if (reply.startsWith("OK"))
        return true;

      if (reply.startsWith("ERROR"))
        return false;
    }
  }
  Serial.println("Did not receive OK.");
  return false;
}

bool SendCommand(String command)
{
  Serial.print("Sending command: \"");
  Serial.print(command);
  Serial.println("\"");

  gprsSerial.print(command);
  gprsSerial.print("\r\n");

  return WaitForResponse();
}

bool SendCommandCustomResp(String command, String resp)
{
  Serial.print("Sending command: \"");
  Serial.print(command);
  Serial.println("\"");

  gprsSerial.print(command);
  gprsSerial.print("\r\n");

  return WaitForCustomResponse(resp);
}

bool WaitForCustomResponse(String resp)
{
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) // TIMEOUT 5 SECONDS!
  {
    String reply = gprsSerial.readStringUntil('\n');
    if (reply.length() > 0)
    {
      Serial.print("Received: \"");
      Serial.print(reply);
      Serial.println("\"");

      if (reply.startsWith(resp))
        return true;

      if (reply.startsWith("ERROR"))
        return false;
    }
  }
  Serial.print("Did not receive ");
  Serial.println(resp);
  return false;
}