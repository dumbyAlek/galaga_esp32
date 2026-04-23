// ================================================================
//  sensors.ino — IMU Sensor Management
//
//  taskSensorPoll()  : runs at 50Hz, reads MPU-6050 accelerometer,
//                      applies calibration offset and deadzone,
//                      writes result to shared accelX.
//
//  runCalibration()  : called once on first boot. Samples the
//                      resting accelX 50 times, averages them,
//                      saves as calOffsetX to EEPROM. Subsequent
//                      reads subtract this offset so the ship
//                      stays centred when the device is flat.
// ================================================================

// ──────────────────────────────────────────────────────────────────
//  TASK: SENSOR POLL  (50 Hz)
// ──────────────────────────────────────────────────────────────────
void taskSensorPoll() {
  if (gameState != STATE_PLAYING) return;

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Subtract calibration offset, then apply deadzone
  float raw = a.acceleration.y - calOffsetX;
  accelX = (fabsf(raw) > TILT_DEADZONE) ? raw : 0.0f;
}

// ──────────────────────────────────────────────────────────────────
//  CALIBRATION  (first boot only)
//
//  Shows instruction screen, waits 2 seconds, then samples
//  CAL_SAMPLES accel readings and saves the average offset.
//  The calibration magic byte (EE_CALMAG_VAL) is written to
//  EEPROM so this never runs again unless EEPROM is wiped.
// ──────────────────────────────────────────────────────────────────
void runCalibration() {
  dispTop.clearDisplay();
  dispBot.clearDisplay();

  // Instruction screen
  vPrint( 8,  4, F("CALIBRATION"),   1);
  vDrawFastHLine(0, 14, VIRTUAL_W, SSD1306_WHITE);
  vPrint( 4, 22, F("Place device"),  1);
  vPrint( 4, 34, F("flat on a"),     1);
  vPrint( 4, 46, F("surface."),      1);
  vPrint( 4, 68, F("Hold still..."), 1);
  vPrint( 4, 82, F("Sampling in"),   1);
  vPrint( 4, 94, F("2 seconds."),    1);

  dispTop.display();
  dispBot.display();
  delay(2000);

  // Sample and average
  float sum = 0.0f;
  for (uint8_t i = 0; i < CAL_SAMPLES; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    sum += a.acceleration.y;
    delay(20);
  }
  calOffsetX = sum / CAL_SAMPLES;
  eepromSaveCalOffset();   // scoring.ino writes to EEPROM

  // Confirmation screen
  dispTop.clearDisplay();
  dispBot.clearDisplay();

  vPrint(4, 20, F("Calibrated!"), 1);
  vPrint(4, 36, F("Offset:"),     1);

  // Print float manually (no %f in flash strings)
  int16_t whole = (int16_t)calOffsetX;
  int16_t frac  = (int16_t)(fabsf(calOffsetX - (float)whole) * 100);
  char buf[12];
  snprintf(buf, sizeof(buf), "%d.%02d", whole, frac);
  vPrintStr(60, 36, buf, 1);

  dispTop.display();
  dispBot.display();
  delay(1500);

  Serial.print(F("[CAL] Done. Offset = "));
  Serial.println(calOffsetX);
}
