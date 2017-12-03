//************************************************************************
//
// Title: G8NJJ SDR Control Arduino sketch
// File: analogio.h
// copyright Laurence Barker G8NJJ 2017
//
//************************************************************************


extern byte GHSTemp;                            // heatsink temperature, C
extern byte GForwardPower;                      // forward power, watts
extern byte GReversePower;                      // reverse power, watts
extern byte GPACurrent;                         // PA current, A, 1dp
extern byte GVSWR;                              // VSWR, 1dp. 0 if invalid.



//
// initialise analogue I/O
//
void InitAnalogueIO(void);


//
// analogue I/O tick update
//
void AnalogueIOTick(void);


//
// HighVSWRCheck()
// returns true if VSWR high, else false
//
bool HighVSWRCheck();


//
// HighTempCheck()
// returns true if heatsink temp high, else false
//
bool HighTempCheck();

//
// GetPeakPower()
// returns the peak power geld in the circular buffer
//
byte GetPeakPower();

