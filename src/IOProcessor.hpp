#ifndef _IOPROCESSOR__
#define _IOPROCESSOR__
#include <Arduino.h>

class IOProcessor {
  private:
    unsigned long const TRIGGER_DURATION = 2;
    unsigned long const WAIT_FOR_SIGNAL = 15000;
    unsigned long const HOLD_SIGNAL = 15000;
    unsigned long const WAIT_FOR_RESET = 45000;
    unsigned long const WAIT_FOR_RESET_MWT = 15000;
    unsigned long const AC_DURATION = 4000;
    unsigned long const TRIGGER_WAIT = 1000;

    uint8_t _triggerInput;
    uint8_t _acRelais1;
    uint8_t _acRelais2;
    uint8_t _activateRelais;
    uint8_t _signal;
    uint8_t _scaler;

    uint8_t _mwt;
    bool waitForMwt = false;

    bool triggered = false;
    unsigned long isOneSince = 0;

    unsigned long setSignalAt = 0;
    unsigned long resetSignalAt = 0;
    unsigned long startAcAt = 0;
    unsigned long endAcAt = 0;
    unsigned long resetTriggerAt = 0;

  public:
    IOProcessor(uint8_t triggerInput, uint8_t acRelais1, uint8_t acRelais2, uint8_t activateRelais, uint8_t signal, uint8_t scaler, uint8_t mwt);
    void checkMwt(unsigned long now);
    bool checkInput(unsigned long now);
    void trigger(unsigned long now);
    void processPorts(unsigned long now);
    void tick(unsigned long now);
};

#endif
