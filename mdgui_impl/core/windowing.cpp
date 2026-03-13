#include "windowing_internal.hpp"

extern "C" {
int mdgui_message_box_ex(MDGUI_Context *ctx, const char *id, const char *title,
                         const char *text, int style, const char *button1,
                         const char *button2, int text_align) {
  if (!ctx)
    return 0;
  if (!title)
    title = "MESSAGE";
  if (!text)
    text = "";
  if (!id)
    id = "default";
  if (!button1)
    button1 = "OK";
  if (!button2)
    button2 = "CANCEL";

  std::vector<std::string> lines;
  {
    const char *start = text;
    const char *p = text;
    while (true) {
      if (*p == '\n' || *p == '\0') {
        lines.emplace_back(start, (size_t)(p - start));
        if (*p == '\0')
          break;
        start = p + 1;
      }
      ++p;
    }
    if (lines.empty())
      lines.emplace_back("");
  }

  int text_w = 80;
  if (mdgui_fonts[1]) {
    text_w = 0;
    for (const auto &ln : lines) {
      const int lw = mdgui_fonts[1]->measureTextWidth(ln.c_str());
      if (lw > text_w)
        text_w = lw;
    }
    if (text_w < 80)
      text_w = 80;
  }
  int box_w = text_w + 24;
  if (box_w < 170)
    box_w = 170;
  const int line_h = 12;
  const int top_pad = 22;
  const int bottom_pad = 28;
  int box_h = top_pad + ((int)lines.size() * line_h) + bottom_pad;
  if (box_h < 72)
    box_h = 72;
  const int x = (get_logical_render_w(ctx) - box_w) / 2;
  const int y = (get_logical_render_h(ctx) - box_h) / 2;

  (void)id;
  if (!mdgui_begin_window(ctx, title, x, y, box_w, box_h)) {
    return 2; // Return "Cancel" if
              // closed via X
  }
  if (ctx->window.current_window >= 0) {
    auto &msg_win = ctx->window.windows[ctx->window.current_window];
    msg_win.z = ++ctx->window.z_counter;
    msg_win.disallow_maximize = true;
    msg_win.is_message_box = true;
    msg_win.is_maximized = false;
    // Message boxes should remain
    // fixed-size; avoid feedback with
    // auto min-size tracking when
    // footer/button geometry is
    // anchored to bottom chrome.
    msg_win.fixed_rect = true;
    msg_win.w = box_w;
    msg_win.h = box_h;
    msg_win.min_w = box_w;
    msg_win.min_h = box_h;
  }

  const auto &win = ctx->window.windows[ctx->window.current_window];
  const int title_h = 12;
  const int footer_h = 16;
  // Content is clipped to (win.y +
  // win.h - 4); keep footer fully
  // inside that.
  const int footer_y = (win.y + win.h - 4) - footer_h;
  const int body_y = win.y + title_h;
  int body_h = footer_y - body_y;
  if (body_h < 1)
    body_h = 1;

  mdgui_fill_rect_idx(nullptr, CLR_MSG_BG, win.x, body_y, win.w, body_h);
  mdgui_fill_rect_idx(nullptr, CLR_MSG_BAR, win.x, footer_y, win.w, footer_h);

  const int text_y0 = win.y + top_pad;
  for (size_t i = 0; i < lines.size(); ++i) {
    const char *ln = lines[i].c_str();
    int tx = win.x + 8;
    if (text_align == MDGUI_TEXT_ALIGN_CENTER && mdgui_fonts[1]) {
      const int tw = mdgui_fonts[1]->measureTextWidth(ln);
      tx = win.x + (win.w - tw) / 2;
    }
    const int ty = text_y0 + (int)i * line_h;
    if (mdgui_fonts[1]) {
      mdgui_fonts[1]->drawText(ln, nullptr, tx, ty, CLR_TEXT_LIGHT);
    }
  }

  int result = 0;
  const int btn_w = 70;
  const int btn_h = 12;
  const int btn_abs_y = footer_y + (footer_h - btn_h) / 2;
  const int saved_content_y = ctx->window.content_y;
  const int saved_indent = ctx->layout.indent;
  const bool saved_has_last = ctx->layout.has_last_item;
  const bool saved_same_line = ctx->layout.same_line;
  const int button_logical_y = btn_abs_y + win.text_scroll;
  if (style == MDGUI_MSGBOX_TWO_BUTTON) {
    const int b1_abs_x = win.x + (win.w / 4) - (btn_w / 2);
    const int b2_abs_x = win.x + ((win.w * 3) / 4) - (btn_w / 2);
    ctx->window.content_y = button_logical_y;
    ctx->layout.indent = b1_abs_x - ctx->window.origin_x;
    ctx->layout.has_last_item = false;
    ctx->layout.same_line = false;
    if (mdgui_button(ctx, button1, btn_w, btn_h))
      result = 1;
    ctx->window.content_y = button_logical_y;
    ctx->layout.indent = b2_abs_x - ctx->window.origin_x;
    ctx->layout.has_last_item = false;
    ctx->layout.same_line = false;
    if (mdgui_button(ctx, button2, btn_w, btn_h))
      result = 2;
  } else {
    const int b_abs_x = win.x + (win.w / 2) - (btn_w / 2);
    ctx->window.content_y = button_logical_y;
    ctx->layout.indent = b_abs_x - ctx->window.origin_x;
    ctx->layout.has_last_item = false;
    ctx->layout.same_line = false;
    if (mdgui_button(ctx, button1, btn_w, btn_h))
      result = 1;
  }
  ctx->window.content_y = saved_content_y;
  ctx->layout.indent = saved_indent;
  ctx->layout.has_last_item = saved_has_last;
  ctx->layout.same_line = saved_same_line;

  mdgui_end_window(ctx);
  return result;
}

int mdgui_message_box(MDGUI_Context *ctx, const char *id, const char *title,
                      const char *text, int style, const char *button1,
                      const char *button2) {
  return mdgui_message_box_ex(ctx, id, title, text, style, button1, button2,
                              MDGUI_TEXT_ALIGN_LEFT);
}

int mdgui_get_window_z(MDGUI_Context *ctx, const char *title) {
  if (!ctx || !title)
    return -1;
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    if (ctx->window.windows[i].id == title)
      return ctx->window.windows[i].z;
  }
  return -1;
}

void mdgui_set_window_open(MDGUI_Context *ctx, const char *title, int open) {
  if (!ctx || !title)
    return;
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    if (ctx->window.windows[i].id == title) {
      auto &win = ctx->window.windows[i];
      win.closed = (open == 0);
      if (open) {
        win.z = ++ctx->window.z_counter;
      } else {
        win.open_menu_index = -1;
        win.open_menu_path.clear();
        win.menu_overlay_defs.clear();
        win.text_scroll_dragging = false;
        if (ctx->window.current_window == i)
          ctx->window.current_window = -1;
        if (ctx->window.dragging_window == i)
          ctx->window.dragging_window = -1;
        if (ctx->window.resizing_window == i)
          ctx->window.resizing_window = -1;
        if (ctx->interaction.active_text_input_window == i) {
          ctx->interaction.active_text_input_window = -1;
          ctx->interaction.active_text_input_id = 0;
          ctx->interaction.active_text_input_cursor = 0;
          ctx->interaction.active_text_input_sel_anchor = 0;
          ctx->interaction.active_text_input_sel_start = 0;
          ctx->interaction.active_text_input_sel_end = 0;
          ctx->interaction.active_text_input_drag_select = false;
          ctx->interaction.active_text_input_multiline = false;
          ctx->interaction.active_text_input_scroll_y = 0;
        }
        if (ctx->interaction.combo_overlay_window == i) {
          ctx->interaction.combo_overlay_pending = false;
          ctx->interaction.combo_overlay_window = -1;
          ctx->interaction.combo_overlay_items = nullptr;
        }
        if (ctx->interaction.combo_capture_window == i) {
          ctx->interaction.combo_capture_active = false;
          ctx->interaction.combo_capture_seen_this_frame = false;
          ctx->interaction.combo_capture_window = -1;
          ctx->interaction.combo_capture_x = 0;
          ctx->interaction.combo_capture_y = 0;
          ctx->interaction.combo_capture_w = 0;
          ctx->interaction.combo_capture_h = 0;
        }
      }
      return;
    }
  }
}

int mdgui_is_window_open(MDGUI_Context *ctx, const char *title) {
  if (!ctx || !title)
    return 0;
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    if (ctx->window.windows[i].id == title)
      return ctx->window.windows[i].closed ? 0 : 1;
  }
  return 0;
}

void mdgui_focus_window(MDGUI_Context *ctx, const char *title) {
  if (!ctx || !title)
    return;
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    if (ctx->window.windows[i].id == title && !ctx->window.windows[i].closed) {
      ctx->window.windows[i].z = ++ctx->window.z_counter;
      return;
    }
  }
}

void mdgui_set_window_rect(MDGUI_Context *ctx, const char *title, int x, int y,
                           int w, int h) {
  if (!ctx || !title)
    return;
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    auto &win = ctx->window.windows[i];
    if (win.id != title)
      continue;
    if (w < 80)
      w = 80;
    if (h < 60)
      h = 60;
    win.x = x;
    win.y = y;
    win.w = w;
    win.h = h;
    win.restored_x = x;
    win.restored_y = y;
    win.restored_w = w;
    win.restored_h = h;
    win.is_maximized = false;
    win.fixed_rect = true;
    return;
  }
}

int mdgui_get_window_rect(MDGUI_Context *ctx, const char *title, int *x, int *y,
                          int *w, int *h) {
  if (!ctx || !title)
    return 0;
  for (auto &win : ctx->window.windows) {
    if (win.id == title && !win.closed) {
      if (x)
        *x = win.x;
      if (y)
        *y = win.y;
      if (w)
        *w = win.w;
      if (h)
        *h = win.h;
      return 1;
    }
  }
  return 0;
}

int mdgui_get_window_count(MDGUI_Context *ctx) {
  if (!ctx)
    return 0;
  return (int)ctx->window.windows.size();
}

int mdgui_get_window_rect_by_index(MDGUI_Context *ctx, int index, int *x, int *y,
                                   int *w, int *h, int *z, int *open) {
  if (!ctx || index < 0 || index >= (int)ctx->window.windows.size())
    return 0;
  const auto &win = ctx->window.windows[(size_t)index];
  if (x)
    *x = win.x;
  if (y)
    *y = win.y;
  if (w)
    *w = win.w;
  if (h)
    *h = win.h;
  if (z)
    *z = win.z;
  if (open)
    *open = win.closed ? 0 : 1;
  return 1;
}

void mdgui_set_window_min_size(MDGUI_Context *ctx, const char *title, int min_w,
                               int min_h) {
  if (!ctx || !title)
    return;
  const int nw = normalize_window_min_w(min_w);
  const int nh = normalize_window_min_h(min_h);
  set_pending_window_min_size(ctx, title, nw, nh);
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    auto &win = ctx->window.windows[i];
    if (win.id != title)
      continue;
    win.user_min_from_percent = false;
    win.user_min_w_percent = 0.0f;
    win.user_min_h_percent = 0.0f;
    win.user_min_w = nw;
    win.user_min_h = nh;
    win.min_w = nw;
    win.min_h = nh;
    if (win.w < win.min_w)
      win.w = win.min_w;
    if (win.h < win.min_h)
      win.h = win.min_h;
    if (win.restored_w < win.min_w)
      win.restored_w = win.min_w;
    if (win.restored_h < win.min_h)
      win.restored_h = win.min_h;
    if (ctx->window.tile_manager_enabled)
      tile_windows_internal(ctx);
    return;
  }
}

void mdgui_get_window_min_size(MDGUI_Context *ctx, const char *title, int *min_w,
                               int *min_h) {
  if (min_w)
    *min_w = 50;
  if (min_h)
    *min_h = 30;
  if (!ctx || !title)
    return;

  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    auto &win = ctx->window.windows[i];
    if (win.id != title)
      continue;
    if (win.user_min_from_percent) {
      win.user_min_w =
          min_from_percent(get_logical_render_w(ctx), win.user_min_w_percent, 50);
      win.user_min_h =
          min_from_percent(get_logical_render_h(ctx), win.user_min_h_percent, 30);
    }
    if (min_w)
      *min_w = win.user_min_w;
    if (min_h)
      *min_h = win.user_min_h;
    return;
  }

  int pending_w = 0, pending_h = 0;
  if (get_pending_window_min_size(ctx, title, &pending_w, &pending_h)) {
    if (min_w)
      *min_w = pending_w;
    if (min_h)
      *min_h = pending_h;
  }
}

void mdgui_set_window_min_size_percent(MDGUI_Context *ctx, const char *title,
                                       float min_w_percent,
                                       float min_h_percent) {
  if (!ctx || !title)
    return;
  const float wp = normalize_window_min_percent(min_w_percent);
  const float hp = normalize_window_min_percent(min_h_percent);
  set_pending_window_min_size_percent(ctx, title, wp, hp);
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    auto &win = ctx->window.windows[i];
    if (win.id != title)
      continue;
    win.user_min_from_percent = true;
    win.user_min_w_percent = wp;
    win.user_min_h_percent = hp;
    apply_window_user_min_size(ctx, win);
    if (win.w < win.min_w)
      win.w = win.min_w;
    if (win.h < win.min_h)
      win.h = win.min_h;
    if (win.restored_w < win.min_w)
      win.restored_w = win.min_w;
    if (win.restored_h < win.min_h)
      win.restored_h = win.min_h;
    if (ctx->window.tile_manager_enabled)
      tile_windows_internal(ctx);
    return;
  }
}

int mdgui_get_window_min_size_percent(MDGUI_Context *ctx, const char *title,
                                      float *min_w_percent,
                                      float *min_h_percent) {
  if (min_w_percent)
    *min_w_percent = 0.0f;
  if (min_h_percent)
    *min_h_percent = 0.0f;
  if (!ctx || !title)
    return 0;

  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    const auto &win = ctx->window.windows[i];
    if (win.id != title)
      continue;
    if (!win.user_min_from_percent)
      return 0;
    if (min_w_percent)
      *min_w_percent = win.user_min_w_percent;
    if (min_h_percent)
      *min_h_percent = win.user_min_h_percent;
    return 1;
  }

  float pending_w = 0.0f, pending_h = 0.0f;
  if (get_pending_window_min_size_percent(ctx, title, &pending_w, &pending_h)) {
    if (min_w_percent)
      *min_w_percent = pending_w;
    if (min_h_percent)
      *min_h_percent = pending_h;
    return 1;
  }
  return 0;
}

void mdgui_set_windows_locked(MDGUI_Context *ctx, int locked) {
  if (!ctx)
    return;
  ctx->window.windows_locked = (locked != 0);
  if (ctx->window.windows_locked) {
    ctx->window.dragging_window = -1;
    ctx->window.resizing_window = -1;
  }
}

int mdgui_is_windows_locked(MDGUI_Context *ctx) {
  if (!ctx)
    return 0;
  return ctx->window.windows_locked ? 1 : 0;
}

void mdgui_set_window_alpha(MDGUI_Context *ctx, const char *title,
                            unsigned char alpha) {
  if (!ctx || !title)
    return;
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    auto &win = ctx->window.windows[i];
    if (win.id != title)
      continue;
    win.alpha = alpha;
    return;
  }
}

unsigned char mdgui_get_window_alpha(MDGUI_Context *ctx, const char *title) {
  if (!ctx || !title)
    return 255;
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    const auto &win = ctx->window.windows[i];
    if (win.id == title)
      return win.alpha;
  }
  return 255;
}

void mdgui_set_window_scrollbar_visible(MDGUI_Context *ctx, const char *title,
                                        int visible) {
  if (!ctx || !title)
    return;
  const bool normalized = (visible != 0);
  set_pending_window_scrollbar_visibility(ctx, title, normalized);
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    auto &win = ctx->window.windows[i];
    if (win.id != title)
      continue;
    win.scrollbar_visible = normalized;
    if (!normalized) {
      win.text_scroll_dragging = false;
      win.scrollbar_overflow_active = false;
    }
    return;
  }
}

int mdgui_is_window_scrollbar_visible(MDGUI_Context *ctx, const char *title) {
  if (!ctx || !title)
    return 1;
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    const auto &win = ctx->window.windows[i];
    if (win.id == title)
      return win.scrollbar_visible ? 1 : 0;
  }
  bool pending_visible = true;
  if (get_pending_window_scrollbar_visibility(ctx, title, &pending_visible))
    return pending_visible ? 1 : 0;
  return 1;
}

void mdgui_set_windows_alpha(MDGUI_Context *ctx, unsigned char alpha) {
  if (!ctx)
    return;
  ctx->window.default_window_alpha = alpha;
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    ctx->window.windows[i].alpha = alpha;
  }
}

unsigned char mdgui_get_windows_alpha(MDGUI_Context *ctx) {
  if (!ctx)
    return 255;
  return ctx->window.default_window_alpha;
}

void mdgui_set_tile_manager_enabled(MDGUI_Context *ctx, int enabled) {
  if (!ctx)
    return;
  ctx->window.tile_manager_enabled = (enabled != 0);
  if (ctx->window.tile_manager_enabled) {
    tile_windows_internal(ctx);
  }
}

void mdgui_get_tiling_min_size(MDGUI_Context *ctx, int *w, int *h) {
  if (!ctx) {
    if (w)
      *w = 0;
    if (h)
      *h = 0;
    return;
  }
  // This calculates the minimum size without actually moving windows,
  // as out_total_min_w/h are populated even if the current size is small.
  int min_w = 0, min_h = 0;
  tile_windows_internal(ctx, &min_w, &min_h);
  if (w)
    *w = min_w;
  if (h)
    *h = min_h;
}

int mdgui_is_tile_manager_enabled(MDGUI_Context *ctx) {
  if (!ctx)
    return 0;
  return ctx->window.tile_manager_enabled ? 1 : 0;
}

void mdgui_tile_windows(MDGUI_Context *ctx) { tile_windows_internal(ctx); }

void mdgui_set_window_tile_weight(MDGUI_Context *ctx, const char *title,
                                  int weight) {
  if (!ctx || !title)
    return;
  const int normalized = normalize_tile_weight(weight);
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    auto &w = ctx->window.windows[i];
    if (w.id != title)
      continue;
    w.tile_weight = normalized;
    return;
  }
}

int mdgui_get_window_tile_weight(MDGUI_Context *ctx, const char *title) {
  if (!ctx || !title)
    return 1;
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    const auto &w = ctx->window.windows[i];
    if (w.id == title)
      return normalize_tile_weight(w.tile_weight);
  }
  return 1;
}

void mdgui_set_window_tile_side(MDGUI_Context *ctx, const char *title,
                                int side) {
  if (!ctx || !title)
    return;
  const int normalized = normalize_tile_side(side);
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    auto &w = ctx->window.windows[i];
    if (w.id != title)
      continue;
    w.tile_side = normalized;
    return;
  }
}

int mdgui_get_window_tile_side(MDGUI_Context *ctx, const char *title) {
  if (!ctx || !title)
    return MDGUI_TILE_SIDE_AUTO;
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    const auto &w = ctx->window.windows[i];
    if (w.id == title)
      return normalize_tile_side(w.tile_side);
  }
  return MDGUI_TILE_SIDE_AUTO;
}

void mdgui_set_window_tile_excluded(MDGUI_Context *ctx, const char *title,
                                    int excluded) {
  if (!ctx || !title)
    return;
  const bool normalized = (excluded != 0);
  set_pending_tile_excluded_title(ctx, title, normalized);
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    auto &w = ctx->window.windows[i];
    if (w.id != title)
      continue;
    w.tile_excluded = normalized;
    if (ctx->window.tile_manager_enabled)
      tile_windows_internal(ctx);
    return;
  }
}

int mdgui_is_window_tile_excluded(MDGUI_Context *ctx, const char *title) {
  if (!ctx || !title)
    return 0;
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    const auto &w = ctx->window.windows[i];
    if (w.id == title)
      return w.tile_excluded ? 1 : 0;
  }
  return 0;
}

void mdgui_set_window_fullscreen(MDGUI_Context *ctx, const char *title,
                                 int fullscreen) {
  if (!ctx || !title)
    return;
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    auto &w = ctx->window.windows[i];
    if (w.id != title)
      continue;
    if (w.disallow_maximize) {
      w.is_maximized = false;
      return;
    }
    if (fullscreen) {
      w.fixed_rect = false;
      if (!w.is_maximized) {
        w.restored_x = w.x;
        w.restored_y = w.y;
        w.restored_w = w.w;
        w.restored_h = w.h;
      }
      w.is_maximized = true;
    } else {
      w.fixed_rect = false;
      if (w.is_maximized) {
        w.x = w.restored_x;
        w.y = w.restored_y;
        w.w = w.restored_w;
        w.h = w.restored_h;
      }
      w.is_maximized = false;
    }
    return;
  }
}

int mdgui_is_window_fullscreen(MDGUI_Context *ctx, const char *title) {
  if (!ctx || !title)
    return 0;
  for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
    if (ctx->window.windows[i].id == title)
      return ctx->window.windows[i].is_maximized ? 1 : 0;
  }
  return 0;
}

void mdgui_set_custom_cursor_enabled(MDGUI_Context *ctx, int enabled) {
  if (!ctx)
    return;
  ctx->cursor.custom_cursor_enabled = (enabled != 0);
}

int mdgui_is_custom_cursor_enabled(MDGUI_Context *ctx) {
  if (!ctx)
    return 0;
  return ctx->cursor.custom_cursor_enabled ? 1 : 0;
}

}


