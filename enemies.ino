// ================================================================
//  enemies.ino — Enemy Grid & Behaviour
//
//  resetEnemyGrid()   : fills the ENEMY_ROWS × ENEMY_COLS grid
//                       with evenly-spaced alive enemies.
//                       Called on new game and on wave clear.
//
//  spawnEnemyBullet() : picks a random alive enemy and fires
//                       a bullet downward from it.
//
//  Enemy data arrays (enemies[], enemiesAlive) are defined as
//  globals in galaga_esp32.ino.
// ================================================================

// ──────────────────────────────────────────────────────────────────
//  RESET ENEMY GRID
//  Places all enemies in their starting positions.
//  Grid origin: ENEMY_ORIGIN_X, ENEMY_ORIGIN_Y (virtual coords).
//  Grid lives entirely in the top display (vY 8–50).
// ──────────────────────────────────────────────────────────────────
void resetEnemyGrid() {
  enemiesAlive = ENEMY_ROWS * ENEMY_COLS;

  for (uint8_t r = 0; r < ENEMY_ROWS; r++) {
    for (uint8_t c = 0; c < ENEMY_COLS; c++) {
      enemies[r][c] = {
        ENEMY_ORIGIN_X + c * (float)ENEMY_SPACING_X,
        ENEMY_ORIGIN_Y + r * (float)ENEMY_SPACING_Y,
        true
      };
    }
  }

  Serial.print(F("[ENEMY] Grid reset — "));
  Serial.print(enemiesAlive);
  Serial.println(F(" enemies"));
}

// ──────────────────────────────────────────────────────────────────
//  SPAWN ENEMY BULLET
//  Randomly selects an alive enemy to shoot from.
//  Uses up to 20 attempts to find an alive enemy before giving up.
//  Finds the first inactive slot in the enemy bullet pool.
// ──────────────────────────────────────────────────────────────────
void spawnEnemyBullet() {
  for (uint8_t attempt = 0; attempt < 20; attempt++) {
    uint8_t r = random(ENEMY_ROWS);
    uint8_t c = random(ENEMY_COLS);

    if (!enemies[r][c].alive) continue;

    // Found a live enemy — find a free bullet slot
    for (uint8_t i = 0; i < MAX_ENEMY_BULLETS; i++) {
      if (!enemyBullets[i].active) {
        enemyBullets[i] = {
          enemies[r][c].x,
          enemies[r][c].y + 4,   // Start just below enemy body
          true
        };
        return;
      }
    }
    // All enemy bullet slots full — skip this fire cycle
    return;
  }
}
