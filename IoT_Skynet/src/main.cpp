#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <NeuralEngine.h>

void setup() {
  Serial.begin(9600);
  delay(1000);
  startBluetooth();
}
void loop() {
  // main application logic here
}
