#include "ConnectivityManager.h" // Needed by web_handlers.ino (network object)

// ─── MQTT Configuration
// ───────────────────────────────────────────────────────
const char *BROKER = "broker.uniolabs.in";
const int MQTT_PORT = 1883;
// Topic built dynamically: "<IMEI>/aqi/data" — set after IMEI is fetched
String MQTT_TOPIC = "unknown/aqi/data"; // Placeholder, replaced in TaskCore0

// ─── Timing ──────────────────────────────────────────────────────────────────
#define PUBLISH_INTERVAL 7000 // 7 s between MQTT publishes

// ─── State ───────────────────────────────────────────────────────────────────
static bool mqtt_connected = false;
static unsigned long lastPublish = 0;

// ─── Network status globals (used by web /api/network/status) ────────────────
int g_signal_rssi = 99;
int g_signal_dbm = 0;
String g_operator = "Detecting...";
String g_ip = "0.0.0.0";
String g_tech = "LTE";
String g_net_status = "CONNECTING...";

// ════════════════════════════════════════════════════════════════════════════
// sendATCmd — exits immediately when OK/ERROR seen (no full-timeout wait)
// ════════════════════════════════════════════════════════════════════════════
static String sendATCmd(const String &cmd, unsigned long timeout = 2000) {
  while (Serial2.available())
    Serial2.read(); // Flush stale bytes

  Serial.println(">>> " + cmd);
  Serial2.println(cmd);

  String response = "";
  unsigned long start = millis();

  while (millis() - start < timeout) {
    while (Serial2.available())
      response += (char)Serial2.read();

    // Early-exit on terminal tokens (sync responses only)
    if (response.indexOf("\r\nOK\r\n") >= 0 ||
        response.indexOf("\r\nERROR\r\n") >= 0 ||
        response.indexOf("+CME ERROR:") >= 0 ||
        response.indexOf("+QMTPUBEX:") >= 0) {
      delay(5);
      while (Serial2.available())
        response += (char)Serial2.read();
      break;
    }
    delay(1);
  }

  Serial.println(response.length() ? ("<<< " + response) : "<<< (no response)");
  return response;
}

// ════════════════════════════════════════════════════════════════════════════
// waitForURC — waits for an async URC that arrives AFTER the initial OK
// AT+QMTOPEN and AT+QMTCONN respond with OK first, then send the result URC.
// ════════════════════════════════════════════════════════════════════════════
static String waitForURC(const String &token, unsigned long timeout = 12000) {
  String buf = "";
  unsigned long start = millis();
  while (millis() - start < timeout) {
    while (Serial2.available())
      buf += (char)Serial2.read();
    if (buf.indexOf(token) >= 0) {
      delay(5);
      while (Serial2.available())
        buf += (char)Serial2.read();
      break;
    }
    delay(1);
  }
  if (buf.length())
    Serial.println("[URC] " + buf);
  return buf;
}

// ════════════════════════════════════════════════════════════════════════════
// connectMQTT_direct — two-phase: waits for URC after initial OK
// ════════════════════════════════════════════════════════════════════════════
static bool connectMQTT_direct() {
  Serial.println("[MQTT] Connecting to broker: " + String(BROKER));
  mqtt_connected = false;

  // Close stale session
  sendATCmd("AT+QMTCLOSE=0", 2000);

  // Protocol config (sync OK responses)
  sendATCmd("AT+QMTCFG=\"version\",0,4", 500);
  sendATCmd("AT+QMTCFG=\"keepalive\",0,60", 500);

  // ── QMTOPEN: modem sends OK (accepted), then +QMTOPEN: 0,result URC ────────
  String openCmd =
      "AT+QMTOPEN=0,\"" + String(BROKER) + "\"," + String(MQTT_PORT);
  String ack = sendATCmd(openCmd, 3000);

  if (ack.indexOf("ERROR") >= 0) {
    Serial.println("[MQTT] QMTOPEN rejected: " + ack);
    return false;
  }
  // Wait for URC if not already in ack
  String urc =
      (ack.indexOf("+QMTOPEN:") >= 0) ? ack : waitForURC("+QMTOPEN:", 12000);
  if (urc.indexOf("+QMTOPEN: 0,0") < 0) {
    Serial.println("[MQTT] TCP open failed. URC: " + urc);
    return false;
  }
  Serial.println("[MQTT] TCP connected to broker.");

  // ── QMTCONN: same two-phase pattern
  // ─────────────────────────────────────────
  String client_id = (Device_id.length() > 0) ? Device_id : "esp32_aq_default";
  ack = sendATCmd("AT+QMTCONN=0,\"" + client_id + "\"", 3000);

  if (ack.indexOf("ERROR") >= 0) {
    Serial.println("[MQTT] QMTCONN rejected: " + ack);
    return false;
  }
  urc = (ack.indexOf("+QMTCONN:") >= 0) ? ack : waitForURC("+QMTCONN:", 12000);
  if (urc.indexOf("+QMTCONN: 0,0") >= 0) {
    Serial.println("[MQTT] MQTT connected! Topic: " + MQTT_TOPIC);
    mqtt_connected = true;
    return true;
  }

  Serial.println("[MQTT] Auth failed. URC: " + urc);
  return false;
}

// ════════════════════════════════════════════════════════════════════════════
// publishData_direct — build JSON payload and publish via AT+QMTPUBEX
// ════════════════════════════════════════════════════════════════════════════
static void publishData_direct() {
  if (!mqtt_connected) {
    Serial.println("[MQTT] Not connected — skipping publish.");
    return;
  }

  // Hygiene score
  float co2_score =
      100.0 * (1.0 - constrain((co2Concentration - 600.0) / 1400.0, 0.0, 1.0));
  float temp_score =
      100.0 * (1.0 - constrain(abs(temperature - 22.0) / 10.0, 0.0, 1.0));
  float hum_score =
      100.0 * (1.0 - constrain(abs(humidity - 50.0) / 30.0, 0.0, 1.0));
  float hygiene_score =
      (co2_score * 0.5f) + (temp_score * 0.25f) + (hum_score * 0.25f);

  // Build JSON payload
  String json = "{";
  json += "\"hygiene_score\":" + String(hygiene_score, 1) + ",";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"humidity\":" + String(humidity, 1) + ",";
  json += "\"co2\":" + String((int)co2Concentration) + ",";
  json += "\"device_status\":\"online\",";
  json += "\"timestamp\":" + String(millis() / 1000);
  json += "}";

  Serial.println("\n[MQTT] Payload (" + String(json.length()) +
                 " bytes): " + json);

  // AT+QMTPUBEX — send with explicit length
  String pubCmd = "AT+QMTPUBEX=0,0,0,0,\"" + String(MQTT_TOPIC) + "\"," +
                  String(json.length());

  while (Serial2.available())
    Serial2.read(); // Clear buffer
  Serial2.println(pubCmd);
  Serial.println(">>> " + pubCmd);

  // Wait for '>' prompt
  unsigned long start = millis();
  bool prompted = false;
  while (millis() - start < 3000) {
    if (Serial2.available() && Serial2.read() == '>') {
      prompted = true;
      break;
    }
  }

  if (!prompted) {
    Serial.println(
        "[MQTT] No '>' prompt. Publish aborted. Marking disconnected.");
    mqtt_connected = false;
    return;
  }

  Serial2.print(json);
  Serial.println("[MQTT] Payload sent. Waiting for ack...");

  // Wait for +QMTPUBEX ack
  start = millis();
  String ackBuf = "";
  while (millis() - start < 6000) {
    if (Serial2.available()) {
      ackBuf += (char)Serial2.read();
      if (ackBuf.indexOf("+QMTPUBEX: 0,0,0") >= 0)
        break;
    }
  }

  if (ackBuf.indexOf("+QMTPUBEX: 0,0,0") >= 0) {
    Serial.println("[MQTT] PUBLISH SUCCESS!");
  } else {
    Serial.println("[MQTT] PUBLISH FAILED. Response: " + ackBuf);
    if (ackBuf.indexOf("ERROR") >= 0)
      mqtt_connected = false;
  }
  Serial.println();
}

// ════════════════════════════════════════════════════════════════════════════
// checkURC_direct — monitor unsolicited modem events
// ════════════════════════════════════════════════════════════════════════════
static void checkURC_direct() {
  while (Serial2.available()) {
    String line = Serial2.readStringUntil('\n');
    line.trim();
    if (line.length() == 0)
      continue;
    Serial.println("[URC] " + line);
    if (line.indexOf("+QMTSTAT:") >= 0) {
      Serial.println("[MQTT] Disconnected by broker.");
      mqtt_connected = false;
    }
  }
}

// ════════════════════════════════════════════════════════════════════════════
// updateNetworkStatus — AT+CSQ / AT+COPS / AT+QIACT → update globals
// ════════════════════════════════════════════════════════════════════════════
static void updateNetworkStatus() {
  String csq = sendATCmd("AT+CSQ", 2000);
  int idx = csq.indexOf("+CSQ: ");
  if (idx < 0)
    idx = csq.indexOf("+CSQ:");
  if (idx >= 0) {
    int comma = csq.indexOf(",", idx);
    if (comma >= 0) {
      int rssi = csq.substring(idx + 5, comma).toInt();
      g_signal_rssi = rssi;
      g_signal_dbm = (rssi == 99 || rssi == 0) ? 0 : -113 + (rssi * 2);
    }
  }

  String cops = sendATCmd("AT+COPS?", 3000);
  int q1 = cops.indexOf("\"");
  if (q1 >= 0) {
    int q2 = cops.indexOf("\"", q1 + 1);
    if (q2 >= 0)
      g_operator = cops.substring(q1 + 1, q2);
  }

  if (cops.indexOf(",7") >= 0)
    g_tech = "LTE Cat-M/NB";
  else if (cops.indexOf(",13") >= 0)
    g_tech = "LTE NB-IoT";
  else if (cops.indexOf(",2") >= 0)
    g_tech = "3G";
  else if (cops.indexOf(",0") >= 0)
    g_tech = "GSM";
  else
    g_tech = "LTE";

  String qiact = sendATCmd("AT+QIACT?", 2000);
  int qi = qiact.indexOf("\"");
  if (qi >= 0) {
    int qi2 = qiact.indexOf("\"", qi + 1);
    if (qi2 >= 0)
      g_ip = qiact.substring(qi + 1, qi2);
  }

  Serial.printf("[Net] Signal: %d dBm, Op: %s, IP: %s\n", g_signal_dbm,
                g_operator.c_str(), g_ip.c_str());
}

// ════════════════════════════════════════════════════════════════════════════
// TaskCore0 — runs on Core 0
// ════════════════════════════════════════════════════════════════════════════
void TaskCore0(void *pvParameters) {
  Serial.println("[Core0] Starting MQTT task...");

  // Step 1: Wait up to 8 s for ConnectivityManager to populate Device_id
  Serial.println("[Core0] Waiting for IMEI...");
  unsigned long waitStart = millis();
  while (Device_id.length() == 0 && millis() - waitStart < 8000)
    delay(500);

  // Step 2: If still empty, fetch IMEI directly
  if (Device_id.length() == 0) {
    Serial.println("[Core0] Fetching IMEI directly...");
    String imeiResp = sendATCmd("AT+GSN", 3000);
    int nl = imeiResp.indexOf('\n');
    if (nl >= 0) {
      String candidate = imeiResp.substring(nl + 1);
      candidate.trim();
      candidate.replace("OK", "");
      candidate.trim();
      if (candidate.length() > 5) {
        Device_id = candidate;
        mqtt_client_id = candidate;
      }
    }
  }

  if (Device_id.length() > 0) {
    mqtt_client_id = Device_id;
    MQTT_TOPIC = Device_id + "/aqi/data";
    Serial.println("[Core0] IMEI client ID : " + Device_id);
    Serial.println("[Core0] MQTT topic     : " + MQTT_TOPIC);
  } else {
    randomSeed(analogRead(0));
    Device_id = "esp32_aq_" + String(random(0xFFFF), HEX);
    mqtt_client_id = Device_id;
    MQTT_TOPIC = Device_id + "/aqi/data";
    Serial.println("[Core0] Fallback client ID: " + Device_id);
  }

  // Fetch network info, then connect
  updateNetworkStatus();

  if (connectMQTT_direct()) {
    g_net_status = "CONNECTED";
  } else {
    g_net_status = "RECONNECTING";
  }
  lastPublish = millis();
  unsigned long lastNetUpdate = millis();

  for (;;) {
    // Immediate reconnect if disconnected
    if (!mqtt_connected) {
      Serial.println("[MQTT] Disconnected. Reconnecting...");
      g_net_status = "RECONNECTING";
      if (connectMQTT_direct()) {
        g_net_status = "CONNECTED";
        lastPublish = millis();
      } else {
        delay(2000);
      }
    }

    // Periodic publish every 7 s
    if (mqtt_connected && (millis() - lastPublish >= PUBLISH_INTERVAL)) {
      publishData_direct();
      lastPublish = millis();
    }

    // Network status refresh every 30 s
    if (millis() - lastNetUpdate >= 30000) {
      updateNetworkStatus();
      lastNetUpdate = millis();
      g_net_status = mqtt_connected ? "CONNECTED" : "RECONNECTING";
    }

    // Check for unsolicited modem events
    checkURC_direct();

    delay(10);
  }
}
