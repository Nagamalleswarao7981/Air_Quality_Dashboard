// SensorHealthMonitor.h
#ifndef SENSOR_HEALTH_MONITOR_H
#define SENSOR_HEALTH_MONITOR_H

#include "SparkFun_SCD30_Arduino_Library.h"
#include <Arduino.h>
#include <Wire.h>

enum SensorStatus {
  STATUS_OK,
  STATUS_DISCONNECTED,
  STATUS_I2C_ERROR,
  STATUS_FROZEN,
  STATUS_INVALID_DATA,
  STATUS_SENSOR_FAILURE,
  STATUS_INITIALIZING
};

class SensorHealthMonitor {
public:
  SensorHealthMonitor();
  void init();
  void update(float co2, float temp, float hum, bool dataAvailable);
  SensorStatus getStatus();
  String getStatusString();
  String getStatusColor();
  int getErrorCount();
  int getConsecutiveErrors();
  unsigned long getLastUpdateAge();
  String getDataFreshness();
  bool attemptRecovery();
  void clearErrors();
  void resetSensor();

private:
  unsigned long lastUpdateTime;
  unsigned long lastChangeTime;
  float lastCO2, lastTemp, lastHum;
  int errorCount;
  int consecutiveErrors;
  int sameValueCount;
  SensorStatus currentStatus;
  bool sensorInitialized;

  // Validity ranges
  const float CO2_MIN = 350.0;
  const float CO2_MAX = 10000.0;
  const float TEMP_MIN = -40.0;
  const float TEMP_MAX = 85.0;
  const float HUM_MIN = 0.0;
  const float HUM_MAX = 100.0;

  // Thresholds
  const unsigned long FROZEN_THRESHOLD = 60000;
  const int MAX_CONSECUTIVE_ERRORS = 5;
  const int SAME_VALUE_THRESHOLD = 3;

  bool checkI2CConnection();
  bool isInvalidData(float co2, float temp, float hum);
  bool isFrozen(float co2, float temp, float hum);
  void handleNoData();
  void handleInvalidData();
};

// Global extern declaration
extern SensorHealthMonitor sensorHealth;
extern SCD30 airSensor;

#endif // SENSOR_HEALTH_MONITOR_H
