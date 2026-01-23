#include "ConnectivityManager.h"

// Task 1 code to run on Core 0
// MQTT Callback
void handleMqttMessage(String topic, String payload) {
  Serial.println("[MQTT] Callback: " + topic + " - " + payload);

  // Parse JSON: {"cmd": "reboot", "val": 1}
  // Simple check for now
  if (payload.indexOf("\"cmd\":\"reboot\"") >= 0) {
    Serial.println("[CMD] REBOOT REQUESTED");
    ESP.restart();
  } else if (payload.indexOf("\"cmd\":\"ota_start\"") >= 0) {
    Serial.println("[CMD] OTA START REQUESTED");
    // Simple parse: {"cmd":"ota_start","url":"...","ver":"..."}
    int uStart = payload.indexOf("\"url\":\"");
    if (uStart != -1) {
      int uEnd = payload.indexOf("\"", uStart + 7);
      if (uEnd != -1) {
        String url = payload.substring(uStart + 7, uEnd);
        String ver = "v-new";
        int vStart = payload.indexOf("\"ver\":\"");
        if (vStart != -1) {
          int vEnd = payload.indexOf("\"", vStart + 7);
          if (vEnd != -1)
            ver = payload.substring(vStart + 7, vEnd);
        }
        // Run OTA
        network.performOTA(url, ver);
      }
    }
  }
}

// Subscribe flag
bool isSubscribed = false;

// Task 1 code to run on Core 0
void TaskCore0(void *pvParameters) {
  unsigned long previousStatusMillis = 0;

  Serial.println("[Core0] Starting Connectivity Manager...");
  network.init();
  network.setCallback(handleMqttMessage);

  for (;;) {
    // Run state machine (includes checkMessages())
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100))) {
      network.update();
      xSemaphoreGive(mutex);
    }

    // Handle WiFi Config Portal button
    if (digitalRead(TRIGGER_PIN) == LOW) {
      xSemaphoreTake(mutex, portMAX_DELAY);
      led_blue();
      delay(50);
      led_green();
      delay(50);
      WiFiManager wm;
      wm.setConfigPortalTimeout(120);
      String hotspot_name = "AQM-Config";
      if (Device_id.length() > 0)
        hotspot_name = "AQM:" + Device_id;
      wm.startConfigPortal(hotspot_name.c_str());
      xSemaphoreGive(mutex);
    }

    // Application Logic (Publishing & Subscribing)
    if (network.getState() == NET_STATE_CONNECTED &&
        network.isMqttConnected()) {

      // Subscribe once
      if (!isSubscribed) {
        if (Device_id.length() > 0) {
          String topic = "aqm/" + Device_id + "/cmd";
          if (network.subscribe(topic)) {
            Serial.println("[MQTT] Subscribed to " + topic);
            isSubscribed = true;
          }
        }
      }

      // Publish Data
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= publish_interval) {
        previousMillis = currentMillis;

        if (Device_id.length() > 0) {
          String topic = "aqm/" + Device_id + "/data";

          // Construct JSON manually
          String payload = "{";
          payload += "\"co2\":" + String(co2Concentration, 0) + ",";
          payload += "\"temp\":" + String(temperature, 1) + ",";
          payload += "\"hum\":" + String(humidity, 1) + ",";

          SignalInfo sig = network.getSignalStrength();
          payload += "\"signal\":{\"rssi\":" + String(sig.rssi) +
                     ",\"dbm\":" + String(sig.dbm) + "}";
          payload += "}";

          Serial.println("[Core0] Publishing to " + topic);
          network.publish(topic, payload);
        }
      }

      // Publish GSM Status (Every 1s)
      if (currentMillis - previousStatusMillis >= 1000) {
        previousStatusMillis = currentMillis;

        if (Device_id.length() > 0) {
          String topic = "aqm/" + Device_id + "/status";

          NetworkInfo net = network.getNetworkInfo();
          SignalInfo sig = network.getSignalStrength();

          String payload = "{";
          payload += "\"device_id\":\"" + Device_id + "\",";
          payload += "\"timestamp\":" + String(millis() / 1000) + ",";
          payload += "\"sim_present\":" +
                     String((net.simStatus == "READY") ? "true" : "false") +
                     ",";
          payload += "\"sim_status\":\"" +
                     (net.simStatus.length() > 0 ? net.simStatus : "UNKNOWN") +
                     "\",";
          payload += "\"operator\":\"" + net.operatorName + "\",";
          payload += "\"mcc_mnc\":\"404-XX\","; // Placeholder implementation
          payload += "\"signal_rssi\":" + String(sig.dbm) + ",";
          payload += "\"signal_quality\":\"" +
                     String(sig.rssi > 15 ? "GOOD" : "POOR") + "\",";
          payload +=
              "\"network_registered\":" +
              String(network.getState() == NET_STATE_CONNECTED ? "true"
                                                               : "false") +
              ",";
          payload += "\"network_type\":\"" + net.accessTechnology + "\",";
          payload += "\"ip_assigned\":" +
                     String(net.ip != "0.0.0.0" ? "true" : "false");
          payload += "}";

          network.publish(topic, payload);
        }
      }
    } else {
      isSubscribed = false; // Reset if disconnected
    }

    delay(10); // Yield
  }
}
