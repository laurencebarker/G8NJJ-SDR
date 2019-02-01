//************************************************************************
//
// Title: G8NJJ SDR Control Arduino sketch
// File: radiosettings.h
// copyright Laurence Barker G8NJJ 2017
//
//************************************************************************

//
// user defined types
//

typedef enum                      // allowed types for main RX antenna
{
  eRXAnt1,
  eRXAnt2,
  eRXAnt3,
  eRXAux1,
  eRXAux2,
  eRXAux3
} ERXAntennaType;


typedef enum                      // allowed types for RX2 antenna
{
  eRX2Ant3,
  eRX2Aux3
} ERX2AntennaType;


typedef enum                      // allowed types for main RX antenna
{
  eTXAnt1,
  eTXAnt2,
  eTXAnt3
} ETXAntennaType;


typedef enum                      // relay controlled attenuation
{
  e0dB,
  e10dB,
  e20dB,
  e30dB
} ERelayAttenuation;

//
// defines for widths of bit fields
//
#define VNUMOCBITS 8              // number of open collector output bits
#define VNUMRXFILTBITS 12         // number of bits to control RX bandpass filters
#define VNUMTXFILTBITS 7          // number of bits to control TX lowpass filters

//
// references to global variables
//
extern bool GNewI2CSettings;       // true if new settings received
extern ERXAntennaType GRX1Antenna;
extern ERX2AntennaType GRX2Antenna;
extern ETXAntennaType GTXAntenna;
extern ERelayAttenuation GRX1RelayAtten;               // 0, 10, 20, 30dB
extern ERelayAttenuation GRX2RelayAtten;
extern char GRX1VarAtten;                              // 0-31dB
extern char GRX2VarAtten;                              // 0-31dB
extern char GTXVarAtten;                               // HALF this: 0-63, meaning 0-31.5dB
extern byte GRXOCBits;                                 // open collector o/p bits, when RX
extern byte GTXOCBits;                                 // open collector o/p bits, when TX
extern byte GTXFilterBits;                             // TX filter select bits
extern int GRXFilterBits;                              // RX filter bits (bits 11:0)
extern bool GHSFan;                                    // heatsink fan on/off
extern bool GDisablePaTx;                              // true if PC disables PA TX
extern bool GValidLPF;                                 // true if LPF found within limits
extern unsigned int GRX1Freq;                          // RX1 frequency, in KHz


//
// defines for I2C variables (these may not be needed long term)
//
extern byte GI2CByte1;
extern byte GI2CByte2;
extern byte GI2CByte3;
extern byte GI2CByteCount;





//
// initialise radio
//
void InitRadioSettings(void);


//
// radio 16ms tick
//
void RadioSettingsTick(void);


//
// get RX relay attenuation into integer dB
// parameter true if we want RX1
//
int GetRelayAttenuation(bool IsRX1);

//
// get RX variable attenuation into integer dB
// parameter true if we want RX1
//
int GetVariableAttenuation(bool IsRX1);

//
// get total RX  attenuation into integer dB
// parameter true if we want RX1
//
int GetTotalAttenuation(bool IsRX1);


//
// UpdateRadioSettings()
// called after any change to pre-calculate the shift words
//
void UpdateRadioSettings(void);


//
//"getter" functions to get a string pointer for the different enumerated settings
// return a string pointer, from a table
//
const char* GetRX1AntennaStr();
const char* GetRX2AntennaStr();
const char* GetTXAntennaStr();
const char* GetRXRelayAtten(bool IsRX1);
const char* GetFanOnOffStr();

//
// functions to change enumerated values to next value
// these are invoked from the UI editor to simplify the UI code
//
void NextRXAntenna(bool IsRX1);
void NextTXAntenna(void);
void NextRXRelayAtten(bool IsRX1);
void ToggleFan(void);
//
// increment/decrement values for the numerical atten values
// Increment = +/- 1
// editor clips accordingly
//
void UpdateRXAtten(int Increment, bool IsRX1);
void UpdateTXAtten(int Increment);

//
// InitiateSpiTX
// send the current RX or TX settings via SPI
// essentially this is just a big unidirectional shift register (we don't need the read reply)
//
void InitiateSpiTx(void);
