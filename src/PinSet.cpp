#include "PinSet.hpp"

#define DELTA_UPDATE 50ul

/**
 * Creates an array of DebouncedPins of the given size.
 **/
PinSet::PinSet(uint8_t numberOfPins, uint8_t pinNumberList[]) {
  m_pins = new DebouncedPin[numberOfPins];
  m_numberOfPins = numberOfPins;
  m_lastUpdate = 0;

  m_numberOfPackages = numberOfPins >> 2;
  m_nextPackage = 0;
  m_nextSend = 0;

  for (uint8_t i = 0; i < numberOfPins; i++) {
    m_pins[i].init(pinNumberList[i], 50l);
  }
}

/**
 * Called from the Arduino loop. Updates all
 * DebouncedPins.
 **/
void PinSet::update(unsigned long now) {
  if (now > m_lastUpdate) {
    for (uint8_t i = 0; i < m_numberOfPins; i++) {
      m_pins[i].update();
    }

    m_lastUpdate = now + DELTA_UPDATE;
  }
}

/**
 * Returns a package of four InputPins with
 * the package number in the upper nibble.
 **/
uint8_t PinSet::readPackage(uint8_t packageNumber) {
  uint8_t startPin = packageNumber << 2;
  uint8_t value = 0;

  for (uint8_t pin = 0; pin < 4; pin++) {
    value <<= 1;

    if (m_pins[startPin + pin].read()) {
      value |= 1;
    }
  }

  return value | (packageNumber << 4);
}

/**
 * Sends the next package to Serial1
 **/
void PinSet::send(unsigned long now) {
  if (now > m_nextSend) {
    uint8_t pk = readPackage(m_nextPackage);
    Serial.print("Send: "); Serial.println(pk, HEX);
    Serial1.write(pk);
    m_nextPackage++;
    if (m_nextPackage >= m_numberOfPackages) {
      m_nextPackage = 0;
    }

    m_nextSend = now + 200;
  }
}