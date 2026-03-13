#pragma once

#include "base_internal.hpp"

void note_content_bounds(MDGUI_Context *ctx, int right, int bottom);
MDGUI_Font *fallback_font();
MDGUI_Font *resolve_font(MDGUI_Context *ctx, MDGUI_Font *override_font = nullptr);
int font_line_height(MDGUI_Context *ctx, MDGUI_Font *override_font = nullptr);
int font_measure_text(MDGUI_Context *ctx, const char *text,
                      MDGUI_Font *override_font = nullptr);
void font_draw_text(MDGUI_Context *ctx, const char *text, int x, int y,
                    int color_idx, MDGUI_Font *override_font = nullptr);
const MDGUI_Context::SubpassState *current_subpass(const MDGUI_Context *ctx);
int current_viewport_x(const MDGUI_Context *ctx);
int current_viewport_width(const MDGUI_Context *ctx);
void layout_prepare_widget(MDGUI_Context *ctx, int *out_local_x,
                           int *out_logical_y);
int layout_resolve_width(MDGUI_Context *ctx, int local_x, int requested_w,
                         int min_w);
void set_content_clip(MDGUI_Context *ctx);
bool set_widget_clip_intersect_content(MDGUI_Context *ctx, int x, int y, int w,
                                       int h);
int resolve_dynamic_width(MDGUI_Context *ctx, int local_x, int w, int min_w);
void drawrect_rgb(MDGUI_Context *ctx, uint8_t r, uint8_t g, uint8_t b, int x,
                  int y, int w, int h);
int point_in_rect(int px, int py, int x, int y, int w, int h);
void get_current_render_scale(MDGUI_Context *ctx, float *out_sx,
                              float *out_sy);
int clamp_text_cursor(int cursor, int len);
int text_cursor_from_pixel_x(const char *buffer, int len, int target_px);
int layout_current_column_base_x(const MDGUI_Context *ctx);
void layout_commit_widget(MDGUI_Context *ctx, int local_x, int logical_y, int w,
                          int h, int spacing_y);
