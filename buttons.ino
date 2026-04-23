// ================================================================
//  buttons.ino — Button Input Management
//  GPIO13 = SHOOT (ISR)  — fire / confirm selections / wake sleep
//  GPIO12 = NAV   (polled) — cycle cursor / open pause
// ================================================================

void IRAM_ATTR ISR_Shoot() {
  static uint32_t last = 0;
  uint32_t now = millis();
  if ((now - last) < DEBOUNCE_SHOOT_MS) return;
  last = now;
  flagA = true;   // flagA = shoot flag
}

struct PollState {
  bool     lastHigh  = true;
  uint32_t pressTime = 0;
  bool     confirmed = false;
} pinNavState;

void pollNav() {
  uint32_t now    = millis();
  bool     isHigh = (digitalRead(PIN_NAV) == HIGH);
  if (pinNavState.lastHigh && !isHigh) {
    pinNavState.pressTime = now;
    pinNavState.confirmed = false;
  }
  if (!isHigh && !pinNavState.confirmed &&
      (now - pinNavState.pressTime) >= DEBOUNCE_NAV_MS) {
    pinNavState.confirmed = true;
    flagB = true;   // flagB = nav flag
  }
  if (!pinNavState.lastHigh && isHigh) pinNavState.confirmed = false;
  pinNavState.lastHigh = isHigh;
}

bool consumeShoot() {
  if (flagA) { flagA = false; return true; }
  return false;
}
bool consumeNav() {
  if (flagB) { flagB = false; return true; }
  return false;
}

void initButtons() {
  pinMode(PIN_SHOOT, INPUT_PULLUP);
  pinMode(PIN_NAV,   INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_SHOOT), ISR_Shoot, FALLING);
}