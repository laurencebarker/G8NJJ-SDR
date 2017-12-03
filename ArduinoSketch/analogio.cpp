//************************************************************************
//
// Title: G8NJJ SDR Control Arduino sketch
// File: analogio.cpp
// copyright Laurence Barker G8NJJ 2017
//
// contains code for analog read from the ADC for VSWR, current and temp
//************************************************************************

#include <Arduino.h>
#include "eestore.h"
#include "iodefs.h"
#define VBUFLENGTH 32                   // no. of 16ms buffer elements

bool GTempOK;                           // true if heatsink temp below threshold
byte GHSTemp;                           // heatsink temperature, C
byte GForwardPower;                     // forward power, watts
byte GReversePower;                     // reverse power, watts
byte GPACurrent;                        // PA current, A, 1dp
byte GVSWR;                             // VSWR, 1dp. 0 if invalid.

byte GFwdPowerBuffer[VBUFLENGTH];       // circular buffer
byte GCircBufPtr;                       // pointer into circular buffer
#define VLINEVOLTSCALE 0.1074F          // convert ADC reading to volts
#define VCURRENTSCALE 0.2441F           // convert ADC reading to units of 0.1 amps
#define VTEMPSCALE 0.2441F              // convert ADC reading to celcius

//
// initialise analogue I/O
//
void InitAnalogueIO(void)
{
}


//
// analogue I/O 64 ms tick update
//
void AnalogueIOTick(void)
{
  int FwdVoltReading, RevVoltReading;             // raw ADC samples
  int PACurrentReading, TempReading;              // raw ADC samples
  float Unit;                                     // converted measurement
  float VFwd, VRev;                               // forward, reverse line voltages
  
  FwdVoltReading = analogRead(PIN_FWDPOWER);      // read forward power sensor (actually line volts)
  RevVoltReading = analogRead(PIN_REVPOWER);      // read reverse power sensor (actually line volts)
  PACurrentReading = analogRead(PIN_PACURRENT);   // read PA current sensor
  TempReading = analogRead(PIN_HSTEMP);           // read heatsink temp sensor
//
// convert the raw measurements to "normal" units
//
  VFwd = (float)FwdVoltReading * VLINEVOLTSCALE;        // forward line RMS voltage
  Unit = VFwd * VFwd * 0.02;                            // forward power, float (*0.02 not /50.0 for speed)
  GForwardPower = (byte)Unit;                           // convert to stored int
//
// write to circular buffer
//
  GFwdPowerBuffer[GCircBufPtr++] = GForwardPower;
  if (GCircBufPtr >= VBUFLENGTH)
    GCircBufPtr = 0;
    
  VRev = (float)RevVoltReading * VLINEVOLTSCALE;        // reverse line RMS voltage
  Unit = VRev * VRev * 0.02;                            // reverse power, float
  GReversePower = (byte)Unit;                           // convert to stored int

  Unit = (float)PACurrentReading * VCURRENTSCALE;       // units of 0.1A
  GPACurrent = (byte)Unit;                              // convert to int

  Unit = (float)TempReading * VTEMPSCALE;               // units of C
  GHSTemp = (byte)Unit;                                 // convert to int
//
// finally calculate VSWR
//
  if (VFwd != VRev)
    Unit = (VFwd+VRev) / (VFwd - VRev);                 // VSWR
  else
    Unit = 0.0;                                         // unvalid result
  GVSWR = (byte)(Unit * 10.0);                          // 1 decimal place int 
}

//
// HighVSWRCheck()
// returns true if VSWR high, else false
//
bool HighVSWRCheck()
{
  bool result = false;
  if (GVSWR >= GVSWRThreshold)
    result = true;

  return result;  
}

//
// HighTempCheck()
// returns true if heatsink temp high, else false
//
bool HighTempCheck()
{
  bool result = false;
  if (GHSTemp >= GTempThreshold)
    result = true;

  return result;  
}


//
// GetPeakPower()
// returns the peak power geld in the circular buffer
//
byte GetPeakPower()
{
  byte Result = 0;
  byte Cntr;
  byte Power;
  
  for (Cntr=0; Cntr < VBUFLENGTH; Cntr++)
  {
    Power = GFwdPowerBuffer[Cntr];
    if (Power > Result)
      Result = Power;
  }
  return Result;
}

