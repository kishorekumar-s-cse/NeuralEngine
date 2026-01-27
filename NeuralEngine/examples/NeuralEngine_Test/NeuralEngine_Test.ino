#include <WiFi.h>
#include <NeuralEngine.h>

const char* ssid = "hello";
const char* password = "hello123";

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  cl("NeuralEngine FINAL TEST");
}

void loop() {}
