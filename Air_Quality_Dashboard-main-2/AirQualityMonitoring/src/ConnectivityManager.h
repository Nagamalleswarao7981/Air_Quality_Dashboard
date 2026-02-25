#ifndef CONNECTIVITY_MANAGER_H
#define CONNECTIVITY_MANAGER_H

#include "config.h"
#include <Arduino.h>

enum NetworkState {
  NET_STATE_INITIALIZING,
  NET_STATE_CHECKING_SIM,
  NET_STATE_REGISTERING,
  NET_STATE_CONFIGURING_APN,
  NET_STATE_CONNECTING_MQTT,
  NET_STATE_CONNECTED,
  NET_STATE_ERROR,
  NET_STATE_WAITING
};

struct SignalInfo {
  int rssi;
  int ber;
  int dbm;
  int percentage;
};

struct NetworkInfo {
  String status;
  String operatorName;
  String accessTechnology;
  String ip;
  String mcc;
  String mnc;
  String iccid;
  String imsi;
  String simStatus;
};

class ConnectivityManager {
public:
  ConnectivityManager();
  void init();
  void update();

  // Configuration
  void updateConfig(String newApn, String newBroker, int newPort,
                    String newUser, String newPass);
  void scanNetworks();               // Async scan
  void selectNetwork(String mccmnc); // Set manual network
  void setAutoNetwork();             // Set auto mode

  // MQTT Actions
  bool publish(String topic, String payload);
  bool subscribe(String topic);
  void setCallback(void (*callback)(String, String));
  void checkMessages();

  // Status Getters
  String getStatusString();
  String getStatusColor();
  NetworkState getState();
  SignalInfo getSignalStrength();
  NetworkInfo getNetworkInfo();
  bool isMqttConnected();
  String getLastError();
  String getScannerResult(); // Returns JSON of scanned networks

  // Utils
  void restartModem();
  String executeATCommand(String cmd, unsigned long timeout = 1000);
  void performOTA(String url, String version);

private:
  NetworkState currentState;
  unsigned long lastStateChange;
  unsigned long lastUpdate;
  unsigned long lastSignalUpdate;

  // Config
  String _apn;
  String _broker;
  int _port;
  String _user;
  String _pass;
  bool _manualMode;
  String _plmn;

  // Modem State
  bool modemReady;
  bool simReady;
  bool netRegistered;
  bool mqttConnected;
  String lastError;
  String scanResultJson;
  bool scanRequested;

  // Helpers
  void handleState();
  void sendAT(String cmd);
  String sendATWait(String cmd, unsigned long timeout = 1000);
  bool checkSim();
  bool checkRegistration();
  bool configureAPN();
  bool configureMQTT();
  bool connectMQTT();
  void updateSignal();
  void parseSignal(String response);
  void parseRegistration(String response);

  // Callback for incoming messages
  void (*mqttCallback)(String topic, String payload) = nullptr;
};

extern ConnectivityManager network;

#endif
