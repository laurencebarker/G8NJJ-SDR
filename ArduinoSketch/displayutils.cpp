//************************************************************************
//
// Title: G8NJJ SDR Control Arduino sketch
// File: displayutils.cpp
// copyright Laurence Barker G8NJJ 2017
//
// code to add test to the LCD display
// (this is some additional code to the "standard" arduino LCD driver)
//************************************************************************
#include <Arduino.h>

#include "displayutils.h"
#include "iodefs.h"

LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);


//
// initialise Display
//
void initDisplay(void)
{
  lcd.begin(LCD_CHARS, LCD_LINES);
  lcd.clear();
  
}


#define VASCII0 0x30                // zero character in ASCII
//
// local version of "sprintf like" function
// Adds a decimal point before last digit if 3rd parameter set
// note integer value is signed and may be negative!
//
unsigned char mysprintf(char *dest, int Value, bool AddDP)
{
  unsigned char Digit;              // calculated digit
  bool HadADigit = false;           // true when found a non zero digit
  unsigned char DigitCount = 0;     // number of returned digits
  unsigned int Divisor = 10000;     // power of 10 being calculated
//
// deal with negative values first
//
  if (Value < 0)
  {
    *dest++ = '-';    // add to output
    DigitCount++;
    Value = -Value;
  }
//
// now convert the (definitely posirive) number
//
  while (Divisor >= 10)
  {
    Digit = Value / Divisor;        // find digit: integer divide
    if (Digit != 0)
      HadADigit = true;             // flag if non zero so all trailing digits added
    if (HadADigit)                  // if 1st non zero or all subsequent
    {
      *dest++ = Digit + VASCII0;    // add to output
      DigitCount++;
    }
    Value -= (Digit * Divisor);     // get remainder from divide
    Divisor = Divisor / 10;         // ready for next digit
  }
//
// if we need a decimal point, add it now. Also if there hasn't been a preceiding digit
// (i.e. number was like 0.3) add the zero
//
  if (AddDP)
  {
    if (HadADigit == false)
    {
      *dest++ = '0';
      DigitCount++;
    }
    *dest++ = '.';
  DigitCount++;
  }
  *dest++ = Value + VASCII0;
  DigitCount++;
//
// finally terminate with a 0
//
  *dest++ = 0;

  return DigitCount;
}


//
// printAtXY
// add a string to the display ay X/Y position
// 
unsigned char printAtXY(char X, char Y, const char* Text)
{
  lcd.setCursor(X, Y);
  lcd.print(Text);
}


//
// printIntAtXY
// add an integer to the display ay X/Y position
// Adds a decimal point before last digit if 3rd parameter set
// note integer value is signed and may be negative!
// 
unsigned char printIntAtXY(char X, char Y, int Value, bool AddDP)
{
  char LocalStr[20];
  mysprintf(LocalStr, Value, AddDP);
  printAtXY(X, Y, LocalStr);
}

//
// 2**(number of bits - 1) lookup
// index 0 is meaningless!
//
int TopBitValues[13]=
{
  1,1,2,4,8,16,32,64,128,256,512,1024,2048
};

//
// printBitsAtXY
// add an integer displayed in binary to the display ay X/Y position
// 
unsigned char printBitsAtXY(char X, char Y, int Value, int NumBits)
{
  char LocalStr[20];
  char Bit;                         // character for this bit
  int Cntr = 0;                     // bit counter
  int BitValue;                     // binary value of each bit

  BitValue = TopBitValues[NumBits];                 // value of LSB
  for (Cntr = 0; Cntr < NumBits; Cntr++)            // loop through each bit
  {
    if(Value & BitValue)
      Bit = '1';
    else
      Bit = '0';
    LocalStr[Cntr] = Bit;                           // save the ascii char to the string
    BitValue /= 2;                                  // set for next bit
  }
  LocalStr[NumBits]=0;                              // terminate the string
  printAtXY(X, Y, LocalStr);
}



//
// bar display printing
// Need to write all the characters on the bar to overwite any old part of a bar
//
#define VDISPLAYBARSEGMENTS 15


//
// special characters for LCD display. 
// address 0: no segments
// address 1: segment 0 vertically lit
// address 2: segment 0,1 vertically lit
// address 3: segment 0,1,2 vertically lit
// address 4: segment 0,1,2,3 vertically lit
// address 5: segment 0,1,2,3,4 vertically lit

byte SpecialLCDChar0 [8] = 
{
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000
};

byte SpecialLCDChar1 [8] = 
{
  0b00000000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00000000,
  0b00000000
};

byte SpecialLCDChar2 [8] = 
{
  0b00000000,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00000000,
  0b00000000
};

byte SpecialLCDChar3 [8] = 
{
  0b00000000,
  0b00011100,
  0b00011100,
  0b00011100,
  0b00011100,
  0b00011100,
  0b00000000,
  0b00000000
};

byte SpecialLCDChar4 [8] = 
{
  0b00000000,
  0b00011110,
  0b00011110,
  0b00011110,
  0b00011110,
  0b00011110,
  0b00000000,
  0b00000000
};

byte SpecialLCDChar5 [8] = 
{
  0b00000000,
  0b00011111,
  0b00011111,
  0b00011111,
  0b00011111,
  0b00011111,
  0b00000000,
  0b00000000
};



//
// create special characters in the LCD Character generator
//
void CreateLCDSpecialChars()
{
  lcd.createChar(0, SpecialLCDChar0);
  lcd.createChar(1, SpecialLCDChar1);
  lcd.createChar(2, SpecialLCDChar2);
  lcd.createChar(3, SpecialLCDChar3);
  lcd.createChar(4, SpecialLCDChar4);
  lcd.createChar(5, SpecialLCDChar5);
}


//
// draw the actual bar onto the LCD display. 
// parametrer is the fractional length of the bar (0 to 1.0)
//
void DrawBarOnLCD (byte Row, float BarLength)
{
  float BarChars;
  byte BarCompleteChars;
  byte BarLastFraction;
  byte DisplayPosition = 0;
  byte Cntr;

//
// start by finding bar length in characters
//
  BarChars = BarLength * (float)VDISPLAYBARSEGMENTS;        // float characters
  BarCompleteChars = (byte)BarChars;                        // int number of chars, rounding down
  BarChars = BarChars - (float)BarCompleteChars;            // fractional characters
  BarLastFraction = (byte)(BarChars*5.0);                   // number if segments left to display, 0 to 5

//
// now draw
//
  lcd.setCursor(0, Row-1);                                  // cursor to start of row
  if (BarCompleteChars != 0)                                // write complete char positions with solid block
    for (Cntr=0; Cntr < BarCompleteChars; Cntr++)
    {
      lcd.write(5);                                         // use special character 5
      DisplayPosition++;                                    // display position done up to
    }
    lcd.write(BarLastFraction);                             // add last character
    DisplayPosition++;
//
// finally erase anything left
//
  if (DisplayPosition < VDISPLAYBARSEGMENTS)               // now do last character
  {
    BarCompleteChars = VDISPLAYBARSEGMENTS - DisplayPosition;   // char positions to clear
    for (Cntr=0; Cntr < BarCompleteChars; Cntr++)
      lcd.write((byte)0);                                             // use special character 0   
  }
}

