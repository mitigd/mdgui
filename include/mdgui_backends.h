#ifndef MDGUI_BACKENDS_H
#define MDGUI_BACKENDS_H

#include "mdgui_c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MDGUI_BackendCallbacks {
  void *user_data;
  void (*begin_frame)(void *user_data);
  void (*end_frame)(void *user_data);
  void (*set_clip_rect)(void *user_data, int enabled, int x, int y, int w,
                        int h);
  void (*fill_rect_rgba)(void *user_data, unsigned char r, unsigned char g,
                         unsigned char b, unsigned char a, int x, int y,
                         int w, int h);
  void (*draw_line_rgba)(void *user_data, unsigned char r, unsigned char g,
                         unsigned char b, unsigned char a, int x1, int y1,
                         int x2, int y2);
  void (*draw_point_rgba)(void *user_data, unsigned char r, unsigned char g,
                          unsigned char b, unsigned char a, int x, int y);
  int (*draw_glyph_rgba)(void *user_data, unsigned char glyph, int x, int y,
                         unsigned char r, unsigned char g, unsigned char b,
                         unsigned char a);
  int (*get_render_size)(void *user_data, int *out_w, int *out_h);
  int (*get_render_scale)(void *user_data, float *out_sx, float *out_sy);
  unsigned long long (*get_ticks_ms)(void *user_data);
  int (*begin_subpass)(void *user_data, const char *id, int x, int y, int w,
                       int h, float scale, int *out_w, int *out_h);
  void (*end_subpass)(void *user_data);
} MDGUI_BackendCallbacks;

// Vulkan backend emits ImGui-like draw data (vertices/indices/commands)
// that can be consumed by a Vulkan renderer.
typedef struct MDGUI_VkBackend MDGUI_VkBackend;

typedef struct MDGUI_VkVertex {
  float x;
  float y;
  float u;
  float v;
  unsigned int rgba;
} MDGUI_VkVertex;

typedef struct MDGUI_VkDrawCmd {
  unsigned int index_offset;
  unsigned int index_count;
  int textured;
  int clip_enabled;
  int clip_x;
  int clip_y;
  int clip_w;
  int clip_h;
} MDGUI_VkDrawCmd;

typedef struct MDGUI_VkDrawData {
  const MDGUI_VkVertex *vertices;
  unsigned int vertex_count;
  const unsigned int *indices;
  unsigned int index_count;
  const MDGUI_VkDrawCmd *commands;
  unsigned int command_count;
  int display_w;
  int display_h;
} MDGUI_VkDrawData;

// SDL renderer compatibility backend (implemented in src/mdgui_backend_sdl.cpp).
void mdgui_make_sdl_backend(MDGUI_RenderBackend *out_backend, void *sdl_renderer);

// OpenGL compatibility backend adapter.
void mdgui_make_opengl_backend(MDGUI_RenderBackend *out_backend,
                              const MDGUI_BackendCallbacks *callbacks);

// Vulkan compatibility backend adapter.
// Callbacks are optional, but `get_render_size` and `get_ticks_ms` should be
// provided for correct layout/timing behavior.
MDGUI_VkBackend *mdgui_vk_backend_create(const MDGUI_BackendCallbacks *callbacks);
void mdgui_vk_backend_destroy(MDGUI_VkBackend *backend);

// Fills out a render backend that writes draw data into `backend` each frame.
void mdgui_make_vulkan_backend(MDGUI_RenderBackend *out_backend,
                              MDGUI_VkBackend *backend);

// Retrieve draw data for the current frame (valid until the next frame).
void mdgui_vk_backend_get_draw_data(const MDGUI_VkBackend *backend,
                                   MDGUI_VkDrawData *out_draw_data);

// Builds a simple 128x64 RGBA font atlas matching the glyph UVs emitted by
// this backend (16 columns x 8 rows of 8x8 glyphs).
void mdgui_vk_build_font_atlas_rgba32(unsigned int *out_pixels, int out_w,
                                     int out_h);

#ifdef __cplusplus
}
#endif

#endif
