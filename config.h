#pragma once

// ================================================================
//  config.h — Hardware Abstraction & Game Constants
//  Project : Galaga ESP32 — CSE 323 OS Design
// ================================================================

// ── I2C ──────────────────────────────────────────────────────────
#define I2C_SDA             21
#define I2C_SCL             22

// ── Display ──────────────────────────────────────────────────────
#define OLED_RESET          -1

//  Each OLED module is a 128×64 panel in hardware.
//  These are the values passed to the Adafruit_SSD1306 constructor.
#define OLED_PHYS_W         128
#define OLED_PHYS_H         64

//  setRotation(1) rotates each panel 90 degrees clockwise,
//  making each one logically 64 wide x 128 tall (portrait).
//  Two portrait panels stacked vertically = one 64x256 virtual canvas.
//
//  Virtual Y   0 - 127  ->  dispTop (0x3C)   physY = vY
//  Virtual Y 128 - 255  ->  dispBot (0x3D)   physY = vY - 128
//
#define VIRTUAL_W           64
#define VIRTUAL_H           256
#define PAGE_BOUNDARY_Y     128

#define DISP_TOP_ADDR       0x3C
#define DISP_BOT_ADDR       0x3D        // SA0 solder pad bridged HIGH

// ── Brightness ───────────────────────────────────────────────────
#define BRIGHTNESS_LEVELS   4
const uint8_t BRIGHTNESS_VALUES[BRIGHTNESS_LEVELS] = { 10, 60, 150, 255 };

// ── MPU-6050 ─────────────────────────────────────────────────────
#define MPU_ADDR            0x68
#define TILT_DEADZONE       0.3f
#define TILT_SCALE          0.09f       
#define CAL_SAMPLES         50

// ── Buttons ──────────────────────────────────────────────────────
#define PIN_A               13
#define PIN_B               12
#define DEBOUNCE_A_MS       60
#define DEBOUNCE_B_MS       120

// ── Scheduler ────────────────────────────────────────────────────
#define SCHED_STATE_MS      10
#define SCHED_SENSOR_MS     20
#define SCHED_PHYSICS_MS    16
#define SCHED_RENDER_MS     33

// ── Auto-Fire ────────────────────────────────────────────────────
#define AUTO_FIRE_MS        400

// ── Game — scaled to 64x256 canvas ───────────────────────────────
//  Ship lives at bottom of canvas (virtual Y ~249)
#define SHIP_Y              (VIRTUAL_H - 7)
#define SHIP_HALF_W         3           // Narrower to fit 64px width
#define SHIP_SPEED_MAX      2

//  Bullets travel the full 256px height
#define BULLET_SPEED        5
#define BULLET_H            4
#define MAX_BULLETS         4
#define ENEMY_BULLET_SPEED  3
#define MAX_ENEMY_BULLETS   4

//  Enemy grid — 5 cols x 4 rows, fits within 64px width
//  Rightmost enemy centre: 4 + 4*11 = 48  +  half-width 4  = 52 < 64
#define ENEMY_ROWS          4
#define ENEMY_COLS          5
#define ENEMY_ORIGIN_X      4
#define ENEMY_ORIGIN_Y      10
#define ENEMY_SPACING_X     11
#define ENEMY_SPACING_Y     12
#define ENEMY_FIRE_INTERVAL 1400

#define PLAYER_LIVES        3
#define SCORE_PER_KILL      10

// ── EEPROM map ───────────────────────────────────────────────────
//  Byte  0-3  : uint32_t  high score
//  Byte  4    : uint8_t   magic (0xA5)
//  Byte  5    : uint8_t   brightness index (0-3)
//  Byte  6    : uint8_t   buttons swapped (0|1)
//  Byte  7-10 : float     accelX calibration offset
//  Byte  11   : uint8_t   cal magic (0xB6)
#define EEPROM_SIZE         16
#define EE_HISCORE_ADDR     0
#define EE_MAGIC_ADDR       4
#define EE_MAGIC_VAL        0xA5
#define EE_BRIGHTNESS_ADDR  5
#define EE_BTNSWAP_ADDR     6
#define EE_CALOFFSET_ADDR   7
#define EE_CALMAG_ADDR      11
#define EE_CALMAG_VAL       0xB6

// ── Game States ──────────────────────────────────────────────────
enum GameState : uint8_t {
  STATE_HOME      = 0,
  STATE_PLAYING   = 1,
  STATE_PAUSED    = 2,
  STATE_GAME_OVER = 3,
};

// ── Pause Menu Items ─────────────────────────────────────────────
#define PAUSE_RESUME        0
#define PAUSE_BRIGHTNESS    1
#define PAUSE_SWAPBTN       2
#define PAUSE_QUIT          3
#define PAUSE_ITEMS         4
