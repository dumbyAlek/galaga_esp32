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
  // Top panel — title
  vPrint( 0,  0, F("GA"),        2);
  vPrint( 1,  1, F("GA"),        2);
  vPrint( 20,  12, F("LA"),        2);
  vPrint( 21,  13, F("LA"),        2);
  vPrint( 40,  24, F("GA"),        2);
  vPrint( 41,  25, F("GA"),        2);
  vPrint( 0, 34, F("ESP32"), 1);
  vDrawFastHLine(0, 48, VIRTUAL_W, SSD1306_WHITE);
  vPrint( 0, 54, F("TILT=MOVE"), 1);
  vPrint( 0, 66, F("AUTO=FIRE"), 1);
  vPrint( 0, 78, F("NAV=PAUSE/"),1);
  vPrint( 0, 90, F("MENU"), 1);
  vDrawFastHLine(0, 104, VIRTUAL_W, SSD1306_WHITE);

  // Bottom panel — high score + blinking prompt
  vPrint( 0, 112, F("HIGH SCORE:"),   1);
  vPrintUL(0, 124, highScore,          1);
  vDrawFastHLine(0, 138, VIRTUAL_W, SSD1306_WHITE);

  if ((millis() / 500) % 2 == 0) {
    vPrint(0, 148, F("> PRESS NAV <"), 1);
    vPrint(0, 162, F(">  TO  START <"),1);
  }
}

// ──────────────────────────────────────────────────────────────────
//  PLAYING SCENE
// ──────────────────────────────────────────────────────────────────
void renderPlaying() {
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
  vFillRect(1, 88, 62, 88, SSD1306_BLACK);
  vDrawRect(1, 88, 62, 88, SSD1306_WHITE);

  vPrint(6,  93, F("-- PAUSED --"),  1);
  vDrawFastHLine(1, 104, 62, SSD1306_WHITE);

  const uint8_t startY = 109;
  const uint8_t lineH  = 18;

  // Resume
  vPrint(4, startY,
         pauseCursor == PAUSE_RESUME ? F(">RESUME") : F(" RESUME"), 1);

  // Brightness
  char bBuf[12];
  snprintf(bBuf, sizeof(bBuf), "%sBRI:%d/3",
           pauseCursor == PAUSE_BRIGHTNESS ? ">" : " ", brightnessIdx);
  vPrintStr(4, startY + lineH, bBuf, 1);

  // Button swap
  char sBuf[12];
  snprintf(sBuf, sizeof(sBuf), "%sBTN:%s",
           pauseCursor == PAUSE_SWAPBTN ? ">" : " ",
           btnSwapped ? "SWAP" : "NORM");
  vPrintStr(4, startY + lineH*2, sBuf, 1);

  // Quit
  vPrint(4, startY + lineH*3,
         pauseCursor == PAUSE_QUIT ? F(">QUIT") : F(" QUIT"), 1);
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
