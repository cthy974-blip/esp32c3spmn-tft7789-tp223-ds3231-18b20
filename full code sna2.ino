//full code sna2
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>


#include "frame1.h"
#include "frame2.h"
#include "frame3.h"
#include "frame4.h"
#include "frame5.h"
#include "frame6.h"


#define TFT_MOSI 3
#define TFT_SCLK 4
#define TFT_RST 2
#define TFT_DC 1
#define TFT_CS -1

#define DS3231_SDA 8
#define DS3231_SCL 10
#define ONE_WIRE_BUS 0
#define TOUCH_PIN 7

Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);
RTC_DS3231 rtc;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


const uint16_t* frames[] = { frame1, frame2, frame3, frame4, frame5, frame6 };
const int totalFrames = 6;

int mode = 1;
int frameIndex = 0;
bool greetingShown = false;
bool forceRefresh = true;

bool lastTouchState = false;
unsigned long touchStartTime = 0;
const unsigned long holdThreshold = 1000;

float lastTemp = -999;
int lastMinute = -1;
int lastDay = -1;
String lastGreeting = "";

//hiệu ứng đổi màu chữ
unsigned long lastColorChange = 0;
int colorIndex = 0;
const unsigned long colorInterval = 200;
uint16_t scrollColors[] = { ST77XX_BLUE, ST77XX_MAGENTA, ST77XX_GREEN, ST77XX_RED };

void drawCenteredTextWithBackground(int y, int size, uint16_t textColor, uint16_t bgColor, String text) {
  tft.setTextSize(size);
  tft.setTextColor(textColor);
  tft.setTextWrap(false);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  tft.fillRect(0, y, 240, h, bgColor);
  tft.setCursor((240 - w) / 2, y);
  tft.print(text);
}

//lời chúc sinh nhật với màu đổi mỗi 0.5s
void showBirthdayMessage() {
  unsigned long now = millis();
  if (now - lastColorChange >= colorInterval) {
    lastColorChange = now;
    colorIndex = (colorIndex + 1) % 4;
  }

  tft.fillScreen(ST77XX_WHITE);


  int h1 = 20;  // HAPPY BIRTHDAY
  int h2 = 30;  // Cong Thuong
  int h3 = 30;  // 21 tuoi!!!
  int totalHeight = h1 + h2 + h3 + 20;  // thêm khoảng cách giữa các dòng

  int startY = (240 - totalHeight) / 2;

  drawCenteredTextWithBackground(startY, 2, scrollColors[colorIndex], ST77XX_WHITE, "HAPPY BIRTHDAY");
  drawCenteredTextWithBackground(startY + h1 + 10, 3, ST77XX_BLUE, ST77XX_WHITE, "Cong Thuong");
  drawCenteredTextWithBackground(startY + h1 + h2 + 20, 3, ST77XX_RED, ST77XX_WHITE, "21 tuoi!!!");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(TOUCH_PIN, INPUT);
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.init(240, 240, SPI_MODE3);
  tft.setRotation(1);
  tft.setSPISpeed(8000000);
  tft.fillScreen(ST77XX_BLACK);

  Wire.begin(DS3231_SDA, DS3231_SCL);
  rtc.begin();

  // rtc.adjust(DateTime(2025, 10, 15, 0, 55, 0)); // Cập nhật giờ thủ công nếu cần

  DateTime now = rtc.now();
  Serial.print("Giờ DS3231: ");
  Serial.printf("%02d/%02d/%04d %02d:%02d:%02d\n", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());

  sensors.begin();
  tft.drawRGBBitmap(0, 0, frames[0], 240, 240);
}

void loop() {
  bool touchState = digitalRead(TOUCH_PIN);
  unsigned long nowMillis = millis();

  if (touchState && !lastTouchState) {
    touchStartTime = nowMillis;
  }

  if (!touchState && lastTouchState) {
    unsigned long pressDuration = nowMillis - touchStartTime;

    if (pressDuration >= holdThreshold) {
      if (mode == 1) {
        mode = 2;
        forceRefresh = true;
        lastTemp = -999;
        lastMinute = -1;
        lastDay = -1;
        lastGreeting = "";
        tft.fillScreen(ST77XX_BLACK);
      } else {
        mode = 1;
        frameIndex = 0;
        greetingShown = false;
        colorIndex = 0;
        lastColorChange = 0;
        tft.fillScreen(ST77XX_BLACK);
        tft.drawRGBBitmap(0, 0, frames[0], 240, 240);
      }
    } else {
      if (mode == 1) {
        if (!greetingShown) {
          tft.fillScreen(ST77XX_WHITE);
          greetingShown = true;
        } else {
          frameIndex++;
          if (frameIndex >= totalFrames) frameIndex = 1;
          tft.drawRGBBitmap(0, 0, frames[frameIndex], 240, 240);
        }
      }
    }
  }

  lastTouchState = touchState;

  if (mode == 1 && greetingShown && frameIndex == 0) {
    showBirthdayMessage();
  }

  if (mode == 2) {
    DateTime now = rtc.now();
    sensors.requestTemperatures();
    float tempC = sensors.getTempCByIndex(0);
    float displayTemp = tempC - 4.0;

    int greetingY = 10;
    int tempY = 50;
    int dateY = 180;
    int timeY = ((tempY + dateY) / 2) - 10;

    String greetingLine = "heyu_thuong";

    if (greetingLine != lastGreeting) {
      drawCenteredTextWithBackground(greetingY, 2, ST77XX_CYAN, ST77XX_BLACK, greetingLine);
      lastGreeting = greetingLine;
    }

    if (abs(tempC - lastTemp) > 0.1) {
      lastTemp = tempC;
      String tempStr = (tempC == -127.0) ? "Err" : String(displayTemp, 1) + " C";
      uint16_t tempColor = ST77XX_WHITE;
      if (displayTemp > 31.0) tempColor = ST77XX_RED;
      else if (displayTemp < 23.0) tempColor = ST77XX_BLUE;
      drawCenteredTextWithBackground(tempY, 3, tempColor, ST77XX_BLACK, tempStr);
    }

    if (now.minute() != lastMinute) {
      lastMinute = now.minute();
      char timeStr[16];
      sprintf(timeStr, "%02d:%02d", now.hour(), now.minute());
      drawCenteredTextWithBackground(timeY, 6, ST77XX_CYAN, ST77XX_BLACK, String(timeStr));
    }

    if (now.day() != lastDay) {
      lastDay = now.day();
      char dateStr[20];
      sprintf(dateStr, "%02d/%02d/%04d", now.day(), now.month(), now.year());
      drawCenteredTextWithBackground(dateY, 3, ST77XX_MAGENTA, ST77XX_BLACK, String(dateStr));
    }

    forceRefresh = false;
  }

  delay(20);
}