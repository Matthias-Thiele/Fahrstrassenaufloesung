#include <Arduino.h>
#include "DebouncedPin.hpp"

#define LOOP_COUNT 4
#define INPUT_OFFSET 20
#define INPUT_COUNT 4
#define OUTPUT_OFFSET 28
#define OUTPUT_COUNT 8

#define KONTAKT_LB1 3
#define KONTAKT_LB2 4
#define KONTAKT_AH1 1
#define KONTAKT_AH2 2
#define KONTAKT_G3  5
#define MWT_LB 7
#define MWT_AH 6

unsigned long nextTick = 0;
unsigned long watchdocGBCommand = 0;
unsigned long delayedSetLB = UINT32_MAX;
unsigned long delayedSetAH = UINT32_MAX;
unsigned long delayedResetLB = UINT32_MAX;
unsigned long delayedResetAH = UINT32_MAX;

DebouncedPin pinLBin;
DebouncedPin pinLBout;
DebouncedPin pinAHin;
DebouncedPin pinAHout;
unsigned long updatePinsMillis = 0;

// Speichert die Daten und den Zustand für eine Fahrstraßenauflösung
struct loopState {
  int  loopNumber;
  int* loopData;
  int  actStep;
  int  lastStep;
  int  delayCounter;
};

// Auflösung 1: Trigger auf Input 4 - 30 Sekunden später zieht das Relais 3/4 für 6 Sekunden an.
int loop1[] = {'I', 4, 'V', 300, 'H', 3, 'H', 4, 'D', 60, 'L', 4, 'L', 3, 'N', 4, 'G', 0};

// Auflösung 2: Trigger auf Input 3 - 30 Sekunden später zieht das Relais 1/2 für 6 Sekunden an.
int loop2[] = {'I', 3, 'V', 300, 'H', 1, 'H', 2, 'D', 60, 'L', 2, 'L', 1, 'N', 3, 'G', 0};

// Auflösung 3: Trigger auf Input 2 - 14 Sekunden später zieht das Relais 1/2 für 6 Sekunden an. Nach einer weiteren Pause von 3 Sekunden
// zieht das Relais 5 für 6 Sekunden an. 41 Sekunden später ziehen die Relais 6 für 6 Sekunden an.
int loop3[] = {'I', 2, 'D', 140, 'H', 1, 'H', 2, 'D', 60, 'L', 2, 'L', 1, 'D', 30, 'H', 5, 'D', 60, 'L', 5, 'V', 410, 'H', 6, 'D', 60, 'L', 6, 'N', 2, 'G', 0}; 

// Auflösung 4: Trigger auf Input 1 - 14 Sekunden später zieht das Relais 3/4 für 6 Sekunden an. Nach einer weiteren Pause von 3 Sekunden
// zieht das Relais 5 für 6 Sekunden an. 41 Sekunden später ziehen die Relais 7 für 6 Sekunden an.
int loop4[] = {'I', 1, 'D', 140, 'H', 3, 'H', 4, 'D', 60, 'L', 4, 'L', 3, 'D', 30, 'H', 5, 'D', 60, 'L', 5, 'V', 410, 'H', 7, 'D', 60, 'L', 7, 'N', 1, 'G', 0};

loopState ls1, ls2, ls3, ls4;
loopState* allLoops[] = {&ls1, &ls2, &ls3, &ls4};

// Speicher zur Entprellung der vier Eingänge
bool lastInput[] = {false, false, false, false};
int  lastReset[] = {0, 0, 0, 0};

void stateSetup() {
  // Loop Speicher initialisieren
  // Loop Speicher initialisieren
  ls1.loopData = loop1;
  ls1.loopNumber = 1;
  ls1.actStep = 0;
  ls1.lastStep = 999;
  ls1.delayCounter = 0;

  ls2.loopData = loop2;
  ls2.loopNumber = 2;
  ls2.actStep = 0;
  ls2.lastStep = 999;
  ls2.delayCounter = 0;

  ls3.loopData = loop3;
  ls3.loopNumber = 3;
  ls3.actStep = 0;
  ls3.lastStep = 999;
  ls3.delayCounter = 0;

  ls4.loopData = loop4;
  ls4.loopNumber = 4;
  ls4.actStep = 0;
  ls4.lastStep = 999;
  ls4.delayCounter = 0;

  // Fast Mode Input-Pin einstellen
  pinMode(A0, INPUT);

  // Eingänge für die vier Trigger
  pinLBin.init(INPUT_OFFSET + 2, 50);
  pinLBout.init(INPUT_OFFSET + 8, 50);
  pinAHin.init(INPUT_OFFSET + 4, 50);
  pinAHout.init(INPUT_OFFSET + 6, 50);

  // Ausgänge für die 8 Relais
  for (int i = 1; i <= OUTPUT_COUNT; i++) {
    pinMode(OUTPUT_OFFSET + i + i, OUTPUT);
    digitalWrite(OUTPUT_OFFSET + i + i, HIGH);
  }
}

// Schaltet das Relais mit der Port-Nummer 1 bis 8 ein (state == true) oder aus (state == false).
void setRelaisOutput(int port, bool state) {
  Serial.print("Relais "); Serial.print(OUTPUT_OFFSET + port + port); Serial.print(" -> "); Serial.println(state);
  digitalWrite(OUTPUT_OFFSET + port + port, state ? LOW : HIGH);
}

// Liest den Eingangs-Pin für die Fast Mode Einstellung.
// Wenn dieser auf HIGH steht, werden die variablen Zeiten (V)
// auf ein Viertel gekürzt. Die festen Steuerzeiten (D)
// bleiben unverändert.
bool isFastMode() {
  //return digitalRead(A0);
  return true;
}

// Prüft nach, ob ein Trigger-Input aktiviert ist. Hierzu
// muss der Eingang für mindestens zwei Ticks auf HIGH stehen.
bool checkInput(int port) {
  switch (port)   {
    case 1:
      return pinLBin.read();
    
    case 2:
      return pinLBout.read();

    case 3:
      return pinAHin.read();

    case 4:
      return pinAHout.read();

    default:
      return false;
  }

  bool actInput = digitalRead(INPUT_OFFSET + port + port);
  if (actInput) {
    if (lastInput[port - 1]) {
      Serial.print("checkInput set "); Serial.println(INPUT_OFFSET + port + port);
      return true;
    }
  }

  lastInput[port - 1] = actInput;
  return false;
}

// Prüft nach, ob ein Trigger-Input deaktiviert ist. Hierzu
// muss der Eingang für mindestens 100 Ticks auf LOW stehen.
bool checkReset(int port) {
  Serial.print(" LR:"); Serial.println(lastReset[port - 1]);
  bool actInput = digitalRead(INPUT_OFFSET + port + port);
  if (!actInput) {
    if (lastReset[port - 1] > 100) {
      lastReset[port - 1] = 0;
      return true;
    } else {
      lastReset[port - 1]++;
      return false;
    }
  }

  lastReset[port - 1] = 0;
  return false;
}

// Arbeitet den nächsten Schritt dieser Steuerkette ab.
void processLoop(loopState* actLoop) {
  char cmd = actLoop->loopData[actLoop->actStep];
  int param = actLoop->loopData[actLoop->actStep + 1];
  
  if (actLoop->actStep != actLoop->lastStep) {
    Serial.print("Loop: ");
    Serial.print(actLoop->loopNumber);
    Serial.print(", Command: ");
    Serial.print(cmd);
    Serial.print(", param: ");
    Serial.println(param);
    actLoop->lastStep = actLoop->actStep;
  }
  switch (cmd) {
    // Feste oder variable Wartezeit
    case 'D':
    case 'V':
      if (actLoop->delayCounter > 0) {
        // Wartezeit ist noch aktiv
        actLoop->delayCounter--;
        if (actLoop->delayCounter == 0) {
          // Mit Ablauf der Wartezeit zum nächsten Befehl springen
          actLoop->actStep += 2;
        }
        
        return;
      }

      // Wartezeit starten. Bei Fast Mode werden die variablen Wartezeiten
      // ein ein Viertel reduziert.
      actLoop->delayCounter = (isFastMode() && (cmd == 'V')) ? (param >> 2) : param; 
      return;

    // Goto
    case 'G':
      // Springe an die angegebene Stelle in der Steuerkette
      actLoop->actStep = param;
      return;

    // Output setzen oder rücksetzen
    case 'H':
    case 'L':
      setRelaisOutput(param, cmd == 'H');
      actLoop->actStep += 2;
      return;

    // Auf Eingabe warten
    case 'I':
      if (checkInput(param)) {
        actLoop->actStep += 2;  
      }

      return;

    // auf Rücknahme der Eingabe warten
    case 'N':
      if (checkReset(param)) {
        actLoop->actStep += 2;
      }

      return;
      
    // unbekanntes Kommando - Steuerkette auf 0 zurücksetzen
    default:
      actLoop->actStep = 0;
  }
}

/**
 * Aktuellen Zustand der Signalgruppenschalter einlesen
 **/
uint8_t readSGS() {
  uint8_t result = 0;
  for (uint8_t i = 2; i <= 8; i+=2) {
    Serial.print(" - "); Serial.print(digitalRead(INPUT_OFFSET + i)); Serial.print(" ");
  }
  
  if (pinLBin.read() || pinLBout.read()) {
    result |= 1;
  }
  
  if (pinAHin.read() || pinAHout.read()) {
    result |= 2;
  }


  Serial.print(pinLBin.read()); Serial.print(", ");
  Serial.print(pinLBout.read()); Serial.print(", ");
  Serial.print(pinAHin.read()); Serial.print(", ");
  Serial.print(pinAHout.read()); Serial.print(", ");
  Serial.println(result);
  return result;
}

void processGmMwt(uint8_t cmd) {
  bool onOff = (cmd & 8) == 0;
  Serial.print("Gleiskontakt/ MWT: "); Serial.println(cmd, HEX);

  switch (cmd & 7) {
    case 0: // Gleiskontakt Gleis 3
      setRelaisOutput(KONTAKT_G3, onOff);
      break;

    case 1: // Gleiskontakt Liebenzell
      setRelaisOutput(KONTAKT_LB1, onOff);
      setRelaisOutput(KONTAKT_LB2, onOff);
      break;
      
    case 2: // Gleiskontakt Althengstett
      setRelaisOutput(KONTAKT_AH1, onOff);
      setRelaisOutput(KONTAKT_AH2, onOff);
      break;
      
    case 4: // Mitwirktaste Liebenzell
      if (onOff) {
        delayedSetLB = millis() + 5000;
        delayedResetLB = millis() + 7000;
      }
      break;
      
    case 5: // Mitwirktaste Althengstett
      if (onOff) {
        delayedSetAH = millis() + 5000;
        delayedResetAH = millis() + 7000;
      }
      break;
  }
}
/**
 * Seriellen Schnittstelle zum Gleisbild verwalten. Wenn das
 * Gleisbild keine Daten sendet, dann automatisch auf die
 * zeitgesteuerte Betriebsart umschalten.
 **/
void processCommand(unsigned long now) {
  static unsigned long nextSend = 0ul;

  if (now > nextSend) {
    // Signalgruppenschalter Zustand senden
    Serial1.write(readSGS());
    nextSend = now + 500ul;
    static uint8_t lineBreak = 0;
    //Serial.print(", SGS "); Serial.print(readSGS());
    if (lineBreak++ > 20) {
      Serial.println();
      lineBreak = 0;
    }
  }

  if (Serial1.available()) {
    uint8_t cmd = Serial1.read();

    switch (cmd & 0xf0) {
      case 0xd0: // Watchdog reset vom Gleisbild
        watchdocGBCommand = now + 60000ul;
        //Serial.println("Gleisbild Watchdog command received.");
        break;

      case 0xf0: // Gleismelder und Mitwirktasten
        processGmMwt(cmd);
        break;
    }

  }
}

/**
 * Zeitgesteuerte asynchrone Aktionen prüfuen und ausführen
 **/
void processAsync(unsigned long now) {
  if (delayedSetLB < now) {
    // Mitwirktaste LB setzen
    setRelaisOutput(MWT_LB, true);
    delayedSetLB = UINT32_MAX;
  }

  if (delayedResetLB < now) {
    // Mitwirktaste LB lösen
    setRelaisOutput(MWT_LB, false);
    delayedResetLB = UINT32_MAX;
  }

  if (delayedSetAH < now) {
    // Mitwirktaste AH setzen
    setRelaisOutput(MWT_AH, true);
    delayedSetAH = UINT32_MAX;
  }

  if (delayedResetAH < now) {
    // Mitwirktaste AH lösen
    setRelaisOutput(MWT_AH, false);
    delayedResetAH = UINT32_MAX;
  }
}

// Löse den nächsten Schritt in allen Steuerketten aus.
void tick100ms(unsigned long now) {
  processCommand(now);

  if (now > updatePinsMillis) {
    pinLBin.update();
    pinLBout.update();
    pinAHin.update();
    pinAHout.update();
    processAsync(now);
    updatePinsMillis = now + 50;
  }

  if (watchdocGBCommand > now) {
    // Wärtersteuerung über Gleisbild
  } else {
    // Zeitgesteuerte Wärtersteuerung
    if (now > nextTick) {
      for (int i = 0; i < LOOP_COUNT; i++) {
        loopState* actLoop = allLoops[i];
        processLoop(actLoop);
      }

      nextTick = now + 100;
    }
  }
}