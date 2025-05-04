#include "DHT.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <SoftWire.h>
#include "MeoConnect.h"
#include "MeoMessage.h"

#define DHT11Pin 0
#define DHTType DHT11
#define BUTTON_PIN 3
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

MeoConnect meo_con = MeoConnect();
MeoMessage meo_me = MeoMessage();

Adafruit_MPU6050 mpu;
DHT HT(DHT11Pin, DHTType);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

volatile bool buttonPressed = false;
int displayMode = 0;  // 0 = DHT11, 1 = MPU6050

void IRAM_ATTR handleButtonPress() {
  buttonPressed = true;
}

float humi;
float tempC;
float tempF;
float x;
float y;
float z;

void oledDisplayHeader() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Humidity");
  display.setCursor(60, 0);
  display.print("Temperature");
}

void oledDisplay(int size, int x, int y, float value, String unit) {
  int charLen = 12;
  int xo = x + charLen * 3.2;
  int xunit = x + charLen * 3.6;
  int xval = x;

  display.setTextSize(size);
  display.setTextColor(WHITE);

  if (unit == "%") {
    display.setCursor(x, y);
    display.print(value, 0);
    display.print(unit);
  } else {
    if (value > 99) {
      xval = x;
    } else {
      xval = x + charLen;
    }
    display.setCursor(xval, y);
    display.print(value, 0);
    display.drawCircle(xo, y + 2, 2, WHITE);
    display.setCursor(xunit, y);
    display.print(unit);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  HT.begin();
  mpu.begin();
  meo_con.setWifiConfig("tao khong biet luon", "aimabietdc");
  meo_con.setMqttConfig("192.168.50.242", 1883);
  meo_con.initConfig();

  if (!mpu.begin()) {
    Serial.println("Failed to initialize MPU6050!");
    while (1)
      ;
  }
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);

  display.display();
  delay(1000);
  display.clearDisplay();
}

void loop() {
  if (!meo_con.client.connected()) {
    meo_con.reconnect();
    meo_con.client.subscribe("#");
  }
  if (buttonPressed) {
    delay(20);                             // Debounce
    if (digitalRead(BUTTON_PIN) == LOW) {  // Confirm button press
      displayMode = !displayMode;
    }
    buttonPressed = false;
  }

  display.clearDisplay();
    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);
    

  if (displayMode == 0) {

    humi = HT.readHumidity();
    tempC = HT.readTemperature();
    tempF = HT.readTemperature(true);

    if (isnan(humi) || isnan(tempC) || isnan(tempF)) {
      Serial.println("Failed to read from DHT sensor!");

      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(10, 20);
      display.print("Sensor Error!");
      display.display();

      return;
    }

    Serial.print("Humidity: ");
    Serial.print(humi, 0);
    Serial.print("%  Temperature: ");
    Serial.print(tempC, 1);
    Serial.print("C ~ ");
    Serial.print(tempF, 1);
    Serial.println("F");

    display.clearDisplay();
    oledDisplayHeader();
    oledDisplay(3, 5, 28, humi, "%");
    oledDisplay(2, 70, 16, tempC, "C");
    oledDisplay(2, 70, 44, tempF, "F");

    display.display();
  } else {

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("MPU6050 Sensor");
    display.setCursor(0, 20);
    display.print("X: ");
    display.print(accel.acceleration.x);
    display.setCursor(0, 30);
    display.print("Y: ");
    display.print(accel.acceleration.y);
    display.setCursor(0, 40);
    display.print("Z: ");
    display.print(accel.acceleration.z);

    display.display();
  }

  if (digitalRead(BUTTON_PIN)){
    meo_me.textMessageSetter("0");
    meo_con.pubMessageToTopic(meo_me.messageStorage, "meo3/Switch");
    meo_me.reset();
    delay(500);
  }
  else{
  meo_me.textMessageSetter("1");
    meo_con.pubMessageToTopic(meo_me.messageStorage, "meo3/Switch");
    meo_me.reset();
    delay(500);
  }
}