#ifndef WEB_HANDLERS_H
#define WEB_HANDLERS_H

#include <Arduino.h>
#include <WebServer.h>

extern WebServer server;

void setupWeb();

// Page Handlers
void handleRoot();
void handleLogin();
void handleIndex();

// API Handlers
void handleSensorData();   // GET /api/sensors (CO2, Temp, Hum)
void handleGsmData();      // GET /api/gsm (Signal, Operator, Status)
void handleGsmScan();      // POST /api/gsm/scan
void handleGsmSelect();    // POST /api/gsm/select
void handleSystemInfo();   // GET /api/system
void handleSystemReboot(); // POST /api/system/reboot
void handleLogs();         // GET /api/logs

#endif
