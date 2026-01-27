#ifndef NEURAL_ENGINE_H
#define NEURAL_ENGINE_H

#include <Arduino.h>
#include <WiFi.h>

/* ================= LOGGER MACRO ================= */
// Usage: cl("Sensor value:", 123, "Status:", "OK");
#define cl(...) cl_impl(__FILE__, __func__, __LINE__, ##__VA_ARGS__)

/* ================= CORE ================= */
String getESP_ID();

/* ================= INTERNAL SENDER ================= */
void _cl_send(
    const char* file,
    const char* func,
    int line,
    const String &msg
);

/* ================= TEMPLATE LOGGER ================= */
template<typename... Args>
inline void cl_impl(
    const char* file,
    const char* func,
    int line,
    Args... args
) {
    String msg = "";
    // Fold expression to concatenate all arguments into a string
    (msg += ... += (String(args) + " "));
    msg.trim();
    _cl_send(file, func, line, msg);
}

/* ================= SERIAL INPUT ================= */
void clearSerialBuffer(void);
int readInt(void);
void readString(char *buffer, int maxLen);
char readChar(void);
float readFloat(void);
double readDouble(void);

/* ================= EEPROM ================= */
void set_item(const char *key, const char *value);
char* get_item(const char *key);
void clearEEP(void);

#endif