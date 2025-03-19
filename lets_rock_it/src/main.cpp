#include <Arduino.h>

// put function declarations here:
int countAverage(int, float a, bool first);
bool first = true;

#define FlameSensorPin A3

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(A0, INPUT_ANALOG);
}

void loop() {
  // put your main code here, to run repeatedly:
  // digitalWrite(LED_BUILTIN, HIGH);
  // delay(500);
  // digitalWrite(LED_BUILTIN, LOW);
  // delay(500);

  int measuredValue = analogRead(FlameSensorPin);
  int avg = countAverage(measuredValue, 0.8, first);
  Serial.println(avg);

  first = false;
  delay(100);
}

// put function definitions here:
int countAverage(int x, float a=0.8, bool first=false) {
  static float avg;
  if (first){avg = x;}
  avg = a*x + (1-a)*avg;
  return (int)avg;
}