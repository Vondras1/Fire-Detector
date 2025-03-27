// quectel m95 send http request to thingspeak ------------------------------------------------------------------------

#include <SoftwareSerial.h>
SoftwareSerial gprsSerial(2,3); // RX, TX
 
#include <String.h>

void ShowSerialData()
{
  while(gprsSerial.available()!=0)
  Serial.write(gprsSerial.read());
  delay(2000); 
}
 
void setup()
{
  gprsSerial.begin(9600);               // the GPRS baud rate   
  Serial.begin(9600);    // the GPRS baud rate 
  delay(1000);
}
 
void loop()
{
      float h = 50; //random(0,100); //dht.readHumidity();
      float t = 20; //random(-20,50); //dht.readTemperature(); 
         
      Serial.print("Temperature = ");
      Serial.print(t);
      Serial.println(" °C");
      Serial.print("Humidity = ");
      Serial.print(h);
      Serial.println(" %");    
      
   
  if (gprsSerial.available())
    Serial.write(gprsSerial.read());
 
  gprsSerial.println("AT");
  delay(1000);
  ShowSerialData();
/*
  gprsSerial.println("AT+CPIN?"); // pin simkarty
  delay(1000);
  ShowSerialData();

  gprsSerial.println("AT+CREG?");
  delay(2000);
  ShowSerialData();
  
  gprsSerial.println("AT+CSQ"); // kvalita signalu
  delay(2000);
  ShowSerialData();
 */
  gprsSerial.println("AT+CGATT?");
  delay(2000);
  ShowSerialData();

  gprsSerial.println("AT+QICSGP=1,\"internet.t-mobile.cz\",\"gprs\",\"gprs\""); //setting the APN
  delay(2000);
  ShowSerialData();

  gprsSerial.println("AT+QIDNSGIP=\"api.thingspeak.com\""); 
  delay(4000);
  ShowSerialData();

  gprsSerial.println("AT+QIDNSIP=1"); 
  delay(400);
  ShowSerialData();

  gprsSerial.println("AT+QIOPEN=\"TCP\",\"api.thingspeak.com\",\"80\"");//start up the connection, IP: 44.194.247.113
  delay(6000);

  gprsSerial.println("AT+QISEND");//begin send data to remote server
  delay(4000);
  ShowSerialData();
  
  String str="GET https://api.thingspeak.com/update?api_key=S8QL4UGSKU4E3NUH&field1=" + String(t) +"&field2="+String(h);
  Serial.println(str);
  gprsSerial.println(str);//begin send data to remote server
  
  delay(4000);
  ShowSerialData();
 
  gprsSerial.println((char)26);//sending,  end of the data transmission/message termination.
  delay(5000);//waitting for reply, important! the time is base on the condition of internet 
  gprsSerial.println();

  gprsSerial.println("AT+QICLOSE");
  delay(2000);
  ShowSerialData();
 
  gprsSerial.println("AT+QIDEACT");
  delay(2000);
  ShowSerialData();
  
} 


