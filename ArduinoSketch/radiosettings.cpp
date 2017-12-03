//************************************************************************
//
// Title: G8NJJ SDR Control Arduino sketch
// File: radiosettings.cpp
// copyright Laurence Barker G8NJJ 2017
//
// this file controls the SDR hardware, mostly through a large shift register
// the shift register is driven by SPI
// settings from the RedPitaya arrive by I2C
//************************************************************************
#include <Arduino.h>
#include "iodefs.h"
#include "eestore.h"
#include "radiosettings.h"
#include "modeseq.h"
#include <SPI.h>
#include <Wire.h>


//
// global variables
// NOTE important to call UpdateRadioSettings() if these are changed
//
ERXAntennaType GRX1Antenna;
ERX2AntennaType GRX2Antenna;
ETXAntennaType GTXAntenna;
ERelayAttenuation GRX1RelayAtten;               // 0, 10, 20, 30dB
ERelayAttenuation GRX2RelayAtten;
char GRX1VarAtten;                              // 0-31dB
char GRX2VarAtten;                              // 0-31dB
char GTXVarAtten;                               // HALF this: 0-63, meaning 0-31.5dB
byte GRXOCBits;                                 // open collector o/p bits, when RX
byte GTXOCBits;                                 // open collector o/p bits, when TX
byte GTXFilterBits;                             // TX filter select bits
int GRXFilterBits;                              // RX filter bits (bits 11:0)
bool GHSFan;                                    // heatsink fan on/off
bool GNewI2CData;
unsigned int GRX1Freq;      // RX1 frequency, in KHz
unsigned int GRX2Freq;      // RX1 frequency, in KHz
unsigned int GTXFreq;       // TX frequency, in KHz
bool GNewI2CSettings;       // true if new settings received
char GRXAttenWhileTX;       // RX atten value during TX
bool GDisablePaTx;          // true if PC disables radio TX
bool GEnable6MLna;          // true if enable 6m PA

#define DEBUG_PRINT 1
#define VMAXRXATTEN 31                          // 31dB max RX
#define VMAXTXATTEN 63                          // 31.5dB max TX (stored value = double numerical value, 0.5dB steps)

//
// variables for RX and TX filter lookup tables
//
unsigned int GLPFMin, GLPFMax;                  // limits for current lowpass filter
unsigned int GBPFMin, GBPFMax;                  // limits for current bandpass filter
bool GValidLPF;                                 // true if LPF found within limits
unsigned int GBottomFirstBPF;                   // lower limit of 1st rable entry
unsigned int GTopLastBPF;                       // upper limit of last table entry

typedef struct
{
  unsigned int MinFilterFrequency;         // freq in KHz
  unsigned int MaxFilterFrequency;         // freq in KHz
  unsigned int FilterSetting;              // filter bits
} SFilterEntry;


#define VNUMLPF 7
SFilterEntry LPFLookupTable[VNUMLPF]=
{
{0, 2050,      0b00000001},
{2051, 3850,   0b00000010},
{3851, 7250,   0b00000100},
{7251, 14400,  0b00001000},
{14401, 21500, 0b00010000},
{21501, 29750, 0b00100000},
{29751, 52500, 0b01000000},
};



// BPF12 is the bypass filter 
#define VNUMBPF 11
#define VBYPASSFILTERBITS 0b100000000000

SFilterEntry BPFLookupTable[VNUMBPF]=
{
  {1750, 2050,   0b000000000001}, 
  {3450, 3850,   0b000000000010}, 
  {5200, 5600,   0b000000000100}, 
  {6950, 7250,   0b000000001000}, 
  {10000, 10200, 0b000000010000}, 
  {13950, 14400, 0b000000100000}, 
  {18000, 18200, 0b000001000000}, 
  {20950, 21500, 0b000010000000}, 
  {24800, 25000, 0b000100000000}, 
  {27950, 29750, 0b001000000000}, 
  {49500, 52500, 0b010000000000}, 
};

//
// function that looks up the right LPF setting for a new frequency
// if it finds a filter: set min, max limits and the filter bits
void LookupLPF(unsigned int NewFrequency)
{
  byte NewFilter = 255;                   // initialise to impossible value
  byte Index;
  SFilterEntry* FilterPtr;
  
  if((NewFrequency < GLPFMin) || (NewFrequency > GLPFMax))    // search if outside current filter
  {
    FilterPtr = LPFLookupTable;                                  // point at first filter
    for (Index=0; Index < VNUMLPF; Index++)                     // search forwards
    {
      if(FilterPtr -> MaxFilterFrequency >= NewFrequency)       // found a match
      {
        NewFilter=Index;
        break;
      }
      else
        FilterPtr++;
    }
    if(NewFilter != 255)                                      // if valid filter, use it
    {
      GValidLPF = true;
      GLPFMin = FilterPtr -> MinFilterFrequency;
      GLPFMax = FilterPtr -> MaxFilterFrequency;
      GTXFilterBits = (byte)(FilterPtr -> FilterSetting);
    }
    else
      GValidLPF = false;
  }
  
}


//
// function that looks up the right BPF setting for a new frequency
// if it finds a filter: set min, max limits and the filter bits
// if no filter found, mark limits to next filter and use the bypass filter
//
void LookupBPF(unsigned int NewFrequency)
{
  byte NewFilter = 255;                   // initialise to impossible value
  byte Index;
  SFilterEntry* FilterPtr;
  
  if (NewFrequency < GBottomFirstBPF)       // if below 1st filter
  {
    GBPFMin=0;                              // select bypass, until 1st filter
    GBPFMax = GBottomFirstBPF;
    GRXFilterBits = VBYPASSFILTERBITS;
  }
  else if (NewFrequency > GTopLastBPF)
  {
    GBPFMin=GTopLastBPF;                              // select bypass, until 1st filter
    GBPFMax = 65535;                                  // upper limit
    GRXFilterBits = VBYPASSFILTERBITS;
  }
  else                                                // lookup filter from table
  {
    FilterPtr = BPFLookupTable;                                  // point at first filter
    for(Index=0; Index < VNUMBPF; Index++)
    {
      if (FilterPtr -> MaxFilterFrequency >= NewFrequency)      // found a match
      {
        if (NewFrequency >= FilterPtr -> MinFilterFrequency)    // it would match the LPF entry
        {
          GBPFMin = FilterPtr -> MinFilterFrequency;
          GBPFMax = FilterPtr -> MaxFilterFrequency;
          GRXFilterBits = FilterPtr -> FilterSetting;
        }
        else                                                    // must be the blank space before this filter
        {
          GBPFMin = (FilterPtr-1) -> MaxFilterFrequency+1;
          GBPFMax = FilterPtr -> MinFilterFrequency-1;
          GRXFilterBits = VBYPASSFILTERBITS;
        }
        break;
      }
      else
        FilterPtr++;
    }
  }
}

void receiveEvent(int ByteCount);               // declare the I2C RX event handler

//
// initialise radio
//
void InitRadioSettings(void)
{
// TEMP DEBUG
  GHSFan = false;                                    // heatsink fan on/off
  
  SPI.begin();
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  UpdateRadioSettings();                        // prepare data for serial shift

  Wire.begin(0x40);                             // set i2c address = 0x40
  Wire.onReceive(receiveEvent);
  GNewI2CData = false;
  Serial.begin(9600);
  GLPFMin=0;                                    // current LPF filter not initialised
  GLPFMax=0;
  GBPFMin=0;                                    // current BPF filter not initialised
  GBPFMax=0;
//
// find freqs below first filter and above last filter:
//
  GBottomFirstBPF = BPFLookupTable[0].MinFilterFrequency;
  GTopLastBPF = BPFLookupTable[VNUMBPF-1].MaxFilterFrequency;
}

//
// debug printout
//
void Print2Bytes(byte byte1, byte byte2, byte byte3)
{
  Serial.print("cmd: ");
  Serial.print(byte1, HEX);
  Serial.print(", dat: ");
  Serial.print(byte2, HEX);
  Serial.print(", ");
  Serial.println(byte3, HEX);
}

void Print3Bytes(byte byte1, byte byte2, byte byte3, byte byte4)
{
  Serial.print("cmd: ");
  Serial.print(byte1, HEX);
  Serial.print(", dat: ");
  Serial.print(byte2, HEX);
  Serial.print(", ");
  Serial.print(byte3, HEX);
  Serial.print(", ");
  Serial.println(byte4, HEX);
}




//
// I2C receive event handler
//
void receiveEvent(int ByteCount)
{
  byte Msg, Byte1, Byte2, Byte3;

  Msg = Wire.read();      // read message id
  Byte1 = Wire.read();
  Byte2 = Wire.read();
  if (ByteCount == 4)
    Byte3=Wire.read();
    
  switch(Msg)
  {
    case 0x1:      // RX1 frequency
      GRX1Freq = Byte1 + Byte2 * 256;
#if defined DEBUG_PRINT
      Print2Bytes(Msg, Byte1, Byte2);
#endif
      break;

    case 0x2:      // RX2 frequency
      GRX2Freq = Byte1 + Byte2 *256;
#if defined DEBUG_PRINT
      Print2Bytes(Msg, Byte1, Byte2);
#endif
      break;

    case 0x3:      // TX frequency
      GTXFreq = Byte1 + Byte2 * 256;
#if defined DEBUG_PRINT
      Print2Bytes(Msg, Byte1, Byte2);
#endif
      break;

    case 0x4:      // OC, antenna settings
      GTXOCBits = Byte1;
      GRXOCBits = Byte2;
      GTXAntenna = (ETXAntennaType)(Byte3 & 0b11);
      GRX1Antenna = (ERXAntennaType)((Byte3 >> 4) & 0b111);
#if defined DEBUG_PRINT
      Print3Bytes(Msg, Byte1, Byte2, Byte3);
#endif
      break;

    case 0x5:      // TX attenuation
      GTXVarAtten = Byte1;
      GRXAttenWhileTX = Byte2;
      GDisablePaTx = (Byte3 >> 1) & 1;
      GEnable6MLna = Byte3 & 1;
#if defined DEBUG_PRINT
      Print3Bytes(Msg, Byte1, Byte2, Byte3);
#endif
      break;

    case 0x6:      // RX attenuation
      GRX1VarAtten = Byte1;
      GRX2VarAtten = Byte2;
#if defined DEBUG_PRINT
      Print2Bytes(Msg, Byte1, Byte2);
#endif
      break;
  }
  GNewI2CSettings = true;
  UpdateRadioSettings();
}



//
// radio 16ms tick
// update the filters selected for RX and TX
// and re-calculate the shift register settings
//
void RadioSettingsTick(void)
{
  LookupBPF(GRX1Freq);
  LookupLPF(GTXFreq);
  UpdateRadioSettings();
}


//
// get RX relay attenuation into integer dB
// parameter true if we want RX1
//
int GetRelayAttenuation(bool IsRX1)
{
  ERelayAttenuation AttenBits;
  byte Atten;

  if (IsRX1)
    AttenBits = GRX1RelayAtten;
  else
    AttenBits = GRX2RelayAtten;

  Atten = (byte)AttenBits * 10;
  return Atten;
}


//
// get RX variable attenuation into integer dB
// parameter true if we want RX1
//
int GetVariableAttenuation(bool IsRX1)
{
  char Atten;

  if (IsRX1)
    Atten = GRX1VarAtten;
  else
    Atten = GRX2VarAtten;

  return Atten;
}

//
// get total RX attenuation into integer dB
// parameter true if we want RX1
//
int GetTotalAttenuation(bool IsRX1)
{
  byte RelayAtten;
  char VarAtten;
  byte Answer;

  RelayAtten = GetRelayAttenuation(IsRX1);

  if (IsRX1)
    VarAtten = GRX1VarAtten;
  else
    VarAtten = GRX2VarAtten;

  Answer = RelayAtten + VarAtten;
  return Answer;
}


//
// declare the data that will transfer by SPI to radio hardware
// content documented in .docx file but roughly:
// byte 0: part TX atten
// byte 1: part TX atten, part RX2 atten
// byte 2: part RX2 attem RX1 atten
// byte 3: open collector outputs
// byte 4: LPF settings, heatsink fan
// byte 5: 4 BPF bits, RX 1 & 2 relay attenuation
// byte 6: 4 BPF bits, RX2, aux RX bits
// byte 7: 4 BPF bits, ant 1-3, RX bypass
// of these, only ant1-3 and open collector bits are TX/RX dependent
// so most of this only done once!
//
#define VNUMSHIFTBYTES 8
byte TXBitSettings[VNUMSHIFTBYTES];
byte RXBitSettings[VNUMSHIFTBYTES];
byte GSPIShiftSettings[VNUMSHIFTBYTES];


//
// InitiateSpiTX
// send the current RX or TX settings via SPI
// essentially this is just a big unidirectional shift register (we don't need the read reply)
// we want SPI mode 0, MSB first, unknown rate
//
void InitiateSpiTx(void)
{
  bool IsRX;                        // true if it is the RX setting we need to send

  digitalWrite(PIN_SERLOAD, LOW);         // be ready to give a rising edge after the transfer
  if ((GCurrentMode == eNormalRX) || (GCurrentMode == eDebugRX))
  {
    IsRX = true;
    memcpy(GSPIShiftSettings, RXBitSettings, VNUMSHIFTBYTES);          // make a copy of the data to be sent
  }
  else
  {
    IsRX = false;
    memcpy(GSPIShiftSettings, TXBitSettings, VNUMSHIFTBYTES);
  }
  SPI.transfer(GSPIShiftSettings, VNUMSHIFTBYTES);
  digitalWrite(PIN_SERLOAD, HIGH);         // rising edge to latch data after the transfer

}



//
// UpdateRadioSettings()
// called after any change to pre-calculate the shift words
// (either external new data, or user interface new data)
// need to calculate two sets of shift words: one for TX, one for RX 
//
void UpdateRadioSettings(void)
{
  byte DataByte;                    // calculated output byte
  byte Data2;
  bool Bypass = false;              // true if bypass relay activated
  byte RX1Att, RX2Att;

//
// get correct RX attenuations - depends if we are txing
//
  if ((GCurrentMode == eNormalRX) || (GCurrentMode == eDebugRX))    // RX mode
  {
    RX1Att = GRX1VarAtten;
    RX2Att = GRX2VarAtten;
  }
  else                                      // TX mode
  {
    RX1Att = GRXAttenWhileTX;
    RX2Att = GRXAttenWhileTX;
  }

// calculate byte 0
  DataByte = GTXVarAtten >> 4;       // only bottom 2 bits left
  TXBitSettings[0] = DataByte;
  RXBitSettings[0] = DataByte;
// calculate byte 1
  DataByte = (GTXVarAtten << 4) | (RX2Att >> 2);
  TXBitSettings[1] = DataByte;
  RXBitSettings[1] = DataByte;

// calculate byte 2
  DataByte = (RX2Att << 6) | (RX1Att & 0b00111111);
  TXBitSettings[2] = DataByte;
  RXBitSettings[2] = DataByte;

// byte 3 = OC bits - separate
  TXBitSettings[3] = GTXOCBits;
  RXBitSettings[3] = GRXOCBits;

// the last 4 bytes are harder!
// calculate byte 4
  DataByte = (GTXFilterBits & 0b00000111) << 4;
  DataByte = DataByte | ((GTXFilterBits & 0b01111000) >> 3);
  if (GHSFan)
    DataByte |= 0b10000000;
  TXBitSettings[4] = DataByte;
  RXBitSettings[4] = DataByte;

// calculate byte 5
  DataByte = (byte)(GRXFilterBits & 0b0000000001111);
  Data2 = (byte)GRX2RelayAtten;
  DataByte = DataByte | (Data2 << 4);
  Data2 = (byte)GRX1RelayAtten;
  DataByte = DataByte | (Data2 << 6);
  TXBitSettings[5] = DataByte;
  RXBitSettings[5] = DataByte;

// calculate byte 6 - messier still
  DataByte = (byte)(GRXFilterBits >> 4);
  DataByte &= 0b00001111;                   // just bits 5-8 left
  Data2 = (byte)GRX2Antenna;
  DataByte = DataByte | (Data2 << 4);
  if (GRX1Antenna == eRXAux1)               // convert from enumerated to bit positions
  {
    DataByte |= 0b00100000;
    Bypass = true;                          // bypass relay activated if an AUX path
  }
  else if (GRX1Antenna == eRXAux2)
  {
    DataByte |= 0b01000000;
    Bypass = true;                          // bypass relay activated if an AUX path
  }
  else if (GRX1Antenna == eRXAux3)
  {
    DataByte |= 0b10000000;
    Bypass = true;                          // bypass relay activated if an AUX path
  }
  TXBitSettings[6] = DataByte;
  RXBitSettings[6] = DataByte;

// finally calculate byte 7
  DataByte = (byte)(GRXFilterBits >> 8);        // get bits 9-12
  if (Bypass)
    DataByte |= 0b10000000;                     // set bypass bit if we had an aux antenna
//
// now do RX in DataByte, TX in data2
//
  Data2=DataByte;
  if (GRX1Antenna == eRXAnt1)               // convert from enumerated to bit positions
    DataByte |= 0b00010000;
  else if (GRX1Antenna == eRXAnt2)
    DataByte |= 0b00100000;
  else if (GRX1Antenna == eRXAnt3)
    DataByte |= 0b01000000;
  else                                     // if an aux antenna, copy the TX antenna bits to relays 1-3
  {
    if (GTXAntenna == eTXAnt1)               // convert from enumerated to bit positions
      DataByte |= 0b00010000;
    else if (GTXAntenna == eTXAnt2)
      DataByte |= 0b00100000;
    else if (GTXAntenna == eTXAnt3)
      DataByte |= 0b01000000;
  }
  
  if (GTXAntenna == eTXAnt1)               // convert from enumerated to bit positions
    Data2 |= 0b00010000;
  else if (GTXAntenna == eTXAnt2)
    Data2 |= 0b00100000;
  else if (GTXAntenna == eTXAnt3)
    Data2 |= 0b01000000;
  
  RXBitSettings[7] = DataByte;
  TXBitSettings[7] = Data2;
//
// finally send the data to destination
//  
  InitiateSpiTx();
}

//
// string tables for enumerated values
//
const char* RX1AntennaStrings[] =
{
  "Ant1",
  "Ant2",
  "Ant3",
  "Aux1",
  "Aux2",
  "Aux3"
};


const char* RX2AntennaStrings[] =
{
  "Ant3",
  "Aux3"
};

const char* RXRelayAttenStrings[] = 
{
  "0dB ",
  "10dB",
  "20dB",
  "30dB"
};


//
//"getter" functions to get a string pointer for the different enumerated settings
// return a string pointer, from a table
//
const char* GetRX1AntennaStr()
{
  return RX1AntennaStrings[GRX1Antenna];
}


const char* GetRX2AntennaStr()
{
  return RX2AntennaStrings[GRX2Antenna];
}


const char* GetTXAntennaStr()
{
  return RX1AntennaStrings[GTXAntenna];
}


const char* GetRXRelayAtten(bool IsRX1)
{
  ERelayAttenuation AttenBits;

  if (IsRX1)                                                // get correct pointer
    AttenBits = GRX1RelayAtten;
  else
    AttenBits = GRX2RelayAtten;
  return RXRelayAttenStrings[AttenBits];
}


const char* FanStrings[] = 
{
"Off",
"On"  
};


//
// get fan on/off state
//
const char* GetFanOnOffStr()
{
  return FanStrings[(int)GHSFan];
}


//
// functions to change enumerated values to next value
// these are invoked from the UI editor to simplify the UI code
//
void NextRXAntenna(bool IsRX1)                  // round robin Increment RX1 or RX2 to next enumerated type
{
  if (IsRX1)
  {
    if (GRX1Antenna < eRXAux3)
      GRX1Antenna = (ERXAntennaType)(((int)GRX1Antenna) + 1);       // horrible typecasting to increment!
    else
      GRX1Antenna = eRXAnt1;
  }
  else                                        // toggle RX2 antenna
  {
    if (GRX2Antenna == eRX2Ant3)
      GRX2Antenna = eRX2Aux3;
    else
      GRX2Antenna =  eRX2Ant3;
    storeToEeprom(eeeRX2);                    // and save result into EEPROM
  }
}

void NextTXAntenna(void)                      // goto next TX antenna option
{
    if (GTXAntenna < eTXAnt3)
      GTXAntenna = (ETXAntennaType)(((int)GTXAntenna) + 1);
    else
      GTXAntenna = eTXAnt1;
}

void NextRXRelayAtten(bool IsRX1)             // goto next relay attenuation option
{
  ERelayAttenuation AttValue;
  if (IsRX1)
    AttValue = GRX1RelayAtten;
  else
    AttValue = GRX2RelayAtten;
//
// now increment
//
  if (AttValue < e30dB)
    AttValue = (ERelayAttenuation)(((int)AttValue) + 1);
  else
    AttValue = e0dB;
//
// now write the result back
//  
  if (IsRX1)
    GRX1RelayAtten = AttValue;
  else
    GRX2RelayAtten = AttValue;
}

void ToggleFan(void)                        // change fan setting
{
  if (GHSFan == false)
    GHSFan = true;
  else
    GHSFan = false;
}

//
// increment/decrement values for the numerical atten values
// Increment = +/- 1
// editor clips accordingly
//
void UpdateRXAtten(int Increment, bool IsRX1)
{
  char Atten;                             // temp edit value
  if (IsRX1)                              // first get the correct current value
    Atten = GRX1VarAtten;
  else
    Atten = GRX2VarAtten;
//
// update the value
//
  Atten += Increment;
  if (Atten <0)
    Atten = 0;
  if (Atten > VMAXRXATTEN)
    Atten = VMAXRXATTEN;

  if (IsRX1)                              // save back the new value
    GRX1VarAtten = Atten;
  else
    GRX2VarAtten = Atten;
}


void UpdateTXAtten(int Increment)
{
  char Atten;                             // temp edit value
  Atten = GTXVarAtten;
//
// update the value
//
  Atten += Increment;
  if (Atten <0)
    Atten = 0;
  if (Atten > VMAXTXATTEN)
    Atten = VMAXTXATTEN;

  GTXVarAtten = Atten;  
}




