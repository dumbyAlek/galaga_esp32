// ================================================================
//  buttons.ino — Button Input Management (OS MODIFIED)
// ================================================================

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// [OS CONCEPT]: Binary Semaphore
// Eita ekta "Signal" er moto kaj kore. ISR jokhon signal dibe, 
// tokhon-i shudhu Physics task bullet fire korbe.
SemaphoreHandle_t shootSemaphore;

void IRAM_ATTR ISR_Shoot() {
  static uint32_t last = 0;
  uint32_t now = millis();
  if ((now - last) < DEBOUNCE_SHOOT_MS) return;
  last = now;

  // [OS CONCEPT]: Giving Semaphore from ISR
  // Ekhane flagA true korar cheye OS ke signal deya beshi fast.
  static BaseType_t xHigherPriorityTaskWoken;
  xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(shootSemaphore, &xHigherPriorityTaskWoken);
  
  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR(); // Instant Context Switch!
  }
}

struct PollState {
  bool     lastHigh  = true;
  uint32_t pressTime = 0;
  bool     confirmed = false;
} pinNavState;

void pollNav() {
  uint32_t now    = millis();
  bool     isHigh = (digitalRead(PIN_NAV) == HIGH);
  if (pinNavState.lastHigh && !isHigh) {
    pinNavState.pressTime = now;
    pinNavState.confirmed = false;
  }
  if (!isHigh && !pinNavState.confirmed &&
      (now - pinNavState.pressTime) >= DEBOUNCE_NAV_MS) {
    pinNavState.confirmed = true;
    flagB = true;   // Nav er jonno flag thakuk, pblm nai
  }
  if (!pinNavState.lastHigh && isHigh) pinNavState.confirmed = false;
  pinNavState.lastHigh = isHigh;
}

// [OS MODIFICATION]: consumeShoot ekhon Semaphore check korbe
bool consumeShoot() {
  if (xSemaphoreTake(shootSemaphore, 0) == pdTRUE) {
    return true;
  }
  return false;
}

bool consumeNav() {
  if (flagB) { flagB = false; return true; }
  return false;
}

void initButtons() {
  pinMode(PIN_SHOOT, INPUT_PULLUP);
  pinMode(PIN_NAV,   INPUT_PULLUP);
  
  // [OS ADDITION]: Create the Semaphore
  shootSemaphore = xSemaphoreCreateBinary();
  
  attachInterrupt(digitalPinToInterrupt(PIN_SHOOT), ISR_Shoot, FALLING);
}