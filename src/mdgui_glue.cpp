#include "mdgui_primitives.h"
#include "mdgui_c.h"
#include "mdgui_font4x6.h"
#include "mdgui_font5x7.h"
#include "mdgui_font6x8.h"
#include "mdgui_font8x8.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include <algorithm>
#include <fstream>
#include <new>
#include <vector>
#include <string.h>

// Renderer/palette glue for MDGUI primitives.

struct MDGUI_FileFontData {
  std::vector<unsigned char> file_bytes;
  std::vector<unsigned char> bitmap;
  stbtt_bakedchar baked_chars[96] = {};
  int atlas_w = 0;
  int atlas_h = 0;
  int line_height = 0;
  float baseline = 0.0f;
  bool ready = false;
};

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
                     unsigned char color_idx, int scale = 1, int glyph_w = 8,
                     int glyph_h = 8) {
  if (!backend_ready())
    return;
  if (scale < 1)
    scale = 1;
  const Color c = palette_color(color_idx);
  const unsigned char alpha = apply_alpha_mod(c.a);
  for (int py = 0; py < glyph_h; py++) {
    const unsigned char row = glyph[py];
    int px = 0;
    while (px < glyph_w) {
      while (px < glyph_w && !(row & (1u << px)))
        ++px;
      if (px >= glyph_w)
        break;

      const int run_start = px;
      while (px < glyph_w && (row & (1u << px)))
        ++px;

      const int dx = x + run_start * scale;
      const int dy = y + py * scale;
      const int run_width = (px - run_start) * scale;
      if (scale == 1 && run_width == 1) {
        g_backend.draw_point_rgba(g_backend.user_data, c.r, c.g, c.b, alpha, dx,
                                  dy);
      } else {
        g_backend.fill_rect_rgba(g_backend.user_data, c.r, c.g, c.b, alpha, dx,
                                 dy, run_width, scale);
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

bool load_file_bytes(const char *path, std::vector<unsigned char> &out) {
  if (!path || !*path)
    return false;
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open())
    return false;
  const std::streamsize size = file.tellg();
  if (size <= 0)
    return false;
  file.seekg(0, std::ios::beg);
  out.resize((size_t)size);
  return file.read((char *)out.data(), size).good();
}

MDGUI_FileFontData *create_file_font_data(const char *path, float pixel_height) {
  if (!path || !*path)
    return nullptr;

  auto *data = new (std::nothrow) MDGUI_FileFontData();
  if (!data)
    return nullptr;
  if (pixel_height < 8.0f)
    pixel_height = 8.0f;
  if (!load_file_bytes(path, data->file_bytes)) {
    delete data;
    return nullptr;
  }

  const int atlas_sizes[] = {256, 512, 1024};
  int bake_result = 0;
  for (int atlas_size : atlas_sizes) {
    data->atlas_w = atlas_size;
    data->atlas_h = atlas_size;
    data->bitmap.assign((size_t)atlas_size * (size_t)atlas_size, 0);
    memset(data->baked_chars, 0, sizeof(data->baked_chars));
    bake_result = stbtt_BakeFontBitmap(data->file_bytes.data(), 0, pixel_height,
                                       data->bitmap.data(), atlas_size,
                                       atlas_size, 32, 96, data->baked_chars);
    if (bake_result > 0)
      break;
  }
  if (bake_result <= 0) {
    delete data;
    return nullptr;
  }

  stbtt_fontinfo font_info;
  if (!stbtt_InitFont(&font_info, data->file_bytes.data(),
                      stbtt_GetFontOffsetForIndex(data->file_bytes.data(), 0))) {
    delete data;
    return nullptr;
  }

  int ascent = 0;
  int descent = 0;
  int line_gap = 0;
  stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);
  const float scale = stbtt_ScaleForPixelHeight(&font_info, pixel_height);
  data->baseline = (float)ascent * scale;
  data->line_height =
      std::max(1, (int)((float)(ascent - descent + line_gap) * scale + 0.5f));
  data->ready = true;
  return data;
}

int baked_char_index(unsigned char c) {
  if (c < 32 || c > 127)
    c = '?';
  return (int)c - 32;
}

void draw_file_font_quad(const MDGUI_FileFontData *data,
                         const stbtt_aligned_quad &q, unsigned char color_idx) {
  if (!data || !data->ready || !backend_ready())
    return;

  const Color c = palette_color(color_idx);
  const int src_x0 = std::max(0, (int)(q.s0 * (float)data->atlas_w + 0.5f));
  const int src_y0 = std::max(0, (int)(q.t0 * (float)data->atlas_h + 0.5f));
  const int src_x1 =
      std::min(data->atlas_w, (int)(q.s1 * (float)data->atlas_w + 0.5f));
  const int src_y1 =
      std::min(data->atlas_h, (int)(q.t1 * (float)data->atlas_h + 0.5f));
  const int dst_x0 = (int)(q.x0 + 0.5f);
  const int dst_y0 = (int)(q.y0 + 0.5f);

  for (int sy = src_y0; sy < src_y1; ++sy) {
    for (int sx = src_x0; sx < src_x1; ++sx) {
      const unsigned char alpha =
          data->bitmap[(size_t)sy * (size_t)data->atlas_w + (size_t)sx];
      if (alpha == 0)
        continue;
      const unsigned char final_alpha = apply_alpha_mod(
          (unsigned char)(((unsigned int)c.a * (unsigned int)alpha + 127u) /
                          255u));
      g_backend.fill_rect_rgba(g_backend.user_data, c.r, c.g, c.b, final_alpha,
                               dst_x0 + (sx - src_x0), dst_y0 + (sy - src_y0),
                               1, 1);
    }
  }
}

const unsigned char *glyph_for_char(unsigned char c, int glyph_w = 8,
                                    int glyph_h = 8) {
  if (c >= 128)
    c = '?';
  if (glyph_w == 6 && glyph_h == 8)
    return (const unsigned char *)mdgui_font6x8_basic[c];
  if (glyph_w == 5 && glyph_h == 7)
    return (const unsigned char *)mdgui_font5x7_basic[c];
  if (glyph_w == 4 && glyph_h == 6)
    return (const unsigned char *)mdgui_font4x6_basic[c];
  return (const unsigned char *)mdgui_font8x8_basic[c];
}

int glyph_width(unsigned char c, int glyph_w = 8, int glyph_h = 8) {
  const int space_width = std::max(2, glyph_w / 2 + 1);
  if (c == ' ')
    return space_width;

  const unsigned char *glyph = glyph_for_char(c, glyph_w, glyph_h);

  int max_x = -1;
  for (int py = 0; py < glyph_h; py++) {
    const unsigned char row = glyph[py];
    for (int px = 0; px < glyph_w; px++) {
      if (row & (1u << px)) {
        max_x = std::max(max_x, px);
      }
    }
  }
  if (max_x < 0)
    return space_width;
  return std::min(glyph_w + 1, max_x + 2);
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

MDGUI_Font::MDGUI_Font()
    : atlas_raw(nullptr), scale_(1), builtin_width_(8), builtin_height_(8),
      custom_(false), callbacks_(nullptr), file_font_(nullptr) {}

MDGUI_Font::MDGUI_Font(int builtin_scale)
    : MDGUI_Font(builtin_scale, 8, 8) {}

MDGUI_Font::MDGUI_Font(int builtin_scale, int builtin_width, int builtin_height)
    : atlas_raw(nullptr), scale_(std::max(1, builtin_scale)),
      builtin_width_(std::max(1, std::min(8, builtin_width))),
      builtin_height_(std::max(1, std::min(8, builtin_height))),
      custom_(false), callbacks_(nullptr), file_font_(nullptr) {}

MDGUI_Font::MDGUI_Font(const MDGUI_FontCallbacks &callbacks)
    : atlas_raw(nullptr), scale_(1), builtin_width_(8), builtin_height_(8),
      custom_(true),
      callbacks_(new (std::nothrow) MDGUI_FontCallbacks(callbacks)),
      file_font_(nullptr) {}

MDGUI_Font::MDGUI_Font(const char *file_path, float pixel_height)
    : atlas_raw(nullptr), scale_(1), builtin_width_(8), builtin_height_(8),
      custom_(false), callbacks_(nullptr),
      file_font_(create_file_font_data(file_path, pixel_height)) {}

MDGUI_Font::~MDGUI_Font() {
  delete callbacks_;
  delete file_font_;
}

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

  if (file_font_ && file_font_->ready) {
    float xpos = (float)x;
    float ypos = (float)y + file_font_->baseline;
    stbtt_aligned_quad quad{};
    stbtt_GetBakedQuad(file_font_->baked_chars, file_font_->atlas_w,
                       file_font_->atlas_h, baked_char_index(c), &xpos, &ypos,
                       &quad, 1);
    draw_file_font_quad(file_font_, quad,
                        (unsigned char)((colorIdx >= 0 && colorIdx <= 255)
                                            ? colorIdx
                                            : 15));
    return std::max(1, (int)(xpos - (float)x + 0.5f));
  }

  const unsigned char *glyph = glyph_for_char(c, builtin_width_, builtin_height_);
  const int xw = glyph_width(c, builtin_width_, builtin_height_) * scale_;

  if (builtin_width_ == 8 && builtin_height_ == 8 && scale_ == 1 &&
      colorIdx >= 0 && colorIdx <= 255) {
    if (!draw_glyph_fast(c, x + 1, y + 1, 0))
      draw_glyph_bits(glyph, x + 1, y + 1, 0);
    if (!draw_glyph_fast(c, x, y, (unsigned char)colorIdx))
      draw_glyph_bits(glyph, x, y, (unsigned char)colorIdx);
    return xw;
  }

  draw_glyph_bits(glyph, x + scale_, y + scale_, 0, scale_, builtin_width_,
                  builtin_height_);
  draw_glyph_bits(glyph, x, y,
                  (colorIdx >= 0 && colorIdx <= 255) ? colorIdx : 15, scale_,
                  builtin_width_, builtin_height_);
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
  if (file_font_ && file_font_->ready) {
    float width = 0.0f;
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
      const stbtt_bakedchar &glyph =
          file_font_->baked_chars[baked_char_index(*p)];
      width += glyph.xadvance;
    }
    return std::max(0, (int)(width + 0.5f));
  }
  int w = 0;
  for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
    w += glyph_width(*p, builtin_width_, builtin_height_) * scale_;
  }
  return w;
}

int MDGUI_Font::lineHeight() const {
  if (custom_) {
    if (!callbacks_ || !callbacks_->get_line_height)
      return 8;
    return callbacks_->get_line_height(callbacks_->user_data);
  }
  if (file_font_ && file_font_->ready)
    return file_font_->line_height;
  return builtin_height_ * scale_;
}

void MDGUI_Font::drawText(const char *s, char *d, int x, int y, int colorIdx) {
  if (!s)
    return;
  if (custom_) {
    if (callbacks_ && callbacks_->draw_text)
      callbacks_->draw_text(callbacks_->user_data, s, x, y, colorIdx);
    return;
  }
  if (file_font_ && file_font_->ready) {
    float xpos = (float)x;
    float ypos = (float)y + file_font_->baseline;
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
      stbtt_aligned_quad quad{};
      stbtt_GetBakedQuad(file_font_->baked_chars, file_font_->atlas_w,
                         file_font_->atlas_h, baked_char_index(*p), &xpos,
                         &ypos, &quad, 1);
      draw_file_font_quad(file_font_, quad,
                          (unsigned char)((colorIdx >= 0 && colorIdx <= 255)
                                              ? colorIdx
                                              : 15));
    }
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

MDGUI_Font *
mdgui_font_create_builtin_variant(MDGUI_BuiltinFontVariant variant, int scale) {
  switch (variant) {
  case MDGUI_BUILTIN_FONT_8X8:
    return new (std::nothrow) MDGUI_Font(scale, 8, 8);
  case MDGUI_BUILTIN_FONT_6X8:
    return new (std::nothrow) MDGUI_Font(scale, 6, 8);
  case MDGUI_BUILTIN_FONT_5X7:
    return new (std::nothrow) MDGUI_Font(scale, 5, 7);
  case MDGUI_BUILTIN_FONT_4X6:
    return new (std::nothrow) MDGUI_Font(scale, 4, 6);
  default:
    return nullptr;
  }
}

MDGUI_Font *mdgui_font_create_custom(const MDGUI_FontCallbacks *callbacks) {
  if (!callbacks)
    return nullptr;
  return new (std::nothrow) MDGUI_Font(*callbacks);
}

MDGUI_Font *mdgui_font_create_from_file(const char *path, float pixel_height) {
  MDGUI_Font *font = new (std::nothrow) MDGUI_Font(path, pixel_height);
  if (!font)
    return nullptr;
  if (font->lineHeight() <= 0) {
    delete font;
    return nullptr;
  }
  return font;
}

void mdgui_font_destroy(MDGUI_Font *font) { delete font; }

int mdgui_font_measure_text(const MDGUI_Font *font, const char *text) {
  return font ? font->measureTextWidth(text) : 0;
}

int mdgui_font_get_line_height(const MDGUI_Font *font) {
  return font ? font->lineHeight() : 0;
}
}


