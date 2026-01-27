#include "NeuralEngine.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h> 
#include <esp_system.h>
#include <EEPROM.h>
#include <ctype.h>
#include <string.h>

/* ================= API CONFIG ================= */

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

/* ================= URL ENCODE ================= */
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
            // 200 = Successfull
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

/* ================= SERIAL UTILS ================= */
void clearSerialBuffer(void) {
    while (Serial.available()) Serial.read();
}

static void readLine(char *buf, int maxLen) {
    int i = 0;
    while (!Serial.available());
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') break;
        if (i < maxLen - 1) buf[i++] = c;
    }
    buf[i] = '\0';
}

int readInt(void) {
    clearSerialBuffer();
    char buf[16];
    readLine(buf, sizeof(buf));
    return atoi(buf);
}

void readString(char *buffer, int maxLen) {
    clearSerialBuffer();
    readLine(buffer, maxLen);
}

char readChar(void) {
    clearSerialBuffer();
    while (!Serial.available());
    return Serial.read();
}

float readFloat(void) {
    clearSerialBuffer();
    char buf[20];
    readLine(buf, sizeof(buf));
    return atof(buf);
}

double readDouble(void) {
    clearSerialBuffer();
    char buf[32];
    readLine(buf, sizeof(buf));
    return atof(buf);
}

/* ================= EEPROM UTILS ================= */
static void initEEP() {
    static bool init = false;
    if (!init) {
        if (!EEPROM.begin(EEPROM_SIZE)) {
            Serial.println("Failed to initialise EEPROM");
            return;
        }
        init = true;
    }
}

static int readLength() {
    int len = 0;
    for (int i = 0; i < 4; i++)
        len |= EEPROM.read(i) << (8 * i);
    return len;
}

static void saveLength(int length) {
    for (int i = 0; i < 4; i++)
        EEPROM.write(i, (length >> (8 * i)) & 0xFF);
    EEPROM.commit();
}

void set_item(const char *key, const char *value) {
    initEEP();
    int len = readLength();
    
    // Safety check for crazy lengths (uninitialized EEPROM)
    if (len < 0 || len > EEPROM_SIZE) len = 0;

    int pos = START_ADDR + len;
    int needed = strlen(key) + strlen(value) + 2;
    
    if (pos + needed >= EEPROM_SIZE) {
        Serial.println("EEPROM Full!");
        return;
    }

    while (*key) EEPROM.write(pos++, *key++);
    EEPROM.write(pos++, '\0');

    while (*value) EEPROM.write(pos++, *value++);
    EEPROM.write(pos++, '\0');

    saveLength(len + needed);
}

char* get_item(const char *key) {
    static char out[64];
    initEEP();

    int len = readLength();
    if (len < 0 || len > EEPROM_SIZE) return NULL;
    
    int pos = START_ADDR;

    while (pos < START_ADDR + len) {
        char temp[32];
        int i = 0;

        // Read Key
        while (EEPROM.read(pos) != '\0' && i < 31)
            temp[i++] = EEPROM.read(pos++);
        temp[i] = '\0';
        pos++; // Skip null

        // Check Key match
        if (strcmp(temp, key) == 0) {
            i = 0;
            // Read Value
            while (EEPROM.read(pos) != '\0' && i < 63)
                out[i++] = EEPROM.read(pos++);
            out[i] = '\0';
            return out;
        }

        // Skip Value if key didn't match
        while (EEPROM.read(pos) != '\0') pos++;
        pos++; // Skip null
    }
    return NULL;
}

void clearEEP(void) {
    initEEP();
    for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
    saveLength(0);
}