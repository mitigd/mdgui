#pragma once

#include <stdint.h>

struct MDGUI_RenderBackend;
struct MDGUI_FontCallbacks;
struct MDGUI_FileFontData;

class MDGUI_Font {
public:
  uint8_t *atlas_raw;
  MDGUI_Font();
  explicit MDGUI_Font(int builtin_scale);
  explicit MDGUI_Font(const MDGUI_FontCallbacks &callbacks);
  explicit MDGUI_Font(const char *file_path, float pixel_height);
  ~MDGUI_Font();
  int drawChar(unsigned char c, int x, int y, int colorIdx);
  int measureTextWidth(const char *s) const;
  int lineHeight() const;
  void drawText(const char *s, char *d, int x, int y, int colorIdx);
  void drawText(const char *s, char *d, int x, int y);

private:
  int scale_;
  bool custom_;
  MDGUI_FontCallbacks *callbacks_;
  MDGUI_FileFontData *file_font_;
};

using MDGuiFont = MDGUI_Font;

extern MDGUI_Font *mdgui_fonts[10];

extern "C" {
void mdgui_bind_backend(const MDGUI_RenderBackend *backend);
void mdgui_backend_begin_frame(void);
void mdgui_backend_end_frame(void);
void mdgui_backend_set_clip_rect(int enabled, int x, int y, int w, int h);
void mdgui_backend_set_alpha_mod(unsigned char alpha);
unsigned char mdgui_backend_get_alpha_mod(void);
int mdgui_backend_get_render_size(int *out_w, int *out_h);
unsigned long long mdgui_backend_get_ticks_ms(void);

void mdgui_fill_rect_idx(char *d, int idx, int x, int y, int w, int h);
void mdgui_draw_hline_idx(char *d, int idx, int x1, int y, int x2);
void mdgui_draw_vline_idx(char *d, int idx, int x, int y1, int y2);
void mdgui_draw_frame_idx(char *d, int idx, int x1, int y1, int x2, int y2);
void mdgui_draw_line_idx(char *d, int idx, int x1, int y1, int x2, int y2);
}
