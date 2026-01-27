#include <WiFi.h>
#include <NeuralEngine.h>

const char* ssid = "hello";
const char* password = "hello123";

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  // // Test logging
  // cl("NeuralEngine Library Test started");
  // cl("This is a test log message");
  // cl("Engine Test");

  int a=1,b=0,c=2;
  if(a++ && ++b || ++c){
    cl(a,b,c);
  }
}

void loop() {

}
