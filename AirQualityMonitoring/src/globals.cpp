#include "config.h"

// Globals Definitions
Preferences preferences;
WiFiClient esp;
PubSubClient client(esp);

String mqttbroker = "broker.uniolabs.in";
String publish_topic1 = "";
String subscribe_topic1 = "";
String Device_id = "";
String publish_str = "0.00,0.00,0";
unsigned long previousMillis = 0;
unsigned long publish_interval = 6000;
int publish_interval_shrt = 1;

// Network & MQTT Configuration
String apn = "airteliot.com";
String gsm_operator = "Auto";
String mqtt_server = "broker.uniolabs.in";
int mqtt_port = 1883;
String mqtt_user = "";
String mqtt_password = "";
bool mqtt_tls = false;
int mqtt_keepalive = 60;
String mqtt_client_id = "";

// Network Selection
bool network_manual_mode = false;
String network_plmn = "";

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
SCD30 airSensor;

String Receive = "";
char errorMessage[128];
int16_t error;
float co2Concentration = 0.0;
float temperature = 0.0;
float humidity = 0.0;
float temp_th = 40.0;
float hum_th = 40.0;
float co2_th = 600.0;
