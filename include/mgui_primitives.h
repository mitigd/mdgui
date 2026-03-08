#pragma once

#include <stdint.h>

class MGuiFont {
public:
  uint8_t *atlas_raw;
  MGuiFont();
  int drawChar(unsigned char c, int x, int y, int colorIdx);
  int measureTextWidth(const char *s) const;
  void drawText(const char *s, char *d, int x, int y, int colorIdx);
  void drawText(const char *s, char *d, int x, int y);
};

extern MGuiFont *mgui_fonts[10];

extern "C" {
void mgui_fill_rect_idx(char *d, int idx, int x, int y, int w, int h);
void mgui_draw_hline_idx(char *d, int idx, int x1, int y, int x2);
void mgui_draw_vline_idx(char *d, int idx, int x, int y1, int y2);
void mgui_draw_frame_idx(char *d, int idx, int x1, int y1, int x2, int y2);
}

