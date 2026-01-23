

void led_red() {
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.show();
}

void led_blue() {
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(255, 215, 0));
  pixels.show();
}

void led_green() {
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(0, 0, 255));
  pixels.show();
}

void led_null() {
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();
}

void led_yellow() {
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(255, 255, 0));
  pixels.show();
}

void led_purpule() {
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(160, 32, 240));
  pixels.show();
}
