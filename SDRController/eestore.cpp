//************************************************************************
//
// Title: G8NJJ SDR Control Arduino sketch
// File: eestore.cpp
// copyright Laurence Barker G8NJJ 2017
//
// code to read and write from/to the EEPROM
// used to provide enduring storage for display settings and RX2 antenna source
//************************************************************************
#include <Arduino.h>
#include <EEPROM.h>
#include "eestore.h"
#include "radiosettings.h"

//
// define the locations used in the EEPROM:
//
#define VEERX2 1
#define VEEVSWR 2
#define VEETEMP 3
#define VEEPWRMAX 4
#define VEEVSWRMAX 5

char GVSWRThreshold;                         // VSWR trip threshold
char GTempThreshold;                         // temp trip threshold
char GPowerBarMax;                           // power display bar max value, W
char GVSWRBarMax;                            // VSWR display bar max value

#define VVSWRTRIPMIN 20                                     // min VSWR=2.0
#define VVSWRTRIPMAX 60                                     // min VSWR=6.0
#define VTEMPTRIPMIN 40                                     // min 40C
#define VTEMPTRIPMAX 70                                     // max 70C
#define VPWRBARMIN 20                                       // min 20W
#define VPWRBARMAX 120                                      // max 120W
#define VVSWRBARMIN 30                                      // min 3.0
#define VVSWRBARMAX 60                                      // max 6.0




//
// read out (initialise) variables from eeprom
// NOTE THIS NEEDS TO BE MANUALLY INITIALISED IF A NEW PROCESSOR IS USED!
//
void eepromReadout(void)
{
  byte TempValue;
  
  TempValue = EEPROM.read(VEERX2);                              // TEMPORARY
  GVSWRThreshold = EEPROM.read(VEEVSWR);                        // VSWR threshold = 3.0
  GTempThreshold = EEPROM.read(VEETEMP);                        // temp threshold = 70C
  GPowerBarMax = EEPROM.read(VEEPWRMAX);                        // power bar covers 0 to 150W
  GVSWRBarMax = EEPROM.read(VEEVSWRMAX);                        // VSWR bar covers 0 to 6.0
  GRX2Antenna = (ERX2AntennaType)(TempValue&1);                 // pass 1 bit to set RX2 source
}


//
// Initialise settings in eeprom - done once manually
//
void initialiseEepromValues(void)
{
  EEPROM.write(VEERX2, 0);                          // ANT3 choice
  EEPROM.write(VEEVSWR, 50);                        // VSWR threshold = 3.0
  EEPROM.write(VEETEMP, 70);                        // temp threshold = 70C
  EEPROM.write(VEEPWRMAX, 120);                     // power bar covers 0 to 150W
  EEPROM.write(VEEVSWRMAX, 60);                     // VSWR bar covers 0 to 6.0
  eepromReadout();                                  // get the new values in use
}



//
// function to trigger an EEPROM write for an individual value
// the data should already be in the corresponding global variable
//
void storeToEeprom(EeepromVar Variable)
{
  switch(Variable)
  {
    case eeeRX2:                       // RX 2 source
      EEPROM.write(VEERX2, (byte)GRX2Antenna);
      break;
      
    case eeeVSWR:                      // VSWR trip threshold
      EEPROM.write(VEEVSWR, GVSWRThreshold);
      break;
      
    case eeeTEMP:                      // temp trip threshold
      EEPROM.write(VEETEMP, GTempThreshold);
      break;
      
    case eeePWRMAX:                    // power bar scale
      EEPROM.write(VEEPWRMAX, GPowerBarMax);
      break;
      
    case eeeVSWRMAX:                  // VSWR bar scale
      EEPROM.write(VEEVSWRMAX, GVSWRBarMax);
      break;
  }
}


//
// increment/decrement a global variable. Part of the UI code, but located here to streamline the UI code.
// adjust value by +/- 1 step, with min/max checks
//
void UpdateVSWRTrip(int Increment)                         // VSWR trip threshold +/- 0.1
{
  GVSWRThreshold += Increment;                              // increment/decrement value, then clip
  if (GVSWRThreshold < VVSWRTRIPMIN)
    GVSWRThreshold = VVSWRTRIPMIN;
  if (GVSWRThreshold > VVSWRTRIPMAX)
    GVSWRThreshold = VVSWRTRIPMAX;
}

void UpdateTempTrip(int Increment)                         // temp trip threshold +/- 1C
{
  GTempThreshold += Increment;                              // increment/decrement value, then clip  
  if (GTempThreshold < VTEMPTRIPMIN)
    GTempThreshold = VTEMPTRIPMIN;
  if (GTempThreshold > VTEMPTRIPMAX)
    GTempThreshold = VTEMPTRIPMAX;
}

void UpdatePowerBarSize(int Increment)                      // power display bar max +/- 1W
{
  GPowerBarMax += Increment;                              // increment/decrement value, then clip
  if (GPowerBarMax < VPWRBARMIN)
     GPowerBarMax = VPWRBARMIN; 
  if (GPowerBarMax > VPWRBARMAX)
     GPowerBarMax = VPWRBARMAX; 
}

void UpdateVSWRBarSize(int Increment)                     // VSWR display bar max value +/- 0.1
{
  GVSWRBarMax+= Increment;                              // increment/decrement value, then clip  
  if (GVSWRBarMax < VVSWRBARMIN)
     GVSWRBarMax = VVSWRBARMIN; 
  if (GVSWRBarMax > VVSWRBARMAX)
     GVSWRBarMax = VVSWRBARMAX; 
}


