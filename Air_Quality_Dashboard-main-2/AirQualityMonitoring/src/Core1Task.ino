// Task 2 code to run on Core 1
void TaskCore1(void *pvParameters) {

  // Try to initialize sensor, but don't freeze if it fails
  if (airSensor.begin() == false) {
    Serial.println("Warning: Air sensor not detected on boot.");
  }

  // Initialize health monitoring
  sensorHealth.init();
  Serial.println("[Core1] Sensor health monitoring initialized");

  led_red();

  unsigned long lastSensorRead = 0;
  const unsigned long SENSOR_INTERVAL = 2500;

  while (1) {
    // Handle web requests continuously - FAST RESPONSE
    extern WebServer server;
    server.handleClient();

    // Only read sensors every 2.5 seconds
    if (millis() - lastSensorRead > SENSOR_INTERVAL) {
      lastSensorRead = millis();

      bool dataAvailable = airSensor.dataAvailable();

      if (dataAvailable) {
        Serial.print("co2(ppm):");
        co2Concentration = airSensor.getCO2();
        Serial.print(co2Concentration);

        Serial.print(" temp(C):");
        temperature = airSensor.getTemperature();
        Serial.print(temperature, 1);

        Serial.print(" humidity(%):");
        humidity = airSensor.getHumidity();
        Serial.print(humidity, 1);

        Serial.println();
      } else {
        Serial.println("Waiting for new data");
      }

      // Update sensor health monitoring
      sensorHealth.update(co2Concentration, temperature, humidity,
                          dataAvailable);

      Serial.print("co2Concentration: ");
      Serial.println(co2Concentration);
      Serial.print("temperature: ");
      Serial.println(temperature);
      Serial.print("humidity: ");
      Serial.println(humidity);
      Send_data_to_display();
      if (co2Concentration != 0 || temperature != 0)
        publish_str = String(temperature) + "," + String(humidity) + "," +
                      String(co2Concentration);
    }
  }
}
