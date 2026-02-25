// Production-Grade Sensor Health Monitor for SCD30
#include "SensorHealthMonitor.h"

// SensorHealthMonitor constructor
SensorHealthMonitor::SensorHealthMonitor() {
  lastUpdateTime = 0;
  lastChangeTime = 0;
  lastCO2 = -1;
  lastTemp = -999;
  lastHum = -1;
  errorCount = 0;
  consecutiveErrors = 0;
  sameValueCount = 0;
  currentStatus = STATUS_INITIALIZING;
  sensorInitialized = false;
}

void SensorHealthMonitor::init() {
  currentStatus = STATUS_INITIALIZING;
  errorCount = 0;
  consecutiveErrors = 0;
  sameValueCount = 0;
  lastUpdateTime = millis();
  lastChangeTime = millis();
  sensorInitialized = checkI2CConnection();

  if (!sensorInitialized) {
    currentStatus = STATUS_DISCONNECTED;
    Serial.println("[HEALTH] Sensor not detected on I2C bus");
  } else {
    currentStatus = STATUS_OK;
    Serial.println("[HEALTH] Sensor initialized successfully");
  }
}

void SensorHealthMonitor::update(float co2, float temp, float hum,
                                 bool dataAvailable) {
  if (!dataAvailable) {
    handleNoData();
    return;
  }

  if (isInvalidData(co2, temp, hum)) {
    handleInvalidData();
    return;
  }

  if (isFrozen(co2, temp, hum)) {
    currentStatus = STATUS_FROZEN;
    errorCount++;
    Serial.println("[HEALTH] Frozen data detected");
    return;
  }

  lastCO2 = co2;
  lastTemp = temp;
  lastHum = hum;
  lastUpdateTime = millis();
  lastChangeTime = millis();
  consecutiveErrors = 0;
  sameValueCount = 0;

  if (currentStatus != STATUS_OK) {
    Serial.println("[HEALTH] Sensor recovered to OK state");
  }
  currentStatus = STATUS_OK;
}

SensorStatus SensorHealthMonitor::getStatus() {
  if (millis() - lastUpdateTime > 300000 &&
      currentStatus != STATUS_DISCONNECTED) {
    return STATUS_SENSOR_FAILURE;
  }
  return currentStatus;
}

String SensorHealthMonitor::getStatusString() {
  switch (currentStatus) {
  case STATUS_OK:
    return "OK";
  case STATUS_DISCONNECTED:
    return "DISCONNECTED";
  case STATUS_I2C_ERROR:
    return "I2C_ERROR";
  case STATUS_FROZEN:
    return "FROZEN";
  case STATUS_INVALID_DATA:
    return "INVALID_DATA";
  case STATUS_SENSOR_FAILURE:
    return "SENSOR_FAILURE";
  case STATUS_INITIALIZING:
    return "INITIALIZING";
  default:
    return "UNKNOWN";
  }
}

String SensorHealthMonitor::getStatusColor() {
  switch (currentStatus) {
  case STATUS_OK:
    return "#10b981";
  case STATUS_FROZEN:
    return "#f59e0b";
  case STATUS_INVALID_DATA:
    return "#f59e0b";
  case STATUS_DISCONNECTED:
    return "#ef4444";
  case STATUS_I2C_ERROR:
    return "#ef4444";
  case STATUS_SENSOR_FAILURE:
    return "#ef4444";
  case STATUS_INITIALIZING:
    return "#64748b";
  default:
    return "#64748b";
  }
}

int SensorHealthMonitor::getErrorCount() { return errorCount; }

int SensorHealthMonitor::getConsecutiveErrors() { return consecutiveErrors; }

unsigned long SensorHealthMonitor::getLastUpdateAge() {
  return (millis() - lastUpdateTime) / 1000;
}

String SensorHealthMonitor::getDataFreshness() {
  unsigned long age = getLastUpdateAge();
  if (age < 10)
    return "FRESH";
  if (age < 60)
    return "STALE";
  return "FROZEN";
}

bool SensorHealthMonitor::attemptRecovery() {
  Serial.println("[HEALTH] Attempting sensor recovery...");

  if (!checkI2CConnection()) {
    Serial.println("[HEALTH] I2C connection failed");
    return false;
  }

  if (airSensor.begin() == false) {
    Serial.println("[HEALTH] Sensor re-initialization failed");
    consecutiveErrors++;
    currentStatus = STATUS_SENSOR_FAILURE;
    return false;
  }

  Serial.println("[HEALTH] Sensor recovery successful");
  consecutiveErrors = 0;
  currentStatus = STATUS_OK;
  lastUpdateTime = millis();
  return true;
}

void SensorHealthMonitor::clearErrors() {
  errorCount = 0;
  consecutiveErrors = 0;
  sameValueCount = 0;
  Serial.println("[HEALTH] Error counters cleared");
}

void SensorHealthMonitor::resetSensor() {
  Serial.println("[HEALTH] Performing sensor reset...");
  Wire.endTransmission();
  delay(100);
  Wire.begin();
  delay(100);
  attemptRecovery();
}

bool SensorHealthMonitor::checkI2CConnection() {
  Wire.beginTransmission(0x61);
  byte error = Wire.endTransmission();

  if (error != 0) {
    Serial.println("[HEALTH] SCD30 not found on I2C (0x61)");
    return false;
  }

  Serial.println("[HEALTH] SCD30 detected on I2C");
  return true;
}

bool SensorHealthMonitor::isInvalidData(float co2, float temp, float hum) {
  if (co2 == 0 && temp == 0 && hum == 0) {
    Serial.println("[HEALTH] All zero values detected");
    return true;
  }

  if (co2 < CO2_MIN || co2 > CO2_MAX) {
    Serial.printf("[HEALTH] CO2 out of range: %.1f ppm\n", co2);
    return true;
  }

  if (temp < TEMP_MIN || temp > TEMP_MAX) {
    Serial.printf("[HEALTH] Temperature out of range: %.1f C\n", temp);
    return true;
  }

  if (hum < HUM_MIN || hum > HUM_MAX) {
    Serial.printf("[HEALTH] Humidity out of range: %.1f %%\n", hum);
    return true;
  }

  return false;
}

bool SensorHealthMonitor::isFrozen(float co2, float temp, float hum) {
  if (co2 == lastCO2 && temp == lastTemp && hum == lastHum) {
    sameValueCount++;

    if (sameValueCount >= SAME_VALUE_THRESHOLD) {
      unsigned long timeSinceChange = millis() - lastChangeTime;
      if (timeSinceChange > FROZEN_THRESHOLD) {
        Serial.printf("[HEALTH] Data frozen for %lu seconds\n",
                      timeSinceChange / 1000);
        return true;
      }
    }
  } else {
    sameValueCount = 0;
    lastChangeTime = millis();
  }

  return false;
}

void SensorHealthMonitor::handleNoData() {
  consecutiveErrors++;
  errorCount++;

  Serial.printf("[HEALTH] No data available (consecutive: %d)\n",
                consecutiveErrors);

  if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
    if (!checkI2CConnection()) {
      currentStatus = STATUS_DISCONNECTED;
      Serial.println("[HEALTH] Sensor disconnected from I2C bus");
    } else {
      currentStatus = STATUS_I2C_ERROR;
      Serial.println("[HEALTH] Sensor not responding (I2C error)");
    }

    if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS * 2) {
      attemptRecovery();
    }
  }
}

void SensorHealthMonitor::handleInvalidData() {
  consecutiveErrors++;
  errorCount++;
  currentStatus = STATUS_INVALID_DATA;
  Serial.println("[HEALTH] Invalid data received");

  if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
    currentStatus = STATUS_SENSOR_FAILURE;
    attemptRecovery();
  }
}

// Global instance
SensorHealthMonitor sensorHealth;
