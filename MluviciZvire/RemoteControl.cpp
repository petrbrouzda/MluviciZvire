#include "RemoteControl.h"

RemoteControl::RemoteControl(LoggerInterface *logger)
{
    this->logger = logger;

    for( int i = 0; i<NUM_PINS; i++ ) {
        pinMode( this->pins[i], INPUT );
    }
}

RemoteControlEvent RemoteControl::checkState()
{
    for( int i = 0; i<NUM_PINS; i++ ) {
        int s = digitalRead( this->pins[i] );
        int prev = this->prevPinState[i];
        this->prevPinState[i] = s;
        if( s==HIGH && prev==LOW ) {
            logger->log( "Tlacitko: %c", this->pinNames[i] );
            return this->pinEvents[i];
        }
    }
    return RC_NO_EVENT;
}
