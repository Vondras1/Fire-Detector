/*
  SD card read/write

  This example shows how to read and write data to and from an SD card file
  The circuit:
   SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4 (for MKRZero SD: SDCARD_SS_PIN)

  created   Nov 2010
  by David A. Mellis
  modified 9 Apr 2012
  by Tom Igoe

  This example code is in the public domain.

*/

#include "Arduino.h"
#include <SPI.h>
#include "SD.h"

#define CSPIN 9

//functions 
int getNumFiles(File dir);
bool writeData(File mf, int smoke, int flame, int gas);
bool writeHeader(File mf, String header);

File myFile;
String full_filename;

void setup() {
  String filename = "tabor"; // default file name, can be changed

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


  Serial.print("Initializing SD card...");

  if (!SD.begin(CSPIN)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  
  File root = SD.open("/");

  int num_files = getNumFiles(root);

  Serial.println("Counter: " + String(num_files));

  full_filename = filename + String(num_files) + String(".txt") ;
  
  Serial.println(full_filename);

  myFile = SD.open(full_filename, FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing header to " + full_filename);
    // write header to file
    writeHeader(myFile, "smoke,flame,gas");
    // close the file:
    myFile.close();
    Serial.println(" done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening file");
  }
  myFile.close();

  /*
  // re-open the file for reading:
  myFile = SD.open(full_filename);
  if (myFile) {
    Serial.println(full_filename + ": ");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening " + full_filename);
  }
  */
  
}

void loop() {

  myFile = SD.open(full_filename, FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to " + full_filename);
    int smoke = random(1,1024), flame = random(1,1024), gas = random(1,1024);
    // write to file
    writeData(myFile, smoke, flame, gas);
    // close the file:
    myFile.close();
    Serial.println(" done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening file");
  }
  myFile.close();

  delay(1000);
}

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

bool writeData(File mf, int smoke, int flame, int gas){
  mf.print(smoke);
  mf.print(',');
  mf.print(flame);
  mf.print(',');
  mf.print(gas);
  mf.print('\n');
  return true;
}

bool writeHeader(File mf, String header){
  mf.println(header);
  return true;
}