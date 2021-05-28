#include "Wire.h"
#include "DS3231.h"
#include "GyverButton.h"

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

RTClib RTC;
DS3231 DS;

#define CL_CENTER_X 4
#define CL_CENTER_Y 4

void ShowTime() {
  DateTime time = RTC.now();

  byte x = CL_CENTER_X; // 4
  byte y = CL_CENTER_Y; // 20
  display.clearDisplay();
  display.setCursor(x, y);
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(3);
  display.printf("%02d:%02d", time.hour(), time.minute());

  display.setCursor(x + 94, y + 7);
  display.setTextSize(2);
  display.printf("%02d", time.second());

  display.setCursor(x + 12, y + 29);
  display.printf("%02d/%02d/%02d", time.day(), time.month(), time.year() % (time.year() / 100 * 100));

  display.setCursor(x + 24, y + 50);
  display.setTextSize(1);
  display.println("Wednesday");

  display.drawLine(x, y + 24, x + 32, y + 24, SSD1306_WHITE);
  display.display();
}

void setup() {
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();
}

void loop() {
  ShowTime();
}
