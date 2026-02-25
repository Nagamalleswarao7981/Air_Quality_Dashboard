void Send_data_to_display()
{
  unsigned char hum[] = {0x5A, 0xA5, 0x05, 0x82, 0x10, 0x00, 0x00, 0x00};
  unsigned char temp[] = {0x5A, 0xA5, 0x05, 0x82, 0x11, 0x00, 0x00, 0x00};
  unsigned char co2[] = {0x5A, 0xA5, 0x05, 0x82, 0x12, 0x00, 0x00, 0x00};
  unsigned char hum_icon[] = {0x5A, 0xA5, 0x05, 0x82, 0x14, 0x00, 0x00, 0x00};
  unsigned char temp_icon[] = {0x5A, 0xA5, 0x05, 0x82, 0x13, 0x00, 0x00, 0x00};
  unsigned char co2_icon[] = {0x5A, 0xA5, 0x05, 0x82, 0x15, 0x00, 0x00, 0x00};

  int hum_t = static_cast<int>(humidity);
  int temp_t = static_cast<int>(temperature);
  int co2_t = static_cast<int>(co2Concentration);
  // Serial.println(hum_t); Serial.println(temp_t); Serial.println(co2_t);
  hum[6] = highByte(hum_t);
  hum[7] = lowByte(hum_t);
  co2[6] = highByte(co2_t);
  co2[7] = lowByte(co2_t);
  temp[6] = highByte(temp_t);
  temp[7] = lowByte(temp_t);
  if (humidity >= hum_th)
  {
    hum_icon[7] = 0X01;
    Serial1.write(hum_icon, sizeof(hum_icon));
    Serial1.write(hum, sizeof(hum));
  }
  else
  {
    hum_icon[7] = 0X00;
    Serial1.write(hum_icon, sizeof(hum_icon));
    Serial1.write(hum, sizeof(hum));
  }
  if (temperature >= temp_th)
  {
    temp_icon[7] = 0X01;
    Serial1.write(temp_icon, sizeof(temp_icon));
    Serial1.write(temp, sizeof(temp));
  }
  else
  {
    temp_icon[7] = 0X00;  // FIX: was incorrectly hum_icon[7]
    Serial1.write(temp_icon, sizeof(temp_icon));
    Serial1.write(temp, sizeof(temp));
  }
  if (co2Concentration >= co2_th)
  {
    co2_icon[7] = 0X01;
    Serial1.write(co2_icon, sizeof(co2_icon));
    Serial1.write(co2, sizeof(co2));
  }
  else
  {
    co2_icon[7] = 0X00;
    Serial1.write(co2_icon, sizeof(co2_icon));
    Serial1.write(co2, sizeof(co2));
  }


}
