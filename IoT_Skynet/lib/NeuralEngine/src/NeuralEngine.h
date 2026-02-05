#ifndef NEURAL_ENGINE_H
#define NEURAL_ENGINE_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h> // For EEPROM Non Volatile memory

#define cl(...) cl_impl(__FILE__, __func__, __LINE__, ##__VA_ARGS__)

/* ================= CORE ================= */



void set_ssid(const char* ssid_ptr);
void set_pwd(const char* pwd_ptr);


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

// SSID AND PWD

void set_ssid(const char* ssid_ptr);
void set_pwd(const char* pwd_ptr); 
void deleteAllNodes(); 
void CLLdisplay();   

// Bluetooth BLE with WiFi
void startBluetooth();


#endif
