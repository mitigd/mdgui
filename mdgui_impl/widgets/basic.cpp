#include "../core/windowing_internal.hpp"

extern "C" {
void mdgui_same_line(MDGUI_Context *ctx) {
  if (!ctx || ctx->window.current_window < 0)
    return;
  if (ctx->layout.has_last_item)
    ctx->layout.same_line = true;
}

void mdgui_spacing(MDGUI_Context *ctx, int pixels);

void mdgui_new_line(MDGUI_Context *ctx) {
  if (!ctx || ctx->window.current_window < 0)
    return;
  if (!ctx->layout.has_last_item) {
    mdgui_spacing(ctx, ctx->layout.style.spacing_y);
    return;
  }
  const int new_y = ctx->layout.last_item_y + ctx->layout.last_item_h +
                    ctx->layout.style.spacing_y;
  if (ctx->layout.columns_active &&
      ctx->layout.columns_index < (int)ctx->layout.columns_bottoms.size()) {
    int &col_bottom = ctx->layout.columns_bottoms[ctx->layout.columns_index];
    if (new_y > col_bottom)
      col_bottom = new_y;
    if (col_bottom > ctx->layout.columns_max_bottom)
      ctx->layout.columns_max_bottom = col_bottom;
    ctx->window.content_y = col_bottom;
  } else {
    ctx->window.content_y = new_y;
  }
  ctx->layout.same_line = false;
}

void mdgui_spacing(MDGUI_Context *ctx, int pixels) {
  if (!ctx || ctx->window.current_window < 0)
    return;
  if (pixels < 0)
    pixels = 0;
  if (ctx->layout.columns_active &&
      ctx->layout.columns_index < (int)ctx->layout.columns_bottoms.size()) {
    int &col_bottom = ctx->layout.columns_bottoms[ctx->layout.columns_index];
    col_bottom += pixels;
    if (col_bottom > ctx->layout.columns_max_bottom)
      ctx->layout.columns_max_bottom = col_bottom;
    ctx->window.content_y = col_bottom;
  } else {
    ctx->window.content_y += pixels;
  }
  ctx->layout.same_line = false;
}

void mdgui_indent(MDGUI_Context *ctx, int pixels) {
  if (!ctx || ctx->window.current_window < 0)
    return;
  if (pixels <= 0)
    pixels = ctx->layout.style.indent_step;
  ctx->layout.indent_stack.push_back(pixels);
  ctx->layout.indent += pixels;
}

void mdgui_unindent(MDGUI_Context *ctx) {
  if (!ctx || ctx->window.current_window < 0 || ctx->layout.indent_stack.empty())
    return;
  const int pixels = ctx->layout.indent_stack.back();
  ctx->layout.indent_stack.pop_back();
  ctx->layout.indent -= pixels;
  if (ctx->layout.indent < 0)
    ctx->layout.indent = 0;
}

int mdgui_begin_columns(MDGUI_Context *ctx, int columns) {
  if (!ctx || ctx->window.current_window < 0 || columns < 1)
    return 0;
  if (ctx->layout.columns_active)
    return 0;
  int avail_w = resolve_dynamic_width(ctx, ctx->layout.indent, 0, 24);
  const int gaps = (columns - 1) * ctx->layout.style.spacing_x;
  int col_w = (avail_w - gaps) / columns;
  if (col_w < 12)
    col_w = 12;
  ctx->layout.columns_active = true;
  ctx->layout.columns_count = columns;
  ctx->layout.columns_index = 0;
  ctx->layout.columns_start_x = ctx->layout.indent;
  ctx->layout.columns_start_y = ctx->window.content_y;
  ctx->layout.columns_width = col_w;
  ctx->layout.columns_max_bottom = ctx->window.content_y;
  ctx->layout.columns_bottoms.assign((size_t)columns, ctx->window.content_y);
  ctx->layout.same_line = false;
  ctx->layout.has_last_item = false;
  return 1;
}

void mdgui_next_column(MDGUI_Context *ctx) {
  if (!ctx || !ctx->layout.columns_active)
    return;
  if (ctx->layout.columns_index + 1 < ctx->layout.columns_count) {
    ctx->layout.columns_index += 1;
    if (ctx->layout.columns_index < (int)ctx->layout.columns_bottoms.size())
      ctx->window.content_y = ctx->layout.columns_bottoms[ctx->layout.columns_index];
  }
  ctx->layout.same_line = false;
  ctx->layout.has_last_item = false;
}

void mdgui_end_columns(MDGUI_Context *ctx) {
  if (!ctx || !ctx->layout.columns_active)
    return;
  ctx->window.content_y = ctx->layout.columns_max_bottom;
  ctx->layout.columns_active = false;
  ctx->layout.columns_count = 0;
  ctx->layout.columns_index = 0;
  ctx->layout.columns_width = 0;
  ctx->layout.columns_bottoms.clear();
  ctx->layout.same_line = false;
  ctx->layout.has_last_item = false;
  mdgui_spacing(ctx, ctx->layout.style.spacing_y);
}

int mdgui_begin_row(MDGUI_Context *ctx, int columns) {
  return mdgui_begin_columns(ctx, columns);
}

void mdgui_end_row(MDGUI_Context *ctx) { mdgui_end_columns(ctx); }

int mdgui_button(MDGUI_Context *ctx, const char *text, int w, int h) {
  if (!ctx || ctx->window.current_window < 0)
    return 0;
  ctx->layout.window_has_nonlabel_widget = true;
  const auto &win = ctx->window.windows[ctx->window.current_window];
  MDGUI_Font *font = resolve_font(ctx);
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  const int text_h = font_line_height(ctx);
  if (h < 8)
    h = std::max(12, text_h + 4);
  const int text_min_w =
      (text && font) ? (font_measure_text(ctx, text) + 8) : 0;
  w = layout_resolve_width(ctx, local_x, w, std::max(12, text_min_w));

  const int abs_x = ctx->window.origin_x + local_x;
  const int abs_y = logical_y - win.text_scroll;
  const int topmost = is_current_window_topmost(ctx);
  const int hovered =
      topmost &&
      point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, abs_x, abs_y, w, h);
  const int depressed = hovered && ctx->render.input.mouse_down;

  if (depressed) {
    mdgui_fill_rect_idx(nullptr, CLR_BUTTON_PRESSED, abs_x, abs_y, w, h);
    mdgui_draw_hline_idx(nullptr, CLR_BUTTON_DARK, abs_x, abs_y, abs_x + w);
    mdgui_draw_vline_idx(nullptr, CLR_BUTTON_DARK, abs_x + w - 1, abs_y,
                         abs_y + h);
    mdgui_draw_vline_idx(nullptr, CLR_BUTTON_LIGHT, abs_x, abs_y, abs_y + h);
    mdgui_draw_hline_idx(nullptr, CLR_BUTTON_LIGHT, abs_x, abs_y + h - 1,
                         abs_x + w);
  } else {
    mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, abs_x, abs_y, w, h);
    mdgui_draw_hline_idx(nullptr, CLR_BUTTON_LIGHT, abs_x, abs_y, abs_x + w);
    mdgui_draw_vline_idx(nullptr, CLR_BUTTON_LIGHT, abs_x + w - 1, abs_y,
                         abs_y + h);
    mdgui_draw_vline_idx(nullptr, CLR_BUTTON_DARK, abs_x, abs_y, abs_y + h);
    mdgui_draw_hline_idx(nullptr, CLR_BUTTON_DARK, abs_x, abs_y + h - 1,
                         abs_x + w);
  }

  if (text && font && w > 2 && h > 2) {
    const int tx = abs_x + (w - font_measure_text(ctx, text)) / 2;
    const int ty = abs_y + (h - text_h) / 2;
    if (set_widget_clip_intersect_content(ctx, abs_x + 1, abs_y + 1, w - 2,
                                          h - 2)) {
      font_draw_text(ctx, text, tx, ty, CLR_TEXT_LIGHT);
    }
    set_content_clip(ctx);
  }
  note_content_bounds(ctx, abs_x + w, logical_y + h);
  layout_commit_widget(ctx, local_x, logical_y, w, h, ctx->layout.style.spacing_y);

  return hovered && ctx->render.input.mouse_pressed && win.z == ctx->window.z_counter;
}

void mdgui_label_font(MDGUI_Context *ctx, const char *text, MDGUI_Font *font) {
  if (!ctx || !text || !resolve_font(ctx, font) || ctx->window.current_window < 0)
    return;
  const auto &win = ctx->window.windows[ctx->window.current_window];
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  const int top_inset = std::max(1, ctx->layout.style.spacing_y / 2);
  const int line_h = std::max(ctx->layout.style.label_h, font_line_height(ctx, font));
  const int label_logical_y = logical_y + top_inset;
  const int abs_x = ctx->window.origin_x + local_x;
  const int abs_y = label_logical_y - win.text_scroll;
  font_draw_text(ctx, text, abs_x, abs_y, CLR_TEXT_LIGHT, font);
  note_content_bounds(ctx, abs_x + font_measure_text(ctx, text, font),
                      label_logical_y + line_h);
  layout_commit_widget(ctx, local_x, logical_y, 1, line_h + top_inset,
                       ctx->layout.style.spacing_y);
}

void mdgui_label(MDGUI_Context *ctx, const char *text) {
  mdgui_label_font(ctx, text, nullptr);
}

void mdgui_label_wrapped_font(MDGUI_Context *ctx, const char *text, int w,
                              MDGUI_Font *font) {
  if (!ctx || !text || !resolve_font(ctx, font) || ctx->window.current_window < 0)
    return;
  const auto &win = ctx->window.windows[ctx->window.current_window];
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  const int top_inset = std::max(1, ctx->layout.style.spacing_y / 2);
  const int wrapped_logical_y = logical_y + top_inset;
  const int abs_x = ctx->window.origin_x + local_x;
  const int wrap_w = layout_resolve_width(ctx, local_x, w, 20);
  const int margin_l = 2;
  const int margin_r = 2;
  const int draw_x = abs_x + margin_l;
  const int inner_wrap_w = std::max(10, wrap_w - (margin_l + margin_r));
  const int line_h = std::max(ctx->layout.style.label_h, font_line_height(ctx, font));

  int lines_drawn = 0;
  int max_right = draw_x;

  auto draw_line = [&](const std::string &line) {
    const int abs_y =
        (wrapped_logical_y + lines_drawn * line_h) - win.text_scroll;
    font_draw_text(ctx, line.c_str(), draw_x, abs_y, CLR_TEXT_LIGHT, font);
    const int line_w = font_measure_text(ctx, line.c_str(), font);
    if (draw_x + line_w > max_right)
      max_right = draw_x + line_w;
    ++lines_drawn;
  };

  auto draw_word_wrapped = [&](const std::string &word) {
    if (word.empty()) {
      draw_line(std::string());
      return;
    }
    size_t offset = 0;
    while (offset < word.size()) {
      size_t best = 1;
      size_t limit = word.size() - offset;
      for (size_t n = 1; n <= limit; ++n) {
        const std::string candidate = word.substr(offset, n);
        if (font_measure_text(ctx, candidate.c_str(), font) <= inner_wrap_w) {
          best = n;
        } else {
          break;
        }
      }
      draw_line(word.substr(offset, best));
      offset += best;
    }
  };

  std::string all(text);
  size_t para_start = 0;
  while (para_start <= all.size()) {
    size_t para_end = all.find('\n', para_start);
    if (para_end == std::string::npos)
      para_end = all.size();
    const std::string paragraph = all.substr(para_start, para_end - para_start);

    if (paragraph.empty()) {
      draw_line(std::string());
    } else {
      size_t i = 0;
      std::string current;
      while (i < paragraph.size()) {
        while (i < paragraph.size() &&
               std::isspace((unsigned char)paragraph[i]) != 0) {
          ++i;
        }
        if (i >= paragraph.size())
          break;
        const size_t word_start = i;
        while (i < paragraph.size() &&
               std::isspace((unsigned char)paragraph[i]) == 0) {
          ++i;
        }
        const std::string word = paragraph.substr(word_start, i - word_start);
        const std::string candidate =
            current.empty() ? word : (current + " " + word);

        if (font_measure_text(ctx, candidate.c_str(), font) <= inner_wrap_w) {
          current = candidate;
          continue;
        }

        if (!current.empty()) {
          draw_line(current);
          current.clear();
          if (font_measure_text(ctx, word.c_str(), font) <= inner_wrap_w) {
            current = word;
          } else {
            draw_word_wrapped(word);
          }
        } else {
          draw_word_wrapped(word);
        }
      }
      if (!current.empty())
        draw_line(current);
    }

    if (para_end == all.size())
      break;
    para_start = para_end + 1;
  }

  if (lines_drawn < 1)
    lines_drawn = 1;
  const int bottom_margin = (lines_drawn > 1) ? 4 : 2;
  const int total_h = lines_drawn * line_h + bottom_margin;
  note_content_bounds(ctx, max_right + margin_r, wrapped_logical_y + total_h);
  layout_commit_widget(ctx, local_x, logical_y, wrap_w, total_h + top_inset,
                       ctx->layout.style.spacing_y);
}

void mdgui_label_wrapped(MDGUI_Context *ctx, const char *text, int w) {
  mdgui_label_wrapped_font(ctx, text, w, nullptr);
}

int mdgui_checkbox(MDGUI_Context *ctx, const char *text, bool *checked) {
  if (!ctx || !checked || ctx->window.current_window < 0)
    return 0;
  ctx->layout.window_has_nonlabel_widget = true;
  const auto &win = ctx->window.windows[ctx->window.current_window];
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  const int ix = ctx->window.origin_x + local_x;
  MDGUI_Font *font = resolve_font(ctx);
  const int line_h = font_line_height(ctx);
  const int box_size = std::max(10, line_h);
  const int row_h = std::max(box_size, line_h);
  const int iy = logical_y - win.text_scroll + (row_h - box_size) / 2;

  // 3D-drawn checkbox
  mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, ix, iy, box_size, box_size);
  mdgui_draw_hline_idx(nullptr, CLR_ACCENT, ix, iy, ix + box_size);
  mdgui_draw_hline_idx(nullptr, CLR_ACCENT, ix, iy + box_size - 1,
                       ix + box_size);
  mdgui_draw_vline_idx(nullptr, CLR_ACCENT, ix, iy, iy + box_size);
  mdgui_draw_vline_idx(nullptr, CLR_ACCENT, ix + box_size - 1, iy,
                       iy + box_size);

  if (*checked) {
    mdgui_draw_line_idx(nullptr, CLR_TEXT_LIGHT, ix + 2, iy + 2,
                        ix + box_size - 3, iy + box_size - 3);
    mdgui_draw_line_idx(nullptr, CLR_TEXT_LIGHT, ix + box_size - 3, iy + 2,
                        ix + 2, iy + box_size - 3);
  }

  if (text && font) {
    const int text_y = logical_y - win.text_scroll + (row_h - line_h) / 2;
    font_draw_text(ctx, text, ix + box_size + 4, text_y, CLR_TEXT_LIGHT);
  }
  const int label_w = (text && font) ? font_measure_text(ctx, text) : 0;
  note_content_bounds(ctx, ix + box_size + 4 + label_w, logical_y + row_h);

  int result = 0;
  if (ctx->render.input.mouse_pressed && is_current_window_topmost(ctx) &&
      point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, ix, iy,
                    box_size + label_w + 4, row_h)) {
    *checked = !*checked;
    result = 1;
  }

  layout_commit_widget(ctx, local_x, logical_y, box_size + label_w + 4, row_h,
                       ctx->layout.style.spacing_y);
  return result;
}

int mdgui_slider(MDGUI_Context *ctx, const char *text, float *val, float min,
                 float max, int w) {
  if (!ctx || !val || ctx->window.current_window < 0)
    return 0;
  ctx->layout.window_has_nonlabel_widget = true;
  const auto &win = ctx->window.windows[ctx->window.current_window];
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  w = layout_resolve_width(ctx, local_x, w, 24);
  const int ix = ctx->window.origin_x + local_x;
  MDGUI_Font *font = resolve_font(ctx);
  const int line_h = font_line_height(ctx);
  const int thumb_h = std::max(10, line_h + 2);
  const int text_w = (text && font) ? font_measure_text(ctx, text) : 0;
  const int content_left = win.x + 2;
  const int content_right = win.x + win.w - 4;
  const int right_label_x = ix + w + 4;
  const int right_room = content_right - right_label_x;
  const bool label_above = (text && font && text_w > 0 && text_w > right_room);
  const int top_label_pad = label_above ? (line_h + 1) : 0;
  const int iy = logical_y - win.text_scroll + top_label_pad;
  const int track_h = std::max(4, thumb_h / 3);
  const int thumb_w = 8;
  const int track_y = iy + (thumb_h - track_h) / 2;

  // Track (inset 3D)
  mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, ix, track_y, w, track_h);
  mdgui_draw_hline_idx(nullptr, CLR_BUTTON_DARK, ix, track_y, ix + w);
  mdgui_draw_vline_idx(nullptr, CLR_BUTTON_DARK, ix, track_y, track_y + track_h);
  mdgui_draw_hline_idx(nullptr, CLR_BUTTON_LIGHT, ix, track_y + track_h - 1,
                       ix + w);
  mdgui_draw_vline_idx(nullptr, CLR_BUTTON_LIGHT, ix + w - 1, track_y,
                       track_y + track_h);

  float ratio = (*val - min) / (max - min);
  if (ratio < 0)
    ratio = 0;
  if (ratio > 1)
    ratio = 1;

  const int thumb_travel = std::max(1, w - thumb_w);
  int tx = ix + (int)(ratio * (float)thumb_travel + 0.5f);

  // Thumb (outset 3D)
  mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, tx, iy, thumb_w, thumb_h);
  mdgui_draw_hline_idx(nullptr, CLR_BUTTON_LIGHT, tx, iy, tx + thumb_w);
  mdgui_draw_vline_idx(nullptr, CLR_BUTTON_LIGHT, tx, iy, iy + thumb_h);
  mdgui_draw_hline_idx(nullptr, CLR_BUTTON_DARK, tx, iy + thumb_h - 1,
                       tx + thumb_w);
  mdgui_draw_vline_idx(nullptr, CLR_BUTTON_DARK, tx + thumb_w - 1, iy,
                       iy + thumb_h);

  int result = 0;
  if (ctx->render.input.mouse_down && is_current_window_topmost(ctx) &&
      point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, ix, iy, w,
                    thumb_h)) {
    float new_ratio =
        (float)(ctx->render.input.mouse_x - ix) / (float)thumb_travel;
    if (new_ratio < 0)
      new_ratio = 0;
    if (new_ratio > 1)
      new_ratio = 1;
    *val = min + new_ratio * (max - min);
    result = 1;
  }

  int label_right = ix + w;
  if (text && font && text_w > 0) {
    int label_x = right_label_x;
    int label_y = iy + 1;

    // Prefer right-side labels, but
    // if they don't fit then place
    // above slider.
    if (text_w > right_room) {
      label_x = ix;
      label_y = iy - line_h - 1;
    }
    if (label_x < content_left)
      label_x = content_left;
    if (label_x + text_w > content_right)
      label_x = std::max(content_left, content_right - text_w);

    font_draw_text(ctx, text, label_x, label_y, CLR_TEXT_LIGHT);
    label_right = label_x + text_w;
  }
  note_content_bounds(ctx, std::max(ix + w, label_right),
                      logical_y + top_label_pad + thumb_h);
  layout_commit_widget(ctx, local_x, logical_y, w, top_label_pad + thumb_h,
                       ctx->layout.style.spacing_y);
  return result;
}

int mdgui_collapsing_header(MDGUI_Context *ctx, const char *id,
                            const char *text, int w, int default_open) {
  if (!ctx || ctx->window.current_window < 0 || !text || !resolve_font(ctx))
    return 0;
  ctx->layout.window_has_nonlabel_widget = true;
  auto &win = ctx->window.windows[ctx->window.current_window];
  MDGUI_Font *font = resolve_font(ctx);

  int local_x = 0;
  int logical_y = 0;
  const bool had_prior_item = ctx->layout.has_last_item;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  const int section_top_gap =
      had_prior_item ? std::max(0, ctx->layout.style.section_spacing_y) : 0;
  const int header_logical_y = logical_y + section_top_gap;
  w = layout_resolve_width(ctx, local_x, w, 24);
  const int text_w = font ? font_measure_text(ctx, text) : 0;
  const int needed_w = 12 + text_w + 4;
  if (w < needed_w)
    w = needed_w;
  const int ix = ctx->window.origin_x + local_x;
  const int iy = header_logical_y - win.text_scroll;
  const int line_h = font_line_height(ctx);
  const int row_h = std::max(12, line_h + 4);
  const int topmost = is_current_window_topmost(ctx);
  const int hovered =
      topmost &&
      point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, ix, iy, w, row_h);

  bool *open =
      find_or_create_collapsing_state(win, id ? id : text, default_open != 0);
  if (hovered && ctx->render.input.mouse_pressed) {
    *open = !*open;
  }

  const int bg = hovered ? CLR_BUTTON_LIGHT : CLR_BUTTON_SURFACE;
  mdgui_fill_rect_idx(nullptr, bg, ix, iy, w, row_h);
  mdgui_draw_hline_idx(nullptr, CLR_BUTTON_LIGHT, ix, iy, ix + w);
  mdgui_draw_vline_idx(nullptr, CLR_BUTTON_LIGHT, ix + w - 1, iy, iy + row_h);
  mdgui_draw_hline_idx(nullptr, CLR_BUTTON_DARK, ix, iy + row_h - 1, ix + w);
  mdgui_draw_vline_idx(nullptr, CLR_BUTTON_DARK, ix, iy, iy + row_h);

  const int text_y = iy + (row_h - line_h) / 2;
  font_draw_text(ctx, *open ? "v" : ">", ix + 3, text_y, CLR_TEXT_LIGHT);
  font_draw_text(ctx, text, ix + 12, text_y, CLR_TEXT_LIGHT);

  const int text_right = ix + 12 + text_w;
  int right_needed = std::max(ix + w, text_right);
  note_content_bounds(ctx, right_needed, header_logical_y + row_h);
  const int after_gap = *open ? 4 : std::max(4, ctx->layout.style.spacing_y);
  layout_commit_widget(ctx, local_x, logical_y, w, section_top_gap + row_h,
                       after_gap);
  return *open ? 1 : 0;
}

int mdgui_begin_collapsing_header_group(MDGUI_Context *ctx, const char *id,
                                        const char *text, int w,
                                        int default_open, int child_indent) {
  const int open = mdgui_collapsing_header(ctx, id, text, w, default_open);
  if (open) {
    if (child_indent <= 0)
      child_indent = ctx->layout.style.indent_step;
    mdgui_indent(ctx, child_indent);
  }
  return open;
}

void mdgui_end_collapsing_header_group(MDGUI_Context *ctx) {
  mdgui_unindent(ctx);
}

void mdgui_separator(MDGUI_Context *ctx, int w) {
  if (!ctx || ctx->window.current_window < 0)
    return;
  ctx->layout.window_has_nonlabel_widget = true;
  const auto &win = ctx->window.windows[ctx->window.current_window];
  const int sep_h = 2;
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  w = layout_resolve_width(ctx, local_x, w, 4);
  const int ix = ctx->window.origin_x + local_x;
  const int iy = logical_y - win.text_scroll;
  mdgui_draw_hline_idx(nullptr, CLR_ACCENT, ix, iy, ix + w);
  mdgui_draw_hline_idx(nullptr, CLR_ACCENT, ix, iy + 1, ix + w);
  note_content_bounds(ctx, ix + w, logical_y + sep_h);
  layout_commit_widget(ctx, local_x, logical_y, w, sep_h, ctx->layout.style.spacing_y);
}

int mdgui_listbox(MDGUI_Context *ctx, const char **items, int item_count,
                  int *selected, int w, int rows) {
  if (!ctx || !selected || !items || item_count <= 0)
    return 0;
  ctx->layout.window_has_nonlabel_widget = true;
  const auto &win = ctx->window.windows[ctx->window.current_window];
  if (rows < 1)
    rows = 1;
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  w = layout_resolve_width(ctx, local_x, w, 24);
  const int ix = ctx->window.origin_x + local_x;
  const int iy = logical_y - win.text_scroll;
  const int topmost = is_current_window_topmost(ctx);
  MDGUI_Font *font = resolve_font(ctx);
  const int line_h = font_line_height(ctx);
  const int row_h = std::max(10, line_h + 2);
  const int box_h = rows * row_h;

  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy - 1, ix + w + 1);
  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy + box_h + 1,
                       ix + w + 1);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy - 1,
                       iy + box_h + 1);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, ix + w + 1, iy - 1,
                       iy + box_h + 1);
  mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, ix, iy, w, box_h);

  int clicked = 0;
  for (int i = 0; i < rows; i++) {
    const int item_idx = i;
    if (item_idx >= item_count)
      break;
    const int ry = iy + (i * row_h);
    const int hovered =
        topmost &&
        point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, ix, ry, w, row_h);
    if (item_idx == *selected)
      mdgui_fill_rect_idx(nullptr, CLR_ACCENT, ix, ry, w, row_h);
    else if (hovered)
      mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, ix, ry, w, row_h);
    if (font && items[item_idx]) {
      font_draw_text(ctx, items[item_idx], ix + 2, ry + (row_h - line_h) / 2,
                     CLR_MENU_TEXT);
    }
    if (hovered && ctx->render.input.mouse_pressed) {
      *selected = item_idx;
      clicked = 1;
    }
  }

  note_content_bounds(ctx, ix + w, logical_y + box_h);
  layout_commit_widget(ctx, local_x, logical_y, w, box_h, ctx->layout.style.spacing_y);
  return clicked;
}

int mdgui_combo(MDGUI_Context *ctx, const char *label, const char **items,
                int item_count, int *selected, int w) {
  if (!ctx || !selected || !items || item_count <= 0 || ctx->window.current_window < 0)
    return 0;
  ctx->layout.window_has_nonlabel_widget = true;
  auto &win = ctx->window.windows[ctx->window.current_window];
  const int topmost = is_current_window_topmost(ctx);
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  w = layout_resolve_width(ctx, local_x, w, 40);
  MDGUI_Font *font = resolve_font(ctx);
  const int line_h = font_line_height(ctx);
  const int item_h = std::max(10, line_h + 2);
  const int box_h = std::max(12, line_h + 4);
  const int label_h = (label && font) ? ctx->layout.style.label_h : 0;
  const int box_logical_y = logical_y + label_h;
  if (*selected < 0)
    *selected = 0;
  if (*selected >= item_count)
    *selected = item_count - 1;

  const int ix = ctx->window.origin_x + local_x;
  const int iy = box_logical_y - win.text_scroll;
  const int combo_id = ((ix & 0xffff) << 16) ^ (iy & 0xffff) ^ (w << 2);
  const int open = (win.open_combo_id == combo_id);

  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy - 1, ix + w + 1);
  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy + box_h + 1,
                       ix + w + 1);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy - 1,
                       iy + box_h + 1);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, ix + w + 1, iy - 1,
                       iy + box_h + 1);
  mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, ix, iy, w, box_h);
  mdgui_draw_vline_idx(nullptr, CLR_BUTTON_DARK, ix + w - 12, iy, iy + box_h);
  if (font && items[*selected]) {
    const int text_y = iy + (box_h - line_h) / 2;
    const int text_clip_x = ix + 1;
    const int text_clip_y = iy + 1;
    const int text_clip_w = std::max(1, w - 14);
    const int text_clip_h = std::max(1, box_h - 2);
    if (set_widget_clip_intersect_content(ctx, text_clip_x, text_clip_y,
                                          text_clip_w, text_clip_h)) {
      font_draw_text(ctx, items[*selected], ix + 2, text_y, CLR_MENU_TEXT);
      set_content_clip(ctx);
    }
    font_draw_text(ctx, "v", ix + w - 9, text_y, CLR_MENU_TEXT);
  }
  if (label && font) {
    font_draw_text(ctx, label, ix, logical_y - win.text_scroll, CLR_TEXT_LIGHT);
  }

  const int hovered =
      topmost &&
      point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, ix, iy, w, box_h);
  if (hovered && ctx->render.input.mouse_pressed) {
    win.open_combo_id = open ? -1 : combo_id;
    win.z = ++ctx->window.z_counter;
    ctx->render.input.mouse_pressed = 0; // consume click so
                                  // underlying widgets don't
                                  // also fire
  } else if (ctx->render.input.mouse_pressed && open) {
    const int popup_h = item_count * item_h;
    const int in_popup = point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y,
                                       ix, iy + box_h, w, popup_h);
    if (!in_popup)
      win.open_combo_id = -1;
  }

  int changed = 0;

  if (win.open_combo_id == combo_id) {
    const int popup_y = iy + box_h;
    const int popup_h = item_count * item_h;
    for (int i = 0; i < item_count; i++) {
      const int ry = popup_y + (i * item_h);
      const int row_hover =
          topmost && point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, ix,
                                   ry, w, item_h);
      if (row_hover && ctx->render.input.mouse_pressed) {
        if (*selected != i) {
          *selected = i;
          changed = 1;
        }
        win.open_combo_id = -1;
        win.z = ++ctx->window.z_counter;
        ctx->render.input.mouse_pressed = 0; // consume popup
                                      // selection click
      }
    }
    note_content_bounds(ctx, ix + w, box_logical_y + box_h + popup_h);
    ctx->interaction.combo_overlay_pending = true;
    ctx->interaction.combo_overlay_window = ctx->window.current_window;
    ctx->interaction.combo_overlay_x = ix;
    ctx->interaction.combo_overlay_y = popup_y;
    ctx->interaction.combo_overlay_w = w;
    ctx->interaction.combo_overlay_item_h = item_h;
    ctx->interaction.combo_overlay_item_count = item_count;
    ctx->interaction.combo_overlay_selected = *selected;
    ctx->interaction.combo_overlay_items = items;
  }

  note_content_bounds(ctx, ix + w, box_logical_y + box_h);
  layout_commit_widget(ctx, local_x, logical_y, w, label_h + box_h,
                       ctx->layout.style.spacing_y);
  return changed;
}

int mdgui_tab_bar(MDGUI_Context *ctx, const char *id, const char **tabs,
                  int tab_count, int *selected, int w) {
  if (!ctx || !tabs || tab_count <= 0 || !selected || ctx->window.current_window < 0)
    return 0;
  ctx->layout.window_has_nonlabel_widget = true;
  auto &win = ctx->window.windows[ctx->window.current_window];
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  w = layout_resolve_width(ctx, local_x, w, 40);
  const int bar_h = 12;
  const int ix = ctx->window.origin_x + local_x;
  const int iy = logical_y - win.text_scroll;

  std::vector<int> tab_w;
  std::vector<int> tab_x;
  tab_w.reserve((size_t)tab_count);
  tab_x.reserve((size_t)tab_count);
  int total_w = 0;
  for (int i = 0; i < tab_count; ++i) {
    const char *label = tabs[i] ? tabs[i] : "";
    int tw = 18;
    if (mdgui_fonts[1]) {
      const int measured = mdgui_fonts[1]->measureTextWidth(label);
      if (measured + 10 > tw)
        tw = measured + 10;
    }
    tab_x.push_back(total_w);
    tab_w.push_back(tw);
    total_w += tw;
  }

  int index = *selected;
  if (index < 0)
    index = 0;
  if (index >= tab_count)
    index = tab_count - 1;
  *selected = index;

  const int topmost = is_current_window_topmost(ctx);
  const int hovered =
      topmost &&
      point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, ix, iy, w, bar_h);

  const bool overflow = (total_w > w);
  const int nav_btn_w = overflow ? 10 : 0;
  const int track_x = ix + nav_btn_w;
  const int track_w = std::max(1, w - (nav_btn_w * 2));
  auto *tab_state = find_or_create_tab_bar_state(win, id);
  int scroll_x = tab_state ? tab_state->scroll_x : 0;
  int max_scroll = total_w - track_w;
  if (max_scroll < 0)
    max_scroll = 0;
  if (scroll_x < 0)
    scroll_x = 0;
  if (scroll_x > max_scroll)
    scroll_x = max_scroll;

  const bool selected_changed =
      tab_state ? (tab_state->last_selected != index) : false;
  if (selected_changed) {
    const int selected_x0 = tab_x[index];
    const int selected_x1 = selected_x0 + tab_w[index];
    if (selected_x0 < scroll_x) {
      scroll_x = selected_x0;
    } else if (selected_x1 > scroll_x + track_w) {
      scroll_x = selected_x1 - track_w;
    }
    if (scroll_x < 0)
      scroll_x = 0;
    if (scroll_x > max_scroll)
      scroll_x = max_scroll;
  }

  int changed = 0;
  if (overflow && hovered && ctx->render.input.mouse_wheel != 0) {
    scroll_x -= ctx->render.input.mouse_wheel * 20;
    if (scroll_x < 0)
      scroll_x = 0;
    if (scroll_x > max_scroll)
      scroll_x = max_scroll;
  }

  if (ctx->render.input.mouse_pressed && topmost && hovered) {
    if (overflow &&
        point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, ix, iy, nav_btn_w,
                      bar_h)) {
      scroll_x -= 40;
      if (scroll_x < 0)
        scroll_x = 0;
    } else if (overflow && point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y,
                                         ix + w - nav_btn_w, iy, nav_btn_w,
                                         bar_h)) {
      scroll_x += 40;
      if (scroll_x > max_scroll)
        scroll_x = max_scroll;
    } else if (point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, track_x, iy,
                             track_w, bar_h)) {
      const int local_click = (ctx->render.input.mouse_x - track_x) + scroll_x;
      for (int i = 0; i < tab_count; ++i) {
        if (local_click >= tab_x[i] && local_click < tab_x[i] + tab_w[i]) {
          if (*selected != i) {
            *selected = i;
            changed = 1;
          }
          break;
        }
      }
    }
  }

  if (tab_state) {
    tab_state->scroll_x = scroll_x;
    tab_state->last_selected = *selected;
  }

  mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, ix, iy, w, bar_h);
  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy - 1, ix + w + 1);
  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy + bar_h, ix + w + 1);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy - 1, iy + bar_h);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, ix + w, iy - 1, iy + bar_h);

  if (overflow) {
    const int left_hover = topmost && point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y,
                                                    ix, iy, nav_btn_w, bar_h);
    const int right_hover = topmost && point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y,
                                                     ix + w - nav_btn_w, iy, nav_btn_w, bar_h);
    mdgui_fill_rect_idx(nullptr, left_hover ? CLR_ACCENT : CLR_BUTTON_PRESSED, ix, iy,
                        nav_btn_w, bar_h);
    mdgui_fill_rect_idx(nullptr, right_hover ? CLR_ACCENT : CLR_BUTTON_PRESSED,
                        ix + w - nav_btn_w, iy, nav_btn_w, bar_h);
    mdgui_draw_line_idx(nullptr, CLR_TEXT_LIGHT, ix + 6, iy + 3, ix + 3, iy + 5);
    mdgui_draw_line_idx(nullptr, CLR_TEXT_LIGHT, ix + 6, iy + 7, ix + 3, iy + 5);
    const int rx = ix + w - nav_btn_w;
    mdgui_draw_line_idx(nullptr, CLR_TEXT_LIGHT, rx + 3, iy + 3, rx + 6, iy + 5);
    mdgui_draw_line_idx(nullptr, CLR_TEXT_LIGHT, rx + 3, iy + 7, rx + 6, iy + 5);
  }

  if (set_widget_clip_intersect_content(ctx, track_x, iy, track_w, bar_h)) {
    for (int i = 0; i < tab_count; ++i) {
      const int tx = track_x + tab_x[i] - scroll_x;
      const int tw = tab_w[i];
      if (tx + tw <= track_x || tx >= track_x + track_w)
        continue;
      const bool active = (*selected == i);
      mdgui_fill_rect_idx(nullptr, active ? CLR_ACCENT : CLR_BUTTON_SURFACE, tx,
                          iy + 1, tw, bar_h - 2);
      mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, tx + tw - 1, iy + 1,
                           iy + bar_h - 2);
      if (mdgui_fonts[1]) {
        const char *label = tabs[i] ? tabs[i] : "";
        mdgui_fonts[1]->drawText(label, nullptr, tx + 4, iy + 2, CLR_MENU_TEXT);
      }
    }
  }
  set_content_clip(ctx);

  note_content_bounds(ctx, ix + w, logical_y + bar_h);
  layout_commit_widget(ctx, local_x, logical_y, w, bar_h, ctx->layout.style.spacing_y);
  return changed;
}

int mdgui_begin_tab_pane(MDGUI_Context *ctx, const char *id,
                         const char **tabs, int tab_count, int *selected,
                         int w) {
  if (!ctx || !tabs || tab_count <= 0 || !selected || ctx->window.current_window < 0)
    return 0;
  mdgui_tab_bar(ctx, id, tabs, tab_count, selected, w);
  mdgui_spacing(ctx, 2);
  if (*selected < 0)
    *selected = 0;
  if (*selected >= tab_count)
    *selected = tab_count - 1;
  return 1;
}

void mdgui_end_tab_pane(MDGUI_Context *ctx) {
  if (!ctx || ctx->window.current_window < 0)
    return;
  mdgui_spacing(ctx, 2);
}

}


