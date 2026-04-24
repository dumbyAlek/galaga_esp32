// ================================================================
//  physics.ino — Game Physics & Player
//
//  taskPhysics()     : runs at ~62Hz during STATE_PLAYING.
//                      Handles ship movement from tilt,
//                      auto-fire timer, player bullet movement,
//                      enemy bullet movement, and hit detection.
//
//  initGame()        : resets all game state for a new session.
//  spawnPlayerBullet(): adds a bullet to the player pool.
// ================================================================

// ──────────────────────────────────────────────────────────────────
//  TASK: PHYSICS  (~62 Hz)
// ──────────────────────────────────────────────────────────────────
void taskPhysics() {
  if (gameState != STATE_PLAYING) return;

  uint32_t now = millis();

  // ── Ship movement ───────────────────────────────────────────────
  // accelY is negative when tilted left, positive when tilted right
  float currentScale = (accelY < 0) ? TILT_SCALE_LEFT : TILT_SCALE_RIGHT;
  
  // Apply the direction-specific scale
  shipX += accelY * currentScale * SHIP_SPEED_MAX;
  shipX  = constrain(shipX,
                     (float)SHIP_HALF_W,
                     (float)(VIRTUAL_W - SHIP_HALF_W));
  
  // ── Manual fire — direct pin poll with rate limiter ───────────
  // Polling PIN_SHOOT directly (not ISR flagA) allows:
  // Tap: flagA set by ISR (catches presses < 16ms)
  // Hold: digitalRead catches sustained press
  bool tapped = flagA;
  if (tapped) flagA = false;

  if ((tapped || digitalRead(PIN_SHOOT) == LOW) &&
      (now - lastShootTime) >= SHOOT_RATE_MS) {
    lastShootTime = now;
    spawnPlayerBullet();
  }

  // ── Player bullet movement + enemy collision ─────────────────────
  for (uint8_t i = 0; i < MAX_BULLETS; i++) {
    if (!playerBullets[i].active) continue;

    playerBullets[i].y -= BULLET_SPEED;

    // Off top of screen — deactivate
    if (playerBullets[i].y < 0) {
      playerBullets[i].active = false;
      continue;
    }

    // Check against all alive enemies (enemies.ino data)
    for (uint8_t r = 0; r < ENEMY_ROWS; r++) {
      for (uint8_t c = 0; c < ENEMY_COLS; c++) {
        if (!enemies[r][c].alive) continue;
        if (fabsf(playerBullets[i].x - enemies[r][c].x) < 5 &&
            fabsf(playerBullets[i].y - enemies[r][c].y) < 5) {
          // Hit — handled by scoring.ino helper
          enemies[r][c].alive     = false;
          playerBullets[i].active = false;
          enemiesAlive--;
          registerKill();   // scoring.ino
          goto next_bullet;
        }
      }
    }
    next_bullet:;
  }

  // ── Boss hit detection ───────────────────────────────────────────
  if (bossActive) {
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
      if (!playerBullets[i].active) continue;
      if (fabsf(playerBullets[i].x - bossX) < 10 &&
          fabsf(playerBullets[i].y - bossY) < 10) {
        playerBullets[i].active = false;
        bossHealth--;
        registerKill();   // +10 per hit
        if (bossHealth == 0) {
          bossActive = false;
          score += 40;   // Bonus for killing boss
          Serial.println(F("[BOSS] Defeated! +40 bonus"));
          resetEnemyGrid();   // Back to normal after boss
        }
      }
    }
  }

  // ── Enemy fire timer (enemies.ino fires the actual bullet) ───────
  if ((now - lastEnemyFire) >= ENEMY_FIRE_INTERVAL && enemiesAlive > 0) {
    lastEnemyFire = now;
    spawnEnemyBullet();   // enemies.ino
  }

  // ── Enemy bullet movement + player collision ──────────────────────
  for (uint8_t i = 0; i < MAX_ENEMY_BULLETS; i++) {
    if (!enemyBullets[i].active) continue;

    enemyBullets[i].y += ENEMY_BULLET_SPEED;

    // Off bottom of screen — deactivate
    if (enemyBullets[i].y > VIRTUAL_H) {
      enemyBullets[i].active = false;
      continue;
    }

    // Hit player ship
    if (fabsf(enemyBullets[i].x - shipX)  < (SHIP_HALF_W + 2) &&
        fabsf(enemyBullets[i].y - SHIP_Y) < 6) {
      enemyBullets[i].active = false;
      registerHit();   // scoring.ino — decrements lives, checks game over
    }
  }

  // ── Wave clear check ────────────────────────────────────────────
  if (enemiesAlive == 0 && !bossActive) {
    nextWave();   // enemies.ino — increments wave, spawns boss or grid
  }

  // ── Boss movement ────────────────────────────────────────────────
  if (bossActive) {
    updateBoss(now);
  }
}

// ──────────────────────────────────────────────────────────────────
//  SPAWN PLAYER BULLET
//  Finds the first inactive slot in the pool. If all slots are
//  full, evicts slot 0 (oldest-first eviction policy).
// ──────────────────────────────────────────────────────────────────
void spawnPlayerBullet() {
  for (uint8_t i = 0; i < MAX_BULLETS; i++) {
    if (!playerBullets[i].active) {
      playerBullets[i] = { shipX, (float)(SHIP_Y - 6), true };
      return;
    }
  }
  // Pool full — overwrite slot 0
  playerBullets[0] = { shipX, (float)(SHIP_Y - 6), true };
}

// ──────────────────────────────────────────────────────────────────
//  INIT GAME  (called from state_machine.ino on STATE_HOME → PLAYING)
// ──────────────────────────────────────────────────────────────────
void initGame() {
  shipX = VIRTUAL_W / 2.0f;
  lives = PLAYER_LIVES;
  score = 0;

  for (uint8_t i = 0; i < MAX_BULLETS;       i++) playerBullets[i].active = false;
  for (uint8_t i = 0; i < MAX_ENEMY_BULLETS; i++) enemyBullets[i].active  = false;

  lastEnemyFire = millis();
  waveNumber    = 0;
  bossActive    = false;

  // Init star field
  for (uint8_t i = 0; i < NUM_STARS; i++) {
    stars[i].x     = random(0, VIRTUAL_W);
    stars[i].y     = random(0, VIRTUAL_H);
    stars[i].speed = random(1, 4);
  }

  resetEnemyGrid();

  Serial.println(F("[GAME] New game started"));
}
