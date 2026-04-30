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
  vDrawFastHLine(0, 50, VIRTUAL_W, SSD1306_WHITE);
  vPrint(4,  55, F("TOP SCORES"), 1);
  vDrawFastHLine(0, 65, VIRTUAL_W, SSD1306_WHITE);

  const char* medals[3] = {"1.", "2.", "3."};
  for (uint8_t i = 0; i < 3; i++) {
    vPrintStr(4,  70 + i * 14, medals[i], 1);
    vPrintUL(20, 70 + i * 14, topScores[i], 1);
  }
  vDrawFastHLine(0, 110, VIRTUAL_W, SSD1306_WHITE);

  // ── Menu ───────────────────────────────────────────────────────
  const uint8_t menuY = 160;
  const uint8_t lineH = 18;
  vPrint(4, menuY,              homeCursor==HOME_START    ? F(">START GAME") : F(" START GAME"), 1);
  vPrint(4, menuY + lineH,      homeCursor==HOME_SETTINGS ? F(">SETTINGS")   : F(" SETTINGS"),   1);
  vPrint(4, menuY + lineH*2,    homeCursor==HOME_QUIT     ? F(">QUIT")       : F(" QUIT"),       1);

  // ── Blinking GALAGA title at TOP ────────────────────────────
  if ((millis() / 600) % 2 == 0) {
    vPrint( 0,  0, F("GA"),        2);
    vPrint( 1,  1, F("GA"),        2);
    vPrint( 20,  12, F("LA"),        2);
    vPrint( 21,  13, F("LA"),        2);
    vPrint( 40,  24, F("GA"),        2);
    vPrint( 41,  25, F("GA"),        2);
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
  
  // ── TOP HUD (Overlays everything else) ───────────────────────────
  // Draw a solid black box to hide stars/enemies scrolling under the text
  vFillRect(0, 0, VIRTUAL_W, 37, SSD1306_BLACK);

// 1) Score (Top Left)
  acquireMutex(mtxScore);
  uint32_t localScore = score;
  releaseMutex(mtxScore);
  
  char scoreBuf[16];
  snprintf(scoreBuf, sizeof(scoreBuf), "Score:%lu", localScore);
  vPrintStr(0, 2, scoreBuf, 1);

  // 2) Lives / Hearts (Top Right)
  acquireMutex(mtxLives);
  uint8_t hudLives = lives;
  releaseMutex(mtxLives);

  for (uint8_t i = 0; i < hudLives; i++) {
    // Keep X fixed on the right edge, but push Y down for each extra life
    int16_t hx = VIRTUAL_W - 6; 
    int16_t hy = 2 + (i * 7);   // 5px heart + 2px gap between them

    // Draw the custom 5x5 pixel heart at the new coordinates!
    vDrawPixel(hx+1, hy,   SSD1306_WHITE); vDrawPixel(hx+3, hy,   SSD1306_WHITE);
    vDrawPixel(hx,   hy+1, SSD1306_WHITE); vDrawPixel(hx+1, hy+1, SSD1306_WHITE); vDrawPixel(hx+2, hy+1, SSD1306_WHITE); vDrawPixel(hx+3, hy+1, SSD1306_WHITE); vDrawPixel(hx+4, hy+1, SSD1306_WHITE);
    vDrawPixel(hx,   hy+2, SSD1306_WHITE); vDrawPixel(hx+1, hy+2, SSD1306_WHITE); vDrawPixel(hx+2, hy+2, SSD1306_WHITE); vDrawPixel(hx+3, hy+2, SSD1306_WHITE); vDrawPixel(hx+4, hy+2, SSD1306_WHITE);
    vDrawPixel(hx+1, hy+3, SSD1306_WHITE); vDrawPixel(hx+2, hy+3, SSD1306_WHITE); vDrawPixel(hx+3, hy+3, SSD1306_WHITE);
    vDrawPixel(hx+2, hy+4, SSD1306_WHITE);
  }

  // 3) Cycles (Line 2)
  char cycBuf[16];
  snprintf(cycBuf, sizeof(cycBuf), "Round:%d", cyclesPassed);
  vPrintStr(0, 13, cycBuf, 1);

  // 4) Bosses (Line 3)
  char bossBuf[16];
  snprintf(bossBuf, sizeof(bossBuf), "Boss:%d", bossesKilled);
  vPrintStr(0, 24, bossBuf, 1);

  // 5) Separator Line
  vDrawFastHLine(0, 36, VIRTUAL_W, SSD1306_WHITE);

  // Alien enemies — flat pool, random positions, drift downward
  for (uint8_t e = 0; e < MAX_ENEMIES; e++) {
    if (!enemies[e].alive) continue;
    int16_t ex = (int16_t)enemies[e].x;
    int16_t ey = (int16_t)enemies[e].y;
    // Head (round)
    vDrawPixel(ex-1, ey-5, SSD1306_WHITE);
    vDrawPixel(ex,   ey-5, SSD1306_WHITE);
    vDrawPixel(ex+1, ey-5, SSD1306_WHITE);
    vDrawPixel(ex-2, ey-4, SSD1306_WHITE);
    vDrawPixel(ex+2, ey-4, SSD1306_WHITE);
    // Eyes inside head
    vDrawPixel(ex-1, ey-4, SSD1306_WHITE);
    vDrawPixel(ex+1, ey-4, SSD1306_WHITE);
    // Antennae
    vDrawPixel(ex-2, ey-7, SSD1306_WHITE);
    vDrawPixel(ex+2, ey-7, SSD1306_WHITE);
    vDrawPixel(ex-1, ey-6, SSD1306_WHITE);
    vDrawPixel(ex+1, ey-6, SSD1306_WHITE);
    // Body
    vDrawRect(ex-3, ey-3, 7, 6, SSD1306_WHITE);
    // Arms
    vDrawPixel(ex-4, ey-2, SSD1306_WHITE);
    vDrawPixel(ex+4, ey-2, SSD1306_WHITE);
    // Legs
    vDrawPixel(ex-4, ey+1, SSD1306_WHITE);
    vDrawPixel(ex+4, ey+1, SSD1306_WHITE);
    vDrawPixel(ex-3, ey+3, SSD1306_WHITE);
    vDrawPixel(ex+3, ey+3, SSD1306_WHITE);
  }

  // Stones — irregular falling rocks
  acquireMutex(mtxLives);
  uint8_t localLives = lives; // Critical Section: Lives and Rocks
  releaseMutex(mtxLives);
  for (uint8_t i = 0; i < MAX_STONES; i++) {
    if (!stones[i].active) continue;
    int16_t sx = (int16_t)stones[i].x;
    int16_t sy = (int16_t)stones[i].y;
    uint8_t sw = stones[i].w;
    uint8_t sh = stones[i].h;
    // Draw as filled rect with clipped corners for rocky look
    vFillRect(sx, sy, sw, sh, SSD1306_WHITE);
    // Clip corners to make it look irregular
    vDrawPixel(sx,      sy,      SSD1306_BLACK);
    vDrawPixel(sx+sw-1, sy,      SSD1306_BLACK);
    vDrawPixel(sx,      sy+sh-1, SSD1306_BLACK);
    vDrawPixel(sx+sw-1, sy+sh-1, SSD1306_BLACK);
  }

  // ── Spider Boss ───────────────────────────────────────────────────
  if (bossActive) {
    int16_t bx = (int16_t)bossX;
    int16_t by = (int16_t)bossY;

    // Head (round, sits above body)
    vDrawPixel(bx-2, by-13, SSD1306_WHITE);
    vDrawPixel(bx-1, by-14, SSD1306_WHITE);
    vDrawPixel(bx,   by-14, SSD1306_WHITE);
    vDrawPixel(bx+1, by-14, SSD1306_WHITE);
    vDrawPixel(bx+2, by-13, SSD1306_WHITE);
    vDrawPixel(bx-3, by-12, SSD1306_WHITE);
    vDrawPixel(bx+3, by-12, SSD1306_WHITE);
    vDrawPixel(bx-3, by-11, SSD1306_WHITE);
    vDrawPixel(bx+3, by-11, SSD1306_WHITE);
    vDrawPixel(bx-2, by-10, SSD1306_WHITE);
    vDrawPixel(bx+2, by-10, SSD1306_WHITE);
    // Eyes (2x2 blocks)
    vFillRect(bx-2, by-13, 2, 2, SSD1306_WHITE);
    vFillRect(bx+1, by-13, 2, 2, SSD1306_WHITE);
    // Fangs
    vDrawPixel(bx-1, by-9,  SSD1306_WHITE);
    vDrawPixel(bx+1, by-9,  SSD1306_WHITE);

    // Body (oval-ish with rect)
    vFillRect(bx-7, by-8, 15, 14, SSD1306_WHITE);
    // Body detail — clear centre for texture
    vDrawPixel(bx,   by-4, SSD1306_BLACK);
    vDrawPixel(bx,   by,   SSD1306_BLACK);
    vDrawPixel(bx,   by+4, SSD1306_BLACK);

    // 4 pairs of spider legs (radiating from sides)
    // Left legs
    vDrawPixel(bx-8,  by-7, SSD1306_WHITE);
    vDrawPixel(bx-9,  by-8, SSD1306_WHITE);
    vDrawPixel(bx-10, by-9, SSD1306_WHITE);

    vDrawPixel(bx-8,  by-4, SSD1306_WHITE);
    vDrawPixel(bx-10, by-4, SSD1306_WHITE);
    vDrawPixel(bx-12, by-5, SSD1306_WHITE);

    vDrawPixel(bx-8,  by,   SSD1306_WHITE);
    vDrawPixel(bx-10, by+1, SSD1306_WHITE);
    vDrawPixel(bx-12, by+2, SSD1306_WHITE);

    vDrawPixel(bx-8,  by+4, SSD1306_WHITE);
    vDrawPixel(bx-9,  by+6, SSD1306_WHITE);
    vDrawPixel(bx-10, by+8, SSD1306_WHITE);

    // Right legs
    vDrawPixel(bx+8,  by-7, SSD1306_WHITE);
    vDrawPixel(bx+9,  by-8, SSD1306_WHITE);
    vDrawPixel(bx+10, by-9, SSD1306_WHITE);

    vDrawPixel(bx+8,  by-4, SSD1306_WHITE);
    vDrawPixel(bx+10, by-4, SSD1306_WHITE);
    vDrawPixel(bx+12, by-5, SSD1306_WHITE);

    vDrawPixel(bx+8,  by,   SSD1306_WHITE);
    vDrawPixel(bx+10, by+1, SSD1306_WHITE);
    vDrawPixel(bx+12, by+2, SSD1306_WHITE);

    vDrawPixel(bx+8,  by+4, SSD1306_WHITE);
    vDrawPixel(bx+9,  by+6, SSD1306_WHITE);
    vDrawPixel(bx+10, by+8, SSD1306_WHITE);

    // Health bar
    uint8_t maxHP = 15 + (waveNumber / 5);
    vDrawRect(bx-7, by+8, 15, 3, SSD1306_WHITE);
    uint8_t hpW = (uint8_t)(14.0f * bossHealth / maxHP);
    vFillRect(bx-7, by+8, hpW, 3, SSD1306_WHITE);

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
    // Use vDrawFastVLine to make the bullets 5 pixels long (or adjust as you prefer)
    vDrawFastVLine((int16_t)enemyBullets[i].x, (int16_t)enemyBullets[i].y, 5, SSD1306_WHITE);
  }

  // Ship — bottom panel, vY ~242-249. Fits in 64px width (half-width=3)
  int16_t sx = (int16_t)shipX;
  vFillTriangle(sx,               SHIP_Y - 5,
                sx - SHIP_HALF_W, SHIP_Y,
                sx + SHIP_HALF_W, SHIP_Y,
                SSD1306_WHITE);
  vDrawPixel(sx - 1, SHIP_Y + 1, SSD1306_WHITE);
  vDrawPixel(sx + 1, SHIP_Y + 1, SSD1306_WHITE);
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
  vPrint(2, 150, F("PRESS Shoot"),    1);
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
