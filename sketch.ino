#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Wokwi simulation: DS18B20 temperature + potentiometer-based simulated pulse/SpO2.
// IMPORTANT: Potentiometer values are demonstrations, NOT real medical measurements.
// Add your own ThingSpeak channel Write API Key below.

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";
const char* THINGSPEAK_WRITE_KEY = "PASTE_YOUR_THINGSPEAK_WRITE_API_KEY";
const char* THINGSPEAK_URL = "http://api.thingspeak.com/update";

#define TEMP_PIN 23
#define PULSE_SIM_PIN 35
#define BUZZER_PIN 25

OneWire oneWire(TEMP_PIN);
DallasTemperature tempSensor(&oneWire);
LiquidCrystal_I2C lcd(0x27, 16, 2);

float temperatureC = 0;
int heartRateBpm = 0;
int simulatedSpO2 = 0;
unsigned long lastUpload = 0;
unsigned long lastScreen = 0;
bool showVitals = false;

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 6);
  Serial.print("Connecting to Wokwi WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? "\nWiFi connected" : "\nWiFi not connected; local simulation continues");
}

void setup() {
  Serial.begin(115200);
  pinMode(PULSE_SIM_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  tempSensor.begin();
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Elderly Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  connectWiFi();
  delay(1000);
}

void loop() {
  tempSensor.requestTemperatures();
  float t = tempSensor.getTempCByIndex(0);
  if (t != DEVICE_DISCONNECTED_C && t > -55 && t < 125) temperatureC = t;

  int raw = analogRead(PULSE_SIM_PIN);
  heartRateBpm = map(raw, 0, 4095, 50, 130);
  simulatedSpO2 = map(raw, 0, 4095, 90, 100);

  // Demo-only alert thresholds; not intended for clinical use.
  bool demoAlert = temperatureC >= 38.0 || heartRateBpm < 50 || heartRateBpm > 120 || simulatedSpO2 < 92;
  if (demoAlert) tone(BUZZER_PIN, 1200, 180);
  else noTone(BUZZER_PIN);

  if (millis() - lastScreen >= 2500) {
    lastScreen = millis();
    showVitals = !showVitals;
    lcd.clear();
    if (!showVitals) {
      lcd.setCursor(0, 0); lcd.print("Temp:");
      lcd.print(temperatureC, 1); lcd.print((char)223); lcd.print("C");
      lcd.setCursor(0, 1); lcd.print("Pulse:");
      lcd.print(heartRateBpm); lcd.print(" BPM");
    } else {
      lcd.setCursor(0, 0); lcd.print("SpO2*:");
      lcd.print(simulatedSpO2); lcd.print("%");
      lcd.setCursor(0, 1); lcd.print(demoAlert ? "DEMO ALERT!" : "Status: Demo OK");
    }
  }

  Serial.print("Temperature_C="); Serial.print(temperatureC, 1);
  Serial.print(", HeartRate_BPM_SIM="); Serial.print(heartRateBpm);
  Serial.print(", SpO2_SIM="); Serial.print(simulatedSpO2);
  Serial.print(", Alert_SIM="); Serial.println(demoAlert ? "YES" : "NO");

  // ThingSpeak free channels generally require >=15 seconds between updates.
  if (millis() - lastUpload >= 20000) {
    lastUpload = millis();
    sendToThingSpeak();
  }
  delay(500);
}

void sendToThingSpeak() {
  if (String(THINGSPEAK_WRITE_KEY) == "PASTE_YOUR_THINGSPEAK_WRITE_API_KEY") {
    Serial.println("ThingSpeak skipped: paste your channel Write API Key in sketch.ino");
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("ThingSpeak skipped: WiFi not connected");
    return;
  }
  HTTPClient http;
  String url = String(THINGSPEAK_URL) + "?api_key=" + THINGSPEAK_WRITE_KEY +
    "&field1=" + String(temperatureC, 1) +
    "&field2=" + String(heartRateBpm) +
    "&field3=" + String(simulatedSpO2) +
    "&field4=" + String(analogRead(PULSE_SIM_PIN)) +
    "&field5=" + String(temperatureC >= 38.0 ? 1 : 0);
  http.begin(url);
  int code = http.GET();
  String response = http.getString();
  Serial.print("ThingSpeak HTTP code: "); Serial.println(code);
  Serial.print("ThingSpeak entry response: "); Serial.println(response);
  http.end();
}
