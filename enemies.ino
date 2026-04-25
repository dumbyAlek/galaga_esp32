// ================================================================
//  enemies.ino — Enemy & Stone Spawning
// ================================================================

// ──────────────────────────────────────────────────────────────────
//  SPAWN ONE ENEMY at a random X position near the top
// ──────────────────────────────────────────────────────────────────
void spawnEnemy() {
  for (uint8_t i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].alive) {
      enemies[i].x     = random(5, VIRTUAL_W - 5);
      enemies[i].y     = random(8, 50);    // Appear in top region
      enemies[i].vy    = ENEMY_DRIFT_SPEED + (waveNumber * 0.05f);
      enemies[i].alive = true;
      enemiesAlive++;
      return;
    }
  }
}

// ──────────────────────────────────────────────────────────────────
//  SPAWN WAVE — called on new game or wave clear
//  Fills pool with a burst of enemies
// ──────────────────────────────────────────────────────────────────
void spawnEnemyWave() {
  // Clear existing enemies
  for (uint8_t i = 0; i < MAX_ENEMIES; i++) enemies[i].alive = false;
  enemiesAlive = 0;

  // Spawn 4 + waveNumber enemies (capped at pool size)
  uint8_t count = min((uint8_t)(4 + waveNumber), (uint8_t)MAX_ENEMIES);
  for (uint8_t i = 0; i < count; i++) spawnEnemy();

  Serial.print(F("[WAVE] Spawned ")); Serial.print(count);
  Serial.print(F(" enemies  wave=")); Serial.println(waveNumber);
}

// ──────────────────────────────────────────────────────────────────
//  SPAWN STONE — random size, random X, falls from top
// ──────────────────────────────────────────────────────────────────
void spawnStone() {
  for (uint8_t i = 0; i < MAX_STONES; i++) {
    if (!stones[i].active) {
      stones[i].x      = random(3, VIRTUAL_W - 6);
      stones[i].y      = 0;
      stones[i].w      = random(3, 7);
      stones[i].h      = random(3, 6);
      stones[i].vy     = random(STONE_SPEED_MIN, STONE_SPEED_MAX + 1);
      stones[i].active = true;
      return;
    }
  }
}

// ──────────────────────────────────────────────────────────────────
//  SHOOT — pick a random alive enemy to fire
// ──────────────────────────────────────────────────────────────────
void spawnEnemyBullet() {
  for (uint8_t attempt = 0; attempt < 20; attempt++) {
    uint8_t i = random(MAX_ENEMIES);
    if (!enemies[i].alive) continue;
    for (uint8_t j = 0; j < MAX_ENEMY_BULLETS; j++) {
      if (!enemyBullets[j].active) {
        enemyBullets[j] = { enemies[i].x, enemies[i].y + 5, true };
        return;
      }
    }
    return;
  }
}

// ──────────────────────────────────────────────────────────────────
//  NEXT WAVE
// ──────────────────────────────────────────────────────────────────
void nextWave() {
  waveNumber++;
  Serial.print(F("[WAVE] ")); Serial.println(waveNumber);

  for (uint8_t i = 0; i < MAX_ENEMY_BULLETS; i++) enemyBullets[i].active = false;
  for (uint8_t i = 0; i < MAX_STONES; i++)        stones[i].active = false;

  if (waveNumber % 5 == 0) {
    for (uint8_t i = 0; i < MAX_ENEMIES; i++) enemies[i].alive = false;
    enemiesAlive = 0;
    bossX      = VIRTUAL_W / 2.0f;
    bossY      = 20.0f;
    bossDir    = 1;
    bossHealth = 5 + (waveNumber / 5);   // More HP each boss cycle
    bossActive = true;
    lastBossMove = millis();
    lastBossFire = millis();
    Serial.println(F("[BOSS] Spawned!"));
  } else {
    bossActive = false;
    spawnEnemyWave();
  }
}

// ──────────────────────────────────────────────────────────────────
//  UPDATE BOSS (spider movement + spread fire)
// ──────────────────────────────────────────────────────────────────
void updateBoss(uint32_t now) {
  uint32_t moveInterval = max(80UL, 200UL - (waveNumber / 5) * 20UL);
  if ((now - lastBossMove) >= moveInterval) {
    lastBossMove = now;
    bossX += bossDir * 2;
    if (bossX >= VIRTUAL_W - 10) bossDir = -1;
    if (bossX <= 10)              bossDir =  1;
  }

  uint32_t fireInterval = max(400UL, 900UL - (waveNumber / 5) * 80UL);
  if ((now - lastBossFire) >= fireInterval) {
    lastBossFire = now;
    int16_t offsets[3] = { -6, 0, 6 };
    uint8_t slot = 0;
    for (uint8_t s = 0; s < 3; s++) {
      while (slot < MAX_ENEMY_BULLETS && enemyBullets[slot].active) slot++;
      if (slot >= MAX_ENEMY_BULLETS) break;
      enemyBullets[slot++] = { bossX + offsets[s], bossY + 14, true };
    }
  }
}