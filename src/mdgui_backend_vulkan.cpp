#include "mdgui_backends.h"

#include "mdgui_font8x8.h"

#include <cstring>
#include <new>
#include <vector>
#include <string.h>

struct ClipRect {
  int enabled;
  int x;
  int y;
  int w;
  int h;
};

struct MDGUI_VkBackend {
  void *user_data = nullptr;
  int (*get_render_size)(void *user_data, int *out_w, int *out_h) = nullptr;
  unsigned long long (*get_ticks_ms)(void *user_data) = nullptr;

  ClipRect clip = {0, 0, 0, 0, 0};
  std::vector<MDGUI_VkVertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<MDGUI_VkDrawCmd> commands;
  int display_w = 320;
  int display_h = 240;
};

namespace {

unsigned int pack_rgba(unsigned char r, unsigned char g, unsigned char b,
                       unsigned char a) {
  return ((unsigned int)a << 24) | ((unsigned int)b << 16) |
         ((unsigned int)g << 8) | (unsigned int)r;
}

bool same_clip(const ClipRect &a, const MDGUI_VkDrawCmd &b) {
  return a.enabled == b.clip_enabled && a.x == b.clip_x && a.y == b.clip_y &&
         a.w == b.clip_w && a.h == b.clip_h;
}

void ensure_command(MDGUI_VkBackend *backend, int textured) {
  if (!backend)
    return;

  if (!backend->commands.empty()) {
    MDGUI_VkDrawCmd &last = backend->commands.back();
    if (last.textured == textured && same_clip(backend->clip, last))
      return;
  }

  MDGUI_VkDrawCmd cmd = {};
  cmd.index_offset = (unsigned int)backend->indices.size();
  cmd.index_count = 0;
  cmd.textured = textured;
  cmd.clip_enabled = backend->clip.enabled;
  cmd.clip_x = backend->clip.x;
  cmd.clip_y = backend->clip.y;
  cmd.clip_w = backend->clip.w;
  cmd.clip_h = backend->clip.h;
  backend->commands.push_back(cmd);
}

void push_tri(MDGUI_VkBackend *backend, unsigned int i0, unsigned int i1,
              unsigned int i2) {
  if (!backend || backend->commands.empty())
    return;
  backend->indices.push_back(i0);
  backend->indices.push_back(i1);
  backend->indices.push_back(i2);
  backend->commands.back().index_count += 3;
}

void push_quad(MDGUI_VkBackend *backend, float x, float y, float w, float h,
               unsigned int rgba, float u0, float v0, float u1, float v1,
               int textured) {
  if (!backend || w <= 0.0f || h <= 0.0f)
    return;

  ensure_command(backend, textured);

  const unsigned int base = (unsigned int)backend->vertices.size();
  backend->vertices.push_back({x, y, u0, v0, rgba});
  backend->vertices.push_back({x + w, y, u1, v0, rgba});
  backend->vertices.push_back({x + w, y + h, u1, v1, rgba});
  backend->vertices.push_back({x, y + h, u0, v1, rgba});

  push_tri(backend, base + 0u, base + 1u, base + 2u);
  push_tri(backend, base + 0u, base + 2u, base + 3u);
}

void vk_begin_frame(void *user_data) {
  MDGUI_VkBackend *backend = (MDGUI_VkBackend *)user_data;
  if (!backend)
    return;
  backend->vertices.clear();
  backend->indices.clear();
  backend->commands.clear();
  backend->clip = {0, 0, 0, 0, 0};

  int w = 320;
  int h = 240;
  if (backend->get_render_size)
    backend->get_render_size(backend->user_data, &w, &h);
  backend->display_w = (w > 0) ? w : 320;
  backend->display_h = (h > 0) ? h : 240;
}

void vk_set_clip_rect(void *user_data, int enabled, int x, int y, int w,
                      int h) {
  MDGUI_VkBackend *backend = (MDGUI_VkBackend *)user_data;
  if (!backend)
    return;
  backend->clip.enabled = enabled ? 1 : 0;
  backend->clip.x = x;
  backend->clip.y = y;
  backend->clip.w = w;
  backend->clip.h = h;
}

void vk_fill_rect_rgba(void *user_data, unsigned char r, unsigned char g,
                       unsigned char b, unsigned char a, int x, int y, int w,
                       int h) {
  MDGUI_VkBackend *backend = (MDGUI_VkBackend *)user_data;
  if (!backend)
    return;
  push_quad(backend, (float)x, (float)y, (float)w, (float)h,
            pack_rgba(r, g, b, a), 0.0f, 0.0f, 0.0f, 0.0f, 0);
}

void vk_draw_line_rgba(void *user_data, unsigned char r, unsigned char g,
                       unsigned char b, unsigned char a, int x1, int y1,
                       int x2, int y2) {
  MDGUI_VkBackend *backend = (MDGUI_VkBackend *)user_data;
  if (!backend)
    return;

  const unsigned int rgba = pack_rgba(r, g, b, a);

  // Keep axis-aligned lines pixel-perfect to match software/SDL behavior.
  if (y1 == y2) {
    const int x_min = (x1 < x2) ? x1 : x2;
    const int x_max = (x1 < x2) ? x2 : x1;
    push_quad(backend, (float)x_min, (float)y1, (float)(x_max - x_min + 1), 1.0f,
              rgba, 0.0f, 0.0f, 0.0f, 0.0f, 0);
    return;
  }
  if (x1 == x2) {
    const int y_min = (y1 < y2) ? y1 : y2;
    const int y_max = (y1 < y2) ? y2 : y1;
    push_quad(backend, (float)x1, (float)y_min, 1.0f, (float)(y_max - y_min + 1),
              rgba, 0.0f, 0.0f, 0.0f, 0.0f, 0);
    return;
  }

  // Pixel-step diagonal/angled lines so UI marks (checkbox, close button)
  // look crisp and centered, matching SDL's aliased integer rasterization.
  int cx = x1;
  int cy = y1;
  const int dx = (x2 >= x1) ? (x2 - x1) : (x1 - x2);
  const int sx = (x1 < x2) ? 1 : -1;
  const int dy_abs = (y2 >= y1) ? (y2 - y1) : (y1 - y2);
  const int sy = (y1 < y2) ? 1 : -1;
  int err = dx - dy_abs;

  while (true) {
    push_quad(backend, (float)cx, (float)cy, 1.0f, 1.0f, rgba, 0.0f, 0.0f, 0.0f,
              0.0f, 0);
    if (cx == x2 && cy == y2)
      break;
    const int e2 = err * 2;
    if (e2 > -dy_abs) {
      err -= dy_abs;
      cx += sx;
    }
    if (e2 < dx) {
      err += dx;
      cy += sy;
    }
  }
}

void vk_draw_point_rgba(void *user_data, unsigned char r, unsigned char g,
                        unsigned char b, unsigned char a, int x, int y) {
  MDGUI_VkBackend *backend = (MDGUI_VkBackend *)user_data;
  if (!backend)
    return;
  push_quad(backend, (float)x, (float)y, 1.0f, 1.0f, pack_rgba(r, g, b, a),
            0.0f, 0.0f, 0.0f, 0.0f, 0);
}

int vk_draw_glyph_rgba(void *user_data, unsigned char glyph, int x, int y,
                       unsigned char r, unsigned char g, unsigned char b,
                       unsigned char a) {
  MDGUI_VkBackend *backend = (MDGUI_VkBackend *)user_data;
  if (!backend)
    return 0;

  const float inv_cols = 1.0f / 16.0f;
  const float inv_rows = 1.0f / 8.0f;
  const float u0 = (float)(glyph % 16) * inv_cols;
  const float v0 = (float)(glyph / 16) * inv_rows;
  const float u1 = u0 + inv_cols;
  const float v1 = v0 + inv_rows;
  push_quad(backend, (float)x, (float)y, 8.0f, 8.0f, pack_rgba(r, g, b, a), u0,
            v0, u1, v1, 1);
  return 1;
}

int vk_get_render_size(void *user_data, int *out_w, int *out_h) {
  MDGUI_VkBackend *backend = (MDGUI_VkBackend *)user_data;
  if (!backend) {
    if (out_w)
      *out_w = 320;
    if (out_h)
      *out_h = 240;
    return 0;
  }
  if (backend->get_render_size)
    return backend->get_render_size(backend->user_data, out_w, out_h);

  if (out_w)
    *out_w = backend->display_w;
  if (out_h)
    *out_h = backend->display_h;
  return 1;
}

unsigned long long vk_get_ticks_ms(void *user_data) {
  MDGUI_VkBackend *backend = (MDGUI_VkBackend *)user_data;
  if (!backend || !backend->get_ticks_ms)
    return 0;
  return backend->get_ticks_ms(backend->user_data);
}

} // namespace

extern "C" {
MDGUI_VkBackend *mdgui_vk_backend_create(
    const MDGUI_BackendCallbacks *callbacks) {
  MDGUI_VkBackend *backend = new (std::nothrow) MDGUI_VkBackend();
  if (!backend)
    return nullptr;
  if (callbacks) {
    backend->user_data = callbacks->user_data;
    backend->get_render_size = callbacks->get_render_size;
    backend->get_ticks_ms = callbacks->get_ticks_ms;
  }
  return backend;
}

void mdgui_vk_backend_destroy(MDGUI_VkBackend *backend) { delete backend; }

void mdgui_make_vulkan_backend(MDGUI_RenderBackend *out_backend,
                              MDGUI_VkBackend *backend) {
  if (!out_backend)
    return;

  memset(out_backend, 0, sizeof(*out_backend));
  if (!backend)
    return;

  out_backend->user_data = backend;
  out_backend->begin_frame = vk_begin_frame;
  out_backend->set_clip_rect = vk_set_clip_rect;
  out_backend->fill_rect_rgba = vk_fill_rect_rgba;
  out_backend->draw_line_rgba = vk_draw_line_rgba;
  out_backend->draw_point_rgba = vk_draw_point_rgba;
  out_backend->draw_glyph_rgba = vk_draw_glyph_rgba;
  out_backend->get_render_size = vk_get_render_size;
  out_backend->get_ticks_ms = vk_get_ticks_ms;
}

void mdgui_vk_backend_get_draw_data(const MDGUI_VkBackend *backend,
                                   MDGUI_VkDrawData *out_draw_data) {
  if (!out_draw_data)
    return;
  memset(out_draw_data, 0, sizeof(*out_draw_data));
  if (!backend)
    return;

  out_draw_data->vertices = backend->vertices.empty() ? nullptr
                                                      : backend->vertices.data();
  out_draw_data->vertex_count = (unsigned int)backend->vertices.size();
  out_draw_data->indices = backend->indices.empty() ? nullptr
                                                    : backend->indices.data();
  out_draw_data->index_count = (unsigned int)backend->indices.size();
  out_draw_data->commands = backend->commands.empty() ? nullptr
                                                      : backend->commands.data();
  out_draw_data->command_count = (unsigned int)backend->commands.size();
  out_draw_data->display_w = backend->display_w;
  out_draw_data->display_h = backend->display_h;
}

void mdgui_vk_build_font_atlas_rgba32(unsigned int *out_pixels, int out_w,
                                     int out_h) {
  if (!out_pixels || out_w != 128 || out_h != 64)
    return;

  memset(out_pixels, 0, (size_t)out_w * (size_t)out_h * sizeof(unsigned int));
  for (int ch = 0; ch < 128; ++ch) {
    const unsigned char *glyph =
        (const unsigned char *)mdgui_font8x8_basic[ch & 127];
    const int gx = (ch % 16) * 8;
    const int gy = (ch / 16) * 8;
    for (int py = 0; py < 8; ++py) {
      const unsigned char row = glyph[py];
      for (int px = 0; px < 8; ++px) {
        if ((row & (1u << px)) == 0)
          continue;
        const int x = gx + px;
        const int y = gy + py;
        out_pixels[(size_t)y * (size_t)out_w + (size_t)x] = 0xffffffffu;
      }
    }
  }
}
}
