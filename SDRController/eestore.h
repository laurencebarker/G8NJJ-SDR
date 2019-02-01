//************************************************************************
//
// Title: G8NJJ SDR Control Arduino sketch
// File: eestore.h
// copyright Laurence Barker G8NJJ 2017
//
//************************************************************************
//
// EEPROM storage definitions
//


//
// this enumerated type identifies one EEPROM variable
//
typedef enum
{
  eeeRX2,                       // RX 2 source
  eeeVSWR,                      // VSWR trip threshold
  eeeTEMP,                      // temp trip threshold
  eeePWRMAX,                    // power bar scale
  eeeVSWRMAX                    // VSWR bar scale
} EeepromVar;



extern char GVSWRThreshold;                         // VSWR trip threshold    (stored 1dp fixed point)
extern char GTempThreshold;                         // temp trip threshold
extern char GPowerBarMax;                           // power display bar max value, W
extern char GVSWRBarMax;                            // VSWR display bar max value (1dp fixed point)


//
// read out (initialise) variables from eeprom
//
void eepromReadout(void);



//
// Initialise settings in eeprom - done once manually
//
void initialiseEepromValues(void);



//
// function to trigger an EEPROM write for an individual value
// the data should already be in the corresponding global variable
//
void storeToEeprom(EeepromVar Variable);



//
// increment/decrement a global variable. Part of the UI code, but located here to streamline the UI code.
// adjust value by +/- 1 step, with min/max checks
//
void UpdateVSWRTrip(int Increment);
void UpdateTempTrip(int Increment);
void UpdateVSWRBarSize(int Increment);
void UpdatePowerBarSize(int Increment);

