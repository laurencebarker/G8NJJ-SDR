//************************************************************************
//
// Title: G8NJJ SDR Control Arduino sketch
// File: modeseq.h
// copyright Laurence Barker G8NJJ 2017
//
//************************************************************************

#include <clickencoder.h>
extern ClickEncoder *encoder;
//
// define the type for the mode sequencer state variable
//
typedef enum
{
  eNormalRX,                      // normal operating mode, RX
  eNormalTX,                      // normal operating mode, TX
  eNormalTXVSWR,                  // normal TX tripped by high VSWR
  eNormalTXTemp,                  // normal TX tripped by high temp
  eNormalTXDisabled,              // normal TX disabled by protocol command from PC
  eDebugRX,                       // debug mode RX
  eDebugTX,                       // debug mode TX
  eDisplayUninitialised           // not a real state!
} ESequencerState;

extern ESequencerState GCurrentMode;         // state variable 


//
// initialise sequencer
//
void InitSequencer(void);


//
// 16ms tick
//
void SequencerTick(void);

