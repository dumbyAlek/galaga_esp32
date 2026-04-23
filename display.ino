// ================================================================
//  display.ino — Virtual MMU & Drawing Primitives
//
//  Implements the Y-split virtual display layer.
//  All game code draws using virtual coordinates (0–127 on both
//  axes). These functions translate virtual Y to the correct
//  physical display and local Y offset.
//
//  Virtual Y  0–63   → dispTop  (physical y = vY)
//  Virtual Y  64–127 → dispBot  (physical y = vY − 64)
// ================================================================

// ── Page routing helpers ─────────────────────────────────────────
inline Adafruit_SSD1306& pageForY(int16_t vy) {
  return (vy < PAGE_BOUNDARY_Y) ? dispTop : dispBot;
}
inline int16_t physY(int16_t vy) {
  return (vy < PAGE_BOUNDARY_Y) ? vy : vy - PAGE_BOUNDARY_Y;
}

// ── Pixel ────────────────────────────────────────────────────────
void vDrawPixel(int16_t vx, int16_t vy, uint16_t c) {
  if (vx < 0 || vx >= VIRTUAL_W || vy < 0 || vy >= VIRTUAL_H) return;
  pageForY(vy).drawPixel(vx, physY(vy), c);
}

// ── Horizontal line — always within one page ─────────────────────
void vDrawFastHLine(int16_t vx, int16_t vy, int16_t w, uint16_t c) {
  if (vy < 0 || vy >= VIRTUAL_H) return;
  pageForY(vy).drawFastHLine(vx, physY(vy), w, c);
}

// ── Vertical line — may cross Y=64 page boundary ─────────────────
void vDrawFastVLine(int16_t vx, int16_t vy, int16_t h, uint16_t c) {
  // Pixel-by-pixel so boundary crossing is handled automatically
  for (int16_t i = vy; i < vy + h; i++) vDrawPixel(vx, i, c);
}

// ── Rectangle outline ────────────────────────────────────────────
void vDrawRect(int16_t vx, int16_t vy, int16_t w, int16_t h, uint16_t c) {
  vDrawFastHLine(vx, vy,         w, c);
  vDrawFastHLine(vx, vy + h - 1, w, c);
  for (int16_t i = vy + 1; i < vy + h - 1; i++) {
    vDrawPixel(vx,         i, c);
    vDrawPixel(vx + w - 1, i, c);
  }
}

// ── Filled rectangle ─────────────────────────────────────────────
void vFillRect(int16_t vx, int16_t vy, int16_t w, int16_t h, uint16_t c) {
  for (int16_t row = vy; row < vy + h; row++)
    vDrawFastHLine(vx, row, w, c);
}

// ── Filled circle ────────────────────────────────────────────────
void vFillCircle(int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  for (int16_t dy = -r; dy <= r; dy++)
    for (int16_t dx = -r; dx <= r; dx++)
      if (dx*dx + dy*dy <= r*r)
        vDrawPixel(cx + dx, cy + dy, c);
}

// ── Filled triangle ──────────────────────────────────────────────
//  Fast path when entirely within one page.
//  Scanline rasteriser for shapes that cross Y=64.
void vFillTriangle(int16_t x0, int16_t y0,
                   int16_t x1, int16_t y1,
                   int16_t x2, int16_t y2, uint16_t c) {
  bool allTop = y0 < PAGE_BOUNDARY_Y && y1 < PAGE_BOUNDARY_Y && y2 < PAGE_BOUNDARY_Y;
  bool allBot = y0 >= PAGE_BOUNDARY_Y && y1 >= PAGE_BOUNDARY_Y && y2 >= PAGE_BOUNDARY_Y;

  if (allTop) {
    dispTop.fillTriangle(x0, y0, x1, y1, x2, y2, c);
    return;
  }
  if (allBot) {
    int16_t pb = PAGE_BOUNDARY_Y;
    dispBot.fillTriangle(x0, y0-pb, x1, y1-pb, x2, y2-pb, c);
    return;
  }
  // Spans boundary — sort by Y then scanline rasterise
  if (y0>y1){int16_t tx=x0,ty=y0;x0=x1;y0=y1;x1=tx;y1=ty;}
  if (y0>y2){int16_t tx=x0,ty=y0;x0=x2;y0=y2;x2=tx;y2=ty;}
  if (y1>y2){int16_t tx=x1,ty=y1;x1=x2;y1=y2;x2=tx;y2=ty;}
  int16_t totalH = y2 - y0;
  for (int16_t row = y0; row <= y2; row++) {
    bool    s2    = row >= y1;
    int16_t segH  = s2 ? (y2-y1) : (y1-y0);
    if (segH == 0) continue;
    float   alpha = (float)(row - y0) / totalH;
    float   beta  = (float)(row - (s2 ? y1 : y0)) / segH;
    int16_t ax    = x0 + (int16_t)((x2-x0) * alpha);
    int16_t bx    = s2 ? x1 + (int16_t)((x2-x1) * beta)
                       : x0 + (int16_t)((x1-x0) * beta);
    if (ax > bx) { int16_t t=ax; ax=bx; bx=t; }
    vDrawFastHLine(ax, row, bx - ax + 1, c);
  }
}

// ── Text helpers ─────────────────────────────────────────────────
void vPrint(int16_t vx, int16_t vy, const __FlashStringHelper* str, uint8_t sz=1) {
  Adafruit_SSD1306& d = pageForY(vy);
  d.setTextSize(sz);
  d.setTextColor(SSD1306_WHITE);
  d.setCursor(vx, physY(vy));
  d.print(str);
}

void vPrintStr(int16_t vx, int16_t vy, const char* str, uint8_t sz=1) {
  Adafruit_SSD1306& d = pageForY(vy);
  d.setTextSize(sz);
  d.setTextColor(SSD1306_WHITE);
  d.setCursor(vx, physY(vy));
  d.print(str);
}

void vPrintUL(int16_t vx, int16_t vy, uint32_t v, uint8_t sz=1) {
  Adafruit_SSD1306& d = pageForY(vy);
  d.setTextSize(sz);
  d.setTextColor(SSD1306_WHITE);
  d.setCursor(vx, physY(vy));
  d.print(v);
}
