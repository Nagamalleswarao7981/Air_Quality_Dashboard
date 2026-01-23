void writeDataToEEPROM() {

  // Write integer to EEPROM
  preferences.putInt("pub_tm", publish_interval_shrt);

  // Write floats to EEPROM
  preferences.putFloat("temp_th", temp_th);
  preferences.putFloat("hum_th", hum_th);
  preferences.putFloat("co2_th", co2_th);

  Serial.println("Data written to EEPROM");
}

void readDataFromEEPROM() {
  
  // Read integer from EEPROM
  publish_interval_shrt = preferences.getInt("pub_tm", 0);
  if(publish_interval_shrt<1)
  publish_interval_shrt=2;
  publish_interval = publish_interval_shrt;
  // Read floats from EEPROM
  temp_th = preferences.getFloat("temp_th", 0.0);
  hum_th = preferences.getFloat("hum_th", 0.0);
  co2_th = preferences.getFloat("co2_th", 0.0);

  // Print the read data
  Serial.println("Read Data from EEPROM:");
  Serial.println("publish_interval: " + String(publish_interval_shrt));
  Serial.println("temp_th: " + String(temp_th, 2));
  Serial.println("hum_th: " + String(hum_th, 2));
  Serial.println("co2_th: " + String(co2_th, 2));
}
