// ================================================================
//  state_machine.ino — Game State Transitions
//  NAV   = cycle cursor
//  SHOOT = confirm / fire
// ================================================================

void taskStateMachine() {
  pollNav();
  bool shot = (gameState != STATE_PLAYING) ? consumeShoot() : false;
  bool nav  = consumeNav();

  switch (gameState) {

    case STATE_HOME:
      if (nav)  homeCursor = (homeCursor + 1) % HOME_ITEMS;
      if (shot) {
        switch (homeCursor) {
          case HOME_START:
            initGame();
            gameState = STATE_PLAYING;
            Serial.println(F("[STATE] HOME -> PLAYING"));
            break;
          case HOME_SETTINGS:
            settingsCursor = 0;
            gameState = STATE_SETTINGS;
            break;
          case HOME_QUIT:
            doShutdown();
            break;
        }
      }
      break;

    case STATE_SETTINGS:
      if (nav)  settingsCursor = (settingsCursor + 1) % SETTINGS_ITEMS;
      if (shot) {
        switch (settingsCursor) {
          case SETTINGS_BRIGHTNESS:
            brightnessIdx = (brightnessIdx + 1) % BRIGHTNESS_LEVELS;
            applyBrightness();
            eepromSaveBrightness();
            break;
          case SETTINGS_BTNSWAP:
            btnSwapped = !btnSwapped;
            eepromSaveBtnSwap();
            break;
          case SETTINGS_BACK:
            gameState = STATE_HOME;
            break;
        }
      }
      break;

    case STATE_PLAYING:
      if (nav) {
        pauseCursor = 0;
        gameState   = STATE_PAUSED;
        Serial.println(F("[STATE] PLAYING -> PAUSED"));
      }
      // SHOOT is consumed directly in physics.ino for firing
      break;

    case STATE_PAUSED:
      if (nav)  pauseCursor = (pauseCursor + 1) % PAUSE_ITEMS;
      if (shot) {
        switch (pauseCursor) {
          case PAUSE_RESUME:
            gameState = STATE_PLAYING;
            break;
          case PAUSE_QUIT:
            if (score > highScore) { highScore = score; eepromSaveHighScore(); }
            gameState = STATE_HOME;
            homeCursor = 0;
            break;
        }
      }
      break;

    case STATE_GAME_OVER:
      if (shot) gameState = STATE_HOME;
      break;
  }
}