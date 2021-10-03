#include "IOProcessor.hpp"

#define FIXED_SCALE 64

    IOProcessor::IOProcessor(uint8_t triggerInput, uint8_t acRelais1, uint8_t acRelais2, uint8_t activateRelais, uint8_t signal, uint8_t scaler, uint8_t mwt) {
      _triggerInput = triggerInput;
      _acRelais1 = acRelais1;
      _acRelais2 = acRelais2;
      _activateRelais = activateRelais;
      _signal = signal;
      _scaler = scaler;
      _mwt = mwt;

      pinMode(triggerInput, INPUT);
      pinMode(acRelais1, OUTPUT);
      digitalWrite(acRelais1, 1);
      pinMode(acRelais2, OUTPUT);
      digitalWrite(acRelais2, 1);
      pinMode(activateRelais, OUTPUT);
      digitalWrite(activateRelais, 1);
      if (signal) {
        pinMode(signal, OUTPUT);
        digitalWrite(signal, 1);
      }

      if (mwt) {
        pinMode(mwt, INPUT);
      }
    }

    // rechnet die Wartezeiten anhand des einstellten 
    // Potentiometerwerts um.
    unsigned long scale(int factor, unsigned long value) {
      unsigned long result = (factor * value) >> 8;
      return result;
    }

    // Wenn auf die Mitwirktaste gewartet werden soll, 
    // prüft diese Funktion, ob die Taste betätigt wurde.
    // Wenn ja, wird zeitverzögert das Rückblocken aktiviert.
    void IOProcessor::checkMwt(unsigned long now) {
      if (digitalRead(_mwt) == 0) {
        //Serial.println("MWT*");
      }
      if (!waitForMwt) {
        return;
      }

      if (digitalRead(_mwt) == 0) {
        Serial.print("MWT pressed: "); Serial.println(_mwt);
        int scaleFactor = FIXED_SCALE; // analogRead(_scaler);
        startAcAt = now + scale(scaleFactor, WAIT_FOR_RESET_MWT);
        endAcAt = startAcAt + AC_DURATION;
        waitForMwt = false;
      }
    }

    // Prüft, ob der Wechselstrominput ein Signal für mehr
    // als die TRIGGER_DURATION Zeit anzeigt. Eine neue
    // Sequenz kann nur gestartet werden, wenn nicht bereits
    // eine andere Sequenz aktiv ist.
    bool IOProcessor::checkInput(unsigned long now) {
      if (triggered) {
        // es ist bereits eine Sequenz aktiv
        return false;
      }

      if (digitalRead(_triggerInput)) {
        // Eingang nicht aktiv
        if (isOneSince > 0) {
          Serial.println("Drop seq.");
        }

        isOneSince = 0;
        return false;
      }

      if (isOneSince == 0) {
        // Eingang jetzt aktiv, war vorher nicht aktiv
        Serial.println("Start seq.");
        isOneSince = now;
      }

      if ((now - isOneSince) < TRIGGER_DURATION) {
        // Eingang noch nicht lang genug aktiv, weiter warten
        return false;
      }

      // Neue Sequenz starten
      return true;
    }

    // Eine neue Sequenz wird gestartet. Setzt die Zeiten für
    // den Signalanzeiger (ein und aus) sowie die Rückblockung,
    // wenn keine MWT definiert wurde.
    void IOProcessor::trigger(unsigned long now) {
      Serial.println("Triggered");
      triggered = true;
      int scaleFactor = FIXED_SCALE; // analogRead(_scaler);

      // Aktivierungszeiten für Signalanzeiger setzen
      setSignalAt = now + scale(scaleFactor, WAIT_FOR_SIGNAL);
      resetSignalAt = setSignalAt + scale(scaleFactor, HOLD_SIGNAL);

      if (_mwt == 0) {
        // wenn keine MWT definiert ist, Zeiten für Rückblockung setzen
        startAcAt = now + scale(scaleFactor, WAIT_FOR_RESET);
        endAcAt = startAcAt + AC_DURATION;
      } else {
        // Rückblockung muss auf die MWT warten
        waitForMwt = true;
      }
    }

    // Prüft die eingestellten Aktionszeiten gegen die aktuelle
    // Zeit und führt bei Bedarf die verbundenen Aktionen durch.
    void IOProcessor::processPorts(unsigned long now) {
      if (!triggered) {
        // keine Sequenz aktiv, nichts zu tun
        return;
      }

      if ((setSignalAt != 0) && (now > setSignalAt) && _signal) {
        // Signalanzeiger einschalten
        Serial.println("Set Signal");
        digitalWrite(_signal, 0);
        setSignalAt = 0;
      }

      if ((resetSignalAt != 0) && (now > resetSignalAt) && _signal) {
        // Signalanzeiger abschalten
        Serial.println("Reset Signal");
        digitalWrite(_signal, 1);
        resetSignalAt = 0;
      }

      if ((startAcAt > 0) && (now >= startAcAt) && !waitForMwt) {
        // Rückblockung beginnen. Die Einstellung für die Wechselspannungsrelais
        // wechselt alle 80 Millisekunden.
        digitalWrite(_activateRelais, 0);

        bool state = now & 0x80;
        digitalWrite(_acRelais1, state);
        digitalWrite(_acRelais2, !state);
      }

      if ((endAcAt > 0) && (now >=endAcAt) && !waitForMwt) {
        // Rückblockung abgeschlossen. Damit der letzte Rückblockimpuls
        // nicht sofort wieder den Eingang aktiviert, muss das Trigger
        // Flag noch eine Sekunde lang (TRIGGER_WAIT) gesperrt bleiben.
        Serial.println("Deactivate AC");
        resetTriggerAt = now + TRIGGER_WAIT;
        startAcAt = 0;
        endAcAt = 0;
        digitalWrite(_activateRelais, 1);
        digitalWrite(_acRelais1, 1);
        digitalWrite(_acRelais2, 1);
      }

      if ((resetTriggerAt > 0) && (now > resetTriggerAt)) {
        // Sequenz abgeschlossen, das Trigger Flag wird zurückgesetzt.
        Serial.println("End sequence");
        triggered = 0;
        resetTriggerAt = 0;
      }
    }

    // Statusprüfung und Aktualisierung in der Arduino loop.
    void IOProcessor::tick(unsigned long now) {
      if (checkInput(now)) {
        // neue Sequenz starten
        trigger(now);
      }

      // Mitwirktasten prüfen und bei Bedarf die Rückblockung einleiten.
      checkMwt(now);

      // Aktionszeiten prüfen und Ausgabeports entsprechend setzen.
      processPorts(now);
    }
