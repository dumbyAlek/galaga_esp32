// ================================================================
//  state_machine.ino — Game State Transitions
//
//  taskStateMachine() runs every 10ms.
//  It polls PIN_B, consumes button flags, and drives all
//  GameState transitions based on the current state and input.
//
//  State flow:
//    HOME        + NAV press    → PLAYING
//    PLAYING     + NAV press    → PAUSED
//    PAUSED      + NAV press    → cycle pause menu cursor
//    PAUSED      + POWER press  → confirm menu selection:
//                                   Resume    → PLAYING
//                                   Brightness→ cycle level (stay PAUSED)
//                                   SwapBtn   → toggle swap (stay PAUSED)
//                                   Quit      → HOME
//    GAME_OVER   + NAV press    → HOME
//    ANY         + POWER press  → doShutdown() (except in pause menu
//                                 where POWER is the confirm button)
// ================================================================

void taskStateMachine() {
  // Poll PIN_B first so flagB is up-to-date this tick
  pollPinB();

  bool pwrPressed = consumePower();
  bool navPressed = consumeNav();

  switch (gameState) {

    // ── HOME ──────────────────────────────────────────────────────
    case STATE_HOME:
      if (pwrPressed) { doShutdown(); return; }
      if (navPressed) {
        initGame();
        gameState = STATE_PLAYING;
        Serial.println(F("[STATE] HOME -> PLAYING"));
      }
      break;

    // ── PLAYING ──────────────────────────────────────────────────
    case STATE_PLAYING:
      if (pwrPressed) { doShutdown(); return; }
      if (navPressed) {
        pauseCursor = 0;
        gameState   = STATE_PAUSED;
        Serial.println(F("[STATE] PLAYING -> PAUSED"));
      }
      break;

    // ── PAUSED — MENU ────────────────────────────────────────────
    case STATE_PAUSED:
      // NAV = move cursor down through menu items
      if (navPressed) {
        pauseCursor = (pauseCursor + 1) % PAUSE_ITEMS;
      }
      // POWER = confirm selected item
      if (pwrPressed) {
        switch (pauseCursor) {

          case PAUSE_RESUME:
            gameState = STATE_PLAYING;
            Serial.println(F("[STATE] PAUSED -> PLAYING (Resume)"));
            break;

          case PAUSE_BRIGHTNESS:
            brightnessIdx = (brightnessIdx + 1) % BRIGHTNESS_LEVELS;
            applyBrightness();
            eepromSaveBrightness();
            Serial.print(F("[BRIGHT] Level ")); Serial.println(brightnessIdx);
            // Stay in pause menu so player can keep cycling
            break;

          case PAUSE_SWAPBTN:
            btnSwapped = !btnSwapped;
            eepromSaveBtnSwap();
            Serial.print(F("[BTN] Swapped: ")); Serial.println(btnSwapped);
            // Stay in pause menu — label updates immediately
            break;

          case PAUSE_QUIT:
            if (score > highScore) {
              highScore = score;
              eepromSaveHighScore();
            }
            gameState = STATE_HOME;
            Serial.println(F("[STATE] PAUSED -> HOME (Quit)"));
            break;
        }
      }
      break;

    // ── GAME OVER ────────────────────────────────────────────────
    case STATE_GAME_OVER:
      if (pwrPressed) { doShutdown(); return; }
      if (navPressed) {
        gameState = STATE_HOME;
        Serial.println(F("[STATE] GAME_OVER -> HOME"));
      }
      break;
  }
}
