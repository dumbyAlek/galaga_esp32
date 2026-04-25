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

  for (uint8_t e = 0; e < MAX_ENEMIES; e++) {
      if (!enemies[e].alive) continue;
      if (fabsf(playerBullets[i].x - enemies[e].x) < 5 &&
          fabsf(playerBullets[i].y - enemies[e].y) < 6) {
        enemies[e].alive        = false;
        playerBullets[i].active = false;
        enemiesAlive--;
        registerKill();
        goto next_bullet;
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
          spawnEnemyWave();
        }
      }
    }
  }

// ── Enemy drift downward ─────────────────────────────────────────
  for (uint8_t e = 0; e < MAX_ENEMIES; e++) {
    if (!enemies[e].alive) continue;
    enemies[e].y += enemies[e].vy;
    if (enemies[e].y > ENEMY_MAX_Y) {
      // Enemy reached player zone — costs a life
      enemies[e].alive = false;
      enemiesAlive--;
      registerHit();
    }
  }

  // ── Periodic new enemy spawn (during non-boss waves) ─────────────
  if (!bossActive && enemiesAlive < MAX_ENEMIES &&
      (now - lastEnemySpawn) >= ENEMY_SPAWN_INTERVAL) {
    lastEnemySpawn = now;
    spawnEnemy();
  }

  // ── Enemy fire timer ──────────────────────────────────────────────
  if ((now - lastEnemyFire) >= ENEMY_FIRE_INTERVAL && enemiesAlive > 0) {
    lastEnemyFire = now;
    spawnEnemyBullet();
  }

  // ── Stone spawning ────────────────────────────────────────────────
  if ((now - lastStoneSpawn) >= STONE_SPAWN_MS) {
    lastStoneSpawn = now;
    spawnStone();
  }

  // ── Stone movement + player collision ─────────────────────────────
  for (uint8_t i = 0; i < MAX_STONES; i++) {
    if (!stones[i].active) continue;
    stones[i].y += stones[i].vy;
    if (stones[i].y > VIRTUAL_H) { stones[i].active = false; continue; }
    // Hit player — dodge or lose a life
    if (fabsf(stones[i].x - shipX)    < (SHIP_HALF_W + stones[i].w / 2) &&
        fabsf(stones[i].y - SHIP_Y) < (stones[i].h + 4)) {
      stones[i].active = false;
      registerHit();
    }
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

  // Wave clear: no enemies alive AND pool has been seeded for a while
  if (enemiesAlive == 0 && !bossActive &&
      (now - lastEnemySpawn) > ENEMY_SPAWN_INTERVAL * 2) {
    nextWave();
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

  lastEnemyFire  = millis();
  lastEnemySpawn = millis();
  lastStoneSpawn = millis() + 3000;   // Stones start 3s after game begins
  waveNumber     = 0;
  bossActive     = false;

  for (uint8_t i = 0; i < MAX_STONES; i++) stones[i].active = false;

  for (uint8_t i = 0; i < NUM_STARS; i++) {
    stars[i].x     = random(0, VIRTUAL_W);
    stars[i].y     = random(0, VIRTUAL_H);
    stars[i].speed = random(1, 4);
  }

  spawnEnemyWave();

  Serial.println(F("[GAME] New game started"));
}
