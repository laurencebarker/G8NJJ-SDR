This is Laurence Barker G8NJJ's Red Pitaya based radio repository

This project is to create an HPSDR compatible transceiver. The goal is a complete, high performance transceiver with a "knobs and switches" type user interface, not a PC keyboard & mouse. I have some of my own RF modules, and a simple Arduino based user interface to display settings and permit debugging.

This repository contains:
1. A "Red Pitaya shield" PCB that mounts onto the Red Pitaya and hosts an Arduino Micro
2. An RF switch PCB providing antenna switches, 10/20dB attenuators, VSWR bridge and forward power sensor
3. A low pass filter PCB for TX filtering
4. documentation, including a description of the Arduino user interface
More to follow!

The Red Pitaya code is by Pavel Demin: see http://pavel-demin.github.io/red-pitaya-notes/

The transceiver uses John Melton's pihpsdr program on a raspberry Pi computer with touchscreen, and attached console. pihpsdr is from https://github.com/g0orx/pihpsdr

You can read about my project as it unfolds (slowly!) at http://www.nicklebyhouse.co.uk/index.php/software-defined-radio
 