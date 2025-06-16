#ifndef __PINOUT_H__
#define __PINOUT_H__

#define LED 8

// MP3 prehravac; RX a TX jsou z pohledu ESP32 (TX = my vysilame, MP3 prijimac prijima)
#define MP3_RX 4
#define MP3_TX 3

// merici delic na akumulatoru a hodnoty rezistoru v delici
#define ACCU 0
// horni odpor delice (pozor, nutno zapsat s desetinnou teckou!)
#define DELIC_R1 40000.0
// dolni odpor delice (pozor, nutno zapsat s desetinnou teckou!)
#define DELIC_R2 12000.0
// kolik to namerilo / kolik to melo byt
#define KALIBRACE (3.30/4.04)

#define LOW_BATTERY_LIMIT 3.5

// radiovy prijimac
#define D0_D 7
#define D1_C 10
#define D2_B 20
#define D3_A 21

#endif
