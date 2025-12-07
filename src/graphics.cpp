// Created by zack on 12/23/2023.
#include "graphics.h"
#include "constants.h"

U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8); //spi

void u8g2Prepare() {
    u8g2.begin();
    u8g2.clearBuffer();
    //u8g2.setFont(u8g2_font_6x10_tf);
    //u8g2.setFontRefHeightExtendedText();
    u8g2.setDrawColor(0); //1 to invert
    u8g2.setFontPosTop();
    u8g2.setFontDirection(0);

    u8g2.setDrawColor(1); //white
    u8g2.drawBox(0, 0, 128, 64); //clear screen

    drawBlankGrid();

    u8g2.drawXBMP(0, 47, dr110_B_width, dr110_B_height, dr110_B_bits); //bottom of screen data

    //top left of grid is (15, 5). Each square is 7x6
    //draw empty circle
//    u8g2.setDrawColor(1); //white
//    u8g2.drawDisc(15 + 7, 5 + 6, 2, U8G2_DRAW_ALL); //filled circle
//    u8g2.setDrawColor(0); //black
//    u8g2.drawCircle(15 + 7, 5 + 6, 2, U8G2_DRAW_ALL); //outline
    u8g2.sendBuffer();
}

void drawBlankGrid() {
    u8g2.setDrawColor(1); //white
    u8g2.drawBox(8, 0, 120, 46); //clear screen

    u8g2.setDrawColor(0); //black
    u8g2.drawXBMP(0, 3, dr110_L_width, dr110_L_height, dr110_L_bits); //instrument names

    u8g2.drawRFrame(8, 0, 120, 46, 7); //draw grid outline
    u8g2.drawRFrame(9, 0, 118, 46, 7); //make sides thicker
    u8g2.drawHLine(12, 1, 112); //make top thicker
    u8g2.drawPixel(12, 2); //make corners thicker
    u8g2.drawPixel(11, 3);
    u8g2.drawPixel(123, 2);
    u8g2.drawPixel(124, 3);

    //draw inside of grid
    for (int y = yOffset; y < (cellHeight * gridRows) + yOffset; y += cellHeight)
        u8g2.drawHLine(10, y, 117); //draw horizontal lines
    for (int x = xOffset; x < (cellWidth * gridCols) + xOffset; x += cellWidth) {
        if (((x - xOffset) / cellWidth) % 4 == 0) {
            u8g2.setDrawColor(1); //white
            u8g2.drawHLine(x - 1, 0, 3); //beat marker triangle top
            u8g2.drawPixel(x, 1);

            u8g2.drawPixel(x, 45); //beat marker triangle bottom
            u8g2.setDrawColor(0); //black
            u8g2.drawHLine(x - 1, 46, 3);
        }

        u8g2.setDrawColor(0); //black
        u8g2.drawLine(x, 2, x, 44); //draw vertical lines
    }
}

void drawText(uint8_t song, uint8_t measure, uint8_t bank, uint8_t rhythm, uint8_t mode) {
    u8g2.setDrawColor(1); //white
    u8g2.drawBox(4, 53, 126, 63); //clear text area

    u8g2.setDrawColor(0); //black

    //draw the first song "I"
    u8g2.drawXBMP(4, 53, i_text_width, i_text_height, i_text_bits); //I
    //draw the second "I"
    if (song) u8g2.drawXBMP(9, 53, i_text_width, i_text_height, i_text_bits); //I

    //Measure numbers 000-999
    u8g2.drawXBMP(17, 53, num_width, num_height, numberLookup(measure/100));
    u8g2.drawXBMP(17 + 9, 53, num_width, num_height, numberLookup((measure/10) % 10));
    u8g2.drawXBMP(17 + (9*2), 53, num_width, num_height, numberLookup(measure % 10));

    //TODO: draw bank A,B,C,D

    //Rhythm number
    u8g2.drawXBMP(71, 53, num_width, num_height, numberLookup(rhythm));

    drawModeText(mode);
}

void drawModeText(uint8_t mode) {
    //Mode text
    const uint8_t * firstWordBMP = song_text_bits; //Song Play (default)
    const uint8_t * secondWordBMP = play_text_bits;

    if (mode == PATT_PLAY) { //Pattern Play
        firstWordBMP = patt_text_bits;
    } else if (mode == SONG_WRITE) { //Song Write
        secondWordBMP = write_text_bits;
    } else if (mode == STEP_WRITE) { //Step Write
        firstWordBMP = step_text_bits;
        secondWordBMP = write_text_bits;
    } else if (mode == TAP_WRITE) { //Tap Write
        firstWordBMP = tap_text_bits;
        secondWordBMP = write_text_bits;
    }

    u8g2.setDrawColor(0); //black
    u8g2.drawXBMP(89, 53, patt_text_width, patt_text_height,firstWordBMP);
    u8g2.drawXBMP(107, 53, play_text_width, play_text_height, secondWordBMP);
}

const uint8_t* numberLookup(uint8_t number) { //return a pointer to a number bitmap
    switch (number) {
        case 0: return num0_bits;
        case 1: return num1_bits;
        case 2: return num2_bits;
        case 3: return num3_bits;
        case 4: return num4_bits;
        case 5: return num5_bits;
        case 6: return num6_bits;
        case 7: return num7_bits;
        case 8: return num8_bits;
        case 9: return num9_bits;
        default: return num0_bits;
    }
}

void drawStep(uint8_t step_x, uint8_t inst_y) { //populate a step on the screen's grid
    //u8g2.setDrawColor(0); //black
    //top left of grid is (15, 5). Each square is 7x6
    u8g2.drawDisc(xOffset + (step_x * cellWidth), yOffset + (inst_y * cellHeight), 2, U8G2_DRAW_ALL); //draw circle
}

void clearStep(uint8_t step_x, uint8_t inst_y) { //clear a step on the screen's grid
    u8g2.setDrawColor(1); //white
    drawStep(step_x, inst_y); //remove the circle

    u8g2.setDrawColor(0); //black
    //top left of grid is (15, 5). Each square is 7x6
    u8g2.drawHLine(15 + (step_x * cellWidth) - 2, 5 + (inst_y * cellHeight) , 5); //draw back the grid
    u8g2.drawLine(15 + (step_x * cellWidth), 5 + (inst_y * cellHeight) - 2,
                  15 + (step_x * cellWidth), 5 + (inst_y * cellHeight) + 3); //draw vertical lines
}

void populateGrid(boolean pattern[gridRows][gridCols]) { //hardcoded to populate from pattern array
    for (int inst_y = 0; inst_y < gridRows; inst_y++)
        for (int step_x = 0; step_x < gridCols; step_x++)
            if (pattern[inst_y][step_x]) drawStep(step_x, inst_y);
}

uint8_t prevStep = 0;
uint8_t prevInst = 0;
uint32_t blinkTimer = millis();
#define blinkInterval 250 //todo: have this work off of the tempo
bool blinkState = false;

void blinkStep(uint8_t step, uint8_t inst, boolean pattern[gridRows][gridCols]) {
    //TODO: make this blink based on temp subdivisions
    const int tolerance = 10; //blink step on grid----------------------------------
    if (abs(millis() - blinkTimer) <= tolerance || millis() > blinkTimer)  {
        blinkTimer += blinkInterval;
        (blinkState) ? drawStep(step, inst) : clearStep(step, inst);

        blinkState = !blinkState; //toggle

        if (step != prevStep || inst != prevInst) { //fix the state of the last step
            if (pattern[prevInst][prevStep]) drawStep(prevStep, prevInst);
            else clearStep(prevStep, prevInst);
            prevStep = step;
            prevInst = inst;
        }

        u8g2.sendBuffer();
    }
}

void blinkModeText(uint8_t step, uint8_t mode) {
    if (step == prevStep) return;

    if ((prevStep > 1) != (step > 1)) { //status has changed
        if (step < 2) { //blank text on first 2 steps
            u8g2.setDrawColor(1); //white
            u8g2.drawBox(89, 53, 89 + patt_text_width + play_text_width, 53 + patt_text_height); //clear text area
            u8g2.setDrawColor(0); //black
        } else {
            drawModeText(mode);
        }

        u8g2.sendBuffer();
    }

    prevStep = step;
}