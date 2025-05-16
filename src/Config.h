#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>
#include <Wire.h>
#include <ESP32Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FastLED.h>


// -----Global defs -----
#define I2C_SDA 8
#define I2C_SCL 9

#define CLK_PIN 3
#define DT_PIN 4
#define SW_PIN 5

#define POUR_PIN 6
#define FLUSH_PIN 7

#define PUMP_IN1 12
#define PUMP_IN2 13

#define LED_PIN 18
#define NUM_LEDS 5
#define LED_TYPE WS2812
#define COLOR_ORDER GRB
#define BRIGHTNESS 10

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// ----- Global objects -----
extern Servo arm;
extern Adafruit_SSD1306 display;
extern CRGB leds[NUM_LEDS];

// ----- Global vars -----
extern const int servoPin;
extern int menuIndex;

#endif
