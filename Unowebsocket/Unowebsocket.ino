#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int LED_PIN = 7;
const int SENSOR_PIN = A0;
unsigned long lastSend = 0;
int lastSensorVal = -1;
String lastMessage = "";

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Web <> UNO Bridge");
  delay(1500);
  updateLCD();
}

void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LED: ");
  lcd.print(digitalRead(LED_PIN) ? "ON " : "OFF");
  int currentSensorVal = analogRead(SENSOR_PIN);
  lastSensorVal = currentSensorVal;
  lcd.setCursor(0, 1);
  lcd.print("Sensor:");
  lcd.print(currentSensorVal);
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.equalsIgnoreCase("LED ON")) {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED:ON");
      updateLCD();
    } else if (line.equalsIgnoreCase("LED OFF")) {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED:OFF");
      updateLCD();
    } else if (line.equalsIgnoreCase("GET STATUS")) {
      String resp = String("LED:") + (digitalRead(LED_PIN) ? "ON" : "OFF");
      resp += String(",SENSOR:") + analogRead(SENSOR_PIN);
      Serial.println(resp);
      updateLCD();
    } else if (line.startsWith("MSG:")) {
      lastMessage = line.substring(4); // cắt bỏ "MSG:"
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(lastMessage);   // hiển thị dòng 1 là message từ web
      lcd.setCursor(0, 1);
      lcd.print("Sensor:");
      lcd.print(lastSensorVal);
    }
  }

  if (millis() - lastSend > 800) {
    lastSend = millis();
    int s = analogRead(SENSOR_PIN);
    if (abs(s - lastSensorVal) > 5) {
      Serial.println(String("SENSOR:") + s);
      lastSensorVal = s;
    }
  }
}
