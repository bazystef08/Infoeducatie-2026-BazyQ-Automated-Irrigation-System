#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <DHT.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

#define BLYNK_TEMPLATE_ID "TMPL4mFYbqtY0"
#define BLYNK_TEMPLATE_NAME "BazyQ"
#define BLYNK_AUTH_TOKEN "HuFEI2zsFufmI8RQUlI_e3-wstvVwSAL"
#include <BlynkSimpleWifi.h>
BlynkTimer timer;
const char* ssid = "BZY WIFI BUC";
const char* password = "Bazy1977";

#define SOIL_PIN A0
const int dry = 750; 
const int wet = 320;

#define BATTERY_PIN A1

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define PUMP_PIN 3

#define BUTTON_PIN 4
#define LED_PIN 5
bool UseMode = false; // False - Auto ; True - Manual ;

#define TRIG_PIN 6
#define ECHO_PIN 7
bool notif = false;

void WriteCentered(const char* text, int y)
{
  int textwidth = u8g2.getStrWidth(text);
  int x = (128 - textwidth) / 2;
  u8g2.drawStr(x, y, text);
}

BLYNK_WRITE(V1)
{
  if (param.asInt() == 1) UseMode = true;
  else UseMode = false;
  digitalWrite(LED_PIN, UseMode);
}

void SendDataToBlynk()
{
  Blynk.virtualWrite(V1, UseMode ? 1 : 0);
  Blynk.virtualWrite(V2, dht.readTemperature());
  Blynk.virtualWrite(V3, dht.readHumidity());
  Blynk.virtualWrite(V4, Soil());
  Blynk.virtualWrite(V5, WaterLevel());
  Blynk.virtualWrite(V6, Battery());
}

int Soil()
{
  int SoilPercent = constrain(map(analogRead(SOIL_PIN), dry, wet, 0, 100), 0, 100);
  return SoilPercent;
}

int WaterLevel()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 25000);
  int cm = duration * 0.0343 / 2;
  if(cm >= 50) return 0;
  else
  {
    int waterDist = constrain(map(cm, 12, 2, 0, 100), 0, 100);
    return waterDist;
  } 
}

int Battery()
{
  int rawBat = analogRead(BATTERY_PIN);
  double vBat = (rawBat * 5.0 / 1023.0) * 5.0;
  int batPct = constrain(map(vBat * 100, 600, 840, 0, 100), 0, 100);
  return batPct;
}

void setup()
{
  Serial.begin(9600);
  u8g2.begin();
  u8g2.setI2CAddress(0x3C * 2);
  u8g2.setFont(u8g2_font_ncenB08_tr); 

// Custom Intro Message
  u8g2.clearBuffer();
  WriteCentered("BazyQ", 15);
  WriteCentered("Avem curent!", 35);
  WriteCentered("Arduino Uno R4 WIFI", 55);
  u8g2.sendBuffer();

// Air Temp and Humid
  dht.begin();

// Soil Humid
  pinMode(SOIL_PIN, INPUT);

// Pump
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);

// AUTO / MANUAL Button and LED
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

// HC-SR04 Sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  delay(5000);
  
// Connect Blynk
  u8g2.clearBuffer();
  WriteCentered("Conectare Blynk...", 35);
  u8g2.sendBuffer();
  Blynk.config(BLYNK_AUTH_TOKEN);
  WiFi.begin(ssid, password);
  timer.setInterval(1000L, SendDataToBlynk);

  delay(1000);
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED) Blynk.run();
  timer.run();

  if(WaterLevel() <= 5)
  {
    digitalWrite(PUMP_PIN, LOW);
    
    Blynk.logEvent("rezervor_gol", "Alerta: Rezervorul BazyQ este gol! Umpleti vasul.");
    notif = true;

    u8g2.clearBuffer();

    u8g2.drawRFrame(5, 5, 118, 54, 4);
    u8g2.drawRFrame(7, 7, 114, 50, 2);
    u8g2.setFont(u8g2_font_ncenB10_tr);
    WriteCentered("ALERTA!", 25);
    u8g2.setFont(u8g2_font_ncenB08_tr);
    WriteCentered("REZERVOR GOL", 40);
    WriteCentered("Umpleti vasul!", 52);

    u8g2.sendBuffer();
  }
  else
  {
    notif = false;
    u8g2.clearBuffer();

    u8g2.setCursor(0, 15);
    if (Blynk.connected()) u8g2.print("Blynk: CONECTAT");
    else u8g2.print("Blynk: OFFLINE");

    u8g2.setCursor(0, 30);
    u8g2.print("AirTemp: ");
    u8g2.print(dht.readTemperature(), 1);
    u8g2.print(" C");

    u8g2.setCursor(0, 45);
    u8g2.print("AirHumid: ");
    u8g2.print(dht.readHumidity(), 1);
    u8g2.print(" %");

    u8g2.setCursor(0, 60);
    u8g2.print("SoilHumid: ");
    u8g2.print(Soil(), 1);
    u8g2.print(" %");

    int barWidth = map(Battery(), 0, 100, 0, 18);
    if (barWidth > 0)
    {
      u8g2.drawRFrame(98, 20, 22, 11, 2);
      u8g2.drawBox(120, 23, 2, 5);
      u8g2.drawBox(100, 22, barWidth, 7);
    }
    else
    {
      u8g2.drawRFrame(93, 20, 28, 13, 2);
      u8g2.drawBox(120, 23, 2, 5);
      u8g2.setCursor(95, 30);
      u8g2.print("USB");
    }

    u8g2.sendBuffer();

    if(UseMode) digitalWrite(PUMP_PIN, HIGH);
    else
    {
      if (Soil() < 50) digitalWrite(PUMP_PIN, HIGH);
      else digitalWrite(PUMP_PIN, LOW);
    }

  }

  if (digitalRead(BUTTON_PIN) == LOW)
  {
    UseMode = !UseMode;
    digitalWrite(LED_PIN, UseMode);
    while(digitalRead(BUTTON_PIN) == LOW) delay(10);
    delay(50);
  }

  delay(10);
}