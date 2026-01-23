String Read_from_gsm()
{
  String str = "";
  while (Serial2.available() > 0)
    str = Serial2.readString();
  Serial.println(str);
  return str;
}

void test_gsm()
{
  Serial2.println("AT");
  delay(1000);
  Read_from_gsm();
  Serial2.println("AT");
  delay(1000);
  Read_from_gsm();
  
}

void Find_imei()
{
  int Count = 0;
Step1:
  String response = "";
  //char convert[20];
  Serial2.write("AT+GSN\r\n");
  delay(500);
  response = Read_from_gsm();
  response = response.substring(response.indexOf("\n") + 1, response.lastIndexOf("OK") - 4);

  Serial.print("IMEI NO :"); Serial.println(response);
  Device_id = response;
  publish_topic1 = "AQM" + response + "/s/AIR_QUALITY/v1/raw";
  subscribe_topic1 = "AQM" + response + "/p/AIR_QUALITY/v1/raw";
  led_blue(); delay(50);led_null();delay(50);led_blue(); delay(50);led_null();delay(50);led_blue(); delay(50);led_null();delay(50);led_blue(); delay(50);led_null();
}






void Incoming_Data(String Input)
{
  Input = Input.substring(Input.indexOf("\",\"", 10) + 3, Input.lastIndexOf("\""));
  Serial.print("INSIDE STRING : "); Serial.println(Input);
if (Input.indexOf("interval:") >= 0)
  {
    Gsm_publish(publish_topic1, "RECEIVED NEW INTERVAL");
    String Interval = Input.substring(Input.indexOf(":") + 1);
    char charArray[Interval.length() + 1];
    Interval.toCharArray(charArray, sizeof(charArray));
    publish_interval_shrt = atoi(charArray);

  }
  else if (Input.indexOf("threshold:") >= 0)
  {
    int colonIndex = Input.indexOf(":") + 1;
    String parameters = Input.substring(colonIndex);
    int commaIndex1 = parameters.indexOf(",");
    String para1 = parameters.substring(0, commaIndex1);
    temp_th = atof(para1.c_str());
    int commaIndex2 = parameters.indexOf(",", commaIndex1 + 1);
    String para2 = parameters.substring(commaIndex1 + 1, commaIndex2);
    hum_th = atof(para2.c_str());
    String para3 = parameters.substring(commaIndex2 + 1);
    co2_th = atof(para3.c_str());
    Gsm_publish(publish_topic1, "RECEIVED NEW THRESHNOLD");
  }


  preferences.begin("my-app", false);
  // Write data to EEPROM
  writeDataToEEPROM();
  // Close the preferences
  preferences.end();
  publish_interval = publish_interval_shrt * 60 * 1000;

}
