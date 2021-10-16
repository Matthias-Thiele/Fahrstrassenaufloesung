#include <Arduino.h>
#include "DebouncedPin.hpp"

#define SHIFT_OUT_BIT 0x200
#define ONE_THRESHOLD 5
#define ZERO_THRESHOLD 3
#define SHIFT_COUNT 10

DebouncedPin::DebouncedPin() {
  m_pin = 0xff;
}

/**
 * Initializes the input pin and start state.
 **/
void DebouncedPin::init(uint8_t pin, unsigned long deltaReadMillis){
  m_pin = pin;
  m_deltaReadMillis = deltaReadMillis;
  m_nextRead = 0;

  pinMode(pin, INPUT_PULLUP);
  
  m_history = 0;
  m_count = 0;
  m_lastState = false;

  for (uint8_t i = 0; i < SHIFT_COUNT; i++) {
    update();
  }
}

/**
 * Reads the digital input pin and updates
 * the debouncer logic.
 **/
void DebouncedPin::update() {
  uint8_t actValue = digitalRead(m_pin);
  if (m_history & SHIFT_OUT_BIT) {
    m_count--;
  }

  if (actValue) {
    m_count++;
  }

  m_history = (m_history << 1) | (actValue & 1);
  //Serial.print("DPin updat history: "); Serial.print(m_history, HEX); Serial.print(", count: "); Serial.print(m_count); Serial.print(", actValue: "); Serial.print(actValue); Serial.print(", last: "), Serial.print(m_lastState);
  if (m_lastState) {
    if (m_count < ZERO_THRESHOLD) {
      m_lastState = false;
    }
  } else {
    if (m_count > ONE_THRESHOLD) {
      m_lastState = true;
    }
  }
  //Serial.print(", last processed: "); Serial.println(m_lastState);
}

/**
 * Returns the actual state of the
 * debounced input pin.
 **/
bool DebouncedPin::read() {
  return m_lastState;
}

/**
 * Returns one time true on positive
 * edge of input pin.
 **/
bool DebouncedPin::triggered() {
  if (m_lastState) {
    if (m_isCleared) {
      m_isCleared = false;
      return true;
    }
  } else {
    m_isCleared = true;
  }

  return false;
}

/**
 * Weiterleitung aus der Arduino loop Funktion, wird
 * für die asynchronen Vorgänge benötigt und muss
 * regelmäßig aufgerufen werden.
 **/
bool DebouncedPin::tick(unsigned long now) {
  if (now > m_nextRead) {
    update();
    m_nextRead = now + m_deltaReadMillis;
    return true;
  } else {
    return false;
  }
}