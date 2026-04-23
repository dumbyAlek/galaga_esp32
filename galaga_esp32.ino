// ================================================================
//  galaga_esp32.ino — Entry Point & Global Definitions
//
//  All global variables are defined here. Every other .ino file
//  in this folder shares this scope (Arduino concatenates all
//  .ino files into one translation unit before compiling).
//
//  File map:
//    galaga_esp32.ino  — setup(), loop(), scheduler, globals
//    display.ino       — virtual MMU, vDraw*() primitives
//    render.ino        — all screen rendering functions
//    buttons.ino       — ISR, poll, consumePower/Nav
//    state_machine.ino — taskStateMachine(), state transitions
//    sensors.ino       — taskSensorPoll(), runCalibration()
//    physics.ino       — taskPhysics(), ship movement, bullets
//    enemies.ino       — enemy grid, spawnEnemyBullet()
//    scoring.ino       — EEPROM, brightness, shutdown
//
//  Libraries (Arduino Library Manager):
//    Adafruit SSD1306 | Adafruit GFX
//    Adafruit MPU6050 | Adafruit Unified Sensor
//    EEPROM (ESP32 built-in)
// ================================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <EEPROM.h>
#include "config.h"

// ──────────────────────────────────────────────────────────────────
//  PERIPHERAL OBJECTS  (used across display.ino, render.ino, etc.)
// ──────────────────────────────────────────────────────────────────
Adafruit_SSD1306 dispTop(OLED_PHYS_W, OLED_PHYS_H, &Wire, OLED_RESET);
Adafruit_SSD1306 dispBot(OLED_PHYS_W, OLED_PHYS_H, &Wire, OLED_RESET);
Adafruit_MPU6050 mpu;

// ──────────────────────────────────────────────────────────────────
//  SETTINGS  (loaded from EEPROM by scoring.ino on boot)
// ──────────────────────────────────────────────────────────────────
uint32_t highScore     = 0;
uint8_t  brightnessIdx = 2;
bool     btnSwapped    = false;
float    calOffsetX    = 0.0f;

// ──────────────────────────────────────────────────────────────────
//  GAME STATE  (read/written by state_machine.ino, physics.ino)
// ──────────────────────────────────────────────────────────────────
GameState gameState   = STATE_HOME;
uint8_t   pauseCursor = 0;

// ──────────────────────────────────────────────────────────────────
//  SENSOR DATA  (written by sensors.ino, read by physics.ino)
// ──────────────────────────────────────────────────────────────────
volatile float accelY = 0.0f;

// ──────────────────────────────────────────────────────────────────
//  GAME DATA  (shared across physics.ino, enemies.ino, render.ino)
// ──────────────────────────────────────────────────────────────────
struct Bullet { float x, y; bool active; };
struct Enemy  { float x, y; bool alive;  };

float    shipX        = VIRTUAL_W / 2.0f;
uint8_t  lives        = PLAYER_LIVES;
uint32_t score        = 0;

Bullet   playerBullets[MAX_BULLETS];
Bullet   enemyBullets[MAX_ENEMY_BULLETS];
Enemy    enemies[ENEMY_ROWS][ENEMY_COLS];
uint8_t  enemiesAlive = 0;

uint32_t lastAutoFire  = 0;
uint32_t lastEnemyFire = 0;

// ──────────────────────────────────────────────────────────────────
//  BUTTON FLAGS  (written by buttons.ino ISR/poll)
// ──────────────────────────────────────────────────────────────────
volatile bool flagA = false;
bool          flagB = false;

// ──────────────────────────────────────────────────────────────────
//  ROUND-ROBIN TASK SCHEDULER
//  Each entry holds a timestamp and interval. The dispatcher fires
//  any task whose interval has elapsed since its last run.
// ──────────────────────────────────────────────────────────────────
struct Task { uint32_t lastRun; uint32_t interval; void(*fn)(); };

// Function pointers declared here; implementations live in
// their respective .ino files.
void taskStateMachine();   // state_machine.ino
void taskSensorPoll();     // sensors.ino
void taskPhysics();        // physics.ino
void taskRender();         // render.ino

Task scheduler[] = {
  { 0, SCHED_STATE_MS,   taskStateMachine },
  { 0, SCHED_SENSOR_MS,  taskSensorPoll   },
  { 0, SCHED_PHYSICS_MS, taskPhysics      },
  { 0, SCHED_RENDER_MS,  taskRender       },
};
const uint8_t NUM_TASKS = sizeof(scheduler) / sizeof(Task);

// ──────────────────────────────────────────────────────────────────
//  SETUP
// ──────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(F("\n=== GALAGA ESP32 — CSE 323 ==="));

  // I2C bus
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);
  Serial.println(F("[I2C]  SDA=21  SCL=22  400kHz"));

  // Top OLED — portrait 64x128 after setRotation(1), virtual Y 0-127
  if (!dispTop.begin(SSD1306_SWITCHCAPVCC, DISP_TOP_ADDR)) {
    Serial.println(F("[HALT] Top OLED not found (0x3C)"));
    while (true);
  }
  dispTop.setRotation(3);      // 128x64 landscape -> 64x128 portrait
  dispTop.setTextWrap(false);
  Serial.println(F("[DISP] Top    OK (0x3C)  portrait  vY 0-127"));

  // Bottom OLED — portrait 64x128 after setRotation(1), virtual Y 128-255
  if (!dispBot.begin(SSD1306_SWITCHCAPVCC, DISP_BOT_ADDR)) {
    Serial.println(F("[HALT] Bottom OLED not found (0x3D)"));
    Serial.println(F("       Bridge SA0 pad on bottom OLED PCB"));
    while (true);
  }
  dispBot.setRotation(3);      // 128x64 landscape -> 64x128 portrait
  dispBot.setTextWrap(false);
  Serial.println(F("[DISP] Bottom OK (0x3D)  portrait  vY 128-255"));

  // // Rotate both displays 180 degrees
  // dispTop.setRotation(2);
  // dispBot.setRotation(2);

  // MPU-6050
  if (!mpu.begin()) {
    Serial.println(F("[HALT] MPU-6050 not found (0x68)"));
    while (true);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println(F("[MPU]  OK  ±4G  LPF 21Hz"));

  // Buttons (buttons.ino registers the ISR)
  initButtons();
  Serial.println(F("[BTN]  Power=ISR(GPIO13)  Nav=POLLED(GPIO12)"));

  // EEPROM + settings (scoring.ino)
  EEPROM.begin(EEPROM_SIZE);
  eepromLoad();
  applyBrightness();

  // Calibration (sensors.ino) — only runs if never done before
  if (EEPROM.read(EE_CALMAG_ADDR) != EE_CALMAG_VAL) {
    runCalibration();
  }

  // Splash screen (render.ino)
  showSplash();

  gameState = STATE_HOME;
  Serial.println(F("[BOOT] Ready — STATE_HOME\n"));
}

// ──────────────────────────────────────────────────────────────────
//  MAIN LOOP — Round-Robin Dispatcher
// ──────────────────────────────────────────────────────────────────
void loop() {
  uint32_t now = millis();
  for (uint8_t i = 0; i < NUM_TASKS; i++) {
    if ((now - scheduler[i].lastRun) >= scheduler[i].interval) {
      scheduler[i].lastRun = now;
      scheduler[i].fn();
    }
  }
}
