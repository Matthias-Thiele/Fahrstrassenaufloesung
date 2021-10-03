#ifndef _PINSET__
#define _PINSET__
#include <Arduino.h>
#include "DebouncedPin.hpp"

class PinSet {
  public:
    PinSet(uint8_t numberOfPins, uint8_t pinNumberList[]);
    void update(unsigned long now);
    void send(unsigned long now);
    uint8_t readPackage(uint8_t packageNumber);

  private:
    DebouncedPin *m_pins;
    uint8_t m_numberOfPins;
    unsigned long m_lastUpdate;

    uint8_t m_numberOfPackages;
    uint8_t m_nextPackage;
    unsigned long m_nextSend;
};

#endif

