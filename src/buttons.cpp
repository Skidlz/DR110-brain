// Created by zack on 12/23/2023.
#include "buttons.h"

//Debounced button module------------------------
//6 rows on A0-5 (PORTF)
//4 cols on D2-6 (PORTE)

#define col_cnt 4
#define row_cnt 5
#define bounceDly 7 // the debounce time
#define but_cnt col_cnt * row_cnt
BUTTON btns[but_cnt]; // array of button structs

void (*buttonHandler)(uint8_t, uint8_t); //function pointer

void setButtonHandler(void (*ptr)(uint8_t, uint8_t)) {
    buttonHandler = ptr;
}

void initButtons() { //buttons: lastState, state, tStamp
    PORTF = 0xFF; //enable input pullups
    DDRF = 0x00; //row inputs
    DDRK = 0;//0xFF; //col outputs
    PORTK = 0; //0xFF;
    for (int i = 0; i < but_cnt; i++) btns[i] = (BUTTON) {LOW, LOW, 0}; //init button array

    PORTL = 0xFF;
    DDRL = 0x00; //mechanical pad inputs
}

void checkButtons() {
    for (int col = 0; col < col_cnt; col++) { //d2-d6
        DDRK = (1 << (col)); //pull one col low
        //PORTK &= ~(1 << (col));
        delayMicroseconds(10);
        uint8_t row = ~PINF;
        //if (col == 2) row = ~PINL;
        for (int b = 0; b < row_cnt; b++) {
            int keyNum = (b) + (col * row_cnt); //keyNum
            bool reading = (row & (1 << b));

            //reset debounce tempoTS if state changed
            if (reading != btns[keyNum].lastState) btns[keyNum].tStamp = millis();
            //waited longer than delay
            if ((millis() - btns[keyNum].tStamp) > bounceDly && (reading != btns[keyNum].state)) {
                btns[keyNum].state = reading;

                buttonHandler(keyNum, reading);
            }

            btns[keyNum].lastState = reading;
        }
        //PORTK = 0xFF;
        DDRK = 0;
    }
}