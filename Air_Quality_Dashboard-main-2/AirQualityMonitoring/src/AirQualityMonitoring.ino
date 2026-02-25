#include "SensorHealthMonitor.h"
#include "config.h"

void setupWeb(); // Forward declaration

SemaphoreHandle_t mutex;
TaskHandle_t Task1;
TaskHandle_t Task2;

void setup() {
  pixels.begin();
  Serial.begin(115200);
  Serial2.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, 18, 19);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  mutex = xSemaphoreCreateMutex();

  led_red();
  delay(50);
  led_null();
  delay(50);
  led_red();
  delay(50);
  led_null();
  delay(50);
  led_red();
  delay(50);
  led_null();
  Wire.begin();
  preferences.begin("my-app", false);
  readDataFromEEPROM();
  preferences.end();
  publish_interval = publish_interval * 60 * 1000;

  // Setup WiFi AP for Dashboard
  WiFi.softAP("AQM_Industrial", "12345678");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Setup Web Server
  setupWeb();

  // Create task 1 on Core 0
  xTaskCreatePinnedToCore(TaskCore0,   // Function to run on Core 0
                          "TaskCore0", // Name of the task
                          10000,       // Stack size (bytes)
                          NULL,        // Parameters to pass to the task
                          1,           // Task priority
                          &Task1,      // Task handle
                          1);          // Core number (0 for Core 0)

  // Create task 2 on Core 1
  xTaskCreatePinnedToCore(TaskCore1,   // Function to run on Core 1
                          "TaskCore1", // Name of the task
                          10000,       // Stack size (bytes)
                          NULL,        // Parameters to pass to the task
                          1,           // Task priority
                          &Task2,      // Task handle
                          0);          // Core number (1 for Core 1)
}

void loop() {
  // delay(100);
}
