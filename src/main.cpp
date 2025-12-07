#include <Arduino.h>
#include "graphics.h"
#include "buttons.h"
#include "constants.h"
//DR 110 sequencer. Uses 2.4" SSD1309 OLED display and a button matrix
// 11/24/2023 Zack Nelson

//MIDI and tempoTS values
unsigned long tempoTS;
int tempoInterval = 125; //pause between beats
//TODO: support trigger/clock in and MIDI in
uint8_t incStepFlag = 0; //set by timer ISR
uint8_t playing = false;
byte nextPlayStep = 0; //current step
//boolean mutes[16]; //instrument mutes

const uint8_t buttToInst[] {4, 3, 2, 1, 0, 5, 6};
//CH, OH, Sn, Bd, Acc, Cy, HC
const byte midi[] = {42, 46, 38, 36, 43, 49, 39}; //midi instrument numbers
const byte midiChan = 0;
//7 inst * 16 step = 112 * 8 patterns = 896 * 4 banks = 3,584
//mega2560 EEPROM = 4k
//this leaves 512 bytes for song data
boolean pattern[INST_CNT][16] = {
        { 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1 }, //CH
        { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0 }, //OH
        { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, //Sn
        { 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0 }, //Bd
        //{1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1}, //Acc
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, //Acc
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, //Cy
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, //HC
};

extern BUTTON btns[]; // array of button structs

uint8_t song = 1;
uint8_t measure = 123;
uint8_t bank = 0;
uint8_t rhythm = 4;
uint8_t mode = PATT_PLAY;

uint8_t step = 0;
uint8_t inst = 0;

void sendNote(uint8_t note, uint8_t velo) {
    Serial.write(0x90 | midiChan); //note on
    Serial.write(note); //instrument
    Serial.write(velo); //velocity
    //TODO: support a delay between note on and off
    Serial.write(0x80 | midiChan); //note off
    Serial.write(note); //instrument
    Serial.write(0);
}

//TODO: bank select button and screen
// change pattern length (12/16)
// song mode write/play

void changeMode(uint8_t newMode, void (*ptr)(uint8_t, uint8_t)) {
    if (playing) return; //prevent changing modes while playing

    nextPlayStep = 0;
    step = 0;
    inst = 0;
    mode = newMode;
    setButtonHandler(ptr);

    drawBlankGrid();
    populateGrid(pattern);
    drawText(song, measure, bank, rhythm, mode);
    u8g2.sendBuffer();
}

void stopPlaying() {
    playing = false;
    step = 0; //reset us to the top left cell
    nextPlayStep = 0;
    inst = 0;
    drawText(song, measure, bank, rhythm, mode); //redraw mode since it is blanked on first step
    u8g2.sendBuffer();
}

void tapWriteButHndlr(uint8_t keyNum, uint8_t keyStatus);
void stepWriteButHndlr(uint8_t keyNum, uint8_t keyStatus);

void patPlayButHndlr(uint8_t keyNum, uint8_t keyStatus) {
    if (!keyStatus) return; //do nothing on key release

    //disallow mode change while playing
    if (btns[SHFT_BUT].state && !playing) { //Shift mode
        if (keyNum == SONG_PLAY_BUT) {
            changeMode(SONG_PLAY, patPlayButHndlr); //TODO: change button handler
        } else if (keyNum == PATT_PLAY_BUT) {
            //already in pattern play
            //changeMode(PATT_PLAY, patPlayButHndlr);
        } else if (keyNum == SONG_WRITE_BUT) {
            changeMode(SONG_WRITE, patPlayButHndlr); //TODO: change button handler
        } else if (keyNum == STEP_WRITE_BUT) {
            changeMode(STEP_WRITE, stepWriteButHndlr);
        } else if (keyNum == TAP_WRITE_BUT) {
            changeMode(TAP_WRITE, tapWriteButHndlr);
        } else if (keyNum == STOP_BUT) { //Stop
            stopPlaying();
        }
    } else {
        if (keyNum == START_BUT && !playing) { //Start
            tempoTS = millis();
            playing = true;
            //correct blinking step
            pattern[0][0] ? drawStep(0,0) : clearStep(0, 0);
            u8g2.sendBuffer();
            blinkModeText(15, mode); //force text to blank on next redraw
        } else if (keyNum == STOP_BUT) { //Stop
            stopPlaying();
        } else if (keyNum >= ONE_BUT && keyNum <= EIGHT_BUT) {
            rhythm = keyNum - ONE_BUT + 1; //update pattern number
            drawText(song, measure, bank, rhythm, mode);
            u8g2.sendBuffer();
            //TODO: actually change pattern
        } else if (keyNum >= ACC_BUT) { //Drum pads
            //play drums in realtime from pads
            sendNote(midi[buttToInst[keyNum - ACC_BUT]], 0x7f);
        }

    }
}

//button functionality for step writing
void stepWriteButHndlr(uint8_t keyNum, uint8_t keyStatus) {
    if (btns[SHFT_BUT].state) { //Shift mode
        if (keyNum == SONG_PLAY_BUT) {
            changeMode(SONG_PLAY, patPlayButHndlr); //TODO: change button handler
        } else if (keyNum == PATT_PLAY_BUT) {
            changeMode(PATT_PLAY, patPlayButHndlr);
        } else if (keyNum == SONG_WRITE_BUT) {
            changeMode(SONG_WRITE, patPlayButHndlr); //TODO: change button handler
        } else if (keyNum == STEP_WRITE_BUT) {
            //already in Step Write mode!
            //changeMode(STEP_WRITE, stepWriteButHndlr);
        } else if (keyNum == TAP_WRITE_BUT) {
            changeMode(TAP_WRITE, tapWriteButHndlr);
        } else if (keyNum == PATT_CLEAR_BUT) { //Pattern CLear
            drawBlankGrid();
            u8g2.sendBuffer();

            //clear out pattern array
            for (auto &inst_y: pattern) for (bool &step_x: inst_y) step_x = false;
        }
    } else { //Non-shift
        if (!keyStatus) return; //don't do anything on key release

        if (keyNum == START_BUT) { //set (Start)
            drawStep(step, inst);
            pattern[inst][step++] = true;
            u8g2.sendBuffer();
        } else if (keyNum == STOP_BUT) { //skip (Stop)
            clearStep(step, inst);
            pattern[inst][step++] = false;
            u8g2.sendBuffer();
        } else if (keyNum >= ACC_BUT) { //drum pads
            inst = buttToInst[keyNum - ACC_BUT]; //select instrument
            if (inst >= 7) inst = 0;
        }

        if (step == 16) step = 0; //TODO: support 12 step mode
    }
}//----------------------------------------------------------

void tapWriteButHndlr(uint8_t keyNum, uint8_t keyStatus){
    step = nextPlayStep;

    if (btns[SHFT_BUT].state & !playing) { //Shift mode
        if (keyNum == SONG_PLAY_BUT) {
            changeMode(SONG_PLAY, patPlayButHndlr); //TODO: change button handler
        } else if (keyNum == PATT_PLAY_BUT) {
            changeMode(PATT_PLAY, patPlayButHndlr);
        } else if (keyNum == SONG_WRITE_BUT) {
            changeMode(SONG_WRITE, patPlayButHndlr); //TODO: change button handler
        } else if (keyNum == STEP_WRITE_BUT) {
            changeMode(STEP_WRITE, stepWriteButHndlr);
        } else if (keyNum == TAP_WRITE_BUT) {
            //Already in tap write mode!
            //changeMode(TAP_WRITE, tapWriteButHndlr);
        } else if (keyNum == PATT_CLEAR_BUT) { //Clear pattern
            drawBlankGrid();
            u8g2.sendBuffer();

            //clear out pattern array
            for (auto &inst_y: pattern) for (bool &step_x: inst_y) step_x = false;
        } else if (keyNum == STOP_BUT) { //Stop
            stopPlaying();
        }
    } else { //Non-shift
        if (keyNum == START_BUT && !playing) { //Start
            tempoTS = millis();
            playing = true;
            //correct blinking step
            pattern[0][0] ? drawStep(0,0) : clearStep(0, 0);
            u8g2.sendBuffer();
            blinkModeText(15, mode); //force text to blank on next redraw
        } else if (keyNum == STOP_BUT) { //Stop
            stopPlaying();
        } else if (keyNum >= ACC_BUT) { //Drum pads
            if (!keyStatus) return;

            inst = buttToInst[keyNum - ACC_BUT]; //select instrument
            if (inst >= 7) inst = 0;

            if (playing && !btns[SEVEN_BUT].state) { //not erasing
                step = (nextPlayStep - 1) % 16; //current step
                //if closer to the next step, add one
                if (tempoTS - millis() <= tempoInterval / 2) step = nextPlayStep;
                else sendNote(midi[inst], 0x7f); //only play sound if it doesn't get pushed to the next step

                drawStep(step, inst);
                pattern[inst][step] = true;

                u8g2.sendBuffer();
            } else if (!playing) { //buttons play in real time
                sendNote(midi[inst], 0x7f);
            }
        }
    }
}

void setup(void) {
    //Set up OLED---------------------------
    u8g2Prepare();
    populateGrid(pattern); //add dots
    drawText(song, measure, bank, rhythm, mode);
    u8g2.sendBuffer();

    Serial.begin(31250); //MIDI baud

    initButtons();
    setButtonHandler(&patPlayButHndlr);
    //setButtonHandler(&stepWriteButHndlr);

    //tempoTS-------------------------------
    noInterrupts(); // disable all interrupts
    TCCR1A = TCCR1B = TCNT1 = 0;
    TCCR1B = (1 << CS12) | (1 << WGM12); //div 256 16MHz/256=62,500Hz
    OCR1A = 16; //div 62500Hz/16=3,906.25Hz
    bitSet(TIMSK1, OCIE1A);
    tempoTS = millis();
    interrupts(); // enable all interrupts
}

ISR(TIMER1_COMPA_vect) {
    if (!playing) return;
    const int tolerance = 1;
    //TODO: fix bug where start TS is too old and sequencer hangs
    if (abs(millis() - tempoTS) <= tolerance) {
        tempoTS += tempoInterval;
        incStepFlag = true;
    }
}

void incrementStep() {
    for (int inst_y = 0; inst_y < 7; inst_y++) { //scan instruments
        //if (!mutes[inst_y] & pattern[inst_y][nextPlayStep]) {
        if (pattern[inst_y][nextPlayStep]) sendNote(midi[inst_y], 0x7f);
    }

    ++nextPlayStep %= 16;
}

void loop(void) {
    checkButtons();

    if (!playing) blinkStep(step, inst, pattern);
    if (mode == PATT_PLAY && playing) {
        step = (nextPlayStep - 1) % 16; //move us to the playing step
        blinkModeText(step, mode);
    } else if (mode == TAP_WRITE && playing) { //Tap write mode
        step = (nextPlayStep - 1) % 16; //move us to the playing step
        blinkModeText(step, mode);

        bool screenUpdated = false;
        //erase steps by holding 7 and instrument
        if (btns[PATT_CLEAR_BUT].state) {
            for (int button = 0; button < INST_CNT; button++) { //instrument buttons
                uint8_t inst_y = buttToInst[button];
                if (btns[ACC_BUT + button].state && pattern[inst_y][step]) {
                    pattern[inst_y][step] = false; //blank step
                    clearStep(step, inst_y);
                    screenUpdated = true;
                }
            }

            if (screenUpdated) u8g2.sendBuffer();
        }
    }

    if (incStepFlag) {
        incrementStep();
        incStepFlag = false;
    }

    //TODO: power down screen if inactive for too long
    //u8g2.setPowerSave(true); //go to sleep
}