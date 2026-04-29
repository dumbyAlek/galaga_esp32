// ── Display Segments ───────────────────────────────────────────
enum DisplaySegment : uint8_t {
  SEGMENT_TOP = 0,
  SEGMENT_BOT = 1
};

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
#define TILT_DEADZONE       0.1f
#define TILT_SCALE_LEFT     0.24f  // Adjust this for left sensitivity
#define TILT_SCALE_RIGHT    0.42f  // Adjust this for right sensitivity 
#define CAL_SAMPLES         50

// ── Scheduler ────────────────────────────────────────────────────
#define SCHED_STATE_MS      10
#define SCHED_SENSOR_MS     20
#define SCHED_PHYSICS_MS    16
#define SCHED_RENDER_MS     33

// ── Shoot Button ─────────────────────────────────────────────────
// GPIO13 = SHOOT (ISR) — manual fire, confirm menus, wake from sleep
// GPIO12 = NAV   (polled) — cycle cursor, open pause
#define PIN_SHOOT           12
#define PIN_NAV             13
#define DEBOUNCE_SHOOT_MS   25
#define DEBOUNCE_NAV_MS     30

// ── Game — scaled to 64x256 canvas ───────────────────────────────
//  Ship lives at bottom of canvas (virtual Y ~249)
#define SHIP_Y              (VIRTUAL_H - 7)
#define SHIP_HALF_W         3           // Narrower to fit 64px width
#define SHIP_SPEED_MAX      2

//  Bullets travel the full 256px height
#define BULLET_SPEED        8
#define SHOOT_RATE_MS       150 
#define BULLET_H            4
#define MAX_BULLETS         10
#define ENEMY_BULLET_SPEED  3
#define MAX_ENEMY_BULLETS   4

#define MAX_ENEMIES         8           // Max simultaneous enemies on screen
#define ENEMY_SPAWN_INTERVAL 1800       // ms between new enemy spawns
#define ENEMY_FIRE_INTERVAL  3000
#define ENEMY_DRIFT_SPEED    0.3f       // Enemies slowly drift downward
#define ENEMY_MAX_Y          180        // If enemy reaches this Y, player loses a life

// Stones — obstacles to dodge, cannot be shot
#define MAX_STONES          5
#define STONE_SPAWN_MS      2200        // ms between stone spawns
#define STONE_SPEED_MIN     1
#define STONE_SPEED_MAX     3

#define PLAYER_LIVES        3
#define SCORE_PER_KILL      10

// ── EEPROM map ───────────────────────────────────────────────────
//  Byte  0-3  : uint32_t  high score
//  Byte  4    : uint8_t   magic (0xA5)
//  Byte  5    : uint8_t   brightness index (0-3)
//  Byte  6    : uint8_t   buttons swapped (0|1)
//  Byte  7-10 : float     accelY calibration offset
//  Byte  11   : uint8_t   cal magic (0xB6)
// #define EEPROM_SIZE         16
// #define EE_HISCORE_ADDR     0
// #define EE_MAGIC_ADDR       4
// #define EE_MAGIC_VAL        0xA5
// #define EE_BRIGHTNESS_ADDR  5
// #define EE_BTNSWAP_ADDR     6
// #define EE_CALOFFSET_ADDR   7
// #define EE_CALMAG_ADDR      11
// #define EE_CALMAG_VAL       0xB6

#define EEPROM_SIZE         32
#define EE_HISCORE_ADDR     0   // Bytes 0-11 now reserved for the 3 scores
#define EE_MAGIC_ADDR       12
#define EE_MAGIC_VAL        0xA5
#define EE_BRIGHTNESS_ADDR  13
#define EE_BTNSWAP_ADDR     14
#define EE_CALOFFSET_ADDR   15
#define EE_CALMAG_ADDR      19
#define EE_CALMAG_VAL       0xB6

// ── Game States ──────────────────────────────────────────────────
enum GameState : uint8_t {
  STATE_HOME      = 0,
  STATE_PLAYING   = 1,
  STATE_PAUSED    = 2,
  STATE_GAME_OVER = 3,
  STATE_SETTINGS  = 4,
};

// ── Home Menu Items ──────────────────────────────────────────────
#define HOME_START      0
#define HOME_SETTINGS   1
#define HOME_QUIT       2
#define HOME_ITEMS      3

// ── Settings Menu Items ──────────────────────────────────────────
#define SETTINGS_BRIGHTNESS 0
#define SETTINGS_BTNSWAP    1
#define SETTINGS_BACK       2
#define SETTINGS_ITEMS      3

// ── Pause Menu Items ─────────────────────────────────────────────
#define PAUSE_RESUME    0
#define PAUSE_QUIT      1
#define PAUSE_ITEMS     2
