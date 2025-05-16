#include <Arduino.h>
#include "Dispenser.h"
#include "Config.h"
#include <Wire.h>
#include <ESP32Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FastLED.h>

void setup() {
  pinMode(CLK_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP);
  pinMode(POUR_PIN, INPUT_PULLUP);
  pinMode(FLUSH_PIN, INPUT_PULLUP);
  pinMode(PUMP_IN1, OUTPUT);
  pinMode(PUMP_IN2, OUTPUT);
  digitalWrite(PUMP_IN1, LOW);
  digitalWrite(PUMP_IN2, LOW);

  arm.attach(servoPin, 500, 2500);

  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED not found"));
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  delay(1000);
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
}

void loop() {
  handleEncoder();
  handleButton();
  handlePourButton();
  handleFlush();

  switch(menuIndex) {
    case 0: displayPourMenu(); break;
    case 1: displayVolumeSelector(); break;
    case 2: displayGlassSelector(); updateGlassLEDBar(); break;
    case 3: displayTapMenu(); break;
  }

  updateOLED();
}
