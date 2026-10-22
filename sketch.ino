
#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Wi-Fi
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// Replace with a newly generated ThingSpeak Write API Key
const char* TS_KEY = "EYQQ5L2L89UYNELH";
const char* TS_URL = "https://api.thingspeak.com/update";

// ESP32 pins
#define TEMP_PIN 23
#define PULSE_PIN 35
#define BUZZER_PIN 25

// Demonstration alert thresholds
const float LOW_SPO2 = 92.0;
const float HIGH_TEMP_F = 101.0;
const int LOW_HR = 60;
const int HIGH_HR = 100;

// Sensors and LCD
OneWire oneWire(TEMP_PIN);
DallasTemperature sensor(&oneWire);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Readings
float tempC = 0, tempF = 32;
int heartRate = 75, spo2 = 97;

// Alert states
bool lowOxygen = false;
bool highTemp = false;
bool lowHR = false;
bool highHR = false;

// Timers
unsigned long lastUpload = 0;
unsigned long lastScreen = 0;
unsigned long lastBeep = 0;
unsigned long lastPrint = 0;
unsigned long lastVariation = 0;
unsigned long lastOxygenChange = 0;

// Simulation state
int screen = 0;
int hrVariation = 0;
bool simulateLowOxygen = false;

void setup() {
  Serial.begin(115200);

  pinMode(PULSE_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  sensor.begin();
  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Elderly Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  randomSeed(micros());

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 6);
  Serial.print("Connecting to WiFi");

  for (int i = 0;
       i < 50 && WiFi.status() != WL_CONNECTED; i++) {
    delay(300);
    Serial.print(".");
  }

  Serial.println(WiFi.status() == WL_CONNECTED
                 ? "\nWiFi connected"
                 : "\nWiFi unavailable");
}

void loop() {
  unsigned long now = millis();

  // Read DS18B20 temperature
  sensor.requestTemperatures();
  float t = sensor.getTempCByIndex(0);

  if (t != DEVICE_DISCONNECTED_C && t > -55 && t < 125) {
    tempC = t;
  }

  tempF = tempC * 1.8 + 32.0;

  // Heart rate baseline from potentiometer
  int raw = analogRead(PULSE_PIN);
  int baseHR = map(raw, 0, 4095, 50, 130);

  // Vary heart rate every 3 seconds
  if (now - lastVariation >= 3000) {
    lastVariation = now;
    hrVariation = random(-4, 5);
  }

  heartRate = constrain(baseHR + hrVariation, 50, 130);

  // Alternate low and normal oxygen automatically
  // Normal: 94-99%; Low: 85-91%
  if (now - lastOxygenChange >= 10000) {
    lastOxygenChange = now;
    simulateLowOxygen = !simulateLowOxygen;
  }

  spo2 = simulateLowOxygen
         ? random(85, 92)
         : random(94, 100);

  // Calculate alerts
  lowOxygen = spo2 < LOW_SPO2;
  highTemp = tempF > HIGH_TEMP_F;
  lowHR = heartRate < LOW_HR;
  highHR = heartRate > HIGH_HR;

  bool alert = lowOxygen || highTemp || lowHR || highHR;

  // Buzzer while an alert is active
  if (alert && now - lastBeep >= 1500) {
    tone(BUZZER_PIN, 1200, 250);
    lastBeep = now;
  }

  // Rotate LCD screens every 2.5 seconds
  if (now - lastScreen >= 2500) {
    lastScreen = now;
    screen = (screen + 1) % 3;
    lcd.clear();

    if (screen == 0) {
      lcd.setCursor(0, 0);
      lcd.printf("Temp:%.1f%cF", tempF, (char)223);
      lcd.setCursor(0, 1);
      lcd.printf("HR:%d BPM", heartRate);
    }
    else if (screen == 1) {
      lcd.setCursor(0, 0);
      lcd.printf("SpO2:%d%%", spo2);
      lcd.setCursor(0, 1);
      lcd.print(lowOxygen ? "LOW OXYGEN!" : "Oxygen: NORMAL");
    }
    else {
      lcd.setCursor(0, 0);
      lcd.print("ALERT STATUS:");
      lcd.setCursor(0, 1);

      if (highTemp) lcd.print("HIGH TEMP!");
      else if (lowHR) lcd.print("LOW HEART RATE");
      else if (highHR) lcd.print("HIGH HEART RATE");
      else if (lowOxygen) lcd.print("LOW OXYGEN!");
      else lcd.print("Status: NORMAL");
    }
  }

  // Serial Monitor readings every 2 seconds
  if (now - lastPrint >= 2000) {
    lastPrint = now;

    Serial.println("\n--- LIVE SENSOR READINGS ---");
    Serial.printf("Temperature: %.1f F\n", tempF);
    Serial.printf("Heart Rate: %d BPM\n", heartRate);
    Serial.printf("SpO2: %d%%\n", spo2);

    Serial.printf("Low Oxygen: %s\n", lowOxygen ? "YES" : "NO");
    Serial.printf("High Temperature: %s\n", highTemp ? "YES" : "NO");
    Serial.printf("Low Heart Rate: %s\n", lowHR ? "YES" : "NO");
    Serial.printf("High Heart Rate: %s\n", highHR ? "YES" : "NO");
  }

  // ThingSpeak upload every 20 seconds
  if (now - lastUpload >= 20000) {
    lastUpload = now;
    sendToThingSpeak();
  }

  delay(100);
}

void sendToThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Upload skipped: WiFi disconnected");
    return;
  }

  if (String(TS_KEY) == "UWM73A8N7CO9VELX") {
    Serial.println("Set your new ThingSpeak Write API Key");
    return;
  }

  int heartStatus = lowHR ? -1 : highHR ? 1 : 0;

  String url = String(TS_URL) +
    "?api_key=" + String(TS_KEY) +
    "&field1=" + String(tempF, 1) +
    "&field2=" + String(heartRate) +
    "&field3=" + String(spo2) +
    "&field4=" + String(lowOxygen ? 1 : 0) +
    "&field5=" + String(highTemp ? 1 : 0) +
    "&field6=" + String(heartStatus);

  HTTPClient http;
  http.begin(url);

  int code = http.GET();
  String response = http.getString();

  Serial.printf("ThingSpeak HTTP code: %d\n", code);
  Serial.println("ThingSpeak entry response: " + response);

  http.end();
}
