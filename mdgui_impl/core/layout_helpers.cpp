#include "layout_internal.hpp"

 void get_current_render_scale(MDGUI_Context *ctx, float *out_sx,
                                     float *out_sy) {
  float sx = 1.0f;
  float sy = 1.0f;
  if (ctx && ctx->render.backend.get_render_scale) {
    ctx->render.backend.get_render_scale(ctx->render.backend.user_data, &sx, &sy);
  }
  if (sx <= 0.0f)
    sx = 1.0f;
  if (sy <= 0.0f)
    sy = 1.0f;
  if (out_sx)
    *out_sx = sx;
  if (out_sy)
    *out_sy = sy;
}


 int clamp_text_cursor(int cursor, int len) {
  if (cursor < 0)
    return 0;
  if (cursor > len)
    return len;
  return cursor;
}

 int text_cursor_from_pixel_x(const char *buffer, int len,
                                    int target_px) {
  if (!mdgui_fonts[1] || !buffer)
    return len;
  if (target_px < 0)
    target_px = 0;
  int accum_px = 0;
  for (int i = 0; i < len; ++i) {
    char ch[2] = {buffer[i], '\0'};
    int cw = mdgui_fonts[1]->measureTextWidth(ch);
    if (cw < 1)
      cw = 1;
    if (target_px < accum_px + (cw / 2))
      return i;
    accum_px += cw;
  }
  return len;
}

 int layout_current_column_base_x(const MDGUI_Context *ctx) {
  if (!ctx)
    return 0;
  if (!ctx->layout.columns_active || ctx->layout.columns_count < 1)
    return ctx->layout.indent;
  return ctx->layout.columns_start_x +
         ctx->layout.columns_index * (ctx->layout.columns_width +
                                      ctx->layout.style.spacing_x);
}

 void layout_prepare_widget(MDGUI_Context *ctx, int *out_local_x,
                                  int *out_logical_y) {
  if (!ctx) {
    if (out_local_x)
      *out_local_x = 0;
    if (out_logical_y)
      *out_logical_y = 0;
    return;
  }
  int local_x = layout_current_column_base_x(ctx);
  int logical_y = ctx->window.content_y;
  if (ctx->layout.columns_active &&
      ctx->layout.columns_index < (int)ctx->layout.columns_bottoms.size()) {
    logical_y = ctx->layout.columns_bottoms[ctx->layout.columns_index];
    ctx->window.content_y = logical_y;
  }
  if (ctx->layout.same_line && ctx->layout.has_last_item &&
      !ctx->layout.columns_active) {
    local_x = ctx->layout.last_item_x + ctx->layout.last_item_w +
              ctx->layout.style.spacing_x;
    logical_y = ctx->layout.last_item_y;
  }
  if (out_local_x)
    *out_local_x = local_x;
  if (out_logical_y)
    *out_logical_y = logical_y;
}

 int layout_resolve_width(MDGUI_Context *ctx, int local_x, int requested_w,
                                int min_w) {
  int resolved = resolve_dynamic_width(ctx, local_x, requested_w, min_w);
  if (ctx && ctx->layout.columns_active && requested_w <= 0) {
    if (resolved > ctx->layout.columns_width)
      resolved = ctx->layout.columns_width;
  }
  if (resolved < min_w)
    resolved = min_w;
  return resolved;
}

 void layout_commit_widget(MDGUI_Context *ctx, int local_x, int logical_y,
                                 int w, int h, int spacing_y) {
  if (!ctx)
    return;
  if (spacing_y < 0)
    spacing_y = 0;
  ctx->layout.last_item_x = local_x;
  ctx->layout.last_item_y = logical_y;
  ctx->layout.last_item_w = w;
  ctx->layout.last_item_h = h;
  ctx->layout.has_last_item = true;
  ctx->layout.same_line = false;

  const int bottom = logical_y + h + spacing_y;
  if (ctx->layout.columns_active &&
      ctx->layout.columns_index < (int)ctx->layout.columns_bottoms.size()) {
    int &col_bottom = ctx->layout.columns_bottoms[ctx->layout.columns_index];
    if (bottom > col_bottom)
      col_bottom = bottom;
    if (col_bottom > ctx->layout.columns_max_bottom)
      ctx->layout.columns_max_bottom = col_bottom;
    ctx->window.content_y = col_bottom;
  } else {
    ctx->window.content_y = bottom;
  }
}



