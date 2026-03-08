#include "mdgui_primitives.h"
#include "mdgui_c.h"
#include "mdgui_font8x8.h"
#include <string.h>

// Renderer/palette glue for MDGUI primitives.

namespace {
MDGUI_RenderBackend g_backend = {};
bool g_has_backend = false;

struct Color {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
};

Color g_palette[256];
Color g_custom_palette[256];
bool g_has_custom_palette[256];
int g_theme_id = MDGUI_THEME_DEFAULT;

bool backend_ready() {
  return g_has_backend && g_backend.set_clip_rect && g_backend.fill_rect_rgba &&
         g_backend.draw_line_rgba && g_backend.draw_point_rgba &&
         g_backend.get_render_size && g_backend.get_ticks_ms;
}

void set_theme_slot(int idx, unsigned char r, unsigned char g, unsigned char b,
                    unsigned char a) {
  g_palette[idx] = {r, g, b, a};
}

void fill_base_palette() {
  static const unsigned char ega16[16][3] = {
      {0x00, 0x00, 0x00}, {0x00, 0x00, 0xaa}, {0x00, 0xaa, 0x00},
      {0x00, 0xaa, 0xaa}, {0xaa, 0x00, 0x00}, {0xaa, 0x00, 0xaa},
      {0xaa, 0x55, 0x00}, {0xaa, 0xaa, 0xaa}, {0x55, 0x55, 0x55},
      {0x55, 0x55, 0xff}, {0x55, 0xff, 0x55}, {0x55, 0xff, 0xff},
      {0xff, 0x55, 0x55}, {0xff, 0x55, 0xff}, {0xff, 0xff, 0x55},
      {0xff, 0xff, 0xff},
  };
  static const unsigned char cube6[6] = {0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff};

  for (int i = 0; i < 16; i++) {
    g_palette[i] = {ega16[i][0], ega16[i][1], ega16[i][2], 0xff};
  }
  for (int i = 16; i < 232; i++) {
    const int n = i - 16;
    const int r = (n / 36) % 6;
    const int g = (n / 6) % 6;
    const int b = n % 6;
    g_palette[i] = {cube6[r], cube6[g], cube6[b], 0xff};
  }
  for (int i = 232; i < 256; i++) {
    const unsigned char v = (unsigned char)(8 + (i - 232) * 10);
    g_palette[i] = {v, v, v, 0xff};
  }
}

void apply_theme_builtin(int theme_id) {
  switch (theme_id) {
  case MDGUI_THEME_DARK:
    set_theme_slot(15, 0xdf, 0xdf, 0xdf, 0xff);
    set_theme_slot(25, 0x3a, 0x3a, 0x3a, 0xff);
    set_theme_slot(141, 0x1d, 0x1f, 0x26, 0xff);
    set_theme_slot(143, 0x2b, 0x2f, 0x3a, 0xff);
    set_theme_slot(139, 0x5d, 0x8b, 0xff, 0xff);
    set_theme_slot(241, 0x2b, 0x2f, 0x3a, 0xff);
    set_theme_slot(246, 0xdf, 0xdf, 0xdf, 0xff);
    set_theme_slot(243, 0x23, 0x26, 0x30, 0xff);
    set_theme_slot(244, 0x36, 0x3d, 0x4d, 0xff);
    set_theme_slot(245, 0x3a, 0x3a, 0x3a, 0xff);
    set_theme_slot(247, 0x5d, 0x8b, 0xff, 0xff); // CLR_ACCENT (blue)
    set_theme_slot(248, 0x3a, 0x3a, 0x3a, 0xff); // CLR_WINDOW_BORDER
    break;
  case MDGUI_THEME_AMBER:
    set_theme_slot(15, 0xf3, 0xe2, 0xb2, 0xff);
    set_theme_slot(25, 0x4c, 0x3a, 0x1f, 0xff);
    set_theme_slot(141, 0x1f, 0x17, 0x0f, 0xff);
    set_theme_slot(143, 0x32, 0x23, 0x13, 0xff);
    set_theme_slot(139, 0xbc, 0x8a, 0x2f, 0xff);
    set_theme_slot(241, 0x32, 0x23, 0x13, 0xff);
    set_theme_slot(246, 0xf3, 0xe2, 0xb2, 0xff);
    set_theme_slot(243, 0x28, 0x1d, 0x12, 0xff);
    set_theme_slot(244, 0x6e, 0x4d, 0x1e, 0xff);
    set_theme_slot(245, 0x4c, 0x3a, 0x1f, 0xff);
    set_theme_slot(247, 0xbc, 0x8a, 0x2f, 0xff); // CLR_ACCENT (gold)
    set_theme_slot(248, 0x4c, 0x3a, 0x1f, 0xff); // CLR_WINDOW_BORDER
    break;
  case MDGUI_THEME_GRAPHITE:
    set_theme_slot(15, 0xd8, 0xdd, 0xe6, 0xff);
    set_theme_slot(25, 0x3f, 0x45, 0x52, 0xff);
    set_theme_slot(141, 0x26, 0x2b, 0x33, 0xff);
    set_theme_slot(143, 0x34, 0x3a, 0x46, 0xff);
    set_theme_slot(139, 0x66, 0x8a, 0xc4, 0xff);
    set_theme_slot(241, 0x34, 0x3a, 0x46, 0xff);
    set_theme_slot(246, 0xd8, 0xdd, 0xe6, 0xff);
    set_theme_slot(243, 0x2b, 0x31, 0x3b, 0xff);
    set_theme_slot(244, 0x3f, 0x4f, 0x67, 0xff);
    set_theme_slot(245, 0x3f, 0x45, 0x52, 0xff);
    set_theme_slot(247, 0x66, 0x8a, 0xc4, 0xff); // CLR_ACCENT (steel blue)
    set_theme_slot(248, 0x3f, 0x45, 0x52, 0xff); // CLR_WINDOW_BORDER
    break;
  case MDGUI_THEME_MIDNIGHT:
    set_theme_slot(15, 0xbf, 0xd8, 0xff, 0xff);
    set_theme_slot(25, 0x1c, 0x2a, 0x44, 0xff);
    set_theme_slot(141, 0x08, 0x12, 0x24, 0xff);
    set_theme_slot(143, 0x0d, 0x1f, 0x3a, 0xff);
    set_theme_slot(139, 0x4b, 0x90, 0xff, 0xff);
    set_theme_slot(241, 0x0d, 0x1f, 0x3a, 0xff);
    set_theme_slot(246, 0xbf, 0xd8, 0xff, 0xff);
    set_theme_slot(243, 0x0b, 0x18, 0x30, 0xff);
    set_theme_slot(244, 0x13, 0x3a, 0x74, 0xff);
    set_theme_slot(245, 0x1c, 0x2a, 0x44, 0xff);
    set_theme_slot(247, 0x4b, 0x90, 0xff, 0xff); // CLR_ACCENT (bright blue)
    set_theme_slot(248, 0x1c, 0x2a, 0x44, 0xff); // CLR_WINDOW_BORDER
    break;
  case MDGUI_THEME_OLIVE:
    set_theme_slot(15, 0xd9, 0xe0, 0xc8, 0xff);
    set_theme_slot(25, 0x36, 0x43, 0x30, 0xff);
    set_theme_slot(141, 0x1d, 0x27, 0x1a, 0xff);
    set_theme_slot(143, 0x2a, 0x36, 0x23, 0xff);
    set_theme_slot(139, 0x7d, 0x9a, 0x57, 0xff);
    set_theme_slot(241, 0x2a, 0x36, 0x23, 0xff);
    set_theme_slot(246, 0xd9, 0xe0, 0xc8, 0xff);
    set_theme_slot(243, 0x23, 0x2e, 0x1f, 0xff);
    set_theme_slot(244, 0x43, 0x5a, 0x31, 0xff);
    set_theme_slot(245, 0x36, 0x43, 0x30, 0xff);
    set_theme_slot(247, 0x7d, 0x9a, 0x57, 0xff); // CLR_ACCENT (olive green)
    set_theme_slot(248, 0x36, 0x43, 0x30, 0xff); // CLR_WINDOW_BORDER
    break;
  case MDGUI_THEME_DEFAULT:
  default:
    set_theme_slot(15, 0xc0, 0xc0, 0xc0, 0xff);
    set_theme_slot(25, 0x70, 0x70, 0x70, 0xff);
    set_theme_slot(141, 0x40, 0x40, 0xfc, 0xff);
    set_theme_slot(143, 0x00, 0x00, 0x7c, 0xff);
    set_theme_slot(139, 0x00, 0x00, 0x7c, 0xff);
    set_theme_slot(241, 0x00, 0x00, 0x7c, 0xff);
    set_theme_slot(246, 0xc0, 0xc0, 0xc0, 0xff);
    set_theme_slot(243, 0x40, 0x40, 0xfc, 0xff);
    set_theme_slot(244, 0x00, 0x04, 0xfc, 0xff);
    set_theme_slot(245, 0x70, 0x70, 0x70, 0xff);
    set_theme_slot(247, 0x00, 0xaa, 0x00, 0xff); // CLR_ACCENT (green)
    set_theme_slot(248, 0x70, 0x70, 0x70, 0xff); // CLR_WINDOW_BORDER (gray)
    break;
  }
}

void rebuild_palette() {
  fill_base_palette();
  apply_theme_builtin(g_theme_id);
  for (int i = 0; i < 256; i++) {
    if (g_has_custom_palette[i])
      g_palette[i] = g_custom_palette[i];
  }
}

Color palette_color(unsigned char idx) { return g_palette[idx]; }

void draw_glyph_bits(const unsigned char *glyph, int x, int y,
                    unsigned char color_idx) {
  if (!backend_ready())
    return;
  const Color c = palette_color(color_idx);
  for (int py = 0; py < 8; py++) {
    const unsigned char row = glyph[py];
    for (int px = 0; px < 8; px++) {
      if (row & (1u << px)) {
        g_backend.draw_point_rgba(g_backend.user_data, c.r, c.g, c.b, c.a,
                                  x + px, y + py);
      }
    }
  }
}

bool draw_glyph_fast(unsigned char glyph, int x, int y, unsigned char color_idx) {
  if (!backend_ready() || !g_backend.draw_glyph_rgba)
    return false;
  const Color c = palette_color(color_idx);
  return g_backend.draw_glyph_rgba(g_backend.user_data, glyph, x, y, c.r, c.g,
                                   c.b, c.a) != 0;
}

const unsigned char *glyph_for_char(unsigned char c) {
  if (c >= 128)
    c = '?';
  return (const unsigned char *)mdgui_font8x8_basic[c];
}

int glyph_width(unsigned char c) {
  if (c == ' ')
    return 4;
  const unsigned char *glyph = glyph_for_char(c);
  int max_x = -1;
  for (int py = 0; py < 8; py++) {
    const unsigned char row = glyph[py];
    for (int px = 0; px < 8; px++) {
      if (row & (1u << px)) {
        if (px > max_x)
          max_x = px;
      }
    }
  }
  if (max_x < 0)
    return 4;
  return max_x + 2;
}
} // namespace

extern "C" {
void mdgui_bind_backend(const MDGUI_RenderBackend *backend) {
  if (!backend) {
    g_has_backend = false;
    memset(&g_backend, 0, sizeof(g_backend));
    rebuild_palette();
    return;
  }
  g_backend = *backend;
  g_has_backend = true;
  rebuild_palette();
}

void mdgui_backend_begin_frame(void) {
  if (g_has_backend && g_backend.begin_frame)
    g_backend.begin_frame(g_backend.user_data);
}

void mdgui_backend_end_frame(void) {
  if (g_has_backend && g_backend.end_frame)
    g_backend.end_frame(g_backend.user_data);
}

void mdgui_backend_set_clip_rect(int enabled, int x, int y, int w, int h) {
  if (!backend_ready())
    return;
  g_backend.set_clip_rect(g_backend.user_data, enabled, x, y, w, h);
}

int mdgui_backend_get_render_size(int *out_w, int *out_h) {
  if (!backend_ready()) {
    if (out_w)
      *out_w = 320;
    if (out_h)
      *out_h = 240;
    return 0;
  }
  return g_backend.get_render_size(g_backend.user_data, out_w, out_h);
}

unsigned long long mdgui_backend_get_ticks_ms(void) {
  if (!backend_ready())
    return 0;
  return g_backend.get_ticks_ms(g_backend.user_data);
}

void mdgui_set_theme(int theme_id) {
  if (theme_id < MDGUI_THEME_DEFAULT || theme_id > MDGUI_THEME_OLIVE)
    theme_id = MDGUI_THEME_DEFAULT;
  g_theme_id = theme_id;
  rebuild_palette();
}

int mdgui_get_theme(void) { return g_theme_id; }

void mdgui_set_theme_color(int palette_index, unsigned char r, unsigned char g,
                          unsigned char b, unsigned char a) {
  if (palette_index < 0 || palette_index >= 256)
    return;
  g_custom_palette[palette_index] = {r, g, b, a};
  g_has_custom_palette[palette_index] = true;
  rebuild_palette();
}

void mdgui_clear_theme_color(int palette_index) {
  if (palette_index < 0 || palette_index >= 256)
    return;
  g_has_custom_palette[palette_index] = false;
  rebuild_palette();
}

void mdgui_clear_all_theme_colors(void) {
  memset(g_has_custom_palette, 0, sizeof(g_has_custom_palette));
  rebuild_palette();
}

void mdgui_fill_rect_idx(char *d, int idx, int x, int y, int w, int h) {
  if (!backend_ready())
    return;
  const Color c = palette_color((unsigned char)idx);
  g_backend.fill_rect_rgba(g_backend.user_data, c.r, c.g, c.b, c.a, x, y, w,
                           h);
}

void mdgui_draw_hline_idx(char *d, int idx, int x1, int y, int x2) {
  if (!backend_ready())
    return;
  const Color c = palette_color((unsigned char)idx);
  g_backend.draw_line_rgba(g_backend.user_data, c.r, c.g, c.b, c.a, x1, y,
                           x2 - 1, y);
}

void mdgui_draw_vline_idx(char *d, int idx, int x, int y1, int y2) {
  if (!backend_ready())
    return;
  const Color c = palette_color((unsigned char)idx);
  g_backend.draw_line_rgba(g_backend.user_data, c.r, c.g, c.b, c.a, x, y1, x,
                           y2 - 1);
}

// Draw a beveled frame using palette indices.
void mdgui_draw_frame_idx(char *d, int idx, int x1, int y1, int x2, int y2) {
  mdgui_draw_hline_idx(d, 0, x1, y1, x2);
  mdgui_draw_hline_idx(d, 0, x1, y2 - 1, x2);
  mdgui_draw_vline_idx(d, 0, x1, y1, y2);
  mdgui_draw_vline_idx(d, 0, x2 - 1, y1, y2);

  mdgui_draw_hline_idx(d, 29, x1 + 1, y1 + 1, x2 - 1);
  mdgui_draw_vline_idx(d, 29, x1 + 1, y1 + 1, y2 - 1);

  mdgui_draw_vline_idx(d, 23, x2 - 2, y1 + 1, y2 - 1);
  mdgui_draw_hline_idx(d, 23, x1 + 1, y2 - 2, x2 - 1);
}

// Draw an arbitrary line using Bresenham-like approximation
void mdgui_draw_line_idx(char *d, int idx, int x1, int y1, int x2, int y2) {
  if (!backend_ready())
    return;
  const Color c = palette_color((unsigned char)idx);
  g_backend.draw_line_rgba(g_backend.user_data, c.r, c.g, c.b, c.a, x1, y1,
                           x2, y2);
}
}

MDGuiFont *mdgui_fonts[10] = {nullptr};

MDGuiFont::MDGuiFont() { atlas_raw = nullptr; }

int MDGuiFont::drawChar(unsigned char c, int x, int y, int colorIdx) {
  if (!backend_ready())
    return 6;

  const unsigned char *glyph = glyph_for_char(c);
  const int xw = glyph_width(c);

  if (colorIdx >= 0 && colorIdx <= 255) {
    if (!draw_glyph_fast(c, x + 1, y + 1, 0))
      draw_glyph_bits(glyph, x + 1, y + 1, 0);
    if (!draw_glyph_fast(c, x, y, (unsigned char)colorIdx))
      draw_glyph_bits(glyph, x, y, (unsigned char)colorIdx);
    return xw;
  }

  if (!draw_glyph_fast(c, x + 1, y + 1, 0))
    draw_glyph_bits(glyph, x + 1, y + 1, 0);
  if (!draw_glyph_fast(c, x, y, 15))
    draw_glyph_bits(glyph, x, y, 15);
  return xw;
}

int MDGuiFont::measureTextWidth(const char *s) const {
  if (!s)
    return 0;
  int w = 0;
  for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
    w += glyph_width(*p);
  }
  return w;
}

void MDGuiFont::drawText(const char *s, char *d, int x, int y, int colorIdx) {
  if (!s)
    return;
  for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
    x += drawChar(*p, x, y, colorIdx);
  }
}
void MDGuiFont::drawText(const char *s, char *d, int x, int y) {
  drawText(s, d, x, y, -1); // Use default font palette indices.
}


