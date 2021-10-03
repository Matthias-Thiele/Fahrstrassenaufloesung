#ifndef _DEBOUNCEDPIN__
#define _DEBOUNCEDPIN__
#include <Arduino.h>

class DebouncedPin {
  public:
    DebouncedPin();
    void init(uint8_t pin, unsigned long deltaReadMillis);
    bool tick(unsigned long now);
    void update();
    bool read();
    bool triggered();

  private:
    uint8_t m_pin;
    short m_history;
    uint8_t m_count;
    bool m_lastState;
    unsigned long m_deltaReadMillis;
    unsigned long m_nextRead;
    bool m_isCleared;
};

#endif

