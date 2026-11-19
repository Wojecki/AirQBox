// Sensors Lib
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <SensirionI2cScd4x.h>
#include "Adafruit_SGP30.h"
#include <HardwareSerial.h>
#include "PMS.h"

// External variables
#include "variables.h"

// E-ink Lib
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// E-ink driver
#include "GxEPD2_display_selection_new_style.h"

// Def I2C
#define I2C_SDA 21
#define I2C_SCL 19

// Def UART2 for PMS5003
#define PMS_RX 32
#define PMS_TX 33

// Init obj sensor
Adafruit_BMP280 bmp;
Adafruit_AHTX0 aht;
SensirionI2cScd4x scd4x;
Adafruit_SGP30 sgp;

HardwareSerial pmsSerial(2);
PMS pms(pmsSerial);
PMS::DATA pmsData;

unsigned long lastSgpRead = 0;
unsigned long lastServerPost = 0;
unsigned long lastDisplayUpdate = 0;

// Sensor variables
float temperature = 0.0;
float pressure = 0.0;
float humidity = 0.0;
uint16_t co2 = 0;
uint16_t sgp_eCO2 = 0;
uint16_t sgp_TVOC = 0;
uint16_t pm1_0 = 0, pm2_5 = 0, pm10 = 0;

void updateDisplay();

// Init sensors and screen 
void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100);

  pmsSerial.begin(9600, SERIAL_8N1, PMS_RX, PMS_TX);

  display.init(115200, true, 2, false);

  if (!bmp.begin(0x77)) { Serial.println("BMP280 not found"); }
  if (!aht.begin()) { Serial.println("AHT not found"); }

  scd4x.begin(Wire, 0x62);
  scd4x.stopPeriodicMeasurement();
  delay(500);
  if (scd4x.startPeriodicMeasurement()) {
    Serial.println("SCD40 error");
  }

  if (!sgp.begin()) { Serial.println("SGP30 not found"); }

  // WiFi connection
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  updateDisplay();
}

void loop() {
  unsigned long currentMillis = millis();

  // SGP30 reading(1s)
  if (currentMillis - lastSgpRead >= 1000) {
    lastSgpRead = currentMillis;
    sgp.IAQmeasure(); 
  }

  // Data pulling
  if (currentMillis - lastServerPost >= responseTime) {
    lastServerPost = currentMillis;

    // SCD40 reading
    bool isDataReady = false;
    float scdTemp = 0.0f, scdHum = 0.0f;
    if (!scd4x.getDataReadyStatus(isDataReady) && isDataReady) {
      scd4x.readMeasurement(co2, scdTemp, scdHum);
    }

    // BMP280 reading
    temperature = bmp.readTemperature();
    pressure = bmp.readPressure() / 100.0F;

    // AHT reading
    sensors_event_t humidityEvent, tempEvent;
    aht.getEvent(&humidityEvent, &tempEvent);
    humidity = humidityEvent.relative_humidity;

    // SGP30 reading
    sgp_eCO2 = sgp.eCO2;
    sgp_TVOC = sgp.TVOC;

    // PMS5003 reading
    if (pms.readUntil(pmsData)) {
      pm1_0 = pmsData.PM_AE_UG_1_0;
      pm2_5 = pmsData.PM_AE_UG_2_5;
      pm10  = pmsData.PM_AE_UG_10_0;
    }

    // Sending HTTP POST
    if (WiFi.status() == WL_CONNECTED) {
      String jsonPayload = "{"
        "\"temp\":\"" + String(temperature, 1) + "\","
        "\"pres\":\"" + String(pressure, 1) + "\","
        "\"hum\":\"" + String(humidity, 0) + "\","
        "\"co2\":\"" + String(co2) + "\","
        "\"sgp_co2\":\"" + String(sgp_eCO2) + "\","
        "\"tvoc\":\"" + String(sgp_TVOC) + "\","
        "\"pm1_0\":\"" + String(pm1_0) + "\","
        "\"pm2_5\":\"" + String(pm2_5) + "\","
        "\"pm10\":\"" + String(pm10) + "\""
      "}";

      HTTPClient http;
      http.begin(serverUrl);
      http.addHeader("Content-Type", "application/json");

      int httpResponseCode = http.POST(jsonPayload);
      if (httpResponseCode <= 0) {
        Serial.print("Błąd POST: ");
        Serial.println(http.errorToString(httpResponseCode).c_str());
      }
      http.end();
    }
  }

  // E-paper refresh
  if (currentMillis - lastDisplayUpdate >= displayInterval) {
    lastDisplayUpdate = currentMillis;
    updateDisplay();
  }
}

void updateDisplay() {
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);

  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    // Heading
    display.setCursor(20, 30);
    display.print(displayTitle);
    display.drawFastHLine(1, 40, 380, GxEPD_BLACK);

    // Readings
    display.setCursor(20, 70);
    display.print("Temp: "); display.print(temperature, 1); display.print(" C");

    display.setCursor(20, 100);
    display.print("Hum: "); display.print(humidity, 0); display.print(" %");

    display.setCursor(20, 130);
    display.print("Pres: "); display.print(pressure, 1); display.print(" hPa");

    display.setCursor(20, 160);
    display.print("CO2: "); display.print(co2); display.print(" ppm");

    display.setCursor(20, 190);
    display.print("TVOC : "); display.print(sgp_TVOC); display.print(" ppb");

    display.setCursor(20, 220);
    display.print("PM1.0:"); display.print(pm1_0);
    display.setCursor(20, 250);
    display.print("PM2.5:"); display.print(pm2_5);
    display.setCursor(20, 280);
    display.print("PM10: "); display.print(pm10);

  } while (display.nextPage());

  display.hibernate();
}