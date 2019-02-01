//************************************************************************
//
// Title: G8NJJ SDR Control Arduino sketch
// File: modeseq.cpp
// copyright Laurence Barker G8NJJ 2017
//
// this file includes the user interface code to write the LCD display and handle the encoder.
// Most of the project's code is probably in this module!
//
//************************************************************************
//
//
#include <Arduino.h>
#include <ClickEncoder.h>
#include "modeseq.h"
#include "iodefs.h"
#include "analogio.h"
#include "displayutils.h"
#include "radiosettings.h"
#include "eestore.h"


ESequencerState GCurrentMode;         // state variable 
ESequencerState GDisplayedMode;       // last mode to be displayed
int16_t last, value;

ClickEncoder *encoder;
bool GEditorActive;                   // true if an RX mode editor is active
byte GRXLastLine;                     // chooses one of 21 "bottom lines"
byte GEditorCursorPos;                // cursor position, if in edit mode (always line 4)
byte GBitEditNumBits;                 // number oif bits in edited bit field
byte GBitEditCursorStart;             // 1st position of bit edit field in edit submode

#define VTEMPUPDATECLICKS 64                      // update temp once per second
#define VPOWERUPDATECLICKS 6                      // update TX pwr etc ~10 times per second
byte GTempUpdateClicks;
byte GPowerUpdateClicks;



//
// TX request pin interrupt handler
//
void TXReqIntHandler(void)
{
  if (digitalRead(PIN_TXREQ) == HIGH)              // i/p 1 for RX mode
  {
    if (GCurrentMode >= eDebugRX)
      GCurrentMode = eDebugRX;
    else
      GCurrentMode = eNormalRX;
      digitalWrite(PIN_TXEN, LOW);                // turn off TX
      GRXLastLine = 0;                            // and reset the bottom line of the display
  }
  else                                            // go to TX mode
  {
    if (GCurrentMode >= eDebugRX)                 // if debug, always go to TX; 
    // ">=" test to allow for bounce on the input
    {
      GCurrentMode = eDebugTX;
      digitalWrite(PIN_TXEN, HIGH);
    }
    else if ((GDisablePaTx) || (!GValidLPF))      // if disabled by PC, 
    {
      GCurrentMode = eNormalTXDisabled;
    }
    else if (!HighTempCheck())                    // TX, and temp OK
    {
      GCurrentMode = eNormalTX;
      digitalWrite(PIN_TXEN, HIGH);
    }
    else                                    // TX barred because of high temp
    {
      GCurrentMode = eNormalTXTemp;
      digitalWrite(PIN_TXEN, LOW);
    }
  }
//
// send TX or RX settings to radio hardware  
//
  InitiateSpiTx();
}

//
// initialise sequencer
// start state depends on jumper setting
//
void InitSequencer(void)
{
  encoder = new ClickEncoder(ENC_A, ENC_B, ENC_PB, ENC_STEPS);

//
// work out start state, depending on jumper. Always power up into RX.
//  
  if (digitalRead(PIN_DEBUGJUMPER) == LOW)
    GCurrentMode = eDebugRX;
  else
    GCurrentMode = eNormalRX;
  GDisplayedMode = eDisplayUninitialised;               // set display not written
  attachInterrupt(digitalPinToInterrupt(PIN_TXREQ), TXReqIntHandler, CHANGE);
//
// initialise display update rates
//  
  GTempUpdateClicks = 1;                              // uopdate immediately
  GPowerUpdateClicks = 1;                             // uopdate immediately
}


//
// these are constant strings to build the last line of the display in RX mode
// the line used is selected by the rotary encoder.
// All strings 20 characters long, to clear the whole line
//
const char* LastLineStrings[] = 
{
  "                    ",
  "RX1 Ant:            ",
  "RX2 Ant:            ",
  "TX Ant:             ",
  "RX1 Atten1:   dB    ",
  "RX1 Atten2:   dB    ",
  "RX2 Atten1:   dB    ",
  "RX2 Atten2:   dB    ",
  "TX Atten:     dB    ",
  "RX OC:              ",
  "TX OC:              ",
  "RX filt:            ",
  "TX filt:            ",
  "Heatsink Fan:       ",
  "Enter TX Mode       ",
  "VSWR trip:          ",
  "Temp trip:    C     ",
  "Pwr bar max W:      ",
  "VSWR bar max:       ",
  "Initialise EEPROM   ",
  "Sketch Version: 1.0 "
};
#define VMAXEDITROW 20


//
// draw power bar on display
// parameter is the current forward power, in watts
//
void DrawPowerBar(byte Power)
{
  float Length;
  Length = (float)Power / (float)GPowerBarMax;
  DrawBarOnLCD(2, Length);
}


//
// bargraph limits: 
// power bar displays power as (0 to GPowerBarMax) on row 2
// VSWR bar displays VSWR as (1.0 to GVSWRBarMax) on row 3
// each bar is up to 15 display segments long
//

//
// draw VSWR bar on display
// parameter is VSWR, 1DP fixed point
// left of bar (no segments) = 1.0
//
void DrawVSWRBar(byte VSWR)
{
  float Length;
  if (VSWR != 0)
  {
    Length = (float)(VSWR-10) / (float)(GVSWRBarMax-10);
    DrawBarOnLCD(3, Length);
  }
  
}




//
// RedrawLastLine()
// redraws the bottom line of the display in receive modes
//
void RedrawLastLine(void)
{
//
// first print the initial part:
//
  printAtXY(0,3,LastLineStrings[GRXLastLine]);      // add new text
  
//
// now row dependent updates
//  
  switch(GRXLastLine)
  {
    case 0:               // no line displayed
      break;
      
    case 1:               // RX1 ant
      printAtXY(9, 3, GetRX1AntennaStr());
      break;
      
    case 2:               // RX2 ant
      printAtXY(9, 3, GetRX2AntennaStr());
      break;
      
    case 3:               // TX ant
      printAtXY(8, 3, GetTXAntennaStr());
      break;
      
    case 4:               // RX1 relay atten
      printIntAtXY(12, 3, GetRelayAttenuation(true));
      break;
      
    case 5:               // RX1 variable atten
      printIntAtXY(12, 3, GetVariableAttenuation(true));
      break;
      
    case 6:               // RX2 relay atten
      printIntAtXY(12, 3, GetRelayAttenuation(false));
      break;
      
    case 7:               // RX2 variable atten
      printIntAtXY(12, 3, GetVariableAttenuation(false));
      break;
      
    case 8:               // TX atten
      printIntAtXY(10, 3, GTXVarAtten*5, true);                      // 1dp, so *10; but atten = half value stored
      break;
      
    case 9:               // RX open collector bits
      printBitsAtXY(6, 3, GRXOCBits, VNUMOCBITS);
      break;
      
    case 10:               // TX open collector bits
      printBitsAtXY(6, 3, GTXOCBits, VNUMOCBITS);
      break;
      
    case 11:              // RX filter select bits
      printBitsAtXY(8, 3, GRXFilterBits, VNUMRXFILTBITS);
      break;
      
    case 12:              // TX filter select bits
      printBitsAtXY(8, 3, GTXFilterBits, VNUMTXFILTBITS);
      break;
      
    case 13:              // fan on/off
      printAtXY(14, 3, GetFanOnOffStr());
      break;
      
    case 14:              // enter TX mode
      break;
      
    case 15:              // enter VSWR trip value
      printIntAtXY(11, 3, GVSWRThreshold, true);           // 1dp
      break;
      
    case 16:              // enter temp trip value
      printIntAtXY(11, 3, GTempThreshold);
      break;
      
    case 17:              // enter power bar max
      printIntAtXY(15, 3, GPowerBarMax);
      break;
      
    case 18:              // enter VSWR bar max
      printIntAtXY(14, 3, GVSWRBarMax, true);            // 1dp
      break;
      
    case 19:              // initialse EEPROM (no text needed)
      break;
      
    case 20:              // firmware version (no text needed)
      break;
  }
  UpdateRadioSettings();                          // recalculate the RX values to sent to H/W
// 
// finally if in editor submode, set the cursor where it should be
//  
  if (GEditorActive)
    lcd.setCursor(GEditorCursorPos, 3);
}



//
// redraw display
// this does a complete redraw of the display
//
void redrawDisplay(void)
{
  byte FwdPower;                          // current peak power reading
  
  GDisplayedMode = GCurrentMode;                  // set this is the mode we've drawn
  lcd.clear();
  switch(GCurrentMode)
  {
    case eDebugRX:                       // debug mode RX
    case eNormalRX:                      // normal operating mode, RX
      if (GCurrentMode == eDebugRX)      // orint mode (this is mode dependent)   
        printAtXY(0,0, "RXD");
      else
        printAtXY(0,0, "RX");

      lcd.setCursor(4,0);
      lcd.print((long)GRX1Freq);
      printAtXY(10,0, "KHz");

      printIntAtXY(15,0,GHSTemp);
      lcd.print("C  ");
      printAtXY(0,1, "RX1");
      printAtXY(5,1, GetRX1AntennaStr());
      printIntAtXY(12,1, GetTotalAttenuation(true));
      lcd.print("dB");
      printAtXY(0,2, "RX2");
      printAtXY(5,2, GetRX2AntennaStr());
      printIntAtXY(12,2, GetTotalAttenuation(false));
      lcd.print("dB");
      break;
      
    case eNormalTX:                               // normal operating mode, TX
      printAtXY(0,0,"TX");
      printAtXY(5,0, GetTXAntennaStr());
      printIntAtXY(15,0,GHSTemp);
      lcd.print("C  ");
      FwdPower = GetPeakPower();
      DrawPowerBar(FwdPower);
      printIntAtXY(16,1, FwdPower);
      lcd.print("W  ");
      DrawVSWRBar(GVSWR);
      printIntAtXY(16,2, GVSWR, true);              // print with 1 decimal place
      printIntAtXY(0,3, GPACurrent, true);          // 1 decimal place
      lcd.print("A ");
      break;
      
    case eNormalTXVSWR:                           // normal TX tripped by high VSWR
      printAtXY(0,0,"TX");
      printAtXY(5,0, GetTXAntennaStr());
      printIntAtXY(15,0,GHSTemp);
      lcd.write('C');
      printAtXY(0,1,"High VSWR tripped");
      break;


    case eNormalTXDisabled:                           // normal TX disabled by PC
      printAtXY(0,0,"TX");
      printAtXY(5,0, GetTXAntennaStr());
      printIntAtXY(15,0,GHSTemp);
      lcd.write('C');
      if (!GValidLPF)
        printAtXY(0,1,"Freq > LPF Bank");
      else 
        printAtXY(0,1,"TX disabled by SDR s/w");
      break;


    case eNormalTXTemp:                           // normal TX tripped by high temp
      printAtXY(0,0,"TX");
      printAtXY(5,0, GetTXAntennaStr());
      printIntAtXY(15,0,GHSTemp);
      lcd.print("C ");
      printAtXY(0,1,"High temp tripped");
      break;
      
    case eDebugTX:                                // debug mode TX
      printAtXY(0,0,"TXD");
      printAtXY(5,0, GetTXAntennaStr());
      printIntAtXY(15,0,GHSTemp);
      lcd.print("C ");
      printAtXY(0,1,"Fwd:");
      printIntAtXY(5,1, GForwardPower);
      lcd.print("W  ");
      printAtXY(0,2,"Rev:");
      printIntAtXY(5,2, GReversePower);
      lcd.print("W  ");
      printIntAtXY(0,3, GPACurrent, true);        // 1 decimal place
      lcd.print("A ");
      break;
  }
  RedrawLastLine();                               // draw the bottom row
}


//
// update display
// this updates only the parts of the display that change
// this is the "normal" action, only updating parameters likely to change
// this is to stop the display flickering
//
void updateDisplay(void)
{
  bool UpdateTemp = false;                              // true if to do a temp update
  bool UpdatePower = false;                             // true if to do power/VSWR/current update
//
// work out which "variable rate" updates to do
//  
  if(GTempUpdateClicks-- == 0)
  {
    UpdateTemp = true;
    GTempUpdateClicks = VTEMPUPDATECLICKS-1;
  }
  if(GPowerUpdateClicks-- == 0)
  {
    UpdatePower = true;
    GPowerUpdateClicks = VPOWERUPDATECLICKS-1;
  }
//
// now update the display
//
  switch(GCurrentMode)
  {
    case eDebugRX:                       // debug mode RX
    case eNormalRX:                      // normal operating mode, RX
      if (UpdateTemp)                    // update temperature only infrequently
      {
        printAtXY(15,0,"   ");           // erase any previous number
        printIntAtXY(15,0,GHSTemp);
        lcd.print("C ");
      }
      break;
      
    case eNormalTX:                      // normal operating mode, TX
      if (UpdateTemp)                    // update temperature only infrequently
      {
        printAtXY(15,0,"   ");         // erase any previous number
        printIntAtXY(15,0,GHSTemp);
        lcd.print("C ");
      }

      if (UpdatePower)                    // update power/VSWR/current only infrequently
      {
//
// need code here to draw the power and VSWR bars
//
        printAtXY(0,1,"                    ");         // erase any previous bar
        printAtXY(0,2,"                    ");         // erase any previous bar
  
        DrawPowerBar(GForwardPower);
        printIntAtXY(16,1, GForwardPower);
        lcd.print("W  ");

        DrawVSWRBar(GVSWR);
        if (GVSWR != 0)
          printIntAtXY(16,2, GVSWR, true);
        else
          printAtXY(16, 2, "---");
        printAtXY(0,3,"    ");         // erase any previous number
        printIntAtXY(0,3, GPACurrent, true);
        lcd.print("A ");
      }
      break;
      
    case eNormalTXVSWR:                  // normal TX tripped by high VSWR
    case eNormalTXDisabled:              // normal TX disabled by PC
      if (UpdateTemp)                    // update temperature only infrequently
      {
        printAtXY(15,0,"   ");           // erase any previous number
        printIntAtXY(15,0,GHSTemp);
        lcd.print("C ");
      }
      break;

      
    case eNormalTXTemp:                  // normal TX tripped by high temp
      if (UpdateTemp)                    // update temperature only infrequently
      {
        printAtXY(15,0,"   ");           // erase any previous number
        printIntAtXY(15,0,GHSTemp);
        lcd.print("C ");
      }
      break;
      
    case eDebugTX:                       // debug mode TX
      if (UpdateTemp)                    // update temperature only infrequently
      {
        printAtXY(15,0,"   ");           // erase any previous number
        printIntAtXY(15,0,GHSTemp);
        lcd.print("C ");
      }

      if (UpdatePower)                    // update power/VSWR/current only infrequently
      {
        printAtXY(5,1,"   ");         // erase any previous number
        printIntAtXY(5,1, GForwardPower);
        lcd.print("W  ");
        printAtXY(5,2,"   ");         // erase any previous number
        printIntAtXY(5,2, GReversePower);
        lcd.print("W  ");
        printAtXY(0,3,"   ");         // erase any previous number
        printIntAtXY(0,3, GPACurrent, true);
        lcd.print("A ");
      }
      break;
  }
}


//
// StartNumericalEditorSubmode(int CursorPos)
// this is a "helper" function to make thre settings to enter edit mode
//
void StartNumericalEditorSubmode(int CursorPos)
{
  GEditorActive = true;               // turn on edit submode
  lcd.cursor();
  GEditorCursorPos = CursorPos;
  lcd.setCursor(CursorPos, 3);               // turned on cursor at bottom row defined position
}


//
// StartBitEditorSubmode(int CursorPos)
// this is a "helper" function to make thre settings to enter edit mode
//
void StartBitEditorSubmode(int CursorPos, int NumBits)
{
  GEditorActive = true;               // turn on edit submode
  GBitEditNumBits = NumBits;          // store the field width
  GEditorCursorPos = CursorPos;       // set initial cursor position
  GBitEditCursorStart = CursorPos;   // set start of field
  lcd.cursor();
  lcd.setCursor(CursorPos, 3);               // turned on cursor at bottom row defined position
}

//
// bool BitEditorIncrement(int Increment)
// handles the left/right increment caused by the rotary encoder
// if move out of the bit field to the left, exits edit
// limit motion to the right of the field
// returns true if exited.
//
bool BitEditorIncrement(int Increment)
{
  bool Result= false;

  if (Increment == 1)               // move to right if not already at right edge
  {
    if (GEditorCursorPos < (GBitEditCursorStart + GBitEditNumBits - 1))
    {
      GEditorCursorPos++;                           // update cursor position
      lcd.setCursor(GEditorCursorPos, 3);
    }
  }
  else if (Increment == -1)         // move to left if not already at left; else exit edit
  {
    if (GEditorCursorPos > GBitEditCursorStart)
    {
      GEditorCursorPos--;                           // update cursor position
      lcd.setCursor(GEditorCursorPos, 3);
    }
    else                            // exit edit mode
    {
      lcd.noCursor();
      GEditorActive = false;
      Result = true;                    // redraw display afterwards
    }
  }
  return Result;
}



//
// 16ms tick handler
//
void SequencerTick(void)
{
  int EncoderIncrement;
  bool DisplayUpdated = false;                              // true if display needs to be redrawn
  int BitMask;                                              // bit position used in bit edit
  
  EncoderIncrement = encoder->getValue();                   // only ever 0, 1 or -1
  ClickEncoder::Button btn = encoder->getButton();

//
// redraw (if changed mode) or update the display
// only update if there is no current editor active
// avoids cursor movement)
  if ((GCurrentMode != GDisplayedMode) || (GNewI2CSettings))
  {
    GNewI2CSettings = false;                                // clear it now to avoid race
    DisplayUpdated = true;                                  // force redraw Display at end
  }
  else if (!GEditorActive)
    updateDisplay();
//
// now mode dependent settings
// any state transitions, encoder actions or button press
  switch(GCurrentMode)
  {
    case eNormalRX:                           // RX modes - many encoder actions
    case eDebugRX:
      if (!GEditorActive)                     // no edit mode - scroll, or initiate an editor
      {
//
// firstly see if the encoder wheel has moved to change the bottom line of the display
//
        if(EncoderIncrement >= 1)             // see if move to next line
        {
          if (GRXLastLine < VMAXEDITROW)
          {
            GRXLastLine++;
            RedrawLastLine();
          }
        }
          else if (EncoderIncrement <= -1)    // see if move to previous line
        {
          if (GRXLastLine > 0)
          {
            GRXLastLine--;
            RedrawLastLine();
          }
        }
//
// still in the case where no edit is active:
// now row dependent updates for encoder presses (initiate edits)
//  
        if (btn == ClickEncoder::Clicked)
          switch(GRXLastLine)
          {
            case 0:               // no line displayed
              break;
              
            case 1:               // RX1 ant
              NextRXAntenna(true);
              DisplayUpdated = true;
              break;
              
            case 2:               // RX2 ant
              NextRXAntenna(false);
              DisplayUpdated = true;
              break;
              
            case 3:               // TX ant
              NextTXAntenna();
              DisplayUpdated = true;
              break;
              
            case 4:               // RX1 relay atten
              NextRXRelayAtten(true);
              DisplayUpdated = true;
              break;
              
            case 5:               // RX1 variable atten
              StartNumericalEditorSubmode(12);                // initiate the paramter edit
              break;
              
            case 6:               // RX2 relay atten
              NextRXRelayAtten(false);
              DisplayUpdated = true;
              break;
              
            case 7:               // RX2 variable atten
              StartNumericalEditorSubmode(12);                // initiate the paramter edit
              break;
              
            case 8:               // TX atten
              StartNumericalEditorSubmode(11);                // initiate the paramter edit
              break;
              
            case 9:               // RX open collector bits
              StartBitEditorSubmode(6, VNUMOCBITS);           // initiate bit editor with cursor pos 6
              break;
              
            case 10:               // TX open collector bits
              StartBitEditorSubmode(6, VNUMOCBITS);           // initiate bit editor with cursor pos 6
              break;
              
            case 11:              // RX filter select bits
              StartBitEditorSubmode(8, VNUMRXFILTBITS);           // initiate bit editor with cursor pos 8
              break;
              
            case 12:              // TX filter select bits
              StartBitEditorSubmode(8, VNUMTXFILTBITS);           // initiate bit editor with cursor pos 8
              break;
              
            case 13:              // fan on/off
              ToggleFan();
              DisplayUpdated = true;
              break;
              
            case 14:              // enter TX mode (only in debug mode)
              if (GCurrentMode == eDebugRX)
              {
                GCurrentMode = eDebugTX;
                digitalWrite(PIN_TXEN, HIGH);                // assert TX output
                DisplayUpdated = true;
                InitiateSpiTx();
              }
              break;
              
            case 15:              // enter VSWR trip value
              StartNumericalEditorSubmode(13);                // initiate the paramter edit
              break;
              
            case 16:              // enter temp trip value
              StartNumericalEditorSubmode(12);                // initiate the paramter edit
              break;
              
            case 17:              // enter power bar max
              StartNumericalEditorSubmode(16);                // initiate the paramter edit
              break;
              
            case 18:              // enter VSWR bar max
              StartNumericalEditorSubmode(16);                // initiate the paramter edit
              break;
              
            case 19:              // initialse EEPROM (no text needed)
              initialiseEepromValues();
              DisplayUpdated = true;
              break;

            case 20:              // firmware version
              break;
          }
      }
      else                // an editor is active - find what's being edited
      {                   // bottom row dependent updates
        switch(GRXLastLine)
        {
          case 0:               // no line displayed
            break;
            
          case 1:               // RX1 ant
            break;
            
          case 2:               // RX2 ant
            break;
            
          case 3:               // TX ant
            break;
            
          case 4:               // RX1 relay atten
            break;
            
          case 5:               // RX1 variable atten
            if (EncoderIncrement != 0)
            {
              UpdateRXAtten(EncoderIncrement, true);
              RedrawLastLine();
            }
            if (btn == ClickEncoder::Clicked)           // click = end numerical edit
            {
              lcd.noCursor();
              GEditorActive = false;
              DisplayUpdated = true;                    // redraw display afterwards
            }
            break;
            
          case 6:               // RX2 relay atten
            break;
            
          case 7:               // RX2 variable atten
            if (EncoderIncrement != 0)
            {
              UpdateRXAtten(EncoderIncrement, false);
              RedrawLastLine();
            }
            if (btn == ClickEncoder::Clicked)           // click = end numerical edit
            {
              lcd.noCursor();
              GEditorActive = false;
              DisplayUpdated = true;                    // redraw display afterwards
            }
            break;
            
          case 8:               // TX atten
            if (EncoderIncrement != 0)
            {
              UpdateTXAtten(EncoderIncrement);
              RedrawLastLine();
            }
            if (btn == ClickEncoder::Clicked)           // click = end numerical edit
            {
              lcd.noCursor();
              GEditorActive = false;
              DisplayUpdated = true;                    // redraw display afterwards
            }
            break;
            
          case 9:               // RX open collector bits
            if (btn == ClickEncoder::Clicked)           // click = toggle bit
            {
              BitMask = 1 << (GBitEditCursorStart + GBitEditNumBits - 1 - GEditorCursorPos);
              GRXOCBits ^= BitMask;
              RedrawLastLine();
            }
            DisplayUpdated = BitEditorIncrement(EncoderIncrement);       // handle encoder +/-
            break;
            
          case 10:               // TX open collector bits
            if (btn == ClickEncoder::Clicked)           // click = toggle bit
            {
              BitMask = 1 << (GBitEditCursorStart + GBitEditNumBits - 1 - GEditorCursorPos);
              GTXOCBits ^= BitMask;
              RedrawLastLine();
            }
            DisplayUpdated = BitEditorIncrement(EncoderIncrement);       // handle encoder +/-
            break;
            
          case 11:              // RX filter select bits
            if (btn == ClickEncoder::Clicked)           // click = toggle bit
            {
              BitMask = 1 << (GBitEditCursorStart + GBitEditNumBits - 1 - GEditorCursorPos);
              GRXFilterBits ^= BitMask;
              RedrawLastLine();
            }
            DisplayUpdated = BitEditorIncrement(EncoderIncrement);       // handle encoder +/-
            break;
            
          case 12:              // TX filter select bits
            if (btn == ClickEncoder::Clicked)           // click = toggle bit
            {
              BitMask = 1 << (GBitEditCursorStart + GBitEditNumBits - 1 - GEditorCursorPos);
              GTXFilterBits ^= BitMask;
              RedrawLastLine();
            }
            DisplayUpdated = BitEditorIncrement(EncoderIncrement);       // handle encoder +/-
            break;
            
          case 13:              // fan on/off
            break;
            
          case 14:              // enter TX mode
            break;
            
          case 15:              // enter VSWR trip value
            if (EncoderIncrement != 0)
            {
              UpdateVSWRTrip(EncoderIncrement);
              RedrawLastLine();
            }
            if (btn == ClickEncoder::Clicked)           // click = end numerical edit
            {
              lcd.noCursor();
              GEditorActive = false;
              storeToEeprom(eeeVSWR);                   // write to permanent storage
              DisplayUpdated = true;                    // redraw display afterwards
            }
            break;
            
          case 16:              // enter temp trip value
            if (EncoderIncrement != 0)
            {
              UpdateTempTrip(EncoderIncrement);
              RedrawLastLine();
            }
            if (btn == ClickEncoder::Clicked)           // click = end numerical edit
            {
              lcd.noCursor();
              GEditorActive = false;
              storeToEeprom(eeeTEMP);                   // write to permanent storage
              DisplayUpdated = true;                    // redraw display afterwards
            }
            break;
            
          case 17:              // enter power bar max
            if (EncoderIncrement != 0)
            {
              UpdatePowerBarSize(EncoderIncrement);
              RedrawLastLine();
            }
            if (btn == ClickEncoder::Clicked)           // click = end numerical edit
            {
              lcd.noCursor();
              GEditorActive = false;
              storeToEeprom(eeePWRMAX);                   // write to permanent storage
              DisplayUpdated = true;                    // redraw display afterwards
            }
            break;
            
          case 18:              // enter VSWR bar max
            if (EncoderIncrement != 0)
            {
              UpdateVSWRBarSize(EncoderIncrement);
              RedrawLastLine();
            }
            if (btn == ClickEncoder::Clicked)           // click = end numerical edit
            {
              lcd.noCursor();
              GEditorActive = false;
              storeToEeprom(eeeVSWRMAX);                   // write to permanent storage
              DisplayUpdated = true;                    // redraw display afterwards
            }
            break;
            
          case 19:              // initialse EEPROM (no text needed)
          case 20:              // firmware version
            break;
        }
        
      }
      break;

    case eNormalTX:
      if (HighVSWRCheck())                    // if high VSWR, trip the TX off
      {
        GCurrentMode =  eNormalTXVSWR;
        digitalWrite(PIN_TXEN, LOW);
      }
      if (HighTempCheck())                    // if high temperature, trip the TX off
      {
        GCurrentMode =  eNormalTXTemp;
        digitalWrite(PIN_TXEN, LOW);                // deassert TX output
      }
      break;


    case eDebugTX:                            // we need to chaeck for a "click" to exit TX
      if (btn == ClickEncoder::Clicked)
      {
        GCurrentMode =  eDebugRX;
        digitalWrite(PIN_TXEN, LOW);                // deassert TX output
        GRXLastLine = 0;                            // and reset the bottom line of the display
        InitiateSpiTx();                            // send settings to radio
        
      }
      break;
      

    default:
      break;
  }
//
// finally redraw the display if the settings have changed what's shown
//
  if (DisplayUpdated)
  {
    redrawDisplay();                                // redraw display to show new settings
  }
}

