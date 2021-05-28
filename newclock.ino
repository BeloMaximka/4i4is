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

const char* weekDays[7] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };
struct TimeInfo
{
    byte hour;
    byte minute;
    byte second;
    byte day;
    byte month;
    byte year; // last two digits
    byte weekDay; // 1-7
};

void drawWordUnderline(byte x, byte y, const char* word) {
    display.drawLine(x, y, x + strlen(word) * 5, y, SSD1306_WHITE);;
}

void showState(byte state, byte x, byte y, byte weekDay) {
    switch (state)
    {
    case 0: display.drawLine(x, y + 24, x + 32, y + 24, SSD1306_WHITE);
        break;
    case 1: display.drawLine(x + 54, y + 24, x + 86, y + 24, SSD1306_WHITE);
        break;
    case 2: display.drawLine(x + 94, y + 24, x + 114, y + 24, SSD1306_WHITE);
        break;
    case 3: display.drawLine(x + 12, y + 45, x + 33, y + 45, SSD1306_WHITE);
        break;
    case 4: display.drawLine(x + 48, y + 45, x + 69, y + 45, SSD1306_WHITE);
        break;
    case 5: display.drawLine(x + 84, y + 45, x + 105, y + 45, SSD1306_WHITE);
        break;
    case 6: drawWordUnderline(x + 24, y + 59, weekDays[weekDay - 1]);
        break;
    }
}

void showTime(TimeInfo time, byte state) {
    byte x = CL_CENTER_X; // 4
    byte y = CL_CENTER_Y; // 20
    display.clearDisplay();
    display.setCursor(x, y);

    display.setTextSize(3);
    display.printf("%02d:%02d", time.hour, time.minute);

    display.setCursor(x + 94, y + 7);
    display.setTextSize(2);
    display.printf("%02d", time.second);

    display.setCursor(x + 12, y + 29);
    display.printf("%02d/%02d/%02d", time.day, time.month, time.year);

    display.setCursor(x + 24, y + 50);
    display.setTextSize(1);
    display.println(weekDays[time.weekDay - 1]);

    showState(state, x, y, time.weekDay);
    display.display();
}

void showTime() {
    DateTime time = RTC.now();

    byte x = CL_CENTER_X; // 4
    byte y = CL_CENTER_Y; // 20
    display.clearDisplay();
    display.setCursor(x, y);

    display.setTextSize(3);
    display.printf("%02d:%02d", time.hour(), time.minute());

    display.setCursor(x + 94, y + 7);
    display.setTextSize(2);
    display.printf("%02d", time.second());

    display.setCursor(x + 12, y + 29);
    display.printf("%02d/%02d/%02d", time.day(), time.month(), time.year() % (time.year() / 100 * 100));

    display.setCursor(x + 24, y + 50);
    display.setTextSize(1);
    display.println(weekDays[DS.getDoW() - 1]);

    display.display();
}

void setup() {
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  DS3231 test;
  test.setHour(23); test.setMinute(59); test.setSecond(54);
  TimeInfo timetest{3,3,3,3,3,3,6};
  for(int i = 0; i < 7; i++){
    showTime(timetest,i);
    delay(2000);
  }
}

void loop() {
  showTime();
}
