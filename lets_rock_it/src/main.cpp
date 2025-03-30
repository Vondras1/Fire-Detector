#include <Arduino.h>

// put function declarations here:
int countAverage(int, float a, bool first);
bool first = true;

#define FlameSensorPin A0
#define SmokeSensorPin A1
#define GassSensorPin A2


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(FlameSensorPin, INPUT_ANALOG);
  pinMode(SmokeSensorPin, INPUT_ANALOG);
  pinMode(GassSensorPin, INPUT_ANALOG);
}

void loop() {
  // put your main code here, to run repeatedly:
  // digitalWrite(LED_BUILTIN, HIGH);
  // delay(500);
  // digitalWrite(LED_BUILTIN, LOW);
  // delay(500);

  int measuredSmoke = analogRead(SmokeSensorPin);
  int avg_smoke = countAverage(measuredSmoke, 0.8, first);
  Serial.print("Smoke sensor = ");
  Serial.println(avg_smoke);

  // int measuredGass = analogRead(GassSensorPin);
  // int avg_gass = countAverage(measuredGass, 0.8, first);
  // Serial.print("Gass sensor = ");
  // Serial.println(avg_gass);

  // int measuredFlame = analogRead(FlameSensorPin);
  // int avg_flame = countAverage(measuredFlame, 0.8, first);
  // Serial.print("Flame sensor = ");
  // Serial.println(avg_flame);

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