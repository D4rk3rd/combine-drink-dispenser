#ifndef DISPENSER_H
#define DISPENSER_H

#include <Arduino.h>

void handleEncoder();
void handleButton();
void handlePourButton();
void handleFlush();
void drawMenuOptions(int &selected);
void displayPourMenu();
void displayVolumeSelector();
void displayGlassSelector();
void displayTapMenu();
void updateGlassLEDBar();
void updateOLED();
void onPourButtonPressed();

#endif
