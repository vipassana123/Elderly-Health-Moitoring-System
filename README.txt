# Elderly Health Monitoring Demo

## Wokwi project files
Upload/import `wokwi/sketch.ino`, `wokwi/diagram.json`, and `wokwi/libraries.txt` into a new ESP32 project on Wokwi.

### Circuit
- DS18B20: VCC -> 3V3, GND -> GND, DQ -> GPIO 23
- Potentiometer: VCC -> 3V3, GND -> GND, SIG -> GPIO 35 (demo pulse input)
- Buzzer: pin 2 -> GPIO 25, pin 1 -> GND
- I2C LCD: SDA -> GPIO 21, SCL -> GPIO 22, VCC -> VIN, GND -> GND

If the LCD library/component is not recognised in your Wokwi project, add `LiquidCrystal I2C` using the Library Manager and confirm the LCD's I2C address is 0x27.

## ThingSpeak setup
1. Create/sign into a ThingSpeak account.
2. Create a new channel and enable:
   - Field 1: Temperature_C
   - Field 2: HeartRate_BPM_DEMO
   - Field 3: SpO2_DEMO
   - Field 4: PotentiometerRaw
   - Field 5: DemoAlert
3. Open the channel's API Keys tab and copy its **Write API Key**.
4. Replace `PASTE_YOUR_THINGSPEAK_WRITE_API_KEY` in `wokwi/sketch.ino`.
5. Start simulation and inspect Serial Monitor; uploads occur every 20 seconds.

The ThingSpeak Write API key belongs to your channel/account. This project cannot generate it for you. Keep it private and regenerate it if exposed.

## Web dashboard
Open `web-dashboard/index.html` in a browser for a local demonstration dashboard. It includes demo elderly profiles and illustrative readings. This local version does NOT provide secure accounts, cross-device synchronisation or live ThingSpeak integration. For real deployment, connect Firebase Authentication and a database, configure secure per-user rules, and never store sensitive credentials in public frontend code.

## Medical safety
This is an educational prototype only. The potentiometer-generated heart rate and SpO2 are simulated values, not actual measurements. DS18B20 is a general digital temperature sensor, not a clinical thermometer. Do not use the prototype to make medical decisions or as an emergency alert system.
