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

GButton button1(8);
GButton button2(6);
GButton button3(5);

#define BUZZ_PIN 9

#define CL_CENTER_X 4
#define CL_CENTER_Y 4
#define GN_CENTER_X 4
#define GN_CENTER_Y 20

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

struct InterruptClock
{
  TimeInfo time{};
  bool isPaused = false;
  bool isDisabled = true;
} timer, alarm;

struct
{
  TimeInfo time{};
  bool isPaused = false;
  bool isStarted = false;
} stopwatch;

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

enum MENUS
{
  MENU_FIRST,
  MENU_MAIN,
  MENU_STOPWATCH,
  MENU_TIMER,
  MENU_ALARM,
  MENU_LAST,
};

byte psec = -1;
byte menuState = MENU_MAIN;

void playSound() {
  static long soundTime;
  if (millis() - soundTime > 650) {
    tone(BUZZ_PIN, 500, 400);
    soundTime = millis();
  }
}
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

bool compareTime(TimeInfo t1, TimeInfo t2) {
  if (t1.hour == t2.hour && t1.minute == t2.minute && t1.second == t2.second) return true;
  else return false;
}

void updateTime() {
  if (secPassed())
  {
    if (!timer.isPaused && !timer.isDisabled) {
      if (!decSec(timer.time)) {
        timer.isDisabled = true;
        timer.isPaused = false;
        CheckMenu("Timer", "Ended!", timer.time);
      }
    }
    if (!stopwatch.isPaused && stopwatch.isStarted) {
      if (!incSec(stopwatch.time)) {
        stopwatch.isStarted = false;
        stopwatch.isPaused = true;
      }
    }
    if (!alarm.isDisabled && !alarm.isPaused && compareTime(getTimeSnapshot(), alarm.time)) {
      CheckMenu("Alarm", "Ended!", alarm.time);
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

void showGeneral(const char* upText, const char* downText, TimeInfo time, byte state) {
  byte x = GN_CENTER_X; // 4
  byte y = GN_CENTER_Y; // 20
  display.clearDisplay();
  display.setCursor(x, y);

  display.setTextSize(3);
  display.printf("%02d:%02d", time.hour, time.minute);

  display.setCursor(x + 94, y + 7);
  display.setTextSize(2);
  display.printf("%02d", time.second);

  byte posX = getCenterPosX(upText);
  display.setCursor(posX, 0);
  display.print(upText);

  posX = getCenterPosX(downText);
  display.setCursor(posX, 48);
  display.print(downText);

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

void setMainTime() {
  resetButtons();
  FullTimeInfo time = getFullTimeSnapshot();
  int state = MAIN_YEAR;
  while (true)
  {
    updateTime();
    tickbuttons();
    if (button1.isHold())
    {
      time = {};
      state = MAIN_HOUR;
    }
    else if (button1.isSingle())
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
        resetButtons();
        return;
      }
      state++;
    }
    else if (button2.isSingle())
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
    else if (button3.isSingle())
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

InterruptClock setTime(const char* Text, TimeInfo time = {}) {
  InterruptClock clock;
  resetButtons();
  int state = MAIN_HOUR;
  secPassed();
  while (true)
  {
    updateTime();
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
        clock.time = time;
        clock.isDisabled = false;
        clock.isPaused = false;

        showGeneral(Text, "Sync...", time, state);
        while (!secPassed());
        resetButtons();
        return clock;
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
    showGeneral(Text, "Editing", time, state);
  }
}

void mainMenu() {
  resetButtons();
  while (true)
  {
    updateTime();
    showTime();
    tickbuttons();
    if (button1.isSingle()) setMainTime();
    else if (button2.isSingle()) {
      dec(menuState, MENU_FIRST + 1, MENU_LAST - 1);
      return;
    }
    else if (button3.isSingle()) {
      inc(menuState, MENU_FIRST + 1, MENU_LAST - 1);
      return;
    }
  }
}

void timerMenu() {
  resetButtons();
  while (true)
  {
    updateTime();
    if (timer.isPaused) showGeneral("Timer", "Paused", timer.time, -1);
    else if (timer.isDisabled) showGeneral("Timer", "Ended", timer.time, -1);
    else showGeneral("Timer", "Running", timer.time, -1);
    tickbuttons();
    if (button1.isHold()) timer = setTime("Timer", timer.time);
    else if (button1.isSingle()) {
      if (timer.isDisabled) timer = setTime("Timer", timer.time);
      else timer.isPaused = timer.isPaused ? false : true;
    }
    else if (button2.isSingle()) {
      dec(menuState, MENU_FIRST + 1, MENU_LAST - 1);
      return;
    }
    else if (button3.isSingle()) {
      inc(menuState, MENU_FIRST + 1, MENU_LAST - 1);
      return;
    }
  }
}

void stopwatchMenu() {
  resetButtons();
  while (true)
  {
    updateTime();
    if (stopwatch.isPaused && stopwatch.isStarted) showGeneral("Stopwatch", "Paused", stopwatch.time, -1);
    else if (!stopwatch.isStarted) showGeneral("Stopwatch", "Waiting", stopwatch.time, -1);
    else showGeneral("Stopwatch", "Running", stopwatch.time, -1);
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
    else if (button2.isSingle()) {
      dec(menuState, MENU_FIRST + 1, MENU_LAST - 1);
      return;
    }
    else if (button3.isSingle()) {
      inc(menuState, MENU_FIRST + 1, MENU_LAST - 1);
      return;
    }
  }
}

void alarmMenu() {
  resetButtons();
  while (true)
  {
    updateTime();
    if (alarm.isDisabled) showGeneral("Alarm", "Disabled", alarm.time, -1);
    else showGeneral("Alarm", "Enabled", alarm.time, -1);
    tickbuttons();
    if (button1.isHold()) alarm = setTime("Alarm", alarm.time);
    else if (button1.isSingle()) alarm.isDisabled = alarm.isDisabled ? false : true;
    else if (button2.isSingle()) {
      dec(menuState, MENU_FIRST + 1, MENU_LAST - 1);
      return;
    }
    else if (button3.isSingle()) {
      inc(menuState, MENU_FIRST + 1, MENU_LAST - 1);
      return;
    }
  }
}

void CheckMenu(const char* upText, const char* downText, TimeInfo time) {
  while (true)
  {
    playSound();
    updateTime();
    showGeneral(upText, downText, time, -1);
    tickbuttons();
    if (button1.isSingle() || button2.isSingle() || button3.isSingle()) return;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(BUZZ_PIN, OUTPUT);

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setRotation(2);
  display.display();

  setupButtons();
  DS3231 test;
  test.setHour(23); test.setMinute(9); test.setSecond(54);
}

void loop() {
  switch (menuState)
  {
  case MENU_MAIN: mainMenu();
    break;
  case MENU_STOPWATCH: stopwatchMenu();
    break;
  case MENU_TIMER: timerMenu();
    break;
  case MENU_ALARM: alarmMenu();
    break;
  }
}
