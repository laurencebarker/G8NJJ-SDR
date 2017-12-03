//************************************************************************
//
// Title: G8NJJ SDR Control Arduino sketch
// File: displayutils.h
// copyright Laurence Barker G8NJJ 2017
//
//************************************************************************

#include <LiquidCrystal.h>
extern LiquidCrystal lcd;

//
// initialise Display
//
void initDisplay(void);

//
// local version of "sprintf like" function
// Adds a decimal point before last digit if 3rd parameter set
// note integer value is signed and may be negative!
//
unsigned char mysprintf(char *dest, int Value, bool AddDP);

//
// printAtXY
// add a string to the display ay X/Y position
// 
unsigned char printAtXY(char X, char Y, const char* Text);


//
// printIntAtXY
// add an integer to the display ay X/Y position
// Adds a decimal point before last digit if 3rd parameter set
// note integer value is signed and may be negative!
// 
unsigned char printIntAtXY(char X, char Y, int Value, bool AddDP=false);

//
// printBitsAtXY
// add an integer displayed in binary to the display ay X/Y position
// 
unsigned char printBitsAtXY(char X, char Y, int Value, int NumBits);

//
// draw the actual bar onto the LCD display. 
// parametrer is the fractional length of the bar (0 to 1.0)
//
void DrawBarOnLCD (byte Row, float BarLength);

//
// create special characters in the LCD Character generator
//
void CreateLCDSpecialChars();



