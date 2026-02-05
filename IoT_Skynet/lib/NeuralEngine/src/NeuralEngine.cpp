#include "NeuralEngine.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h> 
#include <esp_system.h>
#include <EEPROM.h>
#include <ctype.h>
#include <string.h>
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
struct WifiNode {
    String ssid;
    String pwd;
    WifiNode* next;
};

WifiNode* head = nullptr;
WifiNode* currentAttempt = nullptr;

bool hasInternet() {
    if (WiFi.status() != WL_CONNECTED) return false;
    
    HTTPClient http;
    WiFiClient client;
    http.setConnectTimeout(3000); 
    
    if (http.begin(client, "http://connectivitycheck.gstatic.com/generate_204")) {
        int code = http.GET();
        http.end();
        return (code == 204 || code == 200);
    }
    return false;
}

void addSSIDNode(String ssidStr) {
    WifiNode* newNode = new WifiNode();
    newNode->ssid = ssidStr;
    newNode->pwd = "";
    
    if (head == nullptr) {
        head = newNode;
        head->next = head; 
    } else {
        WifiNode* temp = head;
        while (temp->next != head) {
          // Serial.printf("%s",temp->ssid);
          temp = temp->next;
        }
        temp->next = newNode;
        newNode->next = head; 
    }
}

void CLLdisplay(){
if (head == nullptr) {
        return;
    } else {
      WifiNode* temp=head;

        while(temp->next!=head){
          Serial.printf("%s\t",temp->ssid);
          temp=temp->next;
          
        } Serial.printf("%s\t",temp->ssid);
    }
}

void updatePwdNode(String pwdStr, int index) {
    if (head == nullptr) return;
    WifiNode* temp = head;
    int count = 0;
    do {
        if (count == index) {
            temp->pwd = pwdStr;
            return;
        }
        temp = temp->next;
        count++;
    } while (temp != head);
}


void deleteAllNodes() {
    if (head == nullptr) return;

    WifiNode* current = head->next;
    WifiNode* temp;

    while (current != head) {
        temp = current;
        current = current->next;
        delete temp;
    }

    delete head;
    head = nullptr;
}


void WiFiManagerTask(void * parameter) {
    if (head == nullptr) vTaskDelete(NULL);
    if (currentAttempt == nullptr) currentAttempt = head;

    for(;;) {
        if (WiFi.status() != WL_CONNECTED || !hasInternet()) {
            
            Serial.printf("\n[Manager] Connection lost or refused. Trying: %s\n", currentAttempt->ssid.c_str());

            WiFi.disconnect(true);
            vTaskDelay(1000 / portTICK_PERIOD_MS); 
            
            WiFi.mode(WIFI_STA);
            WiFi.begin(currentAttempt->ssid.c_str(), currentAttempt->pwd.c_str());

            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                vTaskDelay(500 / portTICK_PERIOD_MS); 
                attempts++;
            }

            // Verify Result
            if (WiFi.status() == WL_CONNECTED && hasInternet()) {
                 Serial.printf("[Manager] SUCCESS: Connected to %s\n", currentAttempt->ssid.c_str());
            } else {
                // If this node failed, move to the next one in the circle immediately
                Serial.println("[Manager] Node failed/refused. Rotating to next SSID...");
                currentAttempt = currentAttempt->next;
                // No long delay here; we want to try the next one ASAP
                vTaskDelay(100 / portTICK_PERIOD_MS);
                continue; 
            }
        }
        
        vTaskDelay(10000 / portTICK_PERIOD_MS); 
    }
}

void set_ssid(const char* ssid_ptr) {
    String raw = String(ssid_ptr);
    int start = 0;
    int end = raw.indexOf('^');
    while (end != -1) {
        addSSIDNode(raw.substring(start, end));
        start = end + 1;
        end = raw.indexOf('^', start);
    }
    addSSIDNode(raw.substring(start));
}

void set_pwd(const char* pwd_ptr) {
    String raw = String(pwd_ptr);
    int start = 0;
    int end = raw.indexOf('^');
    int idx = 0;
    while (end != -1) {
        updatePwdNode(raw.substring(start, end), idx++);
        start = end + 1;
        end = raw.indexOf('^', start);
    }
    updatePwdNode(raw.substring(start), idx);

    xTaskCreatePinnedToCore(WiFiManagerTask, "WiFi_Mgr", 25000, NULL, 1, NULL, 0);
    Serial.println("[System] Background WiFi Manager Active.");
}




/* ================= API CONFIG ================= */
// Note: We will force HTTPS in the code below
static const char* CL_API = "http://www.skynetbee.com/skynetbee/api/iot/esp32/logs_get.php";

/* ================= FIXED PARAMS ================= */
static const char* ROOM_NAME = "Stark_Industries";
static const char* LOG_TYPE = "info";
static const char* AREA = "IoT";

/* ================= EEPROM CONFIG ================= */
#define EEPROM_SIZE 1024
#define START_ADDR 4

/* ================= ESP HARDWARE ID ================= */
String getESP_ID() {
    uint64_t chipid = ESP.getEfuseMac();
    char id[17];
    sprintf(id, "%04X%08X",
            (uint16_t)(chipid >> 32),
            (uint32_t)chipid);
    return String(id);
}

/* URL ENCODE */
static String _cl_encode(const String &s) {
    String out = "";
    char buf[4];
    for (char c : s) {
        if (isalnum(c) || c == '_' || c == '-' || c == '.')
            out += c;
        else {
            sprintf(buf, "%%%02X", (unsigned char)c);
            out += buf;
        }
    }
    return out;
}

/* ================= CORE SENDER (FIXED) ================= */
void _cl_send(
    const char* file,
    const char* func,
    int line,
    const String &msg
) {
    if (msg.length()) Serial.println(msg);
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[ERR]: WiFi NOT connected. Cannot send log.");
        return;
    }
    WiFiClientSecure client;
    client.setInsecure(); // Skip certificate validation (fixes SSL handshake errors)

    // Construct URL Params
    // %5B%5D to prevent 400 Bad Request
    String urlParams = 
        "?room_name=" + String(ROOM_NAME) +
        "&esp_id=" + getESP_ID() +
        "&file_name=" + _cl_encode(file) +
        "&function_name=" + String(func) +
        "&line_number=" + String(line) +
        "&log_type=" + String(LOG_TYPE) +
        "&message=" + _cl_encode(msg) +
        "&area=" + String(AREA) +
        "&pin_device_config=%5B%5D" + 
        "&deviceanduserinfo=%5B%5D";

    // Full URL & Force HTTPS
    String fullUrl = String(CL_API) + urlParams;
    fullUrl.replace("http://", "https://"); // Ensure we use SSL

    // Check URL in Serial Port
    // Serial.println(fullUrl);

    HTTPClient http;
    
    // Begin Connection
    if (http.begin(client, fullUrl)) {
        
        // Handle Redirects (301/302) automatically
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        
        int httpCode = http.GET();

        // Check Response
        if (httpCode > 0) {
            // 200 = OK
            if (httpCode != 200) {
                Serial.print("[API ERR] Code: ");
                Serial.println(httpCode);
                Serial.println(http.getString()); // Print server error message
            }
        } else {
            Serial.print("[CONN ERR]: ");
            Serial.println(http.errorToString(httpCode).c_str());
        }
        
        http.end();
    } else {
        Serial.println("[ERR] Unable to connect to server");
    }
}

// Bluetooth Integrated with WiFi BLE 

#define SERVICE_UUID "12345678-1234-1234-1234-1234567890ab"
#define CHAR_UUID    "abcd1234-ab12-cd34-ef56-abcdef123456"

/* ================= BLE CALLBACK ================= */
class CharCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {

    std::string rx = c->getValue();
    const char *cstr = rx.c_str();

    Serial.print("Received: ");
    Serial.println(cstr);

    deleteAllNodes();
    set_ssid(cstr);
    CLLdisplay();
  }
};

/* ================= BLE INIT FUNCTION ================= */
void startBluetooth() {

  BLEDevice::init("ESP32_BLE");

  BLEServer *server = BLEDevice::createServer();
  BLEService *service = server->createService(SERVICE_UUID);

  BLECharacteristic *ch = service->createCharacteristic(
    CHAR_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE
  );

  ch->setCallbacks(new CharCallbacks());
  ch->addDescriptor(new BLE2902());

  service->start();

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(true);

  BLEDevice::startAdvertising();
  Serial.println("Bluetooth started");
}

//End BL


