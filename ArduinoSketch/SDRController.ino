//************************************************************************
//
// Title: G8NJJ SDR Control Arduino sketch
// File: SDRController
// copyright Laurence Barker G8NJJ 2017
//
// This is the top level sketch file containing inits and main loop
// code operates on a 16ms timer "tick" as its basic clock
// timer tick is counted in this file
//************************************************************************
#include "iodefs.h"
#include <TimerOne.h>
#include <ClickEncoder.h>
#include "modeseq.h"
#include "analogio.h"
#include "displayutils.h"
#include "eestore.h"
#include "radiosettings.h"


//
// global variables
//
#define VNUMSUBTICKS 16               // number of 1ms ticks per "master" tick
boolean GTickTriggered;               // true if a 16ms tick has been triggered
byte GTickCount;                      // 1ms sub-tick counter


//
// 1ms sub-tick handler.
// updates the encoder, ands counts down to a "main" 16ms tick
void timerIsr()
{
  if (GTickCount == 0)
  {
    GTickTriggered = true;                // initiate master tick event
    GTickCount = VNUMSUBTICKS - 1;        // restart count
  }
  else
    GTickCount--;
  encoder->service();
}




void setup() {
  Serial.begin(9600);

  initDisplay();

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);

  pinMode(PIN_TXEN, OUTPUT);                  // sets the digital pin as output
  pinMode(PIN_SERLOAD, OUTPUT);               // sets the digital pin as output
  pinMode(PIN_TXREQ, INPUT);                  // sets the digital pin as input
  eepromReadout();
  InitAnalogueIO();
  InitSequencer();
  InitRadioSettings();
  CreateLCDSpecialChars();
}


//
// 16 ms event loop
// this is triggered by GTickTriggered being set by a timer interrupt
//
void loop()
{
  while (GTickTriggered)
  {
    GTickTriggered = false;
    AnalogueIOTick();
    RadioSettingsTick();                      // update the SDR controlling outputs
    SequencerTick();                          // update the T/R mode sequencer & UI
  }
}

