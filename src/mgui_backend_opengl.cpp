#include "mgui_backends.h"

#include <string.h>

extern "C" {
void mgui_make_opengl_backend(MGUI_RenderBackend *out_backend,
                              const MGUI_BackendCallbacks *callbacks) {
  if (!out_backend)
    return;

  memset(out_backend, 0, sizeof(*out_backend));
  if (!callbacks)
    return;

  out_backend->user_data = callbacks->user_data;
  out_backend->begin_frame = callbacks->begin_frame;
  out_backend->end_frame = callbacks->end_frame;
  out_backend->set_clip_rect = callbacks->set_clip_rect;
  out_backend->fill_rect_rgba = callbacks->fill_rect_rgba;
  out_backend->draw_line_rgba = callbacks->draw_line_rgba;
  out_backend->draw_point_rgba = callbacks->draw_point_rgba;
  out_backend->draw_glyph_rgba = callbacks->draw_glyph_rgba;
  out_backend->get_render_size = callbacks->get_render_size;
  out_backend->get_ticks_ms = callbacks->get_ticks_ms;
}
}
