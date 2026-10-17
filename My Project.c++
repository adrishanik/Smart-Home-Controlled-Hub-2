
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// ---------- Wi-Fi settings ----------
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ---------- LCD connections ----------
LiquidCrystal lcd(13, 14, 16, 17, 18, 19);

// ---------- BME280 sensor ----------
Adafruit_BME280 bme;

// ---------- Web server ----------
WebServer server(80);

// I2C pins for your ESP32
const int SDA_PIN = 21;
const int SCL_PIN = 26;

float temperatureC = 0.0;
float humidity = 0.0;
float pressureHpa = 0.0;

bool sensorReady = false;

// ---------- Read sensor ----------
void readSensor() {
  if (!sensorReady) {
    return;
  }

  temperatureC = bme.readTemperature();
  humidity = bme.readHumidity();
  pressureHpa = bme.readPressure() / 100.0F;
}

// ---------- Update LCD ----------
void updateLCD() {
  lcd.setCursor(0, 0);

  if (!sensorReady) {
    lcd.print("BME280 not found ");
    lcd.setCursor(0, 1);
    lcd.print("Check connections");
    return;
  }

  // Example: T:25.4C H:60%
  lcd.print("T:");
  lcd.print(temperatureC, 1);
  lcd.print((char)223);
  lcd.print("C H:");
  lcd.print((int)humidity);
  lcd.print("%   ");

  // Example: P:1013.2 hPa
  lcd.setCursor(0, 1);
  lcd.print("P:");
  lcd.print(pressureHpa, 1);
  lcd.print(" hPa      ");
}

// ---------- Web dashboard ----------
void handleRoot() {
  String html = R"HTML(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="5">
  <title>ENVIRON-X Dashboard</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: #eef3f7;
      color: #172b3a;
      margin: 0;
      padding: 24px;
      text-align: center;
    }
    h1 { color: #176b68; }
    .card {
      background: white;
      max-width: 420px;
      margin: 14px auto;
      padding: 18px;
      border-radius: 14px;
      box-shadow: 0 3px 12px #00000012;
    }
    .value {
      font-size: 30px;
      font-weight: bold;
      margin-top: 8px;
    }
    .unit { color: #637482; }
    footer { margin-top: 24px; font-size: 13px; }
  </style>
</head>
<body>
  <h1>ENVIRON-X</h1>
  <p>Smart Environmental Monitoring Station</p>
)HTML";

  if (!sensorReady) {
    html += "<div class='card'>BME280 sensor not detected. Check wiring.</div>";
  } else {
    html += "<div class='card'><h3>Temperature</h3><div class='value'>";
    html += String(temperatureC, 1);
    html += " &deg;C</div></div>";

    html += "<div class='card'><h3>Humidity</h3><div class='value'>";
    html += String(humidity, 1);
    html += " %</div></div>";

    html += "<div class='card'><h3>Atmospheric Pressure</h3><div class='value'>";
    html += String(pressureHpa, 1);
    html += " hPa</div></div>";
  }

  html += R"HTML(
  <footer>Updates every 5 seconds | ENVIRON-X</footer>
</body>
</html>
)HTML";

  server.send(200, "text/html", html);
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);

  // Start LCD
  lcd.begin(16, 2);
  lcd.print("ENVIRON-X");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(1500);

  // Start I2C using GPIO 21 and GPIO 26
  Wire.begin(SDA_PIN, SCL_PIN);

  // Try common BME280 I2C addresses
  sensorReady = bme.begin(0x76, &Wire);

  if (!sensorReady) {
    sensorReady = bme.begin(0x77, &Wire);
  }

  if (sensorReady) {
    Serial.println("BME280 detected.");
  } else {
    Serial.println("BME280 not detected. Check SDA/SCL and address.");
  }

  readSensor();
  updateLCD();

  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  lcd.clear();
  lcd.print("Connecting WiFi");

  Serial.print("Connecting to Wi-Fi");

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startTime < 20000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Dashboard URL: http://");
    Serial.println(WiFi.localIP());

    lcd.clear();
    lcd.print("WiFi connected");
    lcd.setCursor(0, 1);
    lcd.print("IP in Serial");
    delay(2000);
  } else {
    Serial.println("Wi-Fi connection failed.");
    Serial.println("Check Wi-Fi credentials and restart.");
    lcd.clear();
    lcd.print("WiFi failed");
    lcd.setCursor(0, 1);
    lcd.print("Check settings");
    delay(2000);
  }

  // Start dashboard server even if Wi-Fi is not connected yet
  server.on("/", handleRoot);
  server.begin();

  updateLCD();
}

// ---------- Main loop ----------
void loop() {
  server.handleClient();

  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate >= 2000) {
    lastUpdate = millis();

    readSensor();
    updateLCD();
  }

  // Retry Wi-Fi if the connection drops
  static unsigned long lastWiFiCheck = 0;

  if (millis() - lastWiFiCheck >= 10000) {
    lastWiFiCheck = millis();

    if (WiFi.status() != WL_CONNECTED) {
      WiFi.reconnect();
    }
  }
}
