#include "../core/windowing_internal.hpp"

extern "C" {
int mdgui_input_text(MDGUI_Context *ctx, const char *label, char *buffer,
                     int buffer_size, int w) {
  if (!ctx || !buffer || buffer_size <= 1 || ctx->window.current_window < 0)
    return 0;
  ctx->layout.window_has_nonlabel_widget = true;
  auto &win = ctx->window.windows[ctx->window.current_window];
  const int topmost = is_current_window_topmost(ctx);
  const int box_h = 12;
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  w = layout_resolve_width(ctx, local_x, w, 40);
  const int ix = ctx->window.origin_x + local_x;
  const int label_h = (label && mdgui_fonts[1]) ? ctx->layout.style.label_h : 0;
  const int box_logical_y = logical_y + label_h;
  const int iy = box_logical_y - win.text_scroll;
  const uintptr_t buf_addr = (uintptr_t)buffer;
  const int input_id = (int)(((buf_addr >> 3) & 0x7fffffff) ^
                             ((ix & 0xffff) << 16) ^ 0x54455854);
  const bool focused = (ctx->interaction.active_text_input_window == ctx->window.current_window &&
                        ctx->interaction.active_text_input_id == input_id);

  const int hovered =
      topmost &&
      point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, ix, iy, w, box_h);
  int len = 0;
  while (len < (buffer_size - 1) && buffer[len] != '\0')
    len++;
  buffer[len] = '\0';
  int pre_cursor =
      focused ? clamp_text_cursor(ctx->interaction.active_text_input_cursor, len) : len;
  int pre_scroll_px = 0;
  if (mdgui_fonts[1]) {
    std::string pre_prefix(buffer, (size_t)pre_cursor);
    const int cursor_px = mdgui_fonts[1]->measureTextWidth(pre_prefix.c_str());
    const int view_w = std::max(1, w - 6);
    if (cursor_px > view_w - 2)
      pre_scroll_px = cursor_px - (view_w - 2);
  }
  if (hovered && ctx->render.input.mouse_pressed) {
    ctx->interaction.active_text_input_window = ctx->window.current_window;
    ctx->interaction.active_text_input_id = input_id;
    ctx->interaction.active_text_input_multiline = false;
    ctx->interaction.active_text_input_scroll_y = 0;
    const int text_x = ix + 3;
    const int target_px = ctx->render.input.mouse_x - text_x + pre_scroll_px;
    const int click_cursor = text_cursor_from_pixel_x(buffer, len, target_px);
    ctx->interaction.active_text_input_cursor = click_cursor;
    ctx->interaction.active_text_input_sel_anchor = click_cursor;
    ctx->interaction.active_text_input_sel_start = click_cursor;
    ctx->interaction.active_text_input_sel_end = click_cursor;
    ctx->interaction.active_text_input_drag_select = ctx->render.input.mouse_down != 0;
    ctx->render.input.mouse_pressed = 0;
  } else if (ctx->render.input.mouse_pressed && focused && topmost &&
             !point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, ix, iy, w,
                            box_h)) {
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

  const bool active = (ctx->interaction.active_text_input_window == ctx->window.current_window &&
                       ctx->interaction.active_text_input_id == input_id);
  int flags = 0;
  if (active) {
    ctx->interaction.active_text_input_cursor =
        clamp_text_cursor(ctx->interaction.active_text_input_cursor, len);
    ctx->interaction.active_text_input_sel_anchor =
        clamp_text_cursor(ctx->interaction.active_text_input_sel_anchor, len);
    ctx->interaction.active_text_input_sel_start =
        clamp_text_cursor(ctx->interaction.active_text_input_sel_start, len);
    ctx->interaction.active_text_input_sel_end =
        clamp_text_cursor(ctx->interaction.active_text_input_sel_end, len);
  }

  if (active && ctx->interaction.active_text_input_drag_select && ctx->render.input.mouse_down &&
      hovered) {
    const int text_x = ix + 3;
    const int target_px = ctx->render.input.mouse_x - text_x + pre_scroll_px;
    const int drag_cursor = text_cursor_from_pixel_x(buffer, len, target_px);
    ctx->interaction.active_text_input_cursor = drag_cursor;
    ctx->interaction.active_text_input_sel_start =
        std::min(ctx->interaction.active_text_input_sel_anchor, drag_cursor);
    ctx->interaction.active_text_input_sel_end =
        std::max(ctx->interaction.active_text_input_sel_anchor, drag_cursor);
  }
  if (active && !ctx->render.input.mouse_down) {
    ctx->interaction.active_text_input_drag_select = false;
  }

  if (active && topmost) {
    const bool has_selection =
        ctx->interaction.active_text_input_sel_end > ctx->interaction.active_text_input_sel_start;

    if (ctx->render.input.key_left) {
      if (has_selection) {
        ctx->interaction.active_text_input_cursor = ctx->interaction.active_text_input_sel_start;
      } else {
        ctx->interaction.active_text_input_cursor -= 1;
      }
      ctx->interaction.active_text_input_sel_start = ctx->interaction.active_text_input_cursor;
      ctx->interaction.active_text_input_sel_end = ctx->interaction.active_text_input_cursor;
    }
    if (ctx->render.input.key_right) {
      if (has_selection) {
        ctx->interaction.active_text_input_cursor = ctx->interaction.active_text_input_sel_end;
      } else {
        ctx->interaction.active_text_input_cursor += 1;
      }
      ctx->interaction.active_text_input_sel_start = ctx->interaction.active_text_input_cursor;
      ctx->interaction.active_text_input_sel_end = ctx->interaction.active_text_input_cursor;
    }
    if (ctx->render.input.key_home) {
      ctx->interaction.active_text_input_cursor = 0;
      ctx->interaction.active_text_input_sel_start = 0;
      ctx->interaction.active_text_input_sel_end = 0;
    }
    if (ctx->render.input.key_end) {
      ctx->interaction.active_text_input_cursor = len;
      ctx->interaction.active_text_input_sel_start = len;
      ctx->interaction.active_text_input_sel_end = len;
    }
    ctx->interaction.active_text_input_cursor =
        clamp_text_cursor(ctx->interaction.active_text_input_cursor, len);

    const int sel_start = ctx->interaction.active_text_input_sel_start;
    const int sel_end = ctx->interaction.active_text_input_sel_end;
    const bool has_selection_now = sel_end > sel_start;

    if (ctx->render.input.key_backspace &&
        (has_selection_now || ctx->interaction.active_text_input_cursor > 0)) {
      if (has_selection_now) {
        memmove(buffer + sel_start, buffer + sel_end,
                (size_t)(len - sel_end + 1));
        len -= (sel_end - sel_start);
        ctx->interaction.active_text_input_cursor = sel_start;
      } else {
        const int remove_idx = ctx->interaction.active_text_input_cursor - 1;
        memmove(buffer + remove_idx, buffer + remove_idx + 1,
                (size_t)(len - remove_idx));
        len -= 1;
        ctx->interaction.active_text_input_cursor -= 1;
      }
      ctx->interaction.active_text_input_sel_start = ctx->interaction.active_text_input_cursor;
      ctx->interaction.active_text_input_sel_end = ctx->interaction.active_text_input_cursor;
      flags |= MDGUI_INPUT_TEXT_CHANGED;
    }
    if (ctx->render.input.key_delete &&
        (has_selection_now || ctx->interaction.active_text_input_cursor < len)) {
      if (has_selection_now) {
        memmove(buffer + sel_start, buffer + sel_end,
                (size_t)(len - sel_end + 1));
        len -= (sel_end - sel_start);
        ctx->interaction.active_text_input_cursor = sel_start;
      } else {
        const int remove_idx = ctx->interaction.active_text_input_cursor;
        memmove(buffer + remove_idx, buffer + remove_idx + 1,
                (size_t)(len - remove_idx));
        len -= 1;
      }
      ctx->interaction.active_text_input_sel_start = ctx->interaction.active_text_input_cursor;
      ctx->interaction.active_text_input_sel_end = ctx->interaction.active_text_input_cursor;
      flags |= MDGUI_INPUT_TEXT_CHANGED;
    }
    if (ctx->render.input.text_input) {
      if (ctx->interaction.active_text_input_sel_end > ctx->interaction.active_text_input_sel_start) {
        const int s0 = ctx->interaction.active_text_input_sel_start;
        const int s1 = ctx->interaction.active_text_input_sel_end;
        memmove(buffer + s0, buffer + s1, (size_t)(len - s1 + 1));
        len -= (s1 - s0);
        ctx->interaction.active_text_input_cursor = s0;
        ctx->interaction.active_text_input_sel_start = s0;
        ctx->interaction.active_text_input_sel_end = s0;
      }
      const unsigned char *p = (const unsigned char *)ctx->render.input.text_input;
      while (*p && len < (buffer_size - 1)) {
        const unsigned char ch = *p++;
        if (ch < 32 || ch > 126)
          continue;
        const int insert_at = ctx->interaction.active_text_input_cursor;
        memmove(buffer + insert_at + 1, buffer + insert_at,
                (size_t)(len - insert_at + 1));
        buffer[insert_at] = (char)ch;
        len += 1;
        ctx->interaction.active_text_input_cursor += 1;
        ctx->interaction.active_text_input_sel_start = ctx->interaction.active_text_input_cursor;
        ctx->interaction.active_text_input_sel_end = ctx->interaction.active_text_input_cursor;
        flags |= MDGUI_INPUT_TEXT_CHANGED;
      }
    }
    if (ctx->render.input.key_enter)
      flags |= MDGUI_INPUT_TEXT_SUBMITTED;
  }

  const int border_color = active ? CLR_ACCENT : CLR_WINDOW_BORDER;
  mdgui_draw_hline_idx(nullptr, border_color, ix - 1, iy - 1, ix + w + 1);
  mdgui_draw_hline_idx(nullptr, border_color, ix - 1, iy + box_h + 1,
                       ix + w + 1);
  mdgui_draw_vline_idx(nullptr, border_color, ix - 1, iy - 1, iy + box_h + 1);
  mdgui_draw_vline_idx(nullptr, border_color, ix + w + 1, iy - 1,
                       iy + box_h + 1);
  mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, ix, iy, w, box_h);

  const int text_x = ix + 3;
  const int text_y = iy + 2;
  int scroll_px = 0;
  if (mdgui_fonts[1]) {
    const int display_cursor = active ? ctx->interaction.active_text_input_cursor : len;
    std::string prefix(buffer, (size_t)display_cursor);
    const int cursor_text_x = mdgui_fonts[1]->measureTextWidth(prefix.c_str());
    const int view_w = std::max(1, w - 6);
    if (cursor_text_x > view_w - 2)
      scroll_px = cursor_text_x - (view_w - 2);

    if (set_widget_clip_intersect_content(ctx, ix + 1, iy + 1,
                                          std::max(1, w - 2),
                                          std::max(1, box_h - 2))) {
      if (active &&
          ctx->interaction.active_text_input_sel_end > ctx->interaction.active_text_input_sel_start) {
        std::string sel_prefix(buffer,
                               (size_t)ctx->interaction.active_text_input_sel_start);
        std::string sel_text(buffer + ctx->interaction.active_text_input_sel_start,
                             (size_t)(ctx->interaction.active_text_input_sel_end -
                                      ctx->interaction.active_text_input_sel_start));
        const int sel_x0 =
            text_x + mdgui_fonts[1]->measureTextWidth(sel_prefix.c_str()) -
            scroll_px;
        const int sel_w = mdgui_fonts[1]->measureTextWidth(sel_text.c_str());
        if (sel_w > 0)
          mdgui_fill_rect_idx(nullptr, CLR_ACCENT, sel_x0, iy + 1, sel_w,
                              box_h - 2);
      }
      mdgui_fonts[1]->drawText(buffer, nullptr, text_x - scroll_px, text_y,
                               CLR_MENU_TEXT);

      if (active) {
        const unsigned long long ticks = mdgui_backend_get_ticks_ms();
        if (((ticks / 500ull) % 2ull) == 0ull) {
          const int cx = text_x + cursor_text_x - scroll_px;
          mdgui_draw_vline_idx(nullptr, CLR_MENU_TEXT, cx, iy + 2,
                               iy + box_h - 2);
        }
      }
    }
    set_content_clip(ctx);
  }

  int right = ix + w;
  if (label && mdgui_fonts[1]) {
    mdgui_fonts[1]->drawText(label, nullptr, ix, logical_y - win.text_scroll,
                             CLR_TEXT_LIGHT);
    const int label_w = mdgui_fonts[1]->measureTextWidth(label);
    if (ix + label_w > right)
      right = ix + label_w;
  }

  note_content_bounds(ctx, right, box_logical_y + box_h);
  layout_commit_widget(ctx, local_x, logical_y, w, label_h + box_h,
                       ctx->layout.style.spacing_y);
  return flags;
}

int mdgui_input_text_multiline(MDGUI_Context *ctx, const char *label,
                               char *buffer, int buffer_size, int w, int h,
                               int flags) {
  if (!ctx || !buffer || buffer_size <= 1 || ctx->window.current_window < 0)
    return 0;
  ctx->layout.window_has_nonlabel_widget = true;
  auto &win = ctx->window.windows[ctx->window.current_window];
  const int topmost = is_current_window_topmost(ctx);
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  w = layout_resolve_width(ctx, local_x, w, 40);
  if (h < 24)
    h = 24;

  const int ix = ctx->window.origin_x + local_x;
  const int label_h = (label && mdgui_fonts[1]) ? ctx->layout.style.label_h : 0;
  const int box_logical_y = logical_y + label_h;
  const int iy = box_logical_y - win.text_scroll;

  int len = 0;
  while (len < (buffer_size - 1) && buffer[len] != '\0')
    len++;
  buffer[len] = '\0';

  std::vector<int> line_starts;
  line_starts.reserve(16);
  line_starts.push_back(0);
  for (int i = 0; i < len; ++i) {
    if (buffer[i] == '\n')
      line_starts.push_back(i + 1);
  }
  const int line_count = (int)line_starts.size();
  auto line_end = [&](int line_idx) {
    if (line_idx < 0)
      return 0;
    if (line_idx >= line_count)
      return len;
    if (line_idx + 1 < line_count)
      return line_starts[line_idx + 1] - 1;
    return len;
  };

  const int line_h = 12;
  const int content_h = line_count * line_h + 4;
  if ((flags & MDGUI_INPUT_TEXT_MULTILINE_AUTO_HEIGHT) != 0 && content_h > h)
    h = content_h;

  const uintptr_t buf_addr = (uintptr_t)buffer;
  const int input_id = (int)(((buf_addr >> 3) & 0x7fffffff) ^
                             ((ix & 0xffff) << 16) ^ 0x4d4c5449);
  const bool focused = (ctx->interaction.active_text_input_window == ctx->window.current_window &&
                        ctx->interaction.active_text_input_id == input_id);
  if (focused && !ctx->interaction.active_text_input_multiline) {
    ctx->interaction.active_text_input_scroll_y = 0;
    ctx->interaction.active_text_input_multiline = true;
  }

  const int hovered =
      topmost &&
      point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, ix, iy, w, h);

  auto cursor_line_for = [&](int cursor_pos) {
    int line_idx = 0;
    for (int i = 0; i < line_count; ++i) {
      if (line_starts[i] <= cursor_pos)
        line_idx = i;
      else
        break;
    }
    return line_idx;
  };

  auto cursor_from_mouse = [&](int mouse_x, int mouse_y) {
    int scroll_y = focused ? ctx->interaction.active_text_input_scroll_y : 0;
    if (scroll_y < 0)
      scroll_y = 0;
    const int text_x = ix + 3;
    const int text_y = iy + 2;
    int line_idx = (mouse_y - text_y + scroll_y) / line_h;
    if (line_idx < 0)
      line_idx = 0;
    if (line_idx >= line_count)
      line_idx = line_count - 1;
    const int ls = line_starts[line_idx];
    const int le = line_end(line_idx);
    const int local_x = mouse_x - text_x;
    return ls + text_cursor_from_pixel_x(buffer + ls, le - ls, local_x);
  };

  if (hovered && ctx->render.input.mouse_pressed) {
    const int click_cursor =
        cursor_from_mouse(ctx->render.input.mouse_x, ctx->render.input.mouse_y);
    ctx->interaction.active_text_input_window = ctx->window.current_window;
    ctx->interaction.active_text_input_id = input_id;
    ctx->interaction.active_text_input_cursor = click_cursor;
    ctx->interaction.active_text_input_sel_anchor = click_cursor;
    ctx->interaction.active_text_input_sel_start = click_cursor;
    ctx->interaction.active_text_input_sel_end = click_cursor;
    ctx->interaction.active_text_input_drag_select = true;
    ctx->interaction.active_text_input_multiline = true;
    ctx->render.input.mouse_pressed = 0;
  } else if (ctx->render.input.mouse_pressed && focused && topmost &&
             !point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, ix, iy, w,
                            h)) {
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

  const bool active = (ctx->interaction.active_text_input_window == ctx->window.current_window &&
                       ctx->interaction.active_text_input_id == input_id);
  int result_flags = 0;
  if (active) {
    ctx->interaction.active_text_input_cursor =
        clamp_text_cursor(ctx->interaction.active_text_input_cursor, len);
    ctx->interaction.active_text_input_sel_anchor =
        clamp_text_cursor(ctx->interaction.active_text_input_sel_anchor, len);
    ctx->interaction.active_text_input_sel_start =
        clamp_text_cursor(ctx->interaction.active_text_input_sel_start, len);
    ctx->interaction.active_text_input_sel_end =
        clamp_text_cursor(ctx->interaction.active_text_input_sel_end, len);
  }
  if (active &&
      (ctx->render.input.text_input || ctx->render.input.key_backspace ||
       ctx->render.input.key_delete || ctx->render.input.key_left || ctx->render.input.key_right ||
       ctx->render.input.key_up || ctx->render.input.key_down || ctx->render.input.key_home ||
       ctx->render.input.key_end || ctx->render.input.key_enter)) {
    ctx->interaction.active_text_input_drag_select = false;
  }
  if (ctx->interaction.active_text_input_scroll_y < 0)
    ctx->interaction.active_text_input_scroll_y = 0;

  if (active && ctx->interaction.active_text_input_drag_select && ctx->render.input.mouse_down &&
      hovered) {
    const int drag_cursor =
        cursor_from_mouse(ctx->render.input.mouse_x, ctx->render.input.mouse_y);
    ctx->interaction.active_text_input_cursor = drag_cursor;
    ctx->interaction.active_text_input_sel_start =
        std::min(ctx->interaction.active_text_input_sel_anchor, drag_cursor);
    ctx->interaction.active_text_input_sel_end =
        std::max(ctx->interaction.active_text_input_sel_anchor, drag_cursor);
  }
  if (active && !ctx->render.input.mouse_down) {
    ctx->interaction.active_text_input_drag_select = false;
  }

  if (active && hovered && ctx->render.input.mouse_wheel != 0) {
    const int view_h = std::max(1, h - 4);
    const int max_scroll = std::max(0, content_h - view_h);
    ctx->interaction.active_text_input_scroll_y -= ctx->render.input.mouse_wheel * 12;
    if (ctx->interaction.active_text_input_scroll_y < 0)
      ctx->interaction.active_text_input_scroll_y = 0;
    if (ctx->interaction.active_text_input_scroll_y > max_scroll)
      ctx->interaction.active_text_input_scroll_y = max_scroll;
  }

  if (active && topmost) {
    const bool has_selection =
        ctx->interaction.active_text_input_sel_end > ctx->interaction.active_text_input_sel_start;
    const int cursor_line = cursor_line_for(ctx->interaction.active_text_input_cursor);
    const int cursor_line_start = line_starts[cursor_line];
    const int cursor_line_end = line_end(cursor_line);
    const int cursor_col_chars =
        std::max(0, std::min(ctx->interaction.active_text_input_cursor - cursor_line_start,
                             cursor_line_end - cursor_line_start));

    if (ctx->render.input.key_left) {
      if (has_selection) {
        ctx->interaction.active_text_input_cursor = ctx->interaction.active_text_input_sel_start;
      } else {
        ctx->interaction.active_text_input_cursor -= 1;
      }
      ctx->interaction.active_text_input_sel_start = ctx->interaction.active_text_input_cursor;
      ctx->interaction.active_text_input_sel_end = ctx->interaction.active_text_input_cursor;
    }
    if (ctx->render.input.key_right) {
      if (has_selection) {
        ctx->interaction.active_text_input_cursor = ctx->interaction.active_text_input_sel_end;
      } else {
        ctx->interaction.active_text_input_cursor += 1;
      }
      ctx->interaction.active_text_input_sel_start = ctx->interaction.active_text_input_cursor;
      ctx->interaction.active_text_input_sel_end = ctx->interaction.active_text_input_cursor;
    }
    if (ctx->render.input.key_home) {
      ctx->interaction.active_text_input_cursor = cursor_line_start;
      ctx->interaction.active_text_input_sel_start = ctx->interaction.active_text_input_cursor;
      ctx->interaction.active_text_input_sel_end = ctx->interaction.active_text_input_cursor;
    }
    if (ctx->render.input.key_end) {
      ctx->interaction.active_text_input_cursor = cursor_line_end;
      ctx->interaction.active_text_input_sel_start = ctx->interaction.active_text_input_cursor;
      ctx->interaction.active_text_input_sel_end = ctx->interaction.active_text_input_cursor;
    }
    if (ctx->render.input.key_up && cursor_line > 0) {
      const int ls = line_starts[cursor_line - 1];
      const int le = line_end(cursor_line - 1);
      ctx->interaction.active_text_input_cursor =
          ls + std::min(cursor_col_chars, std::max(0, le - ls));
      ctx->interaction.active_text_input_sel_start = ctx->interaction.active_text_input_cursor;
      ctx->interaction.active_text_input_sel_end = ctx->interaction.active_text_input_cursor;
    }
    if (ctx->render.input.key_down && cursor_line + 1 < line_count) {
      const int ls = line_starts[cursor_line + 1];
      const int le = line_end(cursor_line + 1);
      ctx->interaction.active_text_input_cursor =
          ls + std::min(cursor_col_chars, std::max(0, le - ls));
      ctx->interaction.active_text_input_sel_start = ctx->interaction.active_text_input_cursor;
      ctx->interaction.active_text_input_sel_end = ctx->interaction.active_text_input_cursor;
    }

    ctx->interaction.active_text_input_cursor =
        clamp_text_cursor(ctx->interaction.active_text_input_cursor, len);
    const int sel_start = ctx->interaction.active_text_input_sel_start;
    const int sel_end = ctx->interaction.active_text_input_sel_end;
    const bool has_selection_now = sel_end > sel_start;

    if (ctx->render.input.key_backspace &&
        (has_selection_now || ctx->interaction.active_text_input_cursor > 0)) {
      if (has_selection_now) {
        memmove(buffer + sel_start, buffer + sel_end,
                (size_t)(len - sel_end + 1));
        len -= (sel_end - sel_start);
        ctx->interaction.active_text_input_cursor = sel_start;
      } else {
        const int remove_idx = ctx->interaction.active_text_input_cursor - 1;
        memmove(buffer + remove_idx, buffer + remove_idx + 1,
                (size_t)(len - remove_idx));
        len -= 1;
        ctx->interaction.active_text_input_cursor -= 1;
      }
      ctx->interaction.active_text_input_sel_start = ctx->interaction.active_text_input_cursor;
      ctx->interaction.active_text_input_sel_end = ctx->interaction.active_text_input_cursor;
      result_flags |= MDGUI_INPUT_TEXT_CHANGED;
    }
    if (ctx->render.input.key_delete &&
        (has_selection_now || ctx->interaction.active_text_input_cursor < len)) {
      if (has_selection_now) {
        memmove(buffer + sel_start, buffer + sel_end,
                (size_t)(len - sel_end + 1));
        len -= (sel_end - sel_start);
        ctx->interaction.active_text_input_cursor = sel_start;
      } else {
        const int remove_idx = ctx->interaction.active_text_input_cursor;
        memmove(buffer + remove_idx, buffer + remove_idx + 1,
                (size_t)(len - remove_idx));
        len -= 1;
      }
      ctx->interaction.active_text_input_sel_start = ctx->interaction.active_text_input_cursor;
      ctx->interaction.active_text_input_sel_end = ctx->interaction.active_text_input_cursor;
      result_flags |= MDGUI_INPUT_TEXT_CHANGED;
    }

    if (ctx->render.input.key_enter && len < (buffer_size - 1)) {
      if (ctx->interaction.active_text_input_sel_end > ctx->interaction.active_text_input_sel_start) {
        const int s0 = ctx->interaction.active_text_input_sel_start;
        const int s1 = ctx->interaction.active_text_input_sel_end;
        memmove(buffer + s0, buffer + s1, (size_t)(len - s1 + 1));
        len -= (s1 - s0);
        ctx->interaction.active_text_input_cursor = s0;
      }
      const int at = ctx->interaction.active_text_input_cursor;
      memmove(buffer + at + 1, buffer + at, (size_t)(len - at + 1));
      buffer[at] = '\n';
      len += 1;
      ctx->interaction.active_text_input_cursor += 1;
      ctx->interaction.active_text_input_sel_start = ctx->interaction.active_text_input_cursor;
      ctx->interaction.active_text_input_sel_end = ctx->interaction.active_text_input_cursor;
      result_flags |= MDGUI_INPUT_TEXT_CHANGED;
    }

    if (ctx->render.input.text_input) {
      if (ctx->interaction.active_text_input_sel_end > ctx->interaction.active_text_input_sel_start) {
        const int s0 = ctx->interaction.active_text_input_sel_start;
        const int s1 = ctx->interaction.active_text_input_sel_end;
        memmove(buffer + s0, buffer + s1, (size_t)(len - s1 + 1));
        len -= (s1 - s0);
        ctx->interaction.active_text_input_cursor = s0;
        ctx->interaction.active_text_input_sel_start = s0;
        ctx->interaction.active_text_input_sel_end = s0;
      }
      const unsigned char *p = (const unsigned char *)ctx->render.input.text_input;
      while (*p && len < (buffer_size - 1)) {
        const unsigned char ch = *p++;
        if (ch < 32 || ch > 126)
          continue;
        const int insert_at = ctx->interaction.active_text_input_cursor;
        memmove(buffer + insert_at + 1, buffer + insert_at,
                (size_t)(len - insert_at + 1));
        buffer[insert_at] = (char)ch;
        len += 1;
        ctx->interaction.active_text_input_cursor += 1;
        ctx->interaction.active_text_input_sel_start = ctx->interaction.active_text_input_cursor;
        ctx->interaction.active_text_input_sel_end = ctx->interaction.active_text_input_cursor;
        result_flags |= MDGUI_INPUT_TEXT_CHANGED;
      }
    }
  }

  // Recompute lines after edits.
  line_starts.clear();
  line_starts.push_back(0);
  for (int i = 0; i < len; ++i) {
    if (buffer[i] == '\n')
      line_starts.push_back(i + 1);
  }

  const int border_color = active ? CLR_ACCENT : CLR_WINDOW_BORDER;
  mdgui_draw_hline_idx(nullptr, border_color, ix - 1, iy - 1, ix + w + 1);
  mdgui_draw_hline_idx(nullptr, border_color, ix - 1, iy + h + 1, ix + w + 1);
  mdgui_draw_vline_idx(nullptr, border_color, ix - 1, iy - 1, iy + h + 1);
  mdgui_draw_vline_idx(nullptr, border_color, ix + w + 1, iy - 1, iy + h + 1);
  mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, ix, iy, w, h);

  if (active) {
    int lc = (int)line_starts.size();
    if (lc < 1)
      lc = 1;
    int caret_line = 0;
    for (int i = 0; i < lc; ++i) {
      if (line_starts[i] <= ctx->interaction.active_text_input_cursor)
        caret_line = i;
      else
        break;
    }
    const int caret_y = caret_line * line_h;
    const int view_h = std::max(1, h - 4);
    if (caret_y < ctx->interaction.active_text_input_scroll_y)
      ctx->interaction.active_text_input_scroll_y = caret_y;
    if (caret_y + line_h > ctx->interaction.active_text_input_scroll_y + view_h)
      ctx->interaction.active_text_input_scroll_y = (caret_y + line_h) - view_h;
    if (ctx->interaction.active_text_input_scroll_y < 0)
      ctx->interaction.active_text_input_scroll_y = 0;
  }

  if (mdgui_fonts[1]) {
    const int text_x = ix + 3;
    const int text_y = iy + 2;
    const int scroll_y = active ? ctx->interaction.active_text_input_scroll_y : 0;

    if (set_widget_clip_intersect_content(ctx, ix + 1, iy + 1,
                                          std::max(1, w - 2),
                                          std::max(1, h - 2))) {
      const int lc = (int)line_starts.size();
      const int sel_start =
          active ? std::min(ctx->interaction.active_text_input_sel_start,
                            ctx->interaction.active_text_input_sel_end)
                 : 0;
      const int sel_end =
          active ? std::max(ctx->interaction.active_text_input_sel_start,
                            ctx->interaction.active_text_input_sel_end)
                 : 0;
      for (int li = 0; li < lc; ++li) {
        int ls = line_starts[li];
        int le = (li + 1 < lc) ? (line_starts[li + 1] - 1) : len;
        const int ly = text_y + li * line_h - scroll_y;
        if (ly + line_h < iy + 1 || ly > iy + h - 2)
          continue;

        if (active && sel_end > sel_start) {
          const int s0 = std::max(sel_start, ls);
          const int s1 = std::min(sel_end, le);
          if (s1 > s0) {
            std::string left(buffer + ls, (size_t)(s0 - ls));
            std::string mid(buffer + s0, (size_t)(s1 - s0));
            const int sx =
                text_x + mdgui_fonts[1]->measureTextWidth(left.c_str());
            const int sw = mdgui_fonts[1]->measureTextWidth(mid.c_str());
            if (sw > 0)
              mdgui_fill_rect_idx(nullptr, CLR_ACCENT, sx, ly, sw, line_h - 1);
          }
        }

        std::string line_text(buffer + ls, (size_t)(le - ls));
        mdgui_fonts[1]->drawText(line_text.c_str(), nullptr, text_x, ly,
                                 CLR_MENU_TEXT);
      }

      if (active) {
        const unsigned long long ticks = mdgui_backend_get_ticks_ms();
        if (((ticks / 500ull) % 2ull) == 0ull) {
          int caret_line = 0;
          for (int i = 0; i < lc; ++i) {
            if (line_starts[i] <= ctx->interaction.active_text_input_cursor)
              caret_line = i;
            else
              break;
          }
          const int cls = line_starts[caret_line];
          std::string left(buffer + cls,
                           (size_t)(ctx->interaction.active_text_input_cursor - cls));
          const int cx =
              text_x + mdgui_fonts[1]->measureTextWidth(left.c_str());
          const int cy = text_y + caret_line * line_h - scroll_y;
          mdgui_draw_vline_idx(nullptr, CLR_MENU_TEXT, cx, cy, cy + line_h - 1);
        }
      }
    }

    set_content_clip(ctx);
  }

  int right = ix + w;
  if (label && mdgui_fonts[1]) {
    mdgui_fonts[1]->drawText(label, nullptr, ix, logical_y - win.text_scroll,
                             CLR_TEXT_LIGHT);
    const int label_w = mdgui_fonts[1]->measureTextWidth(label);
    if (ix + label_w > right)
      right = ix + label_w;
  }

  note_content_bounds(ctx, right, box_logical_y + h);
  layout_commit_widget(ctx, local_x, logical_y, w, label_h + h,
                       ctx->layout.style.spacing_y);
  return result_flags;
}

void mdgui_progress_bar(MDGUI_Context *ctx, float value, int w, int h,
                        const char *overlay_text) {
  if (!ctx || ctx->window.current_window < 0)
    return;
  ctx->layout.window_has_nonlabel_widget = true;
  const auto &win = ctx->window.windows[ctx->window.current_window];
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  w = layout_resolve_width(ctx, local_x, w, 24);
  const int line_h = font_line_height(ctx);
  if (h < 6)
    h = std::max(6, line_h + 2);
  if (value < 0.0f)
    value = 0.0f;
  if (value > 1.0f)
    value = 1.0f;

  const int ix = ctx->window.origin_x + local_x;
  const int iy = logical_y - win.text_scroll;
  const int fill_w = (int)((float)w * value);

  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy - 1, ix + w + 1);
  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy + h + 1,
                       ix + w + 1);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy - 1, iy + h + 1);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, ix + w + 1, iy - 1,
                       iy + h + 1);
  mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, ix, iy, w, h);
  if (fill_w > 0)
    mdgui_fill_rect_idx(nullptr, CLR_ACCENT, ix, iy, fill_w, h);

  if (overlay_text && resolve_font(ctx)) {
    const int tw = font_measure_text(ctx, overlay_text);
    font_draw_text(ctx, overlay_text, ix + (w - tw) / 2,
                   iy + (h - line_h) / 2, CLR_TEXT_LIGHT);
  }
  note_content_bounds(ctx, ix + w, logical_y + h);
  layout_commit_widget(ctx, local_x, logical_y, w, h, ctx->layout.style.spacing_y + 2);
}

void mdgui_frame_time_graph(MDGUI_Context *ctx, const float *frame_ms_samples,
                            int sample_count, float target_fps,
                            float graph_max_ms, int w, int h) {
  if (!ctx || ctx->window.current_window < 0)
    return;
  ctx->layout.window_has_nonlabel_widget = true;
  const auto &win = ctx->window.windows[ctx->window.current_window];

  const int requested_w = w;
  const int requested_h = h;
  const bool fill_mode = (requested_w == 0 && requested_h == 0);
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  if (fill_mode) {
    int avail_w = (win.x + win.w - 2) - (ctx->window.origin_x + local_x);
    if (avail_w < 24)
      avail_w = 24;
    w = avail_w;
  } else {
    w = layout_resolve_width(ctx, local_x, w, 24);
  }
  if (target_fps < 1.0f)
    target_fps = 1.0f;
  if (graph_max_ms < 1.0f)
    graph_max_ms = 1.0f;

  const int ix = ctx->window.origin_x + local_x;
  const int iy = logical_y - win.text_scroll;

  // Height 0/negative mirrors dynamic
  // width behavior: fill remaining
  // viewport.
  if (requested_h == 0 || requested_h < 0) {
    const int viewport_bottom = win.y + win.h - 4;
    int avail_h = viewport_bottom - logical_y;
    if (requested_h < 0)
      avail_h += requested_h;
    h = avail_h;
  }
  if (h < 12)
    h = 12;

  if (!fill_mode) {
    mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy - 1,
                         ix + w + 1);
    mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy + h + 1,
                         ix + w + 1);
    mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy - 1,
                         iy + h + 1);
    mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, ix + w + 1, iy - 1,
                         iy + h + 1);
  }
  mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, ix, iy, w, h);

  const int pad = 2;
  const int plot_x = ix + pad;
  const int plot_y = iy + pad;
  const int plot_w = w - (pad * 2);
  const int plot_h = h - (pad * 2);

  if (plot_w > 1 && plot_h > 1) {
    const float target_ms = 1000.0f / target_fps;
    float target_ratio = target_ms / graph_max_ms;
    if (target_ratio < 0.0f)
      target_ratio = 0.0f;
    if (target_ratio > 1.0f)
      target_ratio = 1.0f;
    const int target_y = plot_y + plot_h - (int)(target_ratio * (float)plot_h);
    mdgui_draw_hline_idx(nullptr, CLR_ACCENT, plot_x, target_y,
                         plot_x + plot_w);

    if (frame_ms_samples && sample_count > 0) {
      for (int px = 0; px < plot_w; ++px) {
        int sample_index = (px * sample_count) / plot_w;
        if (sample_index < 0)
          sample_index = 0;
        if (sample_index >= sample_count)
          sample_index = sample_count - 1;

        float ms = frame_ms_samples[sample_index];
        if (ms < 0.0f)
          ms = 0.0f;
        float ratio = ms / graph_max_ms;
        if (ratio < 0.0f)
          ratio = 0.0f;
        if (ratio > 1.0f)
          ratio = 1.0f;

        int bar_h = (int)(ratio * (float)plot_h);
        if (bar_h < 1 && ms > 0.0f)
          bar_h = 1;
        const int by = plot_y + plot_h - bar_h;

        int bar_color = CLR_ACCENT;
        if (ms > target_ms * 1.2f)
          bar_color = CLR_BUTTON_DARK;
        else if (ms > target_ms)
          bar_color = CLR_BUTTON_LIGHT;

        if (bar_h > 0)
          mdgui_fill_rect_idx(nullptr, bar_color, plot_x + px, by, 1, bar_h);
      }
    }
  }

  int intrinsic_w = w;
  if (requested_w <= 0)
    intrinsic_w = 24;

  // In fill mode, the graph tracks
  // current window height. Reporting
  // that full height as required
  // content would ratchet min_h
  // upward and prevent shrinking.
  // Report only a minimal intrinsic
  // height for min-size calculations.
  const int intrinsic_h = fill_mode ? 12 : h;
  note_content_bounds(ctx, ix + intrinsic_w, logical_y + intrinsic_h);
  if (fill_mode) {
    layout_commit_widget(ctx, local_x, logical_y, w, h, 0);
  } else {
    layout_commit_widget(ctx, local_x, logical_y, w, h, ctx->layout.style.spacing_y);
  }
}

}


