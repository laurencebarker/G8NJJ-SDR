//************************************************************************
//
// Title: G8NJJ SDR Control Arduino sketch
// File: iodefs.h
// copyright Laurence Barker G8NJJ 2017
//
//************************************************************************

//
// IO definitions for SDR controller
// targeted to Arduino Leonardo
//

#define LCD_RS       8
#define LCD_EN       9
#define LCD_D4       10
#define LCD_D5       11
#define LCD_D6       12
#define LCD_D7       13

#define LCD_CHARS   20
#define LCD_LINES    4

//
// rotary encoder defines:
//
#define ENC_A 5             // encoder phase A
#define ENC_B 6             // encoder phase B
#define ENC_PB A4           // encoder pushbutton
#define ENC_STEPS 4         // number of events per indent position

//
// analogue I/O defines
//
#define PIN_FWDPOWER A0
#define PIN_REVPOWER A1
#define PIN_PACURRENT A2
#define PIN_HSTEMP A3

//
// individual digital I/O pin defines
//
#define PIN_TXREQ 0             // TX request IN
#define PIN_TXEN 1              // TX enable OUT
#define PIN_SERLOAD 4           // LOAD for serial shift
#define PIN_DEBUGJUMPER A5      // jumper inserted for debug mode

 

