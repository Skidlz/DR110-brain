// Created by zack on 12/23/2023.

#ifndef DR110_BRAIN_BUTTONS_H
#define DR110_BRAIN_BUTTONS_H

#include <stdint.h>
#include <Arduino.h>

typedef struct BUTTON {
    uint8_t lastState;  //previous reading from the input pin
    bool state;
    unsigned long tStamp; //last toggled time stamp
} BUTTON;

void setButtonHandler(void (*ptr)(uint8_t, uint8_t));
void initButtons();
void checkButtons();

#endif //DR110_BRAIN_BUTTONS_H