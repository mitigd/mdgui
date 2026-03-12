#include "mdgui_c.h"

#include <SDL3/SDL.h>
#include "mdgui_font8x8.h"
#include <string.h>
#include <string>
#include <vector>

namespace {
static SDL_Texture *g_font_atlas = nullptr;
static SDL_Renderer *g_font_atlas_renderer = nullptr;
static int g_font_mod_r = -1;
static int g_font_mod_g = -1;
static int g_font_mod_b = -1;
static bool g_clip_enabled = false;
static SDL_Rect g_clip_rect = {0, 0, 0, 0};

struct CachedSubpassTarget {
  std::string id;
  SDL_Texture *texture = nullptr;
  int pixel_w = 0;
  int pixel_h = 0;
};

struct ActiveSubpassState {
  bool active = false;
  SDL_Texture *previous_target = nullptr;
  float parent_scale_x = 1.0f;
  float parent_scale_y = 1.0f;
  bool parent_clip_enabled = false;
  SDL_Rect parent_clip_rect = {0, 0, 0, 0};
  int parent_x = 0;
  int parent_y = 0;
  int parent_w = 0;
  int parent_h = 0;
  int pixel_w = 0;
  int pixel_h = 0;
  float subpass_scale = 1.0f;
};

static std::vector<CachedSubpassTarget> g_subpass_targets;
static ActiveSubpassState g_active_subpass;

const unsigned char *glyph_for_char(unsigned char c) {
  if (c >= 128)
    c = '?';
  return (const unsigned char *)mdgui_font8x8_basic[c];
}

bool ensure_font_atlas(SDL_Renderer *renderer) {
  if (!renderer)
    return false;
  if (g_font_atlas && g_font_atlas_renderer == renderer)
    return true;

  if (g_font_atlas && g_font_atlas_renderer != renderer) {
    SDL_DestroyTexture(g_font_atlas);
    g_font_atlas = nullptr;
    g_font_atlas_renderer = nullptr;
    g_font_mod_r = -1;
    g_font_mod_g = -1;
    g_font_mod_b = -1;
  }

  const int atlas_cols = 16;
  const int atlas_rows = 8;
  const int glyph_w = 8;
  const int glyph_h = 8;
  const int atlas_w = atlas_cols * glyph_w;
  const int atlas_h = atlas_rows * glyph_h;

  std::vector<unsigned int> pixels((size_t)atlas_w * (size_t)atlas_h, 0u);
  for (int ch = 0; ch < 128; ++ch) {
    const unsigned char *glyph = glyph_for_char((unsigned char)ch);
    const int gx = (ch % atlas_cols) * glyph_w;
    const int gy = (ch / atlas_cols) * glyph_h;
    for (int py = 0; py < glyph_h; ++py) {
      const unsigned char row = glyph[py];
      for (int px = 0; px < glyph_w; ++px) {
        if (row & (1u << px)) {
          const int ax = gx + px;
          const int ay = gy + py;
          pixels[(size_t)ay * (size_t)atlas_w + (size_t)ax] = 0xffffffffu;
        }
      }
    }
  }

  SDL_Texture *tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                       SDL_TEXTUREACCESS_STATIC, atlas_w,
                                       atlas_h);
  if (!tex)
    return false;

  SDL_UpdateTexture(tex, nullptr, pixels.data(), atlas_w * (int)sizeof(unsigned int));
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);

  g_font_atlas = tex;
  g_font_atlas_renderer = renderer;
  g_font_mod_r = -1;
  g_font_mod_g = -1;
  g_font_mod_b = -1;
  return true;
}

int sdl_draw_glyph_rgba(void *user_data, unsigned char glyph, int x, int y,
                        unsigned char r, unsigned char g, unsigned char b,
                        unsigned char a) {
  SDL_Renderer *renderer = (SDL_Renderer *)user_data;
  if (!renderer)
    return 0;
  if (!ensure_font_atlas(renderer))
    return 0;

  if ((int)r != g_font_mod_r || (int)g != g_font_mod_g || (int)b != g_font_mod_b) {
    SDL_SetTextureColorMod(g_font_atlas, r, g, b);
    g_font_mod_r = r;
    g_font_mod_g = g;
    g_font_mod_b = b;
  }
  SDL_SetTextureAlphaMod(g_font_atlas, a);

  const float sx = (float)((glyph % 16) * 8);
  const float sy = (float)((glyph / 16) * 8);
  SDL_FRect src = {sx, sy, 8.0f, 8.0f};
  SDL_FRect dst = {(float)x, (float)y, 8.0f, 8.0f};
  SDL_RenderTexture(renderer, g_font_atlas, &src, &dst);
  return 1;
}

void sdl_set_clip_rect(void *user_data, int enabled, int x, int y, int w,
                       int h) {
  SDL_Renderer *renderer = (SDL_Renderer *)user_data;
  if (!renderer)
    return;
  if (!enabled) {
    SDL_SetRenderClipRect(renderer, nullptr);
    g_clip_enabled = false;
    return;
  }
  SDL_Rect clip = {x, y, w, h};
  SDL_SetRenderClipRect(renderer, &clip);
  g_clip_enabled = true;
  g_clip_rect = clip;
}

void sdl_fill_rect_rgba(void *user_data, unsigned char r, unsigned char g,
                        unsigned char b, unsigned char a, int x, int y, int w,
                        int h) {
  SDL_Renderer *renderer = (SDL_Renderer *)user_data;
  if (!renderer)
    return;
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, r, g, b, a);
  SDL_FRect rect = {(float)x, (float)y, (float)w, (float)h};
  SDL_RenderFillRect(renderer, &rect);
}

void sdl_draw_line_rgba(void *user_data, unsigned char r, unsigned char g,
                        unsigned char b, unsigned char a, int x1, int y1,
                        int x2, int y2) {
  SDL_Renderer *renderer = (SDL_Renderer *)user_data;
  if (!renderer)
    return;
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, r, g, b, a);
  SDL_RenderLine(renderer, (float)x1, (float)y1, (float)x2, (float)y2);
}

void sdl_draw_point_rgba(void *user_data, unsigned char r, unsigned char g,
                         unsigned char b, unsigned char a, int x, int y) {
  SDL_Renderer *renderer = (SDL_Renderer *)user_data;
  if (!renderer)
    return;
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, r, g, b, a);
  SDL_RenderPoint(renderer, (float)x, (float)y);
}

int sdl_get_render_size(void *user_data, int *out_w, int *out_h) {
  SDL_Renderer *renderer = (SDL_Renderer *)user_data;
  if (!renderer)
    return 0;

  int rw = 320;
  int rh = 240;
  SDL_GetRenderOutputSize(renderer, &rw, &rh);

  float sx = 1.0f;
  float sy = 1.0f;
  SDL_GetRenderScale(renderer, &sx, &sy);
  if (sx > 0.0f)
    rw = (int)((float)rw / sx);
  if (sy > 0.0f)
    rh = (int)((float)rh / sy);

  if (rw <= 0)
    rw = 320;
  if (rh <= 0)
    rh = 240;

  if (out_w)
    *out_w = rw;
  if (out_h)
    *out_h = rh;
  return 1;
}

int sdl_get_render_scale(void *user_data, float *out_sx, float *out_sy) {
  SDL_Renderer *renderer = (SDL_Renderer *)user_data;
  if (!renderer) {
    if (out_sx)
      *out_sx = 1.0f;
    if (out_sy)
      *out_sy = 1.0f;
    return 0;
  }
  float sx = 1.0f;
  float sy = 1.0f;
  SDL_GetRenderScale(renderer, &sx, &sy);
  if (out_sx)
    *out_sx = sx;
  if (out_sy)
    *out_sy = sy;
  return 1;
}

CachedSubpassTarget *find_or_create_subpass_target(SDL_Renderer *renderer,
                                                   const char *id, int pixel_w,
                                                   int pixel_h) {
  if (!renderer || !id || pixel_w <= 0 || pixel_h <= 0)
    return nullptr;
  for (auto &entry : g_subpass_targets) {
    if (entry.id != id)
      continue;
    if (entry.texture && entry.pixel_w == pixel_w && entry.pixel_h == pixel_h)
      return &entry;
    if (entry.texture) {
      SDL_DestroyTexture(entry.texture);
      entry.texture = nullptr;
    }
    entry.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                      SDL_TEXTUREACCESS_TARGET, pixel_w,
                                      pixel_h);
    if (!entry.texture)
      return nullptr;
    SDL_SetTextureBlendMode(entry.texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(entry.texture, SDL_SCALEMODE_NEAREST);
    entry.pixel_w = pixel_w;
    entry.pixel_h = pixel_h;
    return &entry;
  }

  CachedSubpassTarget entry{};
  entry.id = id;
  entry.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_TARGET, pixel_w, pixel_h);
  if (!entry.texture)
    return nullptr;
  SDL_SetTextureBlendMode(entry.texture, SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(entry.texture, SDL_SCALEMODE_NEAREST);
  entry.pixel_w = pixel_w;
  entry.pixel_h = pixel_h;
  g_subpass_targets.push_back(entry);
  return &g_subpass_targets.back();
}

int sdl_begin_subpass(void *user_data, const char *id, int x, int y, int w,
                      int h, float scale, int *out_w, int *out_h) {
  SDL_Renderer *renderer = (SDL_Renderer *)user_data;
  if (!renderer || !id || w <= 0 || h <= 0 || scale <= 0.0f ||
      g_active_subpass.active) {
    return 0;
  }

  float parent_sx = 1.0f;
  float parent_sy = 1.0f;
  SDL_GetRenderScale(renderer, &parent_sx, &parent_sy);
  if (parent_sx <= 0.0f)
    parent_sx = 1.0f;
  if (parent_sy <= 0.0f)
    parent_sy = 1.0f;

  const int pixel_w = (int)(w * parent_sx + 0.5f);
  const int pixel_h = (int)(h * parent_sy + 0.5f);
  if (pixel_w <= 0 || pixel_h <= 0)
    return 0;

  CachedSubpassTarget *target =
      find_or_create_subpass_target(renderer, id, pixel_w, pixel_h);
  if (!target || !target->texture)
    return 0;

  g_active_subpass.active = true;
  g_active_subpass.previous_target = SDL_GetRenderTarget(renderer);
  g_active_subpass.parent_scale_x = parent_sx;
  g_active_subpass.parent_scale_y = parent_sy;
  g_active_subpass.parent_clip_enabled = g_clip_enabled;
  g_active_subpass.parent_clip_rect = g_clip_rect;
  g_active_subpass.parent_x = x;
  g_active_subpass.parent_y = y;
  g_active_subpass.parent_w = w;
  g_active_subpass.parent_h = h;
  g_active_subpass.pixel_w = pixel_w;
  g_active_subpass.pixel_h = pixel_h;
  g_active_subpass.subpass_scale = scale;

  SDL_SetRenderTarget(renderer, target->texture);
  SDL_SetRenderScale(renderer, scale, scale);
  SDL_SetRenderClipRect(renderer, nullptr);
  g_clip_enabled = false;
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);

  if (out_w)
    *out_w = (int)(pixel_w / scale + 0.5f);
  if (out_h)
    *out_h = (int)(pixel_h / scale + 0.5f);
  return 1;
}

void sdl_end_subpass(void *user_data) {
  SDL_Renderer *renderer = (SDL_Renderer *)user_data;
  if (!renderer || !g_active_subpass.active)
    return;

  SDL_Texture *finished_target = SDL_GetRenderTarget(renderer);
  SDL_SetRenderTarget(renderer, g_active_subpass.previous_target);
  SDL_SetRenderScale(renderer, 1.0f, 1.0f);
  SDL_SetRenderClipRect(renderer, nullptr);

  SDL_FRect dst = {
      (float)(g_active_subpass.parent_x * g_active_subpass.parent_scale_x),
      (float)(g_active_subpass.parent_y * g_active_subpass.parent_scale_y),
      (float)g_active_subpass.pixel_w,
      (float)g_active_subpass.pixel_h,
  };
  SDL_RenderTexture(renderer, finished_target, nullptr, &dst);

  SDL_SetRenderScale(renderer, g_active_subpass.parent_scale_x,
                     g_active_subpass.parent_scale_y);
  if (g_active_subpass.parent_clip_enabled) {
    SDL_SetRenderClipRect(renderer, &g_active_subpass.parent_clip_rect);
    g_clip_enabled = true;
    g_clip_rect = g_active_subpass.parent_clip_rect;
  } else {
    SDL_SetRenderClipRect(renderer, nullptr);
    g_clip_enabled = false;
  }

  g_active_subpass = ActiveSubpassState{};
}

unsigned long long sdl_get_ticks_ms(void *user_data) {
  (void)user_data;
  return (unsigned long long)SDL_GetTicks();
}
} // namespace

extern "C" {
void mdgui_make_sdl_backend(MDGUI_RenderBackend *out_backend,
                           void *sdl_renderer) {
  if (!out_backend)
    return;
  memset(out_backend, 0, sizeof(*out_backend));
  out_backend->user_data = sdl_renderer;
  out_backend->set_clip_rect = sdl_set_clip_rect;
  out_backend->fill_rect_rgba = sdl_fill_rect_rgba;
  out_backend->draw_line_rgba = sdl_draw_line_rgba;
  out_backend->draw_point_rgba = sdl_draw_point_rgba;
  out_backend->draw_glyph_rgba = sdl_draw_glyph_rgba;
  out_backend->get_render_size = sdl_get_render_size;
  out_backend->get_render_scale = sdl_get_render_scale;
  out_backend->get_ticks_ms = sdl_get_ticks_ms;
  out_backend->begin_subpass = sdl_begin_subpass;
  out_backend->end_subpass = sdl_end_subpass;
}
}
