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

GButton button1(10);
GButton button2(9);
GButton button3(8);

#define CL_CENTER_X 4
#define CL_CENTER_Y 4
#define GN_CENTER_X 4
#define GN_CENTER_Y 16

const char* weekDays[7] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };
const uint8_t daysInMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

struct FullTimeInfo
{
    byte hour;
    byte minute;
    byte second;
    byte day;
    byte month;
    byte year; // last two digits
    byte weekDay; // 1-7
};

struct TimeInfo
{
    byte hour;
    byte minute;
    byte second;
};

inline void tickbuttons() {
    button1.tick();
    button2.tick();
    button3.tick();
}
void inc(byte& var, byte min, byte max) {
    if (var >= max) var = min;
    else var++;
}

void dec(byte& var, byte min, byte max) {
    if (var <= min) var = max;
    else var--;
}

byte getCenterPosX(const char* word) {
    uint16_t w;
    display.getTextBounds(word, 0, 0, nullptr, nullptr, &w, nullptr);
    return ((display.width() - w) / 2);
}
void drawWordUnderline(byte x, byte y, const char* word) {
    uint16_t h, w;
    display.getTextBounds(word, 0, 0, nullptr, nullptr, &w, &h);
    y += h + 1;
    display.drawLine(x, y, x + w, y, SSD1306_WHITE);;
}

void showState(byte state, byte x, byte y, byte weekDay) {
    switch (state)
    {
    case 0: display.drawLine(x + 84, y + 45, x + 105, y + 45, SSD1306_WHITE); // year
        break;
    case 1: display.drawLine(x + 48, y + 45, x + 69, y + 45, SSD1306_WHITE); // month
        break;
    case 2: display.drawLine(x + 12, y + 45, x + 33, y + 45, SSD1306_WHITE); // day
        break;
    case 3: drawWordUnderline(getCenterPosX(weekDays[weekDay - 1]), y + 50, weekDays[weekDay - 1]); // week day
        break;
    case 4: display.drawLine(x, y + 24, x + 32, y + 24, SSD1306_WHITE); // hour
        break;
    case 5: display.drawLine(x + 54, y + 24, x + 86, y + 24, SSD1306_WHITE); // minute
        break;
    case 6: display.drawLine(x + 94, y + 24, x + 114, y + 24, SSD1306_WHITE); // second
        break;
    }
}

void showTime(FullTimeInfo time, byte state) {
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

    display.setTextSize(1);
    byte centerX = getCenterPosX(weekDays[time.weekDay - 1]);
    display.setCursor(centerX, y + 50);
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

    display.setTextSize(1);
    byte centerX = getCenterPosX(weekDays[DS.getDoW() - 1]);
    display.setCursor(centerX, y + 50);
    display.println(weekDays[DS.getDoW() - 1]);

    display.display();
}

void showGeneral(const char* text, TimeInfo time, byte state) {
    byte x = GN_CENTER_X; // 4
    byte y = GN_CENTER_Y; // 20
    display.clearDisplay();
    display.setCursor(x, y);

    display.setTextSize(3);
    display.printf("%02d:%02d", time.hour, time.minute);

    display.setCursor(x + 94, y + 7);
    display.setTextSize(2);
    display.printf("%02d", time.second);
}

FullTimeInfo getTimeSnapshot() {
    DateTime snapshot = RTC.now();
    FullTimeInfo time;
    time.day = snapshot.day();
    time.month = snapshot.month();
    time.year = snapshot.year() % (snapshot.year() / 100 * 100);
    time.hour = snapshot.hour();
    time.minute = snapshot.minute();
    time.second = snapshot.second();
    time.weekDay = DS.getDoW();
    return time;
}

void setTime() {
    FullTimeInfo time = getTimeSnapshot();
    int state = 0;
    while (true)
    {
        tickbuttons();
        if (button1.isPress())
        {
            if (state == 6)
            {
                DS.setSecond(0);
                DS.setDoW(time.weekDay);

                DS.setYear(time.year);
                DS.setMonth(time.month);
                DS.setDate(time.day);

                DS.setHour(time.hour);
                DS.setMinute(time.minute);
                DS.setSecond(time.second);
                return;
            }
            state++;
        }
        else if (button2.isPress())
        {
            switch (state)
            {
            case 0: inc(time.year, 0, 99);
                break;
            case 1: inc(time.month, 1, 12);
                break;
            case 2:
                if (time.month == 2 && isleapYear(time.year + 2000))
                {
                    inc(time.day, 1, 29);
                }
                else
                {
                    inc(time.day, 1, daysInMonth[time.month - 1]);
                }
                break;
            case 3: inc(time.weekDay, 1, 7);
                break;
            case 4: inc(time.hour, 1, 23);
                break;
            case 5: inc(time.minute, 1, 59);
                break;
            case 6: inc(time.second, 1, 59);
                break;
            }
        }
        else if (button3.isPress())
        {
            switch (state)
            {
            case 0: dec(time.year, 0, 99);
                break;
            case 1: dec(time.month, 1, 12);
                break;
            case 2:
                if (time.month == 2 && isleapYear(time.year + 2000))
                {
                    dec(time.day, 1, 29);
                }
                else
                {
                    dec(time.day, 1, daysInMonth[time.month - 1]);
                }
                break;
            case 3: dec(time.weekDay, 1, 7);
                break;
            case 4: dec(time.hour, 1, 23);
                break;
            case 5: dec(time.minute, 1, 59);
                break;
            case 6: dec(time.second, 1, 59);
                break;
            }
        }
        showTime(time, state);
    }
}

void TimeMenu() {
    while (true)
    {
        showTime();
        tickbuttons();
        if (button1.isPress()) setTime();
        else if (button2.isPress()) return;
        else if (button3.isPress()) return;
    }
}

void setup() {
    Serial.begin(9600);
    display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
    display.setTextColor(SSD1306_WHITE);
    display.clearDisplay();
    DS3231 test;
    test.setHour(23); test.setMinute(59); test.setSecond(54);
}

void loop() {
    showTime();
    tickbuttons();
    if (button1.isPress()) setTime();
}
