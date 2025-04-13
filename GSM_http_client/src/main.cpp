// quectel m95 send http request to thingspeak ------------------------------------------------------------------------
/* send data over gprs connection to thingspeak server, 
* https://thingspeak.mathworks.com/channels/2881290/private_show
* use http GET request with url containing data to send
* one cycle (connect to network, to server, send request, close connection) 
* lasts about 1 second, at the end of the loop() is some delay() because 
* free Thingspeak can handle only data update in 15 seconds... 
*/
#include <Arduino.h>
#include <String.h>
#include <SoftwareSerial.h>

// function declarations:
bool WaitForResponse();
bool SendCommand(String command);
bool SendCommandCustomResp(String command, String resp);
bool WaitForCustomResponse(String resp);

SoftwareSerial gprsSerial(2, 3); // RX, TX

void setup()
{
  gprsSerial.begin(9600); // the GPRS baud rate
  Serial.begin(9600);     // the GPRS baud rate
  while (!Serial || !gprsSerial)
    ; // wait until Serial streams are ready
  delay(200);
}

void loop()
{
  float h = random(0,100); //dht.readHumidity();
  float t = random(-20,50); //dht.readTemperature();

  Serial.print("Temperature = ");
  Serial.print(t);
  Serial.println(" °C");
  Serial.print("Humidity = ");
  Serial.print(h);
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

  String str = "GET https://api.thingspeak.com/update?api_key=S8QL4UGSKU4E3NUH&field1=" + String(t) + "&field2=" + String(h);
  gprsSerial.println(str);
  gprsSerial.println((char)26); // end of message
  WaitForCustomResponse("SEND");
  WaitForCustomResponse("CLOSED"); // wait until http connection close
  SendCommandCustomResp("AT+QIDEACT", "DEACT"); // deactivate gprs context
  delay(10000);
}

// fuction definitions ---------------------------
//...

// inspired by https://forum.arduino.cc/t/purpose-of-delay-in-sim800l-at-commands/1024205/8

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

/*
void SendDataTCP(const char *, float data1, float data2)
{
  Serial.println("Sending data");

  Serial.print("Sending: ");
  Serial.print("AT+QISEND=\"");
  Serial.print();
  Serial.println("\"(CR)");
  Serial.print(message); // The SMS text you want to send
  Serial.println("(EM)");

  gprsSerial.print("AT+CMGS=\"");  // Send SMS
  gprsSerial.print(number);
  gprsSerial.print("\"\r"); // NOTE: Command ends with CR, not CRLF
  gprsSerial.print(message); // The SMS text you want to send
  gprsSerial.write(26); // ASCII EndMessage (EM) CTRL+Z character

  WaitForResponse();
}
*/