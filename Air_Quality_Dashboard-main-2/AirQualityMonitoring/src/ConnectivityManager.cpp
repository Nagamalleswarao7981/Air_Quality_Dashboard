#include "ConnectivityManager.h"
#include <Update.h>

// Define TinyGSM Transport
#define TINY_GSM_MODEM_BG96
#include <TinyGsmClient.h>

// Access to global mutex
extern SemaphoreHandle_t mutex;
extern String Device_id; // To set it from IMEI

// Structs for caching
SignalInfo _cachedSignal = {99, 0, 0};
NetworkInfo _cachedNetwork = {"Unknown", "Unknown", "Unknown", "0.0.0.0", "",
                              "",        "",        "",        ""};

// Helper to map PLMN to Name
String lookupOperator(String plmn) {
  plmn.trim();
  if (plmn.startsWith("404") || plmn.startsWith("405")) {
    // Jio
    if (plmn.startsWith("4058") || plmn.startsWith("405 8"))
      return "Jio";

    // Airtel (Common prefixes)
    if (plmn.startsWith("40410") || plmn.startsWith("404 10") ||
        plmn.startsWith("40440") || plmn.startsWith("404 40") ||
        plmn.startsWith("40445") || plmn.startsWith("404 45") ||
        plmn.startsWith("40470") || plmn.startsWith("404 70") ||
        plmn.startsWith("40490") || plmn.startsWith("404 90") ||
        plmn.startsWith("40492") || plmn.startsWith("404 92") ||
        plmn.startsWith("40403") || plmn.startsWith("404 03"))
      return "Airtel";

    // VI (Vodafone Idea)
    if (plmn.startsWith("40486") || plmn.startsWith("404 86") ||
        plmn.startsWith("40484") || plmn.startsWith("404 84") ||
        plmn.startsWith("40411") || plmn.startsWith("404 11") ||
        plmn.startsWith("40420") || plmn.startsWith("404 20"))
      return "VI";

    // BSNL
    if (plmn.startsWith("40438") || plmn.startsWith("404 38") ||
        plmn.startsWith("4045") || plmn.startsWith("404 5"))
      return "BSNL";

    // Fallbacks
    if (plmn.indexOf("Air") != -1)
      return "Airtel";
    if (plmn.indexOf("Jio") != -1)
      return "Jio";
    if (plmn.indexOf("Vod") != -1)
      return "VI";
    if (plmn.indexOf("Idea") != -1)
      return "VI";
    if (plmn.indexOf("BSNL") != -1 || plmn.indexOf("CellOne") != -1)
      return "BSNL";
  }
  return plmn; // Return original if no match
}

ConnectivityManager::ConnectivityManager() {
  currentState = NET_STATE_INITIALIZING;
  lastStateChange = 0;
  lastUpdate = 0;
  lastSignalUpdate = 0;
  modemReady = false;
  simReady = false;
  netRegistered = false;
  mqttConnected = false;
  scanRequested = false;
  scanResultJson = "[]";
}

void ConnectivityManager::init() {
  // Load config from preferences (already loaded in main setup, just use
  // globals or re-read) We'll use the globals from config.h for now as they are
  // populated in main
  extern String apn;
  extern String mqtt_server;
  extern int mqtt_port;
  extern String mqtt_user;
  extern String mqtt_password;
  extern bool network_manual_mode;
  extern String network_plmn;

  _apn = apn;
  _broker = mqtt_server;
  _port = mqtt_port;
  _user = mqtt_user;
  _pass = mqtt_password;
  _manualMode = network_manual_mode;
  _plmn = network_plmn;

  Serial.println("[Net] Connectivity Manager Initialized");
}

void ConnectivityManager::update() {
  handleState();

  // Update Signal strength every 1s
  // Allow updates as long as we are past initialization (AT commands working)
  if (currentState >= NET_STATE_CHECKING_SIM &&
      millis() - lastSignalUpdate > 1000) {
    updateSignal();
    lastSignalUpdate = millis();
  }

  // Check for incoming MQTT messages if connected
  if (isMqttConnected()) {
    checkMessages();
  }
}

void ConnectivityManager::handleState() {
  switch (currentState) {
  case NET_STATE_INITIALIZING:
    if (millis() - lastStateChange > 2000) {
      Serial.println("[Net] Checking Modem...");
      String resp = sendATWait("AT");
      if (resp.indexOf("OK") >= 0) {
        // Get IMEI
        String imeiResp = sendATWait("AT+GSN");
        // Simple parsing for IMEI
        int idx = imeiResp.indexOf("\r\n", 2); // skips command echo
        if (idx != -1) {
          String imei = imeiResp.substring(idx + 1);
          imei.trim();
          // Remove 'OK'
          imei.replace("OK", "");
          imei.trim();
          if (imei.length() > 5) {
            Device_id = imei; // Set global
            // Also set client ID
            extern String mqtt_client_id;
            mqtt_client_id = imei;
            Serial.println("[Net] Found IMEI: " + imei);
          }
        }

        currentState = NET_STATE_CHECKING_SIM;
        lastStateChange = millis();
      }
    }
    break;

  case NET_STATE_CHECKING_SIM:
    if (millis() - lastStateChange > 1000) {
      if (checkSim()) {
        currentState = NET_STATE_REGISTERING;
        Serial.println("[Net] SIM Ready. Registering...");
      } else {
        // Retry sim check
        lastStateChange = millis();
      }
    }
    break;

  case NET_STATE_REGISTERING:
    if (millis() - lastStateChange > 2000) {
      if (checkRegistration()) {
        // Auto-APN Logic
        String cops = sendATWait("AT+COPS?");
        int idx = cops.indexOf("\"");
        if (idx != -1) {
          int end = cops.indexOf("\"", idx + 1);
          if (end != -1) {
            String rawOp = cops.substring(idx + 1, end);
            String opName = lookupOperator(rawOp);

            if (opName == "Airtel")
              _apn = "airteliot.com";
            else if (opName == "Jio")
              _apn = "jionet";
            else if (opName == "VI")
              _apn = "www";
            else if (opName == "BSNL")
              _apn = "bsnlnet";

            Serial.println("[Net] Operator: " + opName + ", Auto-APN: " + _apn);
          }
        }

        Serial.println("[Net] Registered to Network. Configuring APN...");
        currentState = NET_STATE_CONFIGURING_APN;
      } else {
        lastStateChange = millis(); // Retry
      }
    }
    break;

  case NET_STATE_CONFIGURING_APN:
    if (configureAPN()) {
      Serial.println("[Net] APN Configured. Connecting MQTT...");
      currentState = NET_STATE_CONNECTING_MQTT;
    } else {
      Serial.println("[Net] APN Config Failed. Retrying in 1m...");
      currentState = NET_STATE_ERROR;
      lastError = "APN Config Failed";
      lastStateChange = millis();
    }
    break;

  case NET_STATE_CONNECTING_MQTT:
    if (configureMQTT()) {
      if (connectMQTT()) {
        currentState = NET_STATE_CONNECTED;
        mqttConnected = true;
        Serial.println("[Net] MQTT Connected!");
      } else {
        Serial.println("[Net] MQTT Connection Failed! Waiting 30s to retry...");
        currentState = NET_STATE_WAITING; // Wait before retry
        lastError = "MQTT Connection Failed";
        lastStateChange = millis();
      }
    } else {
      Serial.println(
          "[Net] MQTT Configuration Failed! Waiting 30s to retry...");
      currentState = NET_STATE_WAITING; // Wait before retry
      lastError = "MQTT Config Failed";
      lastStateChange = millis();
    }
    break;

  case NET_STATE_CONNECTED:
    // Check connection periodically
    // If signal verify fails or MQTT drops, go back
    break;

  case NET_STATE_WAITING:
    if (millis() - lastStateChange > 30000) { // Retry after 30s
      currentState = NET_STATE_CONNECTING_MQTT;
      Serial.println("[Net] Retrying MQTT Connection...");
    }
    break;

  case NET_STATE_ERROR:
    if (millis() - lastStateChange > 60000) { // Retry from scratch after 1 min
      currentState = NET_STATE_INITIALIZING;
      Serial.println("[Net] Resetting State Machine...");
    }
    break;
  }
}

String ConnectivityManager::sendATWait(String cmd, unsigned long timeout) {
  // Clear buffer
  while (Serial2.available())
    Serial2.read();

  Serial2.println(cmd);

  String resp = "";
  unsigned long start = millis();
  while (millis() - start < timeout) {
    if (Serial2.available()) {
      char c = Serial2.read();
      resp += c;

      // Early exit if we see standard terminators
      if (resp.indexOf("\r\nOK\r\n") != -1 ||
          resp.indexOf("\r\nERROR\r\n") != -1) {
        // Give it a tiny bit more time just in case of buffered chars
        delay(10);
        while (Serial2.available())
          resp += (char)Serial2.read();
        break;
      }
    }
    delay(1); // Prevent full CPU hogging
  }
  Serial.print("[Modem] ");
  Serial.println(resp); // Debug enabled
  return resp;
}

bool ConnectivityManager::checkSim() {
  String resp = sendATWait("AT+CPIN?");
  if (resp.indexOf("READY") != -1) {
    _cachedNetwork.simStatus = "READY";

    // If we haven't fetched static info yet, do it now
    if (_cachedNetwork.iccid == "" || _cachedNetwork.imsi == "") {
      // CCID
      resp = sendATWait("AT+CCID");
      int idx = resp.indexOf("+CCID: ");
      if (idx != -1) {
        _cachedNetwork.iccid = resp.substring(idx + 7);
        _cachedNetwork.iccid.trim();
      } else {
        // Some modems just return the number
        resp.trim();
        if (resp.length() > 10)
          _cachedNetwork.iccid = resp;
      }

      // IMSI
      resp = sendATWait("AT+CIMI");
      resp.trim();
      if (resp.length() > 10 && resp.indexOf("OK") == -1) {
        _cachedNetwork.imsi = resp;
      }
    }
    return true;
  }
  _cachedNetwork.simStatus = "ERROR";
  lastError = "SIM Not Ready";
  return false;
}

bool ConnectivityManager::checkRegistration() {
  String resp = sendATWait("AT+CREG?");
  // +CREG: 0,1 or 0,5 means registered
  if (resp.indexOf(",1") != -1 || resp.indexOf(",5") != -1) {
    netRegistered = true;
    return true;
  }

  // Try LTE registration (CEREG)
  resp = sendATWait("AT+CEREG?");
  if (resp.indexOf(",1") != -1 || resp.indexOf(",5") != -1) {
    netRegistered = true;
    return true;
  }

  netRegistered = false;
  return false;
}

bool ConnectivityManager::configureAPN() {
  sendATWait("AT+QIDEACT=1", 2000); // Deactivate first to ensure clean state

  String cmd = "AT+QICSGP=1,1,\"" + _apn + "\"";
  String resp = sendATWait(cmd);
  if (resp.indexOf("OK") == -1)
    return false;

  resp =
      sendATWait("AT+QIACT=1", 10000); // 10s timeout, activation can take time
  if (resp.indexOf("ERROR") != -1)
    return false;

  return true;
}

bool ConnectivityManager::configureMQTT() {
  Serial.println("[MQTT] Configuring base connection...");

  // Close previous if any
  while (Serial2.available())
    Serial2.read();
  Serial2.println("AT+QMTCLOSE=0");
  delay(1000);

  // Config
  while (Serial2.available())
    Serial2.read();
  Serial2.println("AT+QMTCFG=\"recv/mode\",0,0,1");
  delay(1000);

  // Open connection
  String openCmd = "AT+QMTOPEN=0,\"" + _broker + "\"," + String(_port);
  while (Serial2.available())
    Serial2.read();
  Serial2.println(openCmd);
  Serial.println("[MQTT] Simple Open Sent...");

  // Basic loop delay
  for (int i = 0; i < 8; i++) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();

  String resp = "";
  while (Serial2.available())
    resp += (char)Serial2.read();
  Serial.println("[MQTT] Open Response:\n" + resp);

  return true; // Simple base code always proceeds
}

bool ConnectivityManager::connectMQTT() {
  extern String mqtt_client_id;

  String cmd = "AT+QMTCONN=0,\"" + mqtt_client_id + "\"";
  if (_user.length() > 0)
    cmd += ",\"" + _user + "\",\"" + _pass + "\"";

  while (Serial2.available())
    Serial2.read();
  Serial2.println(cmd);
  Serial.println("[MQTT] Simple Connect Sent...");

  // Basic loop delay
  for (int i = 0; i < 8; i++) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();

  String resp = "";
  while (Serial2.available())
    resp += (char)Serial2.read();
  Serial.println("[MQTT] Connect Response:\n" + resp);

  if (resp.indexOf("ERROR") != -1 && resp.indexOf("+QMTCONN: 0,0") == -1) {
    return false;
  }
  return true;
}

String ConnectivityManager::getStatusString() {
  switch (currentState) {
  case NET_STATE_INITIALIZING:
    return "INITIALIZING";
  case NET_STATE_CONNECTED:
    return "CONNECTED";
  case NET_STATE_ERROR:
    return "ERROR: " + lastError;
  default:
    return "CONNECTING...";
  }
}

String ConnectivityManager::getStatusColor() {
  if (currentState == NET_STATE_CONNECTED)
    return "#10b981";
  return "#f59e0b";
}

// Structs moved to top

void ConnectivityManager::updateSignal() {
  // 1. Signal
  String resp = sendATWait("AT+CSQ");
  Serial.println("[Sig] CSQ Resp: " + resp);
  int idx = resp.indexOf("+CSQ: ");
  if (idx != -1) {
    int comma = resp.indexOf(",", idx);
    if (comma != -1) {
      int rssi = resp.substring(idx + 6, comma).toInt();
      _cachedSignal.rssi = rssi;
      _cachedSignal.dbm = (rssi == 99) ? 0 : -113 + (rssi * 2);

      if (rssi == 99)
        _cachedSignal.percentage = 0;
      else if (rssi >= 31)
        _cachedSignal.percentage = 100;
      else if (rssi <= 0)
        _cachedSignal.percentage = 0;
      else
        _cachedSignal.percentage = map(rssi, 0, 31, 0, 100);

      // Debug parsed values
      Serial.printf("[Sig] RSSI: %d, dBm: %d\n", _cachedSignal.rssi,
                    _cachedSignal.dbm);
    }
  }

  // 2. Network Info (Operator, Tech, IP)

  // Override if No SIM
  if (_cachedNetwork.simStatus != "READY") {
    _cachedNetwork.operatorName = "No SIM";
    _cachedNetwork.accessTechnology = "No Service";
    return; // Skip the rest
  }

  // Only update if connected or registering
  if (currentState >= NET_STATE_REGISTERING) {
    // Force alphanumeric format once (idempotent)
    static bool formatSet = false;
    if (!formatSet) {
      sendATWait("AT+COPS=3,0");
      formatSet = true;
    }

    String resp = sendATWait("AT+COPS?");
    Serial.println("[Sig] COPS Resp: " + resp);
    idx = resp.indexOf("\"");
    if (idx != -1) {
      int end = resp.indexOf("\"", idx + 1);
      if (end != -1) {
        String rawOp = resp.substring(idx + 1, end);
        _cachedNetwork.operatorName = lookupOperator(rawOp);
      }
    } else {
      // Check if registered but no name (e.g. +COPS: 0,2,40445,7)
      // Or not registered
      if (checkRegistration()) {
        // We are registered, but maybe numeric?
        // Try to find numeric commas
        // +COPS: 0,2,40445,7
        // Find 3rd comma
        int c1 = resp.indexOf(",");
        if (c1 != -1) {
          int c2 = resp.indexOf(",", c1 + 1);
          if (c2 != -1) {
            int c3 = resp.indexOf(",", c2 + 1);
            if (c3 != -1) {
              String plmn = resp.substring(c2 + 1, c3);
              _cachedNetwork.operatorName = lookupOperator(plmn);
            } else {
              // Maybe just 3 items
              String plmn = resp.substring(c2 + 1);
              plmn.trim();
              _cachedNetwork.operatorName = lookupOperator(plmn);
            }
          }
        }
        if (_cachedNetwork.operatorName == "Unknown" ||
            _cachedNetwork.operatorName.length() == 0) {
          _cachedNetwork.operatorName = "Connected"; // Fallback
        }
      } else {
        _cachedNetwork.operatorName = "No Service";
      }
    }

    if (resp.indexOf(",7") != -1)
      _cachedNetwork.accessTechnology = "LTE Cat-M/NB";
    else if (resp.indexOf(",0") != -1)
      _cachedNetwork.accessTechnology = "GSM";
    else if (resp.indexOf(",2") != -1)
      _cachedNetwork.accessTechnology = "3G";
    else
      _cachedNetwork.accessTechnology = "LTE";

    // IP
    resp = sendATWait("AT+QIACT?");
    idx = resp.indexOf("\"");
    if (idx != -1) {
      int end = resp.indexOf("\"", idx + 1);
      if (end != -1) {
        _cachedNetwork.ip = resp.substring(idx + 1, end);
      }
    }
  }
}

// Global reference for OTA
extern TinyGsm modem;

void ConnectivityManager::performOTA(String url, String version) {
  Serial.println("[OTA] Starting OTA Update: " + version);
  Serial.println("[OTA] URL: " + url);

  // 1. Notify Backend (Started)
  publish("aqm/" + Device_id + "/ota_status",
          "{\"status\":\"starting\",\"progress\":0}");

  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(60000))) { // Long timeout

    // Parse URL
    // http://host:port/path
    String host;
    int port = 80;
    String path;

    int doubleSlash = url.indexOf("//");
    int start = (doubleSlash != -1) ? doubleSlash + 2 : 0;
    int slash = url.indexOf("/", start);
    int colon = url.indexOf(":", start);

    if (colon != -1 && colon < slash) {
      host = url.substring(start, colon);
      port = url.substring(colon + 1, slash).toInt();
    } else {
      host = url.substring(start, slash);
    }
    path = url.substring(slash);

    Serial.println("[OTA] Host: " + host + " Port: " + String(port) +
                   " Path: " + path);

    // Use local TinyGSM instance or global?
    // We can instantiate one on Serial2
    TinyGsm modemOTA(Serial2);
    TinyGsmClient otaClient(modemOTA);

    Serial.println("[OTA] Connecting to server...");
    if (otaClient.connect(host.c_str(), port)) {
      Serial.println("[OTA] Connected. Sending GET...");
      otaClient.print(String("GET ") + path + " HTTP/1.1\r\n");
      otaClient.print(String("Host: ") + host + "\r\n");
      otaClient.print("Connection: close\r\n\r\n");

      // Read Headers
      long contentLength = 0;
      bool bodyStarted = false;
      String line;

      unsigned long timeout = millis();
      while (otaClient.connected() && millis() - timeout < 10000) {
        if (otaClient.available()) {
          line = otaClient.readStringUntil('\n');
          line.trim();
          // Serial.println(line); // Debug headers
          if (line.startsWith("Content-Length:")) {
            contentLength = line.substring(15).toInt();
          }
          if (line.length() == 0) {
            bodyStarted = true;
            break;
          }
          timeout = millis();
        }
      }

      if (bodyStarted && contentLength > 0) {
        Serial.printf("[OTA] Content-Length: %ld. Begin Update.\n",
                      contentLength);
        if (Update.begin(contentLength)) {
          size_t written = 0;
          uint8_t buff[256];
          while (otaClient.connected() && written < contentLength) {
            size_t avail = otaClient.available();
            if (avail) {
              int toRead = (avail > 256) ? 256 : avail;
              int c = otaClient.readBytes(buff, toRead);
              Update.write(buff, c);
              written += c;

              // Occasional Progress Log (serial only to avoid mutex deadlock)
              if (written % 10240 == 0) {
                Serial.printf("[OTA] Progress: %d%%\n",
                              (written * 100) / contentLength);
              }
            }
            // Check timeout?
          }

          if (Update.end()) {
            Serial.println("[OTA] Update Success! Rebooting...");
            // We can't publish easily here due to mutex.
            // Just release and reboot.
            xSemaphoreGive(mutex);
            delay(100);
            ESP.restart();
          } else {
            Serial.printf("[OTA] Update Failed. Error: %d\n",
                          Update.getError());
          }
        } else {
          Serial.println("[OTA] Not enough space for update");
        }
      } else {
        Serial.println("[OTA] Header parse failed or empty body");
      }
      otaClient.stop();
    } else {
      Serial.println("[OTA] Connect failed");
    }

    xSemaphoreGive(mutex);
    // If we are here, it failed
    publish("aqm/" + Device_id + "/ota_status",
            "{\"status\":\"failed\",\"progress\":0}");
  } else {
    Serial.println("[OTA] Mutex timeout");
  }
}

SignalInfo ConnectivityManager::getSignalStrength() { return _cachedSignal; }

NetworkInfo ConnectivityManager::getNetworkInfo() {
  _cachedNetwork.status = getStatusString(); // Status is dynamic
  return _cachedNetwork;
}

NetworkState ConnectivityManager::getState() { return currentState; }

String ConnectivityManager::getLastError() { return lastError; }

bool ConnectivityManager::isMqttConnected() { return mqttConnected; }

void ConnectivityManager::updateConfig(String newApn, String newBroker,
                                       int newPort, String newUser,
                                       String newPass) {
  _apn = newApn;
  _broker = newBroker;
  _port = newPort;
  _user = newUser;
  _pass = newPass;

  // Save to globals/preferences
  extern String apn;
  apn = newApn;
  extern String mqtt_server;
  mqtt_server = newBroker;
  extern int mqtt_port;
  mqtt_port = newPort;
  extern String mqtt_user;
  mqtt_user = newUser;
  extern String mqtt_password;
  mqtt_password = newPass;

  extern Preferences preferences;
  preferences.begin("my-app", false);
  preferences.putString("apn", apn);
  preferences.putString("mqtt_server", mqtt_server);
  preferences.putInt("mqtt_port", mqtt_port);
  preferences.putString("mqtt_user", mqtt_user);
  preferences.putString("mqtt_password", mqtt_password);
  preferences.end();

  // Trigger reconnect
  currentState = NET_STATE_INITIALIZING; // Full reset or just APN?
  // Ideally:
  if (currentState >= NET_STATE_CONFIGURING_APN) {
    currentState = NET_STATE_CONFIGURING_APN; // Go back to APN config
  }
}

String ConnectivityManager::executeATCommand(String cmd,
                                             unsigned long timeout) {
  // Take mutex to ensure exclusive access to Serial2
  // Increased to 15000ms to avoid "Busy" errors during long network ops
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(15000)) == pdTRUE) {
    String resp = sendATWait(cmd, timeout);
    xSemaphoreGive(mutex);
    return resp;
  } else {
    return "ERROR: Busy (Mutex Timeout) - Try again";
  }
}

// MQTT Implementation
bool ConnectivityManager::publish(String topic, String payload) {
  if (!isMqttConnected())
    return false;

  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(15000)) == pdTRUE) {
    String cmd = "AT+QMTPUB=0,0,0,0,\"" + topic + "\"";

    while (Serial2.available())
      Serial2.read();
    Serial2.println(cmd);

    // Simple delay for prompt
    delay(500);

    // Send payload followed by CTRL+Z
    Serial2.print(payload);
    Serial2.write(26);

    // Simple delay for sending
    delay(3000);

    String resp = "";
    while (Serial2.available())
      resp += (char)Serial2.read();

    // In base code, we assume it went through to keep the loop alive
    Serial.println("[MQTT] Publish Response:\n" + resp);

    xSemaphoreGive(mutex);
    return true;
  }

  Serial.println("[MQTT] Failed to acquire mutex for publish");
  return false;
}

bool ConnectivityManager::subscribe(String topic) {
  if (!isMqttConnected())
    return false;

  // AT+QMTSUB=<tcpconnectID>,<msgID>,"<topic>",<qos>
  String cmd = "AT+QMTSUB=0,1,\"" + topic + "\",0";
  String resp = sendATWait(cmd, 2000);

  // Expect +QMTSUB: 0,1,0 (Success)
  if (resp.indexOf("+QMTSUB: 0,1,0") != -1)
    return true;
  return false;
}

void ConnectivityManager::setCallback(void (*callback)(String, String)) {
  mqttCallback = callback;
}

void ConnectivityManager::checkMessages() {
  // Read any available data from Serial2 (without blocking for checking
  // OK/ERROR) We need to be careful not to consume data if we are expecting a
  // command response But this function is called inside update() which is
  // mutually excluded from commands

  if (Serial2.available()) {
    String line = Serial2.readStringUntil('\n');
    line.trim();

    // Check for URC: +QMTRECV: <tcpconnectID>,<msgID>,"<topic>","<payload>"
    if (line.startsWith("+QMTRECV:")) {
      int firstComma = line.indexOf(',');
      int secondComma = line.indexOf(',', firstComma + 1);
      int thirdComma =
          line.indexOf(',', secondComma + 1); // Start of topic quote

      if (thirdComma != -1) {
        // Parse Topic
        int topicStart = line.indexOf('"', secondComma);
        int topicEnd = line.indexOf('"', topicStart + 1);
        String topic = line.substring(topicStart + 1, topicEnd);

        // Parse Payload
        int payloadStart =
            line.indexOf('"', thirdComma + 1); // might be after topic
        // Actually the format is +QMTRECV: 0,0,"topic","payload"
        // Let's rely on standard parsing

        // Find the second quote of topic
        int payloadQuote1 = line.indexOf('"', topicEnd + 1);
        int payloadQuote2 = line.lastIndexOf('"');

        if (payloadQuote1 != -1 && payloadQuote2 != -1 &&
            payloadQuote2 > payloadQuote1) {
          String payload = line.substring(payloadQuote1 + 1, payloadQuote2);

          Serial.println("[MQTT] Recv: " + topic + " -> " + payload);
          if (mqttCallback != nullptr) {
            mqttCallback(topic, payload);
          }
        }
      }
    }
  }
}

ConnectivityManager network;
