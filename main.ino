#include <Wire.h>
#include <ESP32Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>

#define I2C_SDA 8
#define I2C_SCL 9

#define CLK_PIN 3
#define DT_PIN 4
#define SW_PIN 5

#define POUR_PIN 6

#define PUMP_IN1 12
#define PUMP_IN2 13

#define LED_PIN 18            // GPIO2 (D4)
#define NUM_LEDS 5
#define LED_TYPE WS2812
#define COLOR_ORDER GRB
#define BRIGHTNESS 10

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

CRGB leds[NUM_LEDS];

Servo arm;

int pourPositions[] = {500, 1000, 1500, 2000, 2500};

const int servoPin = 17;
int tempValue = 0;
int selGlassCount = 5;
int selVolume = 8;
int lastStateCLK = HIGH;
bool lastSW = HIGH;
int hue = 0;
int pourMode = 0;
bool lastPourState = HIGH;  // Start high because of pull-up

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

unsigned long lastOLEDUpdate = 0;
const unsigned long OLED_INTERVAL = 200;
int menuIndex = 0;  // 0 = Pour, 1 = Volume, 2 = Glass Count, 3 = Tap
const int maxVolume = 8; // 0.0cl to 4.0cl in 0.5cl steps (0 to 8)

// 'cup_empty', 14x14px
const unsigned char epd_bitmap_cup_empty [] PROGMEM = {
	0xff, 0xfc, 0x80, 0x04, 0x80, 0x04, 0x80, 0x04, 0x40, 0x08, 0x40, 0x08, 0x40, 0x08, 0x40, 0x08, 
	0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x10, 0x20, 0x1f, 0xe0
};

// 'scale', 14x14px
const unsigned char epd_bitmap_scale [] PROGMEM = {
	0xfe, 0x7c, 0x82, 0x00, 0x82, 0x00, 0x82, 0x70, 0x82, 0x00, 0x82, 0x00, 0x82, 0x7c, 0x82, 0x7c, 
	0x82, 0x00, 0x82, 0x00, 0x82, 0x70, 0x82, 0x00, 0x82, 0x00, 0xfe, 0x7c
};

// 'glasscount', 14x14px
const unsigned char epd_bitmap_glasscount [] PROGMEM = {
	0x38, 0x70, 0x38, 0x70, 0x38, 0x70, 0x10, 0x20, 0x93, 0x24, 0x54, 0xa8, 0x38, 0x70, 0x10, 0x20, 
	0x10, 0x20, 0x38, 0x70, 0x28, 0x50, 0x28, 0x50, 0x44, 0x88, 0x44, 0x88
};

// 'tap', 14x14px
const unsigned char epd_bitmap_tap [] PROGMEM = {
	0x00, 0x38, 0x00, 0x40, 0x0f, 0xe0, 0x1f, 0xa0, 0x38, 0xe0, 0x70, 0x40, 0x60, 0x00, 0x60, 0x40, 
	0x60, 0x00, 0x63, 0xf8, 0x62, 0x08, 0x62, 0x08, 0x61, 0x10, 0x61, 0xf0
};

void setup() {
  pinMode(CLK_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP);
  pinMode(POUR_PIN, INPUT_PULLUP);

  digitalWrite(PUMP_IN1, LOW);
  digitalWrite(PUMP_IN2, LOW);

  arm.attach(servoPin, 500, 2500);

  Serial.begin(115200);
  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED not found"));
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Initialize FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  fill_solid(leds, NUM_LEDS, CRGB::Black);  // Or CRGB::Blue, CRGB::Red, etc
  FastLED.show();
  delay(1000);
  fill_solid(leds, NUM_LEDS, CRGB::Red);  // Or CRGB::Blue, CRGB::Red, etc
  FastLED.show();

}

void loop() {
  handleEncoder();
  handleButton();
  handlePourButton();

  // Display different menus based on the menuIndex
  switch(menuIndex) {
    case 0:
      displayPourMenu();
      break;
    case 1:
      displayVolumeSelector();
      break;
    case 2:
      displayGlassSelector();
      updateGlassLEDBar();
      break;
    case 3:
      displayTapMenu();
      break;
  }
  updateOLED();

}

void drawMenuOptions(int& selected) {
  display.clearDisplay();
  display.drawBitmap(4, 1, epd_bitmap_cup_empty, 14, 14, WHITE);
  display.drawBitmap(4, 17, epd_bitmap_scale, 14, 14, WHITE);
  display.drawBitmap(4, 33, epd_bitmap_glasscount, 14, 14, WHITE);
  display.drawBitmap(4, 49, epd_bitmap_tap, 14, 14, WHITE);
  display.fillRect(20, 0, 1, SCREEN_HEIGHT, WHITE);
  switch(selected) {
    case 0:
      display.fillRect(0, 2, 2, 12, WHITE);
      break;
    case 1:
      display.fillRect(0, 18, 2, 12, WHITE);
      break;
    case 2:
      display.fillRect(0, 34, 2, 12, WHITE);
      break;
    case 3:
      display.fillRect(0, 50, 2, 12, WHITE);
      break;
  }
}


void handleEncoder() {
  int currentCLK = digitalRead(CLK_PIN);
  if (currentCLK != lastStateCLK && currentCLK == HIGH) {
    if (digitalRead(DT_PIN) != currentCLK) {
      tempValue++;
    } else {
      tempValue--;
    }

    // Clamp values based on menu
    if (menuIndex == 1) {
      tempValue = constrain(tempValue, 0, maxVolume);  // Volume 0.0cl to 4.0cl
    } else if (menuIndex == 2) {
      tempValue = constrain(tempValue, 1, 5);  // Glass count from 1 to 5
    }

    Serial.print("Current Selection: ");
    Serial.println(tempValue);
  }
  lastStateCLK = currentCLK;
}

void handleButton() {
  bool currentSW = digitalRead(SW_PIN);
  if (lastSW == HIGH && currentSW == LOW) {
    if (menuIndex == 1) {
      // Confirm volume selection
      Serial.print("Selected Volume: ");
      Serial.print(selVolume * 0.5);
      Serial.println(" cl");
    } else if (menuIndex == 2) {
      // Confirm glass count selection
      Serial.print("Selected Glass Count: ");
      Serial.println(selGlassCount);
    } else if (menuIndex == 0) {
      Serial.println("Pour menu");
      // Optionally trigger a pour animation or solenoid here
    }

    // Move to next menu
    menuIndex = (menuIndex + 1) % 4;  // Cycle through 4 menus

    if (menuIndex == 1) {
      tempValue = selVolume;
    } else if (menuIndex == 2) {
      tempValue = selGlassCount;
    }
  }
  lastSW = currentSW;
}

void handlePourButton() {
  static bool lastStableState = HIGH;
  static unsigned long lastChangeTime = 0;

  bool currentReading = digitalRead(POUR_PIN);
  unsigned long now = millis();

  if (currentReading != lastStableState && (now - lastChangeTime) > debounceDelay) {
    lastChangeTime = now;
    lastStableState = currentReading;

    if (currentReading == LOW) {
      Serial.println("Pour button press detected");
      onPourButtonPressed();
    }
  }
}

void displayVolumeSelector() {
  float clAmount = tempValue * 0.5;
  int frameHeight = SCREEN_HEIGHT - 2;
  int barHeight = map(tempValue, 0, maxVolume, 0, frameHeight);
  selVolume = tempValue;

  display.clearDisplay();
  drawMenuOptions(menuIndex);

  // Draw the vertical progress bar
  int font_height = 20;
  int barX = SCREEN_WIDTH - 30;
  int barTopY = 1;
  int barWidth = 20;
  int volX = 25;
  int volY = (SCREEN_HEIGHT - font_height) / 2;
  
  // Display current volume
  display.setCursor(volX, volY);
  display.setTextSize(2);
  display.print(clAmount, 1);
  display.println(" cl");
  display.setTextSize(1);

  display.drawRect(barX, barTopY, barWidth, frameHeight, SSD1306_WHITE);
  int fillY = barTopY + frameHeight - barHeight;
  display.fillRect(barX + 1, fillY, barWidth - 2, barHeight, SSD1306_WHITE);
}

void displayGlassSelector() {
  tempValue = constrain(tempValue, 1, 5);
  display.clearDisplay();
  drawMenuOptions(menuIndex);
  // Display glass count
  display.setCursor(30, 0);
  display.print("Glass count: ");
  display.println(tempValue);
  selGlassCount = tempValue;

  // Draw a simple box to represent the glass
  int boxWidth = 60;
  int boxHeight = 40;
  int boxX = (SCREEN_WIDTH - boxWidth) / 2;
  int boxY = SCREEN_HEIGHT / 2 - boxHeight / 2;
  display.drawRect(boxX, boxY, boxWidth, boxHeight, SSD1306_WHITE);
}

void displayPourMenu() {
  display.clearDisplay();
  drawMenuOptions(menuIndex);
  // Display "Pour" button
  display.setCursor(25, 0);
  display.setTextSize(2);
  display.println("P O U R");
  display.setTextSize(1);
}

void displayTapMenu() {
  display.clearDisplay();
  drawMenuOptions(menuIndex);
}

void onPourButtonPressed() {
  Serial.println("Volume: " + String(selVolume));
  Serial.println("GlassCount: " + String(selGlassCount));

  //digitalWrite(PUMP_IN1, HIGH);
  //digitalWrite(PUMP_IN2, LOW);
  //delay(500);
  //digitalWrite(PUMP_IN1, LOW);
  //digitalWrite(PUMP_IN2, LOW);
  int steps = min(selGlassCount, (int)(sizeof(pourPositions) / sizeof(pourPositions[0])));
  for (int i = 0; i < steps; i++) {
    arm.writeMicroseconds(pourPositions[i]);
    delay(1000);
  }

  arm.writeMicroseconds(1500); // Reset to center
}




void updateGlassLEDBar() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);  // Clear all LEDs

  // Light up tempValue LEDs for real-time feedback while selecting
  int activeLEDs = constrain(tempValue, 1, NUM_LEDS);

  for (int i = 0; i < activeLEDs; i++) {
    leds[i] = CRGB::Red;
  }

  FastLED.show();
}


void updateOLED() {
  if (millis() - lastOLEDUpdate >= OLED_INTERVAL) {
    lastOLEDUpdate = millis();
    display.display();
  }
}
