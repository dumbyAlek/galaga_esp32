// ================================================================
//  scoring.ino — Score, Lives, EEPROM & System
//
//  registerKill()      : increments score on enemy hit.
//  registerHit()       : decrements lives on player hit,
//                        triggers GAME_OVER when lives = 0.
//
//  eepromLoad()        : reads all persisted settings on boot.
//  eepromSave*()       : individual targeted saves — only write
//                        what changed to minimise EEPROM wear.
//
//  applyBrightness()   : applies current brightnessIdx to both
//                        displays via SSD1306 contrast register.
//
//  doShutdown()        : saves high score, powers off displays,
//                        enters ESP32 deep sleep. PIN_A (GPIO13)
//                        is configured as EXT0 wakeup source.
// ================================================================

// ──────────────────────────────────────────────────────────────────
//  SCORE — KILL REGISTERED  (called from physics.ino)
// ──────────────────────────────────────────────────────────────────
void registerKill() {
  score += SCORE_PER_KILL;
  Serial.print(F("[SCORE] Kill +"));
  Serial.print(SCORE_PER_KILL);
  Serial.print(F("  total="));
  Serial.println(score);
}

// ──────────────────────────────────────────────────────────────────
//  LIVES — PLAYER HIT  (called from physics.ino)
// ──────────────────────────────────────────────────────────────────
void registerHit() {
  if (lives > 0) lives--;

  Serial.print(F("[LIVES] Hit! lives="));
  Serial.println(lives);

  if (lives == 0) {
    if (score > highScore) {
      highScore = score;
      eepromSaveHighScore();
    }
    gameState = STATE_GAME_OVER;
    Serial.println(F("[STATE] PLAYING -> GAME_OVER"));
  }
}

// ──────────────────────────────────────────────────────────────────
//  BRIGHTNESS
// ──────────────────────────────────────────────────────────────────
void applyBrightness() {
  uint8_t v = BRIGHTNESS_VALUES[brightnessIdx];
  dispTop.ssd1306_command(SSD1306_SETCONTRAST);
  dispTop.ssd1306_command(v);
  dispBot.ssd1306_command(SSD1306_SETCONTRAST);
  dispBot.ssd1306_command(v);
}

// ──────────────────────────────────────────────────────────────────
//  SHUTDOWN
// ──────────────────────────────────────────────────────────────────
void doShutdown() {
  // Save high score if beaten mid-game
  if (score > highScore) {
    highScore = score;
    eepromSaveHighScore();
  }

  dispTop.ssd1306_command(SSD1306_DISPLAYOFF);
  dispBot.ssd1306_command(SSD1306_DISPLAYOFF);

  Serial.println(F("[SYS] Shutdown — entering deep sleep"));
  Serial.println(F("[SYS] Press power button to wake"));
  Serial.flush();

  // PIN_A (GPIO13) wakes the device from deep sleep
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_A, 0);
  esp_deep_sleep_start();
}

// ──────────────────────────────────────────────────────────────────
//  EEPROM — LOAD ALL SETTINGS  (called from setup())
// ──────────────────────────────────────────────────────────────────
void eepromLoad() {
  if (EEPROM.read(EE_MAGIC_ADDR) != EE_MAGIC_VAL) {
    // First ever boot — write defaults
    highScore     = 0;
    brightnessIdx = 2;
    btnSwapped    = false;
    calOffsetX    = 0.0f;

    EEPROM.write(EE_MAGIC_ADDR,    EE_MAGIC_VAL);
    EEPROM.put(EE_HISCORE_ADDR,    highScore);
    EEPROM.write(EE_BRIGHTNESS_ADDR, brightnessIdx);
    EEPROM.write(EE_BTNSWAP_ADDR,  (uint8_t)0);
    EEPROM.put(EE_CALOFFSET_ADDR,  calOffsetX);
    EEPROM.commit();

    Serial.println(F("[EEPROM] First boot — defaults written"));
    return;
  }

  // Normal boot — read saved values
  EEPROM.get(EE_HISCORE_ADDR, highScore);
  brightnessIdx = EEPROM.read(EE_BRIGHTNESS_ADDR);
  btnSwapped    = (bool)EEPROM.read(EE_BTNSWAP_ADDR);

  // Sanity check brightness index
  if (brightnessIdx >= BRIGHTNESS_LEVELS) brightnessIdx = 2;

  // Load calibration offset if calibration has been done
  if (EEPROM.read(EE_CALMAG_ADDR) == EE_CALMAG_VAL) {
    EEPROM.get(EE_CALOFFSET_ADDR, calOffsetX);
  }

  Serial.print(F("[EEPROM] hi="));      Serial.print(highScore);
  Serial.print(F("  bright="));         Serial.print(brightnessIdx);
  Serial.print(F("  swap="));           Serial.print(btnSwapped);
  Serial.print(F("  calOffset="));      Serial.println(calOffsetX);
}

// ──────────────────────────────────────────────────────────────────
//  EEPROM — INDIVIDUAL SAVES  (targeted writes to minimise wear)
// ──────────────────────────────────────────────────────────────────
void eepromSaveHighScore() {
  EEPROM.put(EE_HISCORE_ADDR, highScore);
  EEPROM.commit();
  Serial.print(F("[EEPROM] High score saved: "));
  Serial.println(highScore);
}

void eepromSaveBrightness() {
  EEPROM.write(EE_BRIGHTNESS_ADDR, brightnessIdx);
  EEPROM.commit();
}

void eepromSaveBtnSwap() {
  EEPROM.write(EE_BTNSWAP_ADDR, (uint8_t)btnSwapped);
  EEPROM.commit();
}

void eepromSaveCalOffset() {
  EEPROM.put(EE_CALOFFSET_ADDR, calOffsetX);
  EEPROM.write(EE_CALMAG_ADDR, EE_CALMAG_VAL);
  EEPROM.commit();
}
