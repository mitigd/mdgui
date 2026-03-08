#ifndef MGUI_BACKENDS_H
#define MGUI_BACKENDS_H

#include "mgui_c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MGUI_BackendCallbacks {
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
  unsigned long long (*get_ticks_ms)(void *user_data);
} MGUI_BackendCallbacks;

// SDL renderer compatibility backend (implemented in src/mgui_backend_sdl.cpp).
void mgui_make_sdl_backend(MGUI_RenderBackend *out_backend, void *sdl_renderer);

// OpenGL compatibility backend adapter.
void mgui_make_opengl_backend(MGUI_RenderBackend *out_backend,
                              const MGUI_BackendCallbacks *callbacks);

// Vulkan compatibility backend adapter.
void mgui_make_vulkan_backend(MGUI_RenderBackend *out_backend,
                              const MGUI_BackendCallbacks *callbacks);

#ifdef __cplusplus
}
#endif

#endif
