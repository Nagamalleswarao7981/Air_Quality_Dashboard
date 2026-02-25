#include <Preferences.h>
#include<EEPROM.h>
#include <WiFiManager.h>
#include <Arduino.h>


#include <Wire.h>
#include "SparkFun_SCD30_Arduino_Library.h" 
#include <Adafruit_NeoPixel.h>
#include <PubSubClient.h>

Preferences preferences;
WiFiClient esp;
PubSubClient client(esp);
String mqttbroker = "kaqm.uniolabs.com";
String publish_topic1 = "";
String subscribe_topic1 = "";
String Device_id = "";
String publish_str = "0.00,0.00,0";
unsigned long previousMillis = 0;
unsigned long publish_interval = 6000;
int publish_interval_shrt = 1;
#define mqttbroker_addr 10

#define TRIGGER_PIN 0
#define PIN 15
#define NUMPIXELS 1
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
SCD30 airSensor;

String Receive = "";
static char errorMessage[128];
static int16_t error;
float co2Concentration = 0.0;
float temperature = 0.0;
float humidity = 0.0;
float temp_th = 40.0;
float hum_th = 40.0;
float co2_th = 600.0;
