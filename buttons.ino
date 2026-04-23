// ================================================================
//  buttons.ino — Button Input Management
//
//  PIN_A (GPIO13): Hardware ISR, FALLING edge.
//    Kept as ISR because ESP32 EXT0 deep-sleep wakeup requires
//    a fixed GPIO with an interrupt attached.
//
//  PIN_B (GPIO12): Software polled every 10ms inside
//    taskStateMachine(). Deliberately NOT an ISR — the 400kHz
//    I2C bus toggling during MPU-6050 reads couples capacitively
//    into adjacent breadboard traces and causes phantom FALLING
//    edges. Polling with a 120ms debounce window filters all
//    such glitches since I2C pulses are ~2.5µs wide.
//
//  Button swap: btnSwapped (set in state_machine.ino) flips
//    the logical role of each button without changing wiring.
//    consumePower() / consumeNav() read through the swap lens.
// ================================================================

// ──────────────────────────────────────────────────────────────────
//  PIN_A — ISR
// ──────────────────────────────────────────────────────────────────
void IRAM_ATTR ISR_PinA() {
  static uint32_t last = 0;
  uint32_t now = millis();
  if ((now - last) < DEBOUNCE_A_MS) return;
  last = now;
  flagA = true;
}

// ──────────────────────────────────────────────────────────────────
//  PIN_B — Polled State Machine
// ──────────────────────────────────────────────────────────────────
struct PollState {
  bool     lastHigh  = true;
  uint32_t pressTime = 0;
  bool     confirmed = false;
} pinBState;

void pollPinB() {
  uint32_t now    = millis();
  bool     isHigh = (digitalRead(PIN_B) == HIGH);

  // Falling edge — button just pressed
  if (pinBState.lastHigh && !isHigh) {
    pinBState.pressTime = now;
    pinBState.confirmed = false;
  }

  // Held LOW long enough — confirmed press
  if (!isHigh && !pinBState.confirmed &&
      (now - pinBState.pressTime) >= DEBOUNCE_B_MS) {
    pinBState.confirmed = true;
    flagB = true;
  }

  // Rising edge — released, reset for next press
  if (!pinBState.lastHigh && isHigh) {
    pinBState.confirmed = false;
  }

  pinBState.lastHigh = isHigh;
}

// ──────────────────────────────────────────────────────────────────
//  LOGICAL BUTTON CONSUMERS  (swap-aware)
//  Returns true once then clears the flag.
// ──────────────────────────────────────────────────────────────────
bool consumePower() {
  if (!btnSwapped) {
    if (flagA) { flagA = false; return true; }
  } else {
    if (flagB) { flagB = false; return true; }
  }
  return false;
}

bool consumeNav() {
  if (!btnSwapped) {
    if (flagB) { flagB = false; return true; }
  } else {
    if (flagA) { flagA = false; return true; }
  }
  return false;
}

// ──────────────────────────────────────────────────────────────────
//  INIT — called from setup() in galaga_esp32.ino
// ──────────────────────────────────────────────────────────────────
void initButtons() {
  pinMode(PIN_A, INPUT_PULLUP);
  pinMode(PIN_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_A), ISR_PinA, FALLING);
}
