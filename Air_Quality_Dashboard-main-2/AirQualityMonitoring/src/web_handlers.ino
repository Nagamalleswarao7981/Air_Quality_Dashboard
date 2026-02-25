// web_handlers.ino - Dashboard API handlers with SPIFFS

#include <FS.h>
#include <SPIFFS.h>
#include <Update.h>

// Web Server Instance
WebServer server(80);

// Helper: Get System Info
String getSystemJSON() {
  String json = "{";
  json += "\"uptime\":" + String(millis() / 1000) + ",";
  json += "\"freeHeap\":" + String(ESP.getFreeHeap());
  json += "}";
  return json;
}

// Helper: Get Sensor Data JSON
String getSensorJSON() {
  String json = "{";
  json += "\"temp\":" + String(temperature, 2) + ",";
  json += "\"hum\":" + String(humidity, 2) + ",";
  json += "\"co2\":" + String(co2Concentration, 0);
  json += "}";
  return json;
}

// Route: Root - Serve Dashboard from SPIFFS
void handleRoot() {
  // Try with leading slash first
  if (!SPIFFS.exists("/index.html")) {
    Serial.println("ERROR: /index.html not found in SPIFFS!");
    server.send(404, "text/plain",
                "Dashboard file not found. Please upload filesystem.");
    return;
  }

  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    Serial.println("ERROR: Failed to open /index.html");
    server.send(500, "text/plain", "Failed to open dashboard file");
    return;
  }

  Serial.println("Serving /index.html from SPIFFS");
  server.streamFile(file, "text/html");
  file.close();
}

// Route: Get Sensor Data
void handleGetSensors() {
  server.send(200, "application/json", getSensorJSON());
}

// Route: Get System Status
void handleGetSystem() {
  server.send(200, "application/json", getSystemJSON());
}

// Route: Get Configuration
void handleGetConfig() {
  String json = "{";
  json += "\"tempThreshold\":" + String(temp_th, 1) + ",";
  json += "\"humThreshold\":" + String(hum_th, 1) + ",";
  json += "\"co2Threshold\":" + String(co2_th, 0);
  json += "}";
  server.send(200, "application/json", json);
}

// Route: Set Configuration
void handleSetConfig() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");

    int tempIdx = body.indexOf("\"tempThreshold\":");
    if (tempIdx > 0) {
      int start = tempIdx + 16;
      int end = body.indexOf(",", start);
      if (end < 0)
        end = body.indexOf("}", start);
      temp_th = body.substring(start, end).toFloat();
    }

    int humIdx = body.indexOf("\"humThreshold\":");
    if (humIdx > 0) {
      int start = humIdx + 15;
      int end = body.indexOf(",", start);
      if (end < 0)
        end = body.indexOf("}", start);
      hum_th = body.substring(start, end).toFloat();
    }

    int co2Idx = body.indexOf("\"co2Threshold\":");
    if (co2Idx > 0) {
      int start = co2Idx + 15;
      int end = body.indexOf(",", start);
      if (end < 0)
        end = body.indexOf("}", start);
      co2_th = body.substring(start, end).toFloat();
    }

    server.send(200, "application/json", "{\"status\":\"success\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
  }
}

// Route: Get GSM/Network Data
void handleGetGSM() {
  String json = "{";
  json += "\"signal_dbm\":-50,"; // Placeholder
  json += "\"operator\":\"Network\"";
  json += "}";
  server.send(200, "application/json", json);
}

// Route: Get Sensor Health
void handleGetSensorHealth() {
  String json = "{";
  json += "\"status\":\"" + sensorHealth.getStatusString() + "\",";
  json += "\"statusColor\":\"" + sensorHealth.getStatusColor() + "\",";
  json += "\"errorCount\":" + String(sensorHealth.getErrorCount()) + ",";
  json +=
      "\"consecutiveErrors\":" + String(sensorHealth.getConsecutiveErrors()) +
      ",";
  json += "\"lastUpdateAge\":" + String(sensorHealth.getLastUpdateAge()) + ",";
  json += "\"dataFreshness\":\"" + sensorHealth.getDataFreshness() + "\",";
  json += "\"currentValues\":{";
  json += "\"co2\":" + String(co2Concentration, 1) + ",";
  json += "\"temp\":" + String(temperature, 1) + ",";
  json += "\"hum\":" + String(humidity, 1);
  json += "},";
  json += "\"calibration\":\"ASC_ENABLED\",";
  json += "\"i2cStatus\":\"OK\",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";
  server.send(200, "application/json", json);
}

// Route: Restart Sensor
void handleRestartSensor() {
  Serial.println("[API] Sensor restart requested");
  bool success = sensorHealth.attemptRecovery();

  String json = "{";
  json += "\"status\":\"" + String(success ? "success" : "failed") + "\",";
  json += "\"message\":\"" +
          String(success ? "Sensor restarted successfully"
                         : "Sensor restart failed") +
          "\"";
  json += "}";

  server.send(200, "application/json", json);
}

// Route: Re-initialize I2C
void handleReinitI2C() {
  Serial.println("[API] I2C re-initialization requested");
  sensorHealth.resetSensor();

  String json = "{";
  json += "\"status\":\"success\",";
  json += "\"message\":\"I2C re-initialized\"";
  json += "}";

  server.send(200, "application/json", json);
}

// Route: Clear Error Counters
void handleClearErrors() {
  Serial.println("[API] Clear errors requested");
  sensorHealth.clearErrors();

  String json = "{";
  json += "\"status\":\"success\",";
  json += "\"message\":\"Error counters cleared\"";
  json += "}";

  server.send(200, "application/json", json);
}

// Route: Reboot Device
void handleReboot() {
  server.send(200, "text/plain", "Rebooting...");
  delay(1000);
  ESP.restart();
}

// Route: Get OTA Status
void handleOTAStatus() {
  // scalable in future, for now just simple
  server.send(200, "application/json", "{\"status\":\"idle\",\"progress\":0}");
}

// Route: OTA Upload Finalize
void handleOTAUpload() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
  delay(500);
  ESP.restart();
}

// Route: OTA Upload Data Processing
void handleOTAUploadData() {
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    int command = U_FLASH;
    if (upload.filename.indexOf("spiffs") > -1 ||
        upload.filename.indexOf("fs") > -1) {
      command = U_SPIFFS;
      Serial.println("[OTA] Updating Filesystem (SPIFFS)...");
    } else {
      Serial.println("[OTA] Updating Firmware...");
    }

    if (!Update.begin(UPDATE_SIZE_UNKNOWN, command)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    // Flashing firmware to ESP
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    // True to set the size to the current progress
    if (Update.end(true)) {
      Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

// Setup Web Server Routes
void setupWeb() {
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("ERROR: SPIFFS Mount Failed!");
    return;
  }
  Serial.println("SPIFFS Mounted Successfully");

  // List files for debugging
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  Serial.println("Files in SPIFFS:");
  while (file) {
    Serial.print("  ");
    Serial.println(file.name());
    file = root.openNextFile();
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/sensors", HTTP_GET, handleGetSensors);
  server.on("/api/system", HTTP_GET, handleGetSystem);
  server.on("/api/config", HTTP_GET, handleGetConfig);
  server.on("/api/config", HTTP_POST, handleSetConfig);
  server.on("/api/gsm", HTTP_GET, handleGetGSM);
  server.on("/api/reboot", HTTP_POST, handleReboot);

  // Sensor Health endpoints
  server.on("/api/sensor/health", HTTP_GET, handleGetSensorHealth);
  server.on("/api/sensor/restart", HTTP_POST, handleRestartSensor);
  server.on("/api/sensor/reinit", HTTP_POST, handleReinitI2C);
  server.on("/api/sensor/clear-errors", HTTP_POST, handleClearErrors);

  // Network Manager Endpoints
  server.on("/api/network/status", HTTP_GET, []() {
    // Use globals populated by updateNetworkStatus() in new_main.ino
    extern int g_signal_rssi;
    extern int g_signal_dbm;
    extern String g_operator;
    extern String g_ip;
    extern String g_tech;
    extern String g_net_status;
    extern bool mqtt_connected; // from new_main.ino (static — use g_net_status)

    String json = "{";
    json += "\"status\":\"" + g_net_status + "\",";
    json += "\"color\":\"" +
            String(g_net_status == "CONNECTED" ? "#10b981" : "#f59e0b") + "\",";
    json += "\"state\":1,"; // 1 = connected (numeric compat)
    json += "\"operator\":\"" + g_operator + "\",";
    json += "\"ip\":\"" + g_ip + "\",";
    json += "\"tech\":\"" + g_tech + "\",";
    json += "\"rssi\":" + String(g_signal_rssi) + ",";
    json += "\"dbm\":" + String(g_signal_dbm) + ",";
    json += "\"mqttConnected\":" +
            String(g_net_status == "CONNECTED" ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/api/network/config", HTTP_GET, []() {
    extern String apn;
    extern String mqtt_server;
    extern int mqtt_port;
    extern String mqtt_user;

    String json = "{";
    json += "\"apn\":\"" + apn + "\",";
    json += "\"broker\":\"" + mqtt_server + "\",";
    json += "\"port\":" + String(mqtt_port) + ",";
    json += "\"user\":\"" + mqtt_user + "\""; // Don't send password
    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/api/network/config", HTTP_POST, []() {
    if (!server.hasArg("apn") || !server.hasArg("broker")) {
      server.send(400, "application/json",
                  "{\"message\":\"Missing parameters\"}");
      return;
    }
    String newApn = server.arg("apn");
    String newBroker = server.arg("broker");
    int newPort = server.arg("port").toInt();
    String newUser = server.arg("user");
    String newPass = server.arg("pass");

    network.updateConfig(newApn, newBroker, newPort, newUser, newPass);
    server.send(200, "application/json",
                "{\"message\":\"Config saved. Reconnecting...\"}");
  });

  // OTA Update endpoints
  server.on("/api/ota/status", HTTP_GET, handleOTAStatus);
  server.on("/api/ota/upload", HTTP_POST, handleOTAUpload, handleOTAUploadData);

  // AT Command Endpoint
  server.on("/api/at/send", HTTP_POST, []() {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"error\":\"No body\"}");
      return;
    }
    String body = server.arg("plain");
    // Simple manual JSON parsing
    String cmd = "";
    unsigned long timeout = 1000;

    int cmdIdx = body.indexOf("\"cmd\":");
    if (cmdIdx != -1) {
      int start = body.indexOf("\"", cmdIdx + 6) + 1;
      int end = body.indexOf("\"", start);
      cmd = body.substring(start, end);
    }

    int toIdx = body.indexOf("\"timeout\":");
    if (toIdx != -1) {
      int start = toIdx + 10;
      int end = body.indexOf(",", start);
      if (end == -1)
        end = body.indexOf("}", start);
      String toStr = body.substring(start, end);
      timeout = toStr.toInt();
    }

    if (cmd.length() == 0) {
      server.send(400, "application/json", "{\"error\":\"Missing cmd\"}");
      return;
    }

    // Execute
    String resp = network.executeATCommand(cmd, timeout);

    // JSON escape the response (basic)
    resp.replace("\n", "\\n");
    resp.replace("\r", "\\r");
    resp.replace("\"", "\\\"");

    String json = "{\"response\":\"" + resp + "\"}";
    server.send(200, "application/json", json);
  });

  server.begin();
  Serial.println("Web Server Started on 192.168.4.1");
}
