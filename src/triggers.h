// Created by zack on 9/22/2024.

#ifndef DR110_BRAIN_TRIGGERS_H
#define DR110_BRAIN_TRIGGERS_H

#define clap_vcs 5 // voice # of CPI
#define clap_vcs_II 8 // " CPII

#define vcs_cnt 10

#define acnt_pin 2 //pin to activate accent on PORTD
#define trig_len 9 //trigger length in 1/10ms
#define CTC_MATCH_OVERFLOW ((F_CPU / 10000) / 8) //10khz

void pollTriggers();

#endif //DR110_BRAIN_TRIGGERS_H
