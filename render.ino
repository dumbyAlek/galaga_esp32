// ================================================================
//  render.ino — Screen Rendering
//
//  All coordinates are in the 64x256 virtual canvas.
//  Top display    covers virtual Y   0-127
//  Bottom display covers virtual Y 128-255
//
//  Game layout:
//    Y   0-10  : score HUD
//    Y  10-70  : enemy grid (4 rows x 5 cols)
//    Y  70-240 : play field (bullets travel here)
//    Y 240-256 : ship + lives HUD
// ================================================================

void taskRender() {
  dispTop.clearDisplay();
  dispBot.clearDisplay();

  switch (gameState) {
    case STATE_HOME:      renderHome();                        break;
    case STATE_SETTINGS:  renderSettings();                    break;
    case STATE_PLAYING:   renderPlaying();                     break;
    case STATE_PAUSED:    renderPlaying(); renderPauseMenu();  break;
    case STATE_GAME_OVER: renderGameOver();                    break;
  }

  dispTop.display();
  dispBot.display();
}

// ──────────────────────────────────────────────────────────────────
//  SPLASH  (full 64x256 canvas)
// ──────────────────────────────────────────────────────────────────
void showSplash() {
  dispTop.clearDisplay();
  dispBot.clearDisplay();

  vPrint( 0,  0, F("GA"),        2);
  vPrint( 1,  1, F("GA"),        2);
  vPrint( 20,  12, F("LA"),        2);
  vPrint( 21,  13, F("LA"),        2);
  vPrint( 40,  24, F("GA"),        2);
  vPrint( 41,  25, F("GA"),        2);
  vPrint( 4,  36, F("ESP32"),  1);
  vPrint( 2,  50, F("CSE 323 Project"),1);
  vDrawFastHLine(0, 64, VIRTUAL_W, SSD1306_WHITE);
  vPrint( 4, 128, F("2x Portrait"),    1);
  vPrint( 4, 142, F("OLED  64x256"),   1);
  vPrint( 4, 156, F("virtual canvas"), 1);

  vPrint(10, 240, F("Loading..."),     1);

  dispTop.display();
  dispBot.display();
  delay(2500);
}

// ──────────────────────────────────────────────────────────────────
//  HOME SCREEN
// ──────────────────────────────────────────────────────────────────
void renderHome() {
  // ── Top 3 high scores ──────────────────────────────────────────
  vPrint(4,  4, F("TOP SCORES"), 1);
  vDrawFastHLine(0, 14, VIRTUAL_W, SSD1306_WHITE);

  const char* medals[3] = {"1.", "2.", "3."};
  for (uint8_t i = 0; i < 3; i++) {
    vPrintStr(4,  18 + i * 14, medals[i], 1);
    vPrintUL(20, 18 + i * 14, topScores[i], 1);
  }
  vDrawFastHLine(0, 62, VIRTUAL_W, SSD1306_WHITE);

  // ── Menu ───────────────────────────────────────────────────────
  const uint8_t menuY = 70;
  const uint8_t lineH = 18;
  vPrint(4, menuY,              homeCursor==HOME_START    ? F(">START GAME") : F(" START GAME"), 1);
  vPrint(4, menuY + lineH,      homeCursor==HOME_SETTINGS ? F(">SETTINGS")   : F(" SETTINGS"),   1);
  vPrint(4, menuY + lineH*2,    homeCursor==HOME_QUIT     ? F(">QUIT")       : F(" QUIT"),       1);

  // ── Blinking GALAGA title at bottom ────────────────────────────
  if ((millis() / 600) % 2 == 0) {
    vPrint(4, 230, F("GALAGA"), 2);
  }
}

// ──────────────────────────────────────────────────────────────────
//  PLAYING SCENE
// ──────────────────────────────────────────────────────────────────
void renderPlaying() {

  // ── Scrolling star field (forward movement illusion) ─────────────
  for (uint8_t i = 0; i < NUM_STARS; i++) {
    stars[i].y += stars[i].speed;
    if (stars[i].y >= VIRTUAL_H) {
      stars[i].y = 0;
      stars[i].x = random(0, VIRTUAL_W);
    }
    vDrawPixel(stars[i].x, stars[i].y, SSD1306_WHITE);
  }
  
  // Score — very top
  vPrintUL(0, 0, score, 1);

  // Enemy grid — top panel, Y 10 to ~62
  for (uint8_t r = 0; r < ENEMY_ROWS; r++) {
    for (uint8_t c = 0; c < ENEMY_COLS; c++) {
      if (!enemies[r][c].alive) continue;
      int16_t ex = (int16_t)enemies[r][c].x;
      int16_t ey = (int16_t)enemies[r][c].y;
      // Body
      vDrawRect(ex-3, ey-2, 7, 5, SSD1306_WHITE);
      // Antennae
      vDrawPixel(ex-4, ey,   SSD1306_WHITE);
      vDrawPixel(ex+4, ey,   SSD1306_WHITE);
      // Legs
      vDrawPixel(ex-2, ey+3, SSD1306_WHITE);
      vDrawPixel(ex+2, ey+3, SSD1306_WHITE);
    }
  }

  // ── Boss ─────────────────────────────────────────────────────────
  if (bossActive) {
    int16_t bx = (int16_t)bossX;
    int16_t by = (int16_t)bossY;
    // Big body
    vDrawRect(bx - 8, by - 5, 17, 11, SSD1306_WHITE);
    vDrawRect(bx - 6, by - 7,  13,  3, SSD1306_WHITE);
    // Eyes
    vDrawPixel(bx - 4, by - 1, SSD1306_WHITE);
    vDrawPixel(bx - 4, by,     SSD1306_WHITE);
    vDrawPixel(bx + 4, by - 1, SSD1306_WHITE);
    vDrawPixel(bx + 4, by,     SSD1306_WHITE);
    // Health bar below boss
    vDrawRect(bx - 8, by + 8, 17, 3, SSD1306_WHITE);
    uint8_t hpW = (uint8_t)(16.0f * bossHealth / 5);
    vFillRect(bx - 8, by + 8, hpW, 3, SSD1306_WHITE);
    // Wave label
    char wBuf[8];
    snprintf(wBuf, sizeof(wBuf), "W%d", waveNumber);
    vPrintStr(0, 0, wBuf, 1);  // replaces score position during boss
  }

  // Player bullets — travel upward through full 256px height
  for (uint8_t i = 0; i < MAX_BULLETS; i++) {
    if (!playerBullets[i].active) continue;
    vDrawFastVLine((int16_t)playerBullets[i].x,
                  (int16_t)playerBullets[i].y,
                  BULLET_H, SSD1306_WHITE);
  }

  // Enemy bullets — travel downward
  for (uint8_t i = 0; i < MAX_ENEMY_BULLETS; i++) {
    if (!enemyBullets[i].active) continue;
    vDrawPixel((int16_t)enemyBullets[i].x, (int16_t)enemyBullets[i].y,   SSD1306_WHITE);
    vDrawPixel((int16_t)enemyBullets[i].x, (int16_t)enemyBullets[i].y+1, SSD1306_WHITE);
  }

  // Ship — bottom panel, vY ~242-249. Fits in 64px width (half-width=3)
  int16_t sx = (int16_t)shipX;
  vFillTriangle(sx,               SHIP_Y - 5,
                sx - SHIP_HALF_W, SHIP_Y,
                sx + SHIP_HALF_W, SHIP_Y,
                SSD1306_WHITE);
  vDrawPixel(sx - 1, SHIP_Y + 1, SSD1306_WHITE);
  vDrawPixel(sx + 1, SHIP_Y + 1, SSD1306_WHITE);

  // Lives — bottom row, right-aligned
  for (uint8_t i = 0; i < lives; i++)
    vFillCircle(VIRTUAL_W - 4 - (i * 7), VIRTUAL_H - 3, 2, SSD1306_WHITE);
}

// ──────────────────────────────────────────────────────────────────
//  PAUSE MENU
//  Centred in the middle of the 64x256 canvas (Y 90-170)
//  Width 60px centred in 64px = X 2 to 62
// ──────────────────────────────────────────────────────────────────
void renderPauseMenu() {
  vFillRect(1, 88, 62, 54, SSD1306_BLACK);
  vDrawRect(1, 88, 62, 54, SSD1306_WHITE);
  vPrint(6,  93, F("-- PAUSED --"), 1);
  vDrawFastHLine(1, 104, 62, SSD1306_WHITE);
  vPrint(4, 112, pauseCursor==PAUSE_RESUME ? F(">RESUME") : F(" RESUME"), 1);
  vPrint(4, 130, pauseCursor==PAUSE_QUIT   ? F(">QUIT")   : F(" QUIT"),   1);
}

// ──────────────────────────────────────────────────────────────────
//  GAME OVER
// ──────────────────────────────────────────────────────────────────
void renderGameOver() {
  vPrint( 4,  20, F("GAME"),        2);
  vPrint( 4,  44, F("OVER"),        2);
  vDrawFastHLine(0, 68, VIRTUAL_W, SSD1306_WHITE);

  vPrint( 4,  78, F("SCORE:"),      1);
  vPrintUL(4,  90, score,           1);
  vDrawFastHLine(0, 104, VIRTUAL_W, SSD1306_WHITE);

  if (score >= highScore && score > 0) {
    vPrint(2, 114, F("** NEW  **"),  1);
    vPrint(2, 126, F("** BEST! **"), 1);
  } else {
    vPrint( 4, 114, F("BEST:"),      1);
    vPrintUL(4, 126, highScore,      1);
  }

  vDrawFastHLine(0, 140, VIRTUAL_W, SSD1306_WHITE);
  vPrint(2, 150, F("PRESS NAV"),    1);
  vPrint(6, 162, F("TO EXIT"),      1);
}

// ──────────────────────────────────────────────────────────────────
//  SETTINGS
// ──────────────────────────────────────────────────────────────────
void renderSettings() {
  vPrint(4,  4, F("SETTINGS"), 1);
  vDrawFastHLine(0, 14, VIRTUAL_W, SSD1306_WHITE);

  char bBuf[14];
  snprintf(bBuf, sizeof(bBuf), "%sBRIGHT:%d/3",
           settingsCursor==SETTINGS_BRIGHTNESS ? ">" : " ", brightnessIdx);
  vPrintStr(4, 24, bBuf, 1);

  char sBuf[14];
  snprintf(sBuf, sizeof(sBuf), "%sBTN:%s",
           settingsCursor==SETTINGS_BTNSWAP ? ">" : " ",
           btnSwapped ? "SWAP" : "NORM");
  vPrintStr(4, 42, sBuf, 1);

  vPrint(4, 60, settingsCursor==SETTINGS_BACK ? F(">BACK") : F(" BACK"), 1);

  vDrawFastHLine(0, 74, VIRTUAL_W, SSD1306_WHITE);
  vPrint(4, 80, F("NAV=cycle"), 1);
  vPrint(4, 92, F("SHOOT=select"), 1);
}
