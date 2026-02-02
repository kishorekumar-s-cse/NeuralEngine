#include <Arduino.h>
#include <NeuralEngine.h>


void setup() {
Serial.begin(9600);
delay(2000);
set_ssid("hello");
set_pwd("hello123");

Serial.println("Hello world");
cl("Check WiFI");
}

void loop() {
  cl("Check WiFI");
  delay(10000);
  // put your main code here, to run repeatedly:
}

