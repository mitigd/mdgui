#include "mgui_primitives.h"
#include "mgui_c.h"
#include "mgui_font8x8.h"
#include <SDL3/SDL.h>
#include <string.h>
#include <vector>

// Renderer/palette glue for the NGUI primitives.

static SDL_Renderer *g_renderer = nullptr;
static SDL_Color g_palette[256];
static SDL_Color g_custom_palette[256];
static bool g_has_custom_palette[256];
static int g_theme_id = MGUI_THEME_DEFAULT;
static SDL_Texture *g_font_atlas = nullptr;
static SDL_Renderer *g_font_atlas_renderer = nullptr;
static int g_font_mod_r = -1;
static int g_font_mod_g = -1;
static int g_font_mod_b = -1;

static void set_theme_slot(int idx, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  g_palette[idx] = {r, g, b, a};
}

static void fill_base_palette() {
  static const uint8_t ega16[16][3] = {
      {0x00, 0x00, 0x00}, {0x00, 0x00, 0xaa}, {0x00, 0xaa, 0x00},
      {0x00, 0xaa, 0xaa}, {0xaa, 0x00, 0x00}, {0xaa, 0x00, 0xaa},
      {0xaa, 0x55, 0x00}, {0xaa, 0xaa, 0xaa}, {0x55, 0x55, 0x55},
      {0x55, 0x55, 0xff}, {0x55, 0xff, 0x55}, {0x55, 0xff, 0xff},
      {0xff, 0x55, 0x55}, {0xff, 0x55, 0xff}, {0xff, 0xff, 0x55},
      {0xff, 0xff, 0xff},
  };
  static const uint8_t cube6[6] = {0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff};

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
    const uint8_t v = (uint8_t)(8 + (i - 232) * 10);
    g_palette[i] = {v, v, v, 0xff};
  }
}

static void apply_theme_builtin(int theme_id) {
  switch (theme_id) {
  case MGUI_THEME_DARK:
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
    break;
  case MGUI_THEME_AMBER:
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
    break;
  case MGUI_THEME_GRAPHITE:
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
    break;
  case MGUI_THEME_MIDNIGHT:
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
    break;
  case MGUI_THEME_OLIVE:
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
    break;
  case MGUI_THEME_DEFAULT:
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
    break;
  }
}

static void rebuild_palette() {
  fill_base_palette();
  apply_theme_builtin(g_theme_id);
  for (int i = 0; i < 256; i++) {
    if (g_has_custom_palette[i])
      g_palette[i] = g_custom_palette[i];
  }
}

void mgui_init_renderer(SDL_Renderer *r) {
  if (g_font_atlas && g_font_atlas_renderer != r) {
    SDL_DestroyTexture(g_font_atlas);
    g_font_atlas = nullptr;
    g_font_atlas_renderer = nullptr;
    g_font_mod_r = -1;
    g_font_mod_g = -1;
    g_font_mod_b = -1;
  }
  g_renderer = r;
  rebuild_palette();
}

static void set_color(uint8_t idx) {
  if (!g_renderer)
    return;
  SDL_Color c = g_palette[idx];
  SDL_SetRenderDrawColor(g_renderer, c.r, c.g, c.b, 255);
}

extern "C" {
void mgui_set_theme(int theme_id) {
  if (theme_id < MGUI_THEME_DEFAULT || theme_id > MGUI_THEME_OLIVE)
    theme_id = MGUI_THEME_DEFAULT;
  g_theme_id = theme_id;
  rebuild_palette();
}

int mgui_get_theme(void) { return g_theme_id; }

void mgui_set_theme_color(int palette_index, unsigned char r, unsigned char g,
                          unsigned char b, unsigned char a) {
  if (palette_index < 0 || palette_index >= 256)
    return;
  g_custom_palette[palette_index] = {r, g, b, a};
  g_has_custom_palette[palette_index] = true;
  rebuild_palette();
}

void mgui_clear_theme_color(int palette_index) {
  if (palette_index < 0 || palette_index >= 256)
    return;
  g_has_custom_palette[palette_index] = false;
  rebuild_palette();
}

void mgui_clear_all_theme_colors(void) {
  memset(g_has_custom_palette, 0, sizeof(g_has_custom_palette));
  rebuild_palette();
}

void mgui_fill_rect_idx(char *d, int idx, int x, int y, int w, int h) {
  set_color((uint8_t)idx);
  SDL_FRect r = {(float)x, (float)y, (float)w, (float)h};
  SDL_RenderFillRect(g_renderer, &r);
}
void mgui_draw_hline_idx(char *d, int idx, int x1, int y, int x2) {
  set_color((uint8_t)idx);
  SDL_RenderLine(g_renderer, (float)x1, (float)y, (float)x2 - 1, (float)y);
}
void mgui_draw_vline_idx(char *d, int idx, int x, int y1, int y2) {
  set_color((uint8_t)idx);
  SDL_RenderLine(g_renderer, (float)x, (float)y1, (float)x, (float)y2 - 1);
}
// Draw a beveled frame using palette indices.
void mgui_draw_frame_idx(char *d, int idx, int x1, int y1, int x2, int y2) {
  // Outer black border
  set_color(0);
  SDL_FRect r = {(float)x1, (float)y1, (float)(x2 - x1), (float)(y2 - y1)};
  SDL_RenderRect(g_renderer, &r);

  // Inner highlights (Index 29)
  set_color(29);
  SDL_RenderLine(g_renderer, (float)x1 + 1, (float)y1 + 1, (float)x2 - 2,
                 (float)y1 + 1);
  SDL_RenderLine(g_renderer, (float)x1 + 1, (float)y1 + 1, (float)x1 + 1,
                 (float)y2 - 2);

  // Inner shadows (Index 23)
  set_color(23);
  SDL_RenderLine(g_renderer, (float)x2 - 2, (float)y1 + 1, (float)x2 - 2,
                 (float)y2 - 2);
  SDL_RenderLine(g_renderer, (float)x1 + 1, (float)y2 - 2, (float)x2 - 2,
                 (float)y2 - 2);
}
}

// Bitmap font implementation.

MGuiFont *mgui_fonts[10] = {nullptr};

MGuiFont::MGuiFont() { atlas_raw = nullptr; }

static const uint8_t *glyph_for_char(unsigned char c) {
  if (c >= 128) {
    c = '?';
  }
  return (const uint8_t *)mgui_font8x8_basic[c];
}

static int glyph_width(unsigned char c) {
  if (c == ' ')
    return 4;
  const uint8_t *glyph = glyph_for_char(c);
  int max_x = -1;
  for (int py = 0; py < 8; py++) {
    const uint8_t row = glyph[py];
    for (int px = 0; px < 8; px++) {
      if (row & (1u << px)) {
        if (px > max_x)
          max_x = px;
      }
    }
  }
  if (max_x < 0)
    return 4;
  return max_x + 2; // one pixel of spacing for a readable bitmap style
}

static void draw_glyph_bits(const uint8_t *glyph, int x, int y, uint8_t color_idx) {
  set_color(color_idx);
  for (int py = 0; py < 8; py++) {
    const uint8_t row = glyph[py];
    for (int px = 0; px < 8; px++) {
      if (row & (1u << px)) {
        SDL_RenderPoint(g_renderer, (float)(x + px), (float)(y + py));
      }
    }
  }
}

static bool ensure_font_atlas() {
  if (!g_renderer)
    return false;
  if (g_font_atlas && g_font_atlas_renderer == g_renderer)
    return true;

  const int atlas_cols = 16;
  const int atlas_rows = 8;
  const int glyph_w = 8;
  const int glyph_h = 8;
  const int atlas_w = atlas_cols * glyph_w;
  const int atlas_h = atlas_rows * glyph_h;

  std::vector<uint32_t> pixels((size_t)atlas_w * (size_t)atlas_h, 0u);
  for (int ch = 0; ch < 128; ++ch) {
    const uint8_t *glyph = glyph_for_char((unsigned char)ch);
    const int gx = (ch % atlas_cols) * glyph_w;
    const int gy = (ch / atlas_cols) * glyph_h;
    for (int py = 0; py < glyph_h; ++py) {
      const uint8_t row = glyph[py];
      for (int px = 0; px < glyph_w; ++px) {
        if (row & (1u << px)) {
          const int ax = gx + px;
          const int ay = gy + py;
          pixels[(size_t)ay * (size_t)atlas_w + (size_t)ax] = 0xffffffffu;
        }
      }
    }
  }

  SDL_Texture *tex =
      SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_STATIC, atlas_w, atlas_h);
  if (!tex)
    return false;
  SDL_UpdateTexture(tex, nullptr, pixels.data(), atlas_w * (int)sizeof(uint32_t));
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);

  g_font_atlas = tex;
  g_font_atlas_renderer = g_renderer;
  g_font_mod_r = -1;
  g_font_mod_g = -1;
  g_font_mod_b = -1;
  return true;
}

static bool draw_glyph_atlas(unsigned char c, int x, int y, uint8_t color_idx) {
  if (!ensure_font_atlas())
    return false;

  const SDL_Color col = g_palette[color_idx];
  if ((int)col.r != g_font_mod_r || (int)col.g != g_font_mod_g ||
      (int)col.b != g_font_mod_b) {
    SDL_SetTextureColorMod(g_font_atlas, col.r, col.g, col.b);
    g_font_mod_r = col.r;
    g_font_mod_g = col.g;
    g_font_mod_b = col.b;
  }

  const float sx = (float)((c % 16) * 8);
  const float sy = (float)((c / 16) * 8);
  SDL_FRect src = {sx, sy, 8.0f, 8.0f};
  SDL_FRect dst = {(float)x, (float)y, 8.0f, 8.0f};
  SDL_RenderTexture(g_renderer, g_font_atlas, &src, &dst);
  return true;
}

int MGuiFont::drawChar(unsigned char c, int x, int y, int colorIdx) {
  if (!g_renderer)
    return 6;

  const uint8_t *glyph = glyph_for_char(c);
  const int xw = glyph_width(c);

  if (colorIdx >= 0 && colorIdx <= 255) {
    if (!draw_glyph_atlas(c, x + 1, y + 1, 0))
      draw_glyph_bits(glyph, x + 1, y + 1, 0);
    if (!draw_glyph_atlas(c, x, y, (uint8_t)colorIdx))
      draw_glyph_bits(glyph, x, y, (uint8_t)colorIdx);
    return xw;
  }

  // Default style uses a simple two-tone treatment.
  if (!draw_glyph_atlas(c, x + 1, y + 1, 0))
    draw_glyph_bits(glyph, x + 1, y + 1, 0);
  if (!draw_glyph_atlas(c, x, y, 15))
    draw_glyph_bits(glyph, x, y, 15);
  return xw;
}

int MGuiFont::measureTextWidth(const char *s) const {
  if (!s)
    return 0;
  int w = 0;
  for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
    w += glyph_width(*p);
  }
  return w;
}

void MGuiFont::drawText(const char *s, char *d, int x, int y, int colorIdx) {
  if (!s)
    return;
  for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
    x += drawChar(*p, x, y, colorIdx);
  }
}
void MGuiFont::drawText(const char *s, char *d, int x, int y) {
  drawText(s, d, x, y, -1); // Use default font palette indices.
}


