#ifndef ___REMOTE_CONTROL_H_
#define ___REMOTE_CONTROL_H_


#include "src/logging/LoggerInterface.h"

#include "pinout.h"

#define NUM_PINS 4


enum RemoteControlEvent {
    RC_NO_EVENT,
    RC_TLACITKO_A,
    RC_TLACITKO_B,
    RC_TLACITKO_C,
    RC_TLACITKO_D
};

class RemoteControl 
{
    public:
        RemoteControl( LoggerInterface * logger );
        RemoteControlEvent checkState();

    private:
        LoggerInterface * logger;
        int pins[NUM_PINS] = { D0_D, D1_C, D2_B, D3_A };
        char pinNames[NUM_PINS] = { 'D', 'C', 'B', 'A' };
        int prevPinState[NUM_PINS] = { 0, 0, 0, 0 };
        RemoteControlEvent pinEvents[NUM_PINS] = { RC_TLACITKO_D, RC_TLACITKO_C, RC_TLACITKO_B, RC_TLACITKO_A };
};


#endif
