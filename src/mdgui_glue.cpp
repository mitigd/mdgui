#include "mdgui_primitives.h"
#include "mdgui_c.h"
#include "mdgui_font8x8.h"
#include "mdgui_primitives.h"
#include <algorithm>
#include <new>
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
unsigned char g_alpha_mod = 0xff;

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
    set_theme_slot(23, 0x55, 0x55, 0x55, 0xff); // CLR_BUTTON_LIGHT
    set_theme_slot(25, 0x3a, 0x3a, 0x3a, 0xff); // CLR_BUTTON_SURFACE
    set_theme_slot(27, 0x1d, 0x1d, 0x1d, 0xff); // CLR_BUTTON_DARK
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
    set_theme_slot(23, 0x6e, 0x55, 0x2f, 0xff); // CLR_BUTTON_LIGHT
    set_theme_slot(25, 0x4c, 0x3a, 0x1f, 0xff); // CLR_BUTTON_SURFACE
    set_theme_slot(27, 0x32, 0x23, 0x0f, 0xff); // CLR_BUTTON_DARK
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
    set_theme_slot(23, 0x5a, 0x60, 0x6d, 0xff); // CLR_BUTTON_LIGHT
    set_theme_slot(25, 0x3f, 0x45, 0x52, 0xff); // CLR_BUTTON_SURFACE
    set_theme_slot(27, 0x27, 0x2d, 0x37, 0xff); // CLR_BUTTON_DARK
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
    set_theme_slot(23, 0x2f, 0x3d, 0x57, 0xff); // CLR_BUTTON_LIGHT
    set_theme_slot(25, 0x1c, 0x2a, 0x44, 0xff); // CLR_BUTTON_SURFACE
    set_theme_slot(27, 0x09, 0x17, 0x31, 0xff); // CLR_BUTTON_DARK
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
    set_theme_slot(23, 0x4f, 0x5c, 0x49, 0xff); // CLR_BUTTON_LIGHT
    set_theme_slot(25, 0x36, 0x43, 0x30, 0xff); // CLR_BUTTON_SURFACE
    set_theme_slot(27, 0x1d, 0x2a, 0x17, 0xff); // CLR_BUTTON_DARK
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
    set_theme_slot(23, 0xa0, 0xa0, 0xa0, 0xff); // CLR_BUTTON_LIGHT
    set_theme_slot(25, 0x70, 0x70, 0x70, 0xff); // CLR_BUTTON_SURFACE
    set_theme_slot(27, 0x40, 0x40, 0x40, 0xff); // CLR_BUTTON_DARK
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

unsigned char apply_alpha_mod(unsigned char a) {
  return (unsigned char)(((unsigned int)a * (unsigned int)g_alpha_mod + 127u) /
                         255u);
}

void draw_glyph_bits(const unsigned char *glyph, int x, int y,
                     unsigned char color_idx, int scale = 1) {
  if (!backend_ready())
    return;
  if (scale < 1)
    scale = 1;
  const Color c = palette_color(color_idx);
  for (int py = 0; py < 8; py++) {
    const unsigned char row = glyph[py];
    for (int px = 0; px < 8; px++) {
      if (row & (1u << px)) {
        const int dx = x + px * scale;
        const int dy = y + py * scale;
        if (scale == 1) {
          g_backend.draw_point_rgba(g_backend.user_data, c.r, c.g, c.b,
                                    apply_alpha_mod(c.a), dx, dy);
        } else {
          g_backend.fill_rect_rgba(g_backend.user_data, c.r, c.g, c.b,
                                   apply_alpha_mod(c.a), dx, dy, scale,
                                   scale);
        }
      }
    }
  }
}

bool draw_glyph_fast(unsigned char glyph, int x, int y, unsigned char color_idx) {
  if (!backend_ready() || !g_backend.draw_glyph_rgba)
    return false;
  const Color c = palette_color(color_idx);
  return g_backend.draw_glyph_rgba(g_backend.user_data, glyph, x, y, c.r, c.g,
                                   c.b, apply_alpha_mod(c.a)) != 0;
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
    g_alpha_mod = 0xff;
    rebuild_palette();
    return;
  }
  g_backend = *backend;
  g_has_backend = true;
  g_alpha_mod = 0xff;
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

void mdgui_backend_set_alpha_mod(unsigned char alpha) { g_alpha_mod = alpha; }

unsigned char mdgui_backend_get_alpha_mod(void) { return g_alpha_mod; }

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

void mdgui_get_accent_color(unsigned char *out_r, unsigned char *out_g,
                            unsigned char *out_b) {
  if (!out_r || !out_g || !out_b)
    return;
  const Color c = palette_color(247); // CLR_ACCENT palette index
  *out_r = c.r;
  *out_g = c.g;
  *out_b = c.b;
}

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
  g_backend.fill_rect_rgba(g_backend.user_data, c.r, c.g, c.b,
                           apply_alpha_mod(c.a), x, y, w, h);
}

void mdgui_draw_hline_idx(char *d, int idx, int x1, int y, int x2) {
  if (!backend_ready())
    return;
  const Color c = palette_color((unsigned char)idx);
  g_backend.draw_line_rgba(g_backend.user_data, c.r, c.g, c.b,
                           apply_alpha_mod(c.a), x1, y,
                           x2 - 1, y);
}

void mdgui_draw_vline_idx(char *d, int idx, int x, int y1, int y2) {
  if (!backend_ready())
    return;
  const Color c = palette_color((unsigned char)idx);
  g_backend.draw_line_rgba(g_backend.user_data, c.r, c.g, c.b,
                           apply_alpha_mod(c.a), x, y1, x,
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
  g_backend.draw_line_rgba(g_backend.user_data, c.r, c.g, c.b,
                           apply_alpha_mod(c.a), x1, y1,
                           x2, y2);
}
}

MDGUI_Font *mdgui_fonts[10] = {nullptr};

MDGUI_Font::MDGUI_Font() : atlas_raw(nullptr), scale_(1), custom_(false),
                           callbacks_(nullptr) {}

MDGUI_Font::MDGUI_Font(int builtin_scale)
    : atlas_raw(nullptr), scale_(std::max(1, builtin_scale)), custom_(false),
      callbacks_(nullptr) {}

MDGUI_Font::MDGUI_Font(const MDGUI_FontCallbacks &callbacks)
    : atlas_raw(nullptr), scale_(1), custom_(true),
      callbacks_(new (std::nothrow) MDGUI_FontCallbacks(callbacks)) {}

MDGUI_Font::~MDGUI_Font() { delete callbacks_; }

int MDGUI_Font::drawChar(unsigned char c, int x, int y, int colorIdx) {
  if (!backend_ready())
    return 6 * scale_;

  if (custom_) {
    if (!callbacks_ || !callbacks_->draw_text)
      return 0;
    char buf[2] = {(char)c, '\0'};
    callbacks_->draw_text(callbacks_->user_data, buf, x, y, colorIdx);
    return measureTextWidth(buf);
  }

  const unsigned char *glyph = glyph_for_char(c);
  const int xw = glyph_width(c) * scale_;

  if (scale_ == 1 && colorIdx >= 0 && colorIdx <= 255) {
    if (!draw_glyph_fast(c, x + 1, y + 1, 0))
      draw_glyph_bits(glyph, x + 1, y + 1, 0);
    if (!draw_glyph_fast(c, x, y, (unsigned char)colorIdx))
      draw_glyph_bits(glyph, x, y, (unsigned char)colorIdx);
    return xw;
  }

  draw_glyph_bits(glyph, x + scale_, y + scale_, 0, scale_);
  draw_glyph_bits(glyph, x, y, (colorIdx >= 0 && colorIdx <= 255) ? colorIdx : 15,
                  scale_);
  return xw;
}

int MDGUI_Font::measureTextWidth(const char *s) const {
  if (!s)
    return 0;
  if (custom_) {
    if (!callbacks_ || !callbacks_->measure_text_width)
      return 0;
    return callbacks_->measure_text_width(callbacks_->user_data, s);
  }
  int w = 0;
  for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
    w += glyph_width(*p) * scale_;
  }
  return w;
}

int MDGUI_Font::lineHeight() const {
  if (custom_) {
    if (!callbacks_ || !callbacks_->get_line_height)
      return 8;
    return callbacks_->get_line_height(callbacks_->user_data);
  }
  return 8 * scale_;
}

void MDGUI_Font::drawText(const char *s, char *d, int x, int y, int colorIdx) {
  if (!s)
    return;
  if (custom_) {
    if (callbacks_ && callbacks_->draw_text)
      callbacks_->draw_text(callbacks_->user_data, s, x, y, colorIdx);
    return;
  }
  for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
    x += drawChar(*p, x, y, colorIdx);
  }
}
void MDGUI_Font::drawText(const char *s, char *d, int x, int y) {
  drawText(s, d, x, y, -1); // Use default font palette indices.
}

extern "C" {
MDGUI_Font *mdgui_font_create_builtin(int scale) {
  return new (std::nothrow) MDGUI_Font(scale);
}

MDGUI_Font *mdgui_font_create_custom(const MDGUI_FontCallbacks *callbacks) {
  if (!callbacks)
    return nullptr;
  return new (std::nothrow) MDGUI_Font(*callbacks);
}

void mdgui_font_destroy(MDGUI_Font *font) { delete font; }

int mdgui_font_measure_text(const MDGUI_Font *font, const char *text) {
  return font ? font->measureTextWidth(text) : 0;
}

int mdgui_font_get_line_height(const MDGUI_Font *font) {
  return font ? font->lineHeight() : 0;
}
}


