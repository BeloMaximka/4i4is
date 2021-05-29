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
#define GN_CENTER_Y 28

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

struct
{
  TimeInfo time{};
  bool isPaused = false;
  bool isEnded = true;
} timer;

struct
{
  TimeInfo time{};
  bool isPaused = false;
  bool isStarted = false;
} stopwatch;

byte psec = -1;

enum STATES
{
  MAIN_YEAR,
  MAIN_MONTH,
  MAIN_DAY,
  MAIN_WEEKDAY,
  MAIN_HOUR,
  MAIN_MINUTE,
  MAIN_SECOND,
};

void setupButtons() {
  button1.setDebounce(10); // 80
  button1.setTimeout(500); // 500
  button1.setClickTimeout(0); // 300
  button2.setDebounce(10); // 80
  button2.setTimeout(500); // 500
  button2.setClickTimeout(0); // 300
  button3.setDebounce(10); // 80
  button3.setTimeout(500); // 500
  button3.setClickTimeout(0); // 300
}

void resetButtons() {
  button1.resetStates();
  button2.resetStates();
  button3.resetStates();
}
// УДАЛИ
long time2sec(byte h, byte m, byte s) {
  return h * 60 + m * 60 + s;
}

long time2sec(TimeInfo t) {
  return t.hour * 60 + t.minute * 60 + t.second;
}

TimeInfo sec2time(long sec) {
  TimeInfo time;
  time.hour = sec / 3600;
  sec -= time.hour * 3600;
  time.minute = sec / 60;
  sec -= time.minute * 60;
  time.second = sec;
}
// УДАЛИ ВЫШЕ

bool incSec(TimeInfo& time) {
  if (time.second == 59)
  {
    time.second = 0;
    if (time.minute == 59)
    {
      time.minute = 0;
      if (time.hour == 23)
      {
        time.second = 59;
        time.minute = 59;
        return false;
      }
      else time.hour++;
    }
    else time.minute++;
  }
  else time.second++;
  return true;
}

bool decSec(TimeInfo& time) {
  if (time.second == 0)
  {
    time.second = 59;
    if (time.minute == 0)
    {
      time.minute = 59;
      if (time.hour == 0)
      {
        time.second = 0;
        time.minute = 0;
        return false;
      }
      else time.hour--;
    }
    else time.minute--;
  }
  else time.second--;
  return true;
}

bool secPassed() {
  DateTime time = RTC.now();
  if (psec != time.second())
  {
    psec = time.second();
    return true;
  }
  return false;
}

void UpdateTime() {
  if (secPassed())
  {
    if (!timer.isPaused && !timer.isEnded) {
      if (!decSec(timer.time)) {
        timer.isEnded = true;
        timer.isPaused = false;
      }
    }
    if (!stopwatch.isPaused && stopwatch.isStarted) {
      if (!incSec(stopwatch.time)) {
        stopwatch.isStarted = false;
        stopwatch.isPaused = true;
      }
    }
  }
}

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
  case MAIN_YEAR: display.drawLine(x + 84, y + 45, x + 105, y + 45, SSD1306_WHITE); // year
    break;
  case MAIN_MONTH: display.drawLine(x + 48, y + 45, x + 69, y + 45, SSD1306_WHITE); // month
    break;
  case MAIN_DAY: display.drawLine(x + 12, y + 45, x + 33, y + 45, SSD1306_WHITE); // day
    break;
  case MAIN_WEEKDAY: drawWordUnderline(getCenterPosX(weekDays[weekDay - 1]), y + 50, weekDays[weekDay - 1]); // week day
    break;
  case MAIN_HOUR: display.drawLine(x, y + 24, x + 32, y + 24, SSD1306_WHITE); // hour
    break;
  case MAIN_MINUTE: display.drawLine(x + 54, y + 24, x + 86, y + 24, SSD1306_WHITE); // minute
    break;
  case MAIN_SECOND: display.drawLine(x + 94, y + 24, x + 114, y + 24, SSD1306_WHITE); // second
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

  byte posX = getCenterPosX(text);
  display.setCursor(posX, 0);
  display.print(text);

  showState(state, x, y, -1);
  display.display();
}

FullTimeInfo getFullTimeSnapshot() {
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

TimeInfo getTimeSnapshot() {
  DateTime snapshot = RTC.now();
  TimeInfo time;
  time.hour = snapshot.hour();
  time.minute = snapshot.minute();
  time.second = snapshot.second();
  return time;
}

void setTime() {
  resetButtons();
  FullTimeInfo time = getFullTimeSnapshot();
  int state = MAIN_YEAR;
  while (true)
  {
    tickbuttons();
    if (button1.isHold())
    {
      time = {};
      state = MAIN_HOUR;
    }
    else if (button1.isPress())
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
      case MAIN_YEAR: inc(time.year, 0, 99);
        break;
      case MAIN_MONTH: inc(time.month, 1, 12);
        break;
      case MAIN_DAY:
        if (time.month == 2 && isleapYear(time.year + 2000))
        {
          inc(time.day, 1, 29);
        }
        else
        {
          inc(time.day, 1, daysInMonth[time.month - 1]);
        }
        break;
      case MAIN_WEEKDAY: inc(time.weekDay, 1, 7);
        break;
      case MAIN_HOUR: inc(time.hour, 0, 23);
        break;
      case MAIN_MINUTE: inc(time.minute, 0, 59);
        break;
      case MAIN_SECOND: inc(time.second, 0, 59);
        break;
      }
    }
    else if (button3.isPress())
    {
      switch (state)
      {
      case MAIN_YEAR: dec(time.year, 0, 99);
        break;
      case MAIN_MONTH: dec(time.month, 1, 12);
        break;
      case MAIN_DAY:
        if (time.month == 2 && isleapYear(time.year + 2000))
        {
          dec(time.day, 1, 29);
        }
        else
        {
          dec(time.day, 1, daysInMonth[time.month - 1]);
        }
        break;
      case MAIN_WEEKDAY: dec(time.weekDay, 1, 7);
        break;
      case MAIN_HOUR: dec(time.hour, 0, 23);
        break;
      case MAIN_MINUTE: dec(time.minute, 0, 59);
        break;
      case MAIN_SECOND: dec(time.second, 0, 59);
        break;
      }
    }
    showTime(time, state);
  }
}

void setTimer(TimeInfo time = {}) {
  resetButtons();
  int state = MAIN_HOUR;
  secPassed();
  while (true)
  {
    tickbuttons();
    if (button1.isHold())
    {
      time = {};
      state = MAIN_HOUR;
    }
    else if (button1.isSingle())
    {
      if (state == MAIN_SECOND)
      {
        timer.time = time;
        timer.isEnded = false;
        timer.isPaused = false;

        showGeneral("Sync...", time, state);
        while (!secPassed());
        return;
      }
      state++;
    }
    else if (button2.isSingle())
    {
      switch (state)
      {
      case MAIN_HOUR: inc(time.hour, 0, 23);
        break;
      case MAIN_MINUTE: inc(time.minute, 0, 59);
        break;
      case MAIN_SECOND: inc(time.second, 0, 59);
        break;
      }
    }
    else if (button3.isSingle())
    {
      switch (state)
      {
      case MAIN_HOUR: dec(time.hour, 0, 23);
        break;
      case MAIN_MINUTE: dec(time.minute, 0, 59);
        break;
      case MAIN_SECOND: dec(time.second, 0, 59);
        break;
      }
    }
    showGeneral("Timer", time, state);
  }
}

void mainMenu() {
  resetButtons();
  while (true)
  {
    showTime();
    tickbuttons();
    if (button1.isSingle()) setTime();
    else if (button2.isSingle()) return;
    else if (button3.isSingle()) return;
  }
}

void timerMenu() {
  resetButtons();
  while (true)
  {
    UpdateTime();
    if (timer.isPaused) showGeneral("Timer(psd)", timer.time, -1);
    else showGeneral("Timer", timer.time, -1);
    tickbuttons();
    if (button1.isHold()) setTimer(timer.time);
    else if (button1.isSingle()) {
      if (timer.isEnded) setTimer(timer.time);
      else timer.isPaused = timer.isPaused ? false : true;
    }
    else if (button2.isSingle()) return;
    else if (button3.isSingle()) return;
  }
}

void stopwatchMenu() {
  resetButtons();
  while (true)
  {
    UpdateTime();
    if (stopwatch.isPaused && stopwatch.isStarted) showGeneral("Stwch psd", stopwatch.time, -1);
    else showGeneral("Stopwatch", stopwatch.time, -1);
    tickbuttons();
    if (button1.isHold()) { 
      stopwatch.isStarted = false;
      stopwatch.time = {}; 
    }
    else if (button1.isSingle()) {
      if (stopwatch.isStarted)
      {
        stopwatch.isPaused = stopwatch.isPaused ? false : true;
      }
      stopwatch.isStarted = true; 
    }
    else if (button2.isSingle()) return;
    else if (button3.isSingle()) return;
  }
}

void setup() {
  Serial.begin(9600);

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.display();

  setupButtons();
  DS3231 test;
  test.setHour(23); test.setMinute(9); test.setSecond(54);
}

void loop() {
  stopwatchMenu();
  timerMenu();
  mainMenu();
}
