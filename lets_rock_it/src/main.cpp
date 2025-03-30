#include <Arduino.h>

// put function declarations here:
int countAverage(int, float a, bool first);
void measureBattery(int *vbat);
bool first = true;


#define FlameSensorPin A3
#define BatteryPin A4
#define DIVIDER_RATIO 0.0054149 // 1M/1.47M resistor divider, 3.3V FS, 10 bit ADC

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(A0, INPUT_ANALOG);
  pinMode(BatteryPin, INPUT_ANALOG);
}

void loop() {
  // put your main code here, to run repeatedly:
  // digitalWrite(LED_BUILTIN, HIGH);
  // delay(500);
  // digitalWrite(LED_BUILTIN, LOW);
  // delay(500);

  int v_bat;
  measureBattery(&v_bat);
  Serial.print("Battery voltage: ");
  Serial.println((float)v_bat*DIVIDER_RATIO);

  delay(1000);
}

// put function definitions here:
int countAverage(int x, float a=0.8, bool first=false) {
  static float avg;
  if (first){avg = x;}
  avg = a*x + (1-a)*avg;
  return (int)avg;
}

void measureBattery(int *vbat){
  const int num_iter = 4;
  int sum_vbat = 0;
  for (int i = 0; i < num_iter; i++){
    sum_vbat += analogRead(BatteryPin);
    delay(2);
  }
  *vbat = sum_vbat/num_iter;
  //Serial.println(analogRead(BatteryPin));
}