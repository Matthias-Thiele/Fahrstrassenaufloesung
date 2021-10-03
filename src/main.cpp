#include <Arduino.h>
#include "IOProcessor.hpp"
#include "StateMachine.hpp"
#include "PinSet.hpp"
#include "Wire.h"

IOProcessor *fw1, *fw2, *fw3;

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
  
  //fw1 = new IOProcessor(PIND4, PIND6, PIND7, PIND5, PIN_A1, PIN_A4, 0);
  fw1 = new IOProcessor(27, 31, 33, 29, 37, 43, 0);

  //fw2 = new IOProcessor(PIND3, PIND6, PIND7, PIND5, PIN_A2, PIN_A4, 0);
  fw2 = new IOProcessor(25, 31, 33, 29, 39, 43, 0);

  // Einfahrt
  //fw3 = new IOProcessor(PIND2, PIND6, PIND7, 8, PIN_A3, PIN_A4, PIN_A5);
  fw3 = new IOProcessor(23, 31, 33, 35, 41, 43, 45);

  stateSetup();

}

unsigned long nextOutput = 0;

void loop() {
  unsigned long now = millis();
  //fw1->tick(now);
  //fw2->tick(now);
  //fw3->tick(now);
  tick100ms(now);
}