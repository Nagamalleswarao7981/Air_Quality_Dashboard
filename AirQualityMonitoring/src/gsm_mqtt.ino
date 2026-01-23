void gsm_mqtt()
{
  //String str=String(Temp)+","+String(Hum)+","+String(Co2_);
 led_green();
  Serial2.write("AT+QICSGP=1,1,\"airteliot.com\"\r\n");
  delay(500);
  Read_from_gsm();
  Serial2.write("AT+QMTCFG=\"version\",0,4\r\n");
  delay(500);
  Read_from_gsm();
  Serial2.write("AT+QMTCFG=\"keepalive\",0,60\r\n");
  delay(500);
  Read_from_gsm();
  Serial2.print("AT+QMTOPEN=0,\"" + mqttbroker + "\",1883\r\n");
  delay(500);
  Read_from_gsm();
  Serial2.print("AT+QMTCONN=0,\"" + Device_id + "\"\r\n");
  delay(500);
  String resultt=Serial2.readString();
  String subString = "ERROR";
//
//int found = ;
//  Serial.println(resultt);
//  resultt="";
//   resultt=Serial2.readString();
  Serial.println(resultt.indexOf(subString));
  if(resultt.indexOf(subString)!=-1)
  {
    Serial2.write("AT+QMTDISC=0\r\n");
    Serial.println("esp32 restarting now..");
    delay(100);
    led_yellow();
    ESP.restart();
  }
  Serial2.write("AT+QMTSUB=0,1,\"");
  Serial2.print(subscribe_topic1);
  Serial2.write("\",0\r\n");
  delay(500);
  Read_from_gsm();
  //  Serial2.write("AT+QMTDISC=0\r\n");
  //  delay(500);
  //  Read_from_gsm();
}

void Gsm_publish(String T_p, String str)
{
  //String str=String(Temp)+","+String(Hum)+","+String(Co2_);
  Serial2.write("AT+QMTPUBEX=0,0,0,0,\"");
  Serial2.print(T_p);
  Serial2.write("\",");
  Serial2.print(str.length());
  Serial2.write("\r\n");
  delay(300);
  Serial2.write(str.c_str(), str.length());
  delay(100);
  Read_from_gsm();
  //  Serial2.write("AT+QMTDISC=0\r\n");
  //  delay(500);
  //  Read_from_gsm();
  Serial.print("GSM MQTT:"); Serial.println(str.c_str());
  led_purpule();
}
