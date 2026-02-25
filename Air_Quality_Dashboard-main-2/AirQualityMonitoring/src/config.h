// Firmware Version
#define FIRMWARE_VERSION "v1.0.0"

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <EEPROM.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "SparkFun_SCD30_Arduino_Library.h"
#include <Adafruit_NeoPixel.h>
#include <PubSubClient.h>
#include <Wire.h>

extern Preferences preferences;
extern WiFiClient esp;
extern PubSubClient client;

extern String mqttbroker;
extern String publish_topic1;
extern String subscribe_topic1;
extern String Device_id;
extern String publish_str;
extern unsigned long previousMillis;
extern unsigned long publish_interval;
extern int publish_interval_shrt;

// Network & MQTT Configuration
extern String apn;
extern String gsm_operator;
extern String mqtt_server;
extern int mqtt_port;
extern String mqtt_user;
extern String mqtt_password;
extern bool mqtt_tls;
extern int mqtt_keepalive;
extern String mqtt_client_id;

// Network Selection
extern bool network_manual_mode;
extern String network_plmn;

#define TRIGGER_PIN 0
#define PIN 15
#define NUMPIXELS 1
extern Adafruit_NeoPixel pixels;
extern SCD30 airSensor;

extern String Receive;
extern char errorMessage[128];
extern int16_t error;
extern float co2Concentration;
extern float temperature;
extern float humidity;
extern float temp_th;
extern float hum_th;
extern float co2_th;

#endif
