#include <LiquidCrystal_I2C.h>
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>


char auth[] = "DSBMSQ3QoBnfiNJQqnLXiiulqQ4C60nUz";
char ssid[] = "1234";
char pass[] = "1234";

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(D4, DHT11);
BlynkTimer timer;
bool pirbutton = 0;

#define Buzzer 10
#define MQ2 A0
#define flame D0
#define PIR D3
#define trig D5
#define echo D6
#define relay1 D7
#define relay2 D8

BLYNK_WRITE(V0) {
  pirbutton = param.asInt();
}

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  pinMode(Buzzer, OUTPUT);
  pinMode(flame, INPUT);
  pinMode(PIR, INPUT);
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
  Blynk.begin(auth, ssid, pass);
  dht.begin();
  timer.setInterval(100L, gassensor);
  timer.setInterval(100L, DHT11sensor);
  timer.setInterval(100L, flamesensor);
  timer.setInterval(100L, pirsensor);
  timer.setInterval(100L, ultrasonic);
}

void gassensor() {
  int value = analogRead(MQ2);
  Serial.println(value);
  value = map(value, 0, 1024, 0, 100);
  if (value <= 35) {
    digitalWrite(Buzzer, LOW);
  } else if (value > 35) {
    Blynk.notify("Warning! Gas leak detected");
    digitalWrite(Buzzer, HIGH);
  }
  Blynk.virtualWrite(V1, value);
  lcd.setCursor(9, 0);
  lcd.print("G :");
  lcd.print(value);
  lcd.print("  ");
}

void DHT11sensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Blynk.virtualWrite(V2, t);
  Blynk.virtualWrite(V3, h);

  lcd.setCursor(0, 0);
  lcd.print("T :");
  lcd.print(t);

  lcd.setCursor(0, 1);
  lcd.print("H :");
  lcd.print(h);
}

void flamesensor() {
  bool value = digitalRead(flame );
  if (value == 1) {
    digitalWrite(Buzzer, LOW);
  } else if (value == 0) {
    Blynk.notify("Warning! Fire was detected");
    digitalWrite(Buzzer, HIGH);
  }
}
void pirsensor() {
  bool value = digitalRead(PIR);
  if (pirbutton == 1) {
    if (value == 0) {
      digitalWrite(Buzzer, LOW);
    } else if (value == 1) {
      Blynk.notify("Warning! Please check your security system");
      digitalWrite(Buzzer, HIGH);
    }
  }
}

void ultrasonic() {
  digitalWrite(trig, LOW);
  delayMicroseconds(4);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long t = pulseIn(echo, HIGH);
  long cm = t / 29 / 2;
  Blynk.virtualWrite(V4, cm);
  lcd.setCursor(9, 1);
  lcd.print("W :");
  lcd.print(cm);
  lcd.print("  ");
}


void loop() {
  Blynk.run();
  timer.run();
}
