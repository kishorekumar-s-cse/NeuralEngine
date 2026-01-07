#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

// char* get_item(const char* input)
// {
//     if (input == 0)
//         return 0;

//     int len = 0;
//     while (input[len] != '\0')
//         len++;

//     char* buffer = (char*)malloc(len + 1);
//     if (buffer == 0)
//         return 0;

//     for (int i = 0; i < len; i++)
//         buffer[i] = input[i];

//     buffer[len] = '\0';
//     return buffer;
// }


// void cl(const char *fmt, ...) {
//     va_list args;
//     va_start(args, fmt);
//     vprintf(fmt, args);   // print to UART/stdout
//     va_end(args);
//     printf("\n");         // new line like Serial.println()
// }
// http://10.39.67.5/cl_func/cl_func/esp32_insert.php


// WIFI Config
static const char* CL_SSID     = "hello";
static const char* CL_PASSWORD = "hello123";
static const char* CL_SERVER  = "http://10.39.67.5/cl_func/cl_func/esp32_insert.php";


static bool _cl_wifi_connected = false;

static void _cl_wifi_init() {
    if (_cl_wifi_connected) return;

    WiFi.begin(CL_SSID, CL_PASSWORD);

    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nConnected!");
    _cl_wifi_connected = true;
}


template<typename... Args>
void cl(Args... args) {
    _cl_wifi_init();   // ensure WiFi is connected

    String finalMsg = "";

    (finalMsg += ... += (String(args) + " "));

    finalMsg.trim();

    Serial.println(finalMsg);
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(CL_SERVER);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        String postData = "message=" + finalMsg;
        http.POST(postData);

        http.end();
    }
}


// --------- CLEAR SERIAL BUFFER ---------
static void clearSerialBuffer(void) {
    while (Serial.available()) {
        char c = Serial.read();
        if (c != '\n' && c != '\r')
            continue;
    }
}

// --------- COMMON STRING LINE READER ---------
static void readLine(char *buf, int maxLen) {
    int i = 0;
    char c;

    while (!Serial.available());

    while (Serial.available()) {
        c = Serial.read();

        if (c == '\r' || c == '\n') {
            buf[i] = '\0';
            break;
        }

        if (i < maxLen - 1) {
            buf[i++] = c;
            Serial.write(c); // echo
        }
    }

    Serial.println();
    buf[i] = '\0';
}

// --------- INT ---------
int readInt(void) {
    clearSerialBuffer();
    char buf[16];
    readLine(buf, sizeof(buf));
    return atoi(buf);
}

// --------- STRING / CHAR ARRAY ---------
void readString(char *buffer, int maxLen) {
    clearSerialBuffer();
    readLine(buffer, maxLen);
}

// --------- SINGLE CHAR ---------
char readChar(void) {
    clearSerialBuffer();
    char c = '\0';

    while (!Serial.available());

    while (Serial.available()) {
        c = Serial.read();
        if (c != '\n' && c != '\r') {
            Serial.write(c);
            break;
        }
    }

    Serial.println();
    return c;
}

// --------- FLOAT ---------
float readFloat(void) {
    clearSerialBuffer();
    char buf[20];
    readLine(buf, sizeof(buf));
    return (float)atof(buf);
}

// --------- DOUBLE ---------
double readDouble(void) {
    clearSerialBuffer();
    char buf[32];
    readLine(buf, sizeof(buf));
    return atof(buf);
}



//EEPROM To Store the data as key and value with char array
#include <EEPROM.h>

#define EEPROM_SIZE 1024
#define START_ADDR 4

static void initEEP() {
    static int init = 0;
    if (!init) {
        EEPROM.begin(EEPROM_SIZE);
        init = 1;
    }
}

static void saveLength(int length) {
    EEPROM.write(0, (length >> 0) & 0xFF);
    EEPROM.write(1, (length >> 8) & 0xFF);
    EEPROM.write(2, (length >> 16) & 0xFF);
    EEPROM.write(3, (length >> 24) & 0xFF);
    EEPROM.commit();
}

static int readLength() {
    int len = 0;
    len |= EEPROM.read(0) << 0;
    len |= EEPROM.read(1) << 8;
    len |= EEPROM.read(2) << 16;
    len |= EEPROM.read(3) << 24;
    return len;
}

// Set Key-Value
void set_item(const char *key, const char *value) {
    initEEP();

    int len = readLength();
    int pos = START_ADDR + len;

    int needed = strlen(key) + strlen(value) + 2;
    if (pos + needed >= EEPROM_SIZE) return;

    for (int i = 0; key[i]; i++) EEPROM.write(pos++, key[i]);
    EEPROM.write(pos++, '\0');

    for (int i = 0; value[i]; i++) EEPROM.write(pos++, value[i]);
    EEPROM.write(pos++, '\0');

    saveLength(len + needed);
    EEPROM.commit();
}

// Get Value by Key
char* get_item_simple(const char *key) {
    static char out[64]; // static persistent buffer

    initEEP(); // ensure EEPROM is initialized

    int len = readLength();
    int pos = START_ADDR;
    char tempKey[32];

    while (pos < START_ADDR + len) {

        int i = 0;
        while (EEPROM.read(pos) != '\0') {
            tempKey[i++] = EEPROM.read(pos++);
        }
        tempKey[i] = '\0';
        pos++;

        int vpos = pos;
        if (strcmp(tempKey, key) == 0) {
            int k = 0;
            while (EEPROM.read(vpos) != '\0' && k < sizeof(out) - 1) {
                out[k++] = EEPROM.read(vpos++);
            }
            out[k] = '\0';
            return out;  // return pointer to static buffer
        }

        while (EEPROM.read(pos) != '\0') pos++;
        pos++;
    }

    return NULL; // key not found
}


// Clear all EEPROM
void clearEEP() {
    initEEP();
    for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
    saveLength(0);
    EEPROM.commit();
}
