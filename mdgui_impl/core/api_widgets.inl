void mdgui_same_line(MDGUI_Context *ctx) {
  if (!ctx || ctx->current_window < 0)
    return;
  if (ctx->layout_has_last_item)
    ctx->layout_same_line = true;
}

void mdgui_spacing(MDGUI_Context *ctx, int pixels);

void mdgui_new_line(MDGUI_Context *ctx) {
  if (!ctx || ctx->current_window < 0)
    return;
  if (!ctx->layout_has_last_item) {
    mdgui_spacing(ctx, ctx->style.spacing_y);
    return;
  }
  const int new_y = ctx->layout_last_item_y + ctx->layout_last_item_h +
                    ctx->style.spacing_y;
  if (ctx->layout_columns_active &&
      ctx->layout_columns_index < (int)ctx->layout_columns_bottoms.size()) {
    int &col_bottom = ctx->layout_columns_bottoms[ctx->layout_columns_index];
    if (new_y > col_bottom)
      col_bottom = new_y;
    if (col_bottom > ctx->layout_columns_max_bottom)
      ctx->layout_columns_max_bottom = col_bottom;
    ctx->content_y = col_bottom;
  } else {
    ctx->content_y = new_y;
  }
  ctx->layout_same_line = false;
}

void mdgui_spacing(MDGUI_Context *ctx, int pixels) {
  if (!ctx || ctx->current_window < 0)
    return;
  if (pixels < 0)
    pixels = 0;
  if (ctx->layout_columns_active &&
      ctx->layout_columns_index < (int)ctx->layout_columns_bottoms.size()) {
    int &col_bottom = ctx->layout_columns_bottoms[ctx->layout_columns_index];
    col_bottom += pixels;
    if (col_bottom > ctx->layout_columns_max_bottom)
      ctx->layout_columns_max_bottom = col_bottom;
    ctx->content_y = col_bottom;
  } else {
    ctx->content_y += pixels;
  }
  ctx->layout_same_line = false;
}

void mdgui_indent(MDGUI_Context *ctx, int pixels) {
  if (!ctx || ctx->current_window < 0)
    return;
  if (pixels <= 0)
    pixels = ctx->style.indent_step;
  ctx->indent_stack.push_back(pixels);
  ctx->layout_indent += pixels;
}

void mdgui_unindent(MDGUI_Context *ctx) {
  if (!ctx || ctx->current_window < 0 || ctx->indent_stack.empty())
    return;
  const int pixels = ctx->indent_stack.back();
  ctx->indent_stack.pop_back();
  ctx->layout_indent -= pixels;
  if (ctx->layout_indent < 0)
    ctx->layout_indent = 0;
}

int mdgui_begin_columns(MDGUI_Context *ctx, int columns) {
  if (!ctx || ctx->current_window < 0 || columns < 1)
    return 0;
  if (ctx->layout_columns_active)
    return 0;
  int avail_w = resolve_dynamic_width(ctx, ctx->layout_indent, 0, 24);
  const int gaps = (columns - 1) * ctx->style.spacing_x;
  int col_w = (avail_w - gaps) / columns;
  if (col_w < 12)
    col_w = 12;
  ctx->layout_columns_active = true;
  ctx->layout_columns_count = columns;
  ctx->layout_columns_index = 0;
  ctx->layout_columns_start_x = ctx->layout_indent;
  ctx->layout_columns_start_y = ctx->content_y;
  ctx->layout_columns_width = col_w;
  ctx->layout_columns_max_bottom = ctx->content_y;
  ctx->layout_columns_bottoms.assign((size_t)columns, ctx->content_y);
  ctx->layout_same_line = false;
  ctx->layout_has_last_item = false;
  return 1;
}

void mdgui_next_column(MDGUI_Context *ctx) {
  if (!ctx || !ctx->layout_columns_active)
    return;
  if (ctx->layout_columns_index + 1 < ctx->layout_columns_count) {
    ctx->layout_columns_index += 1;
    if (ctx->layout_columns_index < (int)ctx->layout_columns_bottoms.size())
      ctx->content_y = ctx->layout_columns_bottoms[ctx->layout_columns_index];
  }
  ctx->layout_same_line = false;
  ctx->layout_has_last_item = false;
}

void mdgui_end_columns(MDGUI_Context *ctx) {
  if (!ctx || !ctx->layout_columns_active)
    return;
  ctx->content_y = ctx->layout_columns_max_bottom;
  ctx->layout_columns_active = false;
  ctx->layout_columns_count = 0;
  ctx->layout_columns_index = 0;
  ctx->layout_columns_width = 0;
  ctx->layout_columns_bottoms.clear();
  ctx->layout_same_line = false;
  ctx->layout_has_last_item = false;
  mdgui_spacing(ctx, ctx->style.spacing_y);
}

int mdgui_begin_row(MDGUI_Context *ctx, int columns) {
  return mdgui_begin_columns(ctx, columns);
}

void mdgui_end_row(MDGUI_Context *ctx) { mdgui_end_columns(ctx); }

int mdgui_button(MDGUI_Context *ctx, const char *text, int w, int h) {
  if (!ctx || ctx->current_window < 0)
    return 0;
  ctx->window_has_nonlabel_widget = true;
  const auto &win = ctx->windows[ctx->current_window];
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

  const int abs_x = ctx->origin_x + local_x;
  const int abs_y = logical_y - win.text_scroll;
  const int topmost = is_current_window_topmost(ctx);
  const int hovered =
      topmost &&
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, abs_x, abs_y, w, h);
  const int depressed = hovered && ctx->input.mouse_down;

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
  layout_commit_widget(ctx, local_x, logical_y, w, h, ctx->style.spacing_y);

  return hovered && ctx->input.mouse_pressed && win.z == ctx->z_counter;
}

void mdgui_label_font(MDGUI_Context *ctx, const char *text, MDGUI_Font *font) {
  if (!ctx || !text || !resolve_font(ctx, font) || ctx->current_window < 0)
    return;
  const auto &win = ctx->windows[ctx->current_window];
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  const int top_inset = std::max(1, ctx->style.spacing_y / 2);
  const int line_h = std::max(ctx->style.label_h, font_line_height(ctx, font));
  const int label_logical_y = logical_y + top_inset;
  const int abs_x = ctx->origin_x + local_x;
  const int abs_y = label_logical_y - win.text_scroll;
  font_draw_text(ctx, text, abs_x, abs_y, CLR_TEXT_LIGHT, font);
  note_content_bounds(ctx, abs_x + font_measure_text(ctx, text, font),
                      label_logical_y + line_h);
  layout_commit_widget(ctx, local_x, logical_y, 1, line_h + top_inset,
                       ctx->style.spacing_y);
}

void mdgui_label(MDGUI_Context *ctx, const char *text) {
  mdgui_label_font(ctx, text, nullptr);
}

void mdgui_label_wrapped_font(MDGUI_Context *ctx, const char *text, int w,
                              MDGUI_Font *font) {
  if (!ctx || !text || !resolve_font(ctx, font) || ctx->current_window < 0)
    return;
  const auto &win = ctx->windows[ctx->current_window];
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  const int top_inset = std::max(1, ctx->style.spacing_y / 2);
  const int wrapped_logical_y = logical_y + top_inset;
  const int abs_x = ctx->origin_x + local_x;
  const int wrap_w = layout_resolve_width(ctx, local_x, w, 20);
  const int margin_l = 2;
  const int margin_r = 2;
  const int draw_x = abs_x + margin_l;
  const int inner_wrap_w = std::max(10, wrap_w - (margin_l + margin_r));
  const int line_h = std::max(ctx->style.label_h, font_line_height(ctx, font));

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
                       ctx->style.spacing_y);
}

void mdgui_label_wrapped(MDGUI_Context *ctx, const char *text, int w) {
  mdgui_label_wrapped_font(ctx, text, w, nullptr);
}

int mdgui_checkbox(MDGUI_Context *ctx, const char *text, bool *checked) {
  if (!ctx || !checked || ctx->current_window < 0)
    return 0;
  ctx->window_has_nonlabel_widget = true;
  const auto &win = ctx->windows[ctx->current_window];
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  const int ix = ctx->origin_x + local_x;
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
  if (ctx->input.mouse_pressed && is_current_window_topmost(ctx) &&
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, ix, iy,
                    box_size + label_w + 4, row_h)) {
    *checked = !*checked;
    result = 1;
  }

  layout_commit_widget(ctx, local_x, logical_y, box_size + label_w + 4, row_h,
                       ctx->style.spacing_y);
  return result;
}

int mdgui_slider(MDGUI_Context *ctx, const char *text, float *val, float min,
                 float max, int w) {
  if (!ctx || !val || ctx->current_window < 0)
    return 0;
  ctx->window_has_nonlabel_widget = true;
  const auto &win = ctx->windows[ctx->current_window];
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  w = layout_resolve_width(ctx, local_x, w, 24);
  const int ix = ctx->origin_x + local_x;
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
  if (ctx->input.mouse_down && is_current_window_topmost(ctx) &&
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, ix, iy, w,
                    thumb_h)) {
    float new_ratio =
        (float)(ctx->input.mouse_x - ix) / (float)thumb_travel;
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
                       ctx->style.spacing_y);
  return result;
}

int mdgui_collapsing_header(MDGUI_Context *ctx, const char *id,
                            const char *text, int w, int default_open) {
  if (!ctx || ctx->current_window < 0 || !text || !resolve_font(ctx))
    return 0;
  ctx->window_has_nonlabel_widget = true;
  auto &win = ctx->windows[ctx->current_window];
  MDGUI_Font *font = resolve_font(ctx);

  int local_x = 0;
  int logical_y = 0;
  const bool had_prior_item = ctx->layout_has_last_item;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  const int section_top_gap =
      had_prior_item ? std::max(0, ctx->style.section_spacing_y) : 0;
  const int header_logical_y = logical_y + section_top_gap;
  w = layout_resolve_width(ctx, local_x, w, 24);
  const int text_w = font ? font_measure_text(ctx, text) : 0;
  const int needed_w = 12 + text_w + 4;
  if (w < needed_w)
    w = needed_w;
  const int ix = ctx->origin_x + local_x;
  const int iy = header_logical_y - win.text_scroll;
  const int line_h = font_line_height(ctx);
  const int row_h = std::max(12, line_h + 4);
  const int topmost = is_current_window_topmost(ctx);
  const int hovered =
      topmost &&
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, ix, iy, w, row_h);

  bool *open =
      find_or_create_collapsing_state(win, id ? id : text, default_open != 0);
  if (hovered && ctx->input.mouse_pressed) {
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
  const int after_gap = *open ? 4 : std::max(4, ctx->style.spacing_y);
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
      child_indent = ctx->style.indent_step;
    mdgui_indent(ctx, child_indent);
  }
  return open;
}

void mdgui_end_collapsing_header_group(MDGUI_Context *ctx) {
  mdgui_unindent(ctx);
}

void mdgui_separator(MDGUI_Context *ctx, int w) {
  if (!ctx || ctx->current_window < 0)
    return;
  ctx->window_has_nonlabel_widget = true;
  const auto &win = ctx->windows[ctx->current_window];
  const int sep_h = 2;
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  w = layout_resolve_width(ctx, local_x, w, 4);
  const int ix = ctx->origin_x + local_x;
  const int iy = logical_y - win.text_scroll;
  mdgui_draw_hline_idx(nullptr, CLR_ACCENT, ix, iy, ix + w);
  mdgui_draw_hline_idx(nullptr, CLR_ACCENT, ix, iy + 1, ix + w);
  note_content_bounds(ctx, ix + w, logical_y + sep_h);
  layout_commit_widget(ctx, local_x, logical_y, w, sep_h, ctx->style.spacing_y);
}

int mdgui_listbox(MDGUI_Context *ctx, const char **items, int item_count,
                  int *selected, int w, int rows) {
  if (!ctx || !selected || !items || item_count <= 0)
    return 0;
  ctx->window_has_nonlabel_widget = true;
  const auto &win = ctx->windows[ctx->current_window];
  if (rows < 1)
    rows = 1;
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  w = layout_resolve_width(ctx, local_x, w, 24);
  const int ix = ctx->origin_x + local_x;
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
        point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, ix, ry, w, row_h);
    if (item_idx == *selected)
      mdgui_fill_rect_idx(nullptr, CLR_ACCENT, ix, ry, w, row_h);
    else if (hovered)
      mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, ix, ry, w, row_h);
    if (font && items[item_idx]) {
      font_draw_text(ctx, items[item_idx], ix + 2, ry + (row_h - line_h) / 2,
                     CLR_MENU_TEXT);
    }
    if (hovered && ctx->input.mouse_pressed) {
      *selected = item_idx;
      clicked = 1;
    }
  }

  note_content_bounds(ctx, ix + w, logical_y + box_h);
  layout_commit_widget(ctx, local_x, logical_y, w, box_h, ctx->style.spacing_y);
  return clicked;
}

int mdgui_combo(MDGUI_Context *ctx, const char *label, const char **items,
                int item_count, int *selected, int w) {
  if (!ctx || !selected || !items || item_count <= 0 || ctx->current_window < 0)
    return 0;
  ctx->window_has_nonlabel_widget = true;
  auto &win = ctx->windows[ctx->current_window];
  const int topmost = is_current_window_topmost(ctx);
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  w = layout_resolve_width(ctx, local_x, w, 40);
  MDGUI_Font *font = resolve_font(ctx);
  const int line_h = font_line_height(ctx);
  const int item_h = std::max(10, line_h + 2);
  const int box_h = std::max(12, line_h + 4);
  const int label_h = (label && font) ? ctx->style.label_h : 0;
  const int box_logical_y = logical_y + label_h;
  if (*selected < 0)
    *selected = 0;
  if (*selected >= item_count)
    *selected = item_count - 1;

  const int ix = ctx->origin_x + local_x;
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
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, ix, iy, w, box_h);
  if (hovered && ctx->input.mouse_pressed) {
    win.open_combo_id = open ? -1 : combo_id;
    win.z = ++ctx->z_counter;
    ctx->input.mouse_pressed = 0; // consume click so
                                  // underlying widgets don't
                                  // also fire
  } else if (ctx->input.mouse_pressed && open) {
    const int popup_h = item_count * item_h;
    const int in_popup = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
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
          topmost && point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, ix,
                                   ry, w, item_h);
      if (row_hover && ctx->input.mouse_pressed) {
        if (*selected != i) {
          *selected = i;
          changed = 1;
        }
        win.open_combo_id = -1;
        win.z = ++ctx->z_counter;
        ctx->input.mouse_pressed = 0; // consume popup
                                      // selection click
      }
    }
    note_content_bounds(ctx, ix + w, box_logical_y + box_h + popup_h);
    ctx->combo_overlay_pending = true;
    ctx->combo_overlay_window = ctx->current_window;
    ctx->combo_overlay_x = ix;
    ctx->combo_overlay_y = popup_y;
    ctx->combo_overlay_w = w;
    ctx->combo_overlay_item_h = item_h;
    ctx->combo_overlay_item_count = item_count;
    ctx->combo_overlay_selected = *selected;
    ctx->combo_overlay_items = items;
  }

  note_content_bounds(ctx, ix + w, box_logical_y + box_h);
  layout_commit_widget(ctx, local_x, logical_y, w, label_h + box_h,
                       ctx->style.spacing_y);
  return changed;
}

int mdgui_tab_bar(MDGUI_Context *ctx, const char *id, const char **tabs,
                  int tab_count, int *selected, int w) {
  if (!ctx || !tabs || tab_count <= 0 || !selected || ctx->current_window < 0)
    return 0;
  ctx->window_has_nonlabel_widget = true;
  auto &win = ctx->windows[ctx->current_window];
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  w = layout_resolve_width(ctx, local_x, w, 40);
  const int bar_h = 12;
  const int ix = ctx->origin_x + local_x;
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
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, ix, iy, w, bar_h);

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
  if (overflow && hovered && ctx->input.mouse_wheel != 0) {
    scroll_x -= ctx->input.mouse_wheel * 20;
    if (scroll_x < 0)
      scroll_x = 0;
    if (scroll_x > max_scroll)
      scroll_x = max_scroll;
  }

  if (ctx->input.mouse_pressed && topmost && hovered) {
    if (overflow &&
        point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, ix, iy, nav_btn_w,
                      bar_h)) {
      scroll_x -= 40;
      if (scroll_x < 0)
        scroll_x = 0;
    } else if (overflow && point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                         ix + w - nav_btn_w, iy, nav_btn_w,
                                         bar_h)) {
      scroll_x += 40;
      if (scroll_x > max_scroll)
        scroll_x = max_scroll;
    } else if (point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, track_x, iy,
                             track_w, bar_h)) {
      const int local_click = (ctx->input.mouse_x - track_x) + scroll_x;
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
    const int left_hover = topmost && point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                                    ix, iy, nav_btn_w, bar_h);
    const int right_hover = topmost && point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
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
  layout_commit_widget(ctx, local_x, logical_y, w, bar_h, ctx->style.spacing_y);
  return changed;
}

int mdgui_begin_tab_pane(MDGUI_Context *ctx, const char *id,
                         const char **tabs, int tab_count, int *selected,
                         int w) {
  if (!ctx || !tabs || tab_count <= 0 || !selected || ctx->current_window < 0)
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
  if (!ctx || ctx->current_window < 0)
    return;
  mdgui_spacing(ctx, 2);
}

int mdgui_input_text(MDGUI_Context *ctx, const char *label, char *buffer,
                     int buffer_size, int w) {
  if (!ctx || !buffer || buffer_size <= 1 || ctx->current_window < 0)
    return 0;
  ctx->window_has_nonlabel_widget = true;
  auto &win = ctx->windows[ctx->current_window];
  const int topmost = is_current_window_topmost(ctx);
  const int box_h = 12;
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  w = layout_resolve_width(ctx, local_x, w, 40);
  const int ix = ctx->origin_x + local_x;
  const int label_h = (label && mdgui_fonts[1]) ? ctx->style.label_h : 0;
  const int box_logical_y = logical_y + label_h;
  const int iy = box_logical_y - win.text_scroll;
  const uintptr_t buf_addr = (uintptr_t)buffer;
  const int input_id = (int)(((buf_addr >> 3) & 0x7fffffff) ^
                             ((ix & 0xffff) << 16) ^ 0x54455854);
  const bool focused = (ctx->active_text_input_window == ctx->current_window &&
                        ctx->active_text_input_id == input_id);

  const int hovered =
      topmost &&
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, ix, iy, w, box_h);
  int len = 0;
  while (len < (buffer_size - 1) && buffer[len] != '\0')
    len++;
  buffer[len] = '\0';
  int pre_cursor =
      focused ? clamp_text_cursor(ctx->active_text_input_cursor, len) : len;
  int pre_scroll_px = 0;
  if (mdgui_fonts[1]) {
    std::string pre_prefix(buffer, (size_t)pre_cursor);
    const int cursor_px = mdgui_fonts[1]->measureTextWidth(pre_prefix.c_str());
    const int view_w = std::max(1, w - 6);
    if (cursor_px > view_w - 2)
      pre_scroll_px = cursor_px - (view_w - 2);
  }
  if (hovered && ctx->input.mouse_pressed) {
    ctx->active_text_input_window = ctx->current_window;
    ctx->active_text_input_id = input_id;
    ctx->active_text_input_multiline = false;
    ctx->active_text_input_scroll_y = 0;
    const int text_x = ix + 3;
    const int target_px = ctx->input.mouse_x - text_x + pre_scroll_px;
    const int click_cursor = text_cursor_from_pixel_x(buffer, len, target_px);
    ctx->active_text_input_cursor = click_cursor;
    ctx->active_text_input_sel_anchor = click_cursor;
    ctx->active_text_input_sel_start = click_cursor;
    ctx->active_text_input_sel_end = click_cursor;
    ctx->active_text_input_drag_select = ctx->input.mouse_down != 0;
    ctx->input.mouse_pressed = 0;
  } else if (ctx->input.mouse_pressed && focused && topmost &&
             !point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, ix, iy, w,
                            box_h)) {
    ctx->active_text_input_window = -1;
    ctx->active_text_input_id = 0;
    ctx->active_text_input_cursor = 0;
    ctx->active_text_input_sel_anchor = 0;
    ctx->active_text_input_sel_start = 0;
    ctx->active_text_input_sel_end = 0;
    ctx->active_text_input_drag_select = false;
    ctx->active_text_input_multiline = false;
    ctx->active_text_input_scroll_y = 0;
  }

  const bool active = (ctx->active_text_input_window == ctx->current_window &&
                       ctx->active_text_input_id == input_id);
  int flags = 0;
  if (active) {
    ctx->active_text_input_cursor =
        clamp_text_cursor(ctx->active_text_input_cursor, len);
    ctx->active_text_input_sel_anchor =
        clamp_text_cursor(ctx->active_text_input_sel_anchor, len);
    ctx->active_text_input_sel_start =
        clamp_text_cursor(ctx->active_text_input_sel_start, len);
    ctx->active_text_input_sel_end =
        clamp_text_cursor(ctx->active_text_input_sel_end, len);
  }

  if (active && ctx->active_text_input_drag_select && ctx->input.mouse_down &&
      hovered) {
    const int text_x = ix + 3;
    const int target_px = ctx->input.mouse_x - text_x + pre_scroll_px;
    const int drag_cursor = text_cursor_from_pixel_x(buffer, len, target_px);
    ctx->active_text_input_cursor = drag_cursor;
    ctx->active_text_input_sel_start =
        std::min(ctx->active_text_input_sel_anchor, drag_cursor);
    ctx->active_text_input_sel_end =
        std::max(ctx->active_text_input_sel_anchor, drag_cursor);
  }
  if (active && !ctx->input.mouse_down) {
    ctx->active_text_input_drag_select = false;
  }

  if (active && topmost) {
    const bool has_selection =
        ctx->active_text_input_sel_end > ctx->active_text_input_sel_start;

    if (ctx->input.key_left) {
      if (has_selection) {
        ctx->active_text_input_cursor = ctx->active_text_input_sel_start;
      } else {
        ctx->active_text_input_cursor -= 1;
      }
      ctx->active_text_input_sel_start = ctx->active_text_input_cursor;
      ctx->active_text_input_sel_end = ctx->active_text_input_cursor;
    }
    if (ctx->input.key_right) {
      if (has_selection) {
        ctx->active_text_input_cursor = ctx->active_text_input_sel_end;
      } else {
        ctx->active_text_input_cursor += 1;
      }
      ctx->active_text_input_sel_start = ctx->active_text_input_cursor;
      ctx->active_text_input_sel_end = ctx->active_text_input_cursor;
    }
    if (ctx->input.key_home) {
      ctx->active_text_input_cursor = 0;
      ctx->active_text_input_sel_start = 0;
      ctx->active_text_input_sel_end = 0;
    }
    if (ctx->input.key_end) {
      ctx->active_text_input_cursor = len;
      ctx->active_text_input_sel_start = len;
      ctx->active_text_input_sel_end = len;
    }
    ctx->active_text_input_cursor =
        clamp_text_cursor(ctx->active_text_input_cursor, len);

    const int sel_start = ctx->active_text_input_sel_start;
    const int sel_end = ctx->active_text_input_sel_end;
    const bool has_selection_now = sel_end > sel_start;

    if (ctx->input.key_backspace &&
        (has_selection_now || ctx->active_text_input_cursor > 0)) {
      if (has_selection_now) {
        memmove(buffer + sel_start, buffer + sel_end,
                (size_t)(len - sel_end + 1));
        len -= (sel_end - sel_start);
        ctx->active_text_input_cursor = sel_start;
      } else {
        const int remove_idx = ctx->active_text_input_cursor - 1;
        memmove(buffer + remove_idx, buffer + remove_idx + 1,
                (size_t)(len - remove_idx));
        len -= 1;
        ctx->active_text_input_cursor -= 1;
      }
      ctx->active_text_input_sel_start = ctx->active_text_input_cursor;
      ctx->active_text_input_sel_end = ctx->active_text_input_cursor;
      flags |= MDGUI_INPUT_TEXT_CHANGED;
    }
    if (ctx->input.key_delete &&
        (has_selection_now || ctx->active_text_input_cursor < len)) {
      if (has_selection_now) {
        memmove(buffer + sel_start, buffer + sel_end,
                (size_t)(len - sel_end + 1));
        len -= (sel_end - sel_start);
        ctx->active_text_input_cursor = sel_start;
      } else {
        const int remove_idx = ctx->active_text_input_cursor;
        memmove(buffer + remove_idx, buffer + remove_idx + 1,
                (size_t)(len - remove_idx));
        len -= 1;
      }
      ctx->active_text_input_sel_start = ctx->active_text_input_cursor;
      ctx->active_text_input_sel_end = ctx->active_text_input_cursor;
      flags |= MDGUI_INPUT_TEXT_CHANGED;
    }
    if (ctx->input.text_input) {
      if (ctx->active_text_input_sel_end > ctx->active_text_input_sel_start) {
        const int s0 = ctx->active_text_input_sel_start;
        const int s1 = ctx->active_text_input_sel_end;
        memmove(buffer + s0, buffer + s1, (size_t)(len - s1 + 1));
        len -= (s1 - s0);
        ctx->active_text_input_cursor = s0;
        ctx->active_text_input_sel_start = s0;
        ctx->active_text_input_sel_end = s0;
      }
      const unsigned char *p = (const unsigned char *)ctx->input.text_input;
      while (*p && len < (buffer_size - 1)) {
        const unsigned char ch = *p++;
        if (ch < 32 || ch > 126)
          continue;
        const int insert_at = ctx->active_text_input_cursor;
        memmove(buffer + insert_at + 1, buffer + insert_at,
                (size_t)(len - insert_at + 1));
        buffer[insert_at] = (char)ch;
        len += 1;
        ctx->active_text_input_cursor += 1;
        ctx->active_text_input_sel_start = ctx->active_text_input_cursor;
        ctx->active_text_input_sel_end = ctx->active_text_input_cursor;
        flags |= MDGUI_INPUT_TEXT_CHANGED;
      }
    }
    if (ctx->input.key_enter)
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
    const int display_cursor = active ? ctx->active_text_input_cursor : len;
    std::string prefix(buffer, (size_t)display_cursor);
    const int cursor_text_x = mdgui_fonts[1]->measureTextWidth(prefix.c_str());
    const int view_w = std::max(1, w - 6);
    if (cursor_text_x > view_w - 2)
      scroll_px = cursor_text_x - (view_w - 2);

    if (set_widget_clip_intersect_content(ctx, ix + 1, iy + 1,
                                          std::max(1, w - 2),
                                          std::max(1, box_h - 2))) {
      if (active &&
          ctx->active_text_input_sel_end > ctx->active_text_input_sel_start) {
        std::string sel_prefix(buffer,
                               (size_t)ctx->active_text_input_sel_start);
        std::string sel_text(buffer + ctx->active_text_input_sel_start,
                             (size_t)(ctx->active_text_input_sel_end -
                                      ctx->active_text_input_sel_start));
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
                       ctx->style.spacing_y);
  return flags;
}

int mdgui_input_text_multiline(MDGUI_Context *ctx, const char *label,
                               char *buffer, int buffer_size, int w, int h,
                               int flags) {
  if (!ctx || !buffer || buffer_size <= 1 || ctx->current_window < 0)
    return 0;
  ctx->window_has_nonlabel_widget = true;
  auto &win = ctx->windows[ctx->current_window];
  const int topmost = is_current_window_topmost(ctx);
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  w = layout_resolve_width(ctx, local_x, w, 40);
  if (h < 24)
    h = 24;

  const int ix = ctx->origin_x + local_x;
  const int label_h = (label && mdgui_fonts[1]) ? ctx->style.label_h : 0;
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
  const bool focused = (ctx->active_text_input_window == ctx->current_window &&
                        ctx->active_text_input_id == input_id);
  if (focused && !ctx->active_text_input_multiline) {
    ctx->active_text_input_scroll_y = 0;
    ctx->active_text_input_multiline = true;
  }

  const int hovered =
      topmost &&
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, ix, iy, w, h);

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
    int scroll_y = focused ? ctx->active_text_input_scroll_y : 0;
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

  if (hovered && ctx->input.mouse_pressed) {
    const int click_cursor =
        cursor_from_mouse(ctx->input.mouse_x, ctx->input.mouse_y);
    ctx->active_text_input_window = ctx->current_window;
    ctx->active_text_input_id = input_id;
    ctx->active_text_input_cursor = click_cursor;
    ctx->active_text_input_sel_anchor = click_cursor;
    ctx->active_text_input_sel_start = click_cursor;
    ctx->active_text_input_sel_end = click_cursor;
    ctx->active_text_input_drag_select = true;
    ctx->active_text_input_multiline = true;
    ctx->input.mouse_pressed = 0;
  } else if (ctx->input.mouse_pressed && focused && topmost &&
             !point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, ix, iy, w,
                            h)) {
    ctx->active_text_input_window = -1;
    ctx->active_text_input_id = 0;
    ctx->active_text_input_cursor = 0;
    ctx->active_text_input_sel_anchor = 0;
    ctx->active_text_input_sel_start = 0;
    ctx->active_text_input_sel_end = 0;
    ctx->active_text_input_drag_select = false;
    ctx->active_text_input_multiline = false;
    ctx->active_text_input_scroll_y = 0;
  }

  const bool active = (ctx->active_text_input_window == ctx->current_window &&
                       ctx->active_text_input_id == input_id);
  int result_flags = 0;
  if (active) {
    ctx->active_text_input_cursor =
        clamp_text_cursor(ctx->active_text_input_cursor, len);
    ctx->active_text_input_sel_anchor =
        clamp_text_cursor(ctx->active_text_input_sel_anchor, len);
    ctx->active_text_input_sel_start =
        clamp_text_cursor(ctx->active_text_input_sel_start, len);
    ctx->active_text_input_sel_end =
        clamp_text_cursor(ctx->active_text_input_sel_end, len);
  }
  if (active &&
      (ctx->input.text_input || ctx->input.key_backspace ||
       ctx->input.key_delete || ctx->input.key_left || ctx->input.key_right ||
       ctx->input.key_up || ctx->input.key_down || ctx->input.key_home ||
       ctx->input.key_end || ctx->input.key_enter)) {
    ctx->active_text_input_drag_select = false;
  }
  if (ctx->active_text_input_scroll_y < 0)
    ctx->active_text_input_scroll_y = 0;

  if (active && ctx->active_text_input_drag_select && ctx->input.mouse_down &&
      hovered) {
    const int drag_cursor =
        cursor_from_mouse(ctx->input.mouse_x, ctx->input.mouse_y);
    ctx->active_text_input_cursor = drag_cursor;
    ctx->active_text_input_sel_start =
        std::min(ctx->active_text_input_sel_anchor, drag_cursor);
    ctx->active_text_input_sel_end =
        std::max(ctx->active_text_input_sel_anchor, drag_cursor);
  }
  if (active && !ctx->input.mouse_down) {
    ctx->active_text_input_drag_select = false;
  }

  if (active && hovered && ctx->input.mouse_wheel != 0) {
    const int view_h = std::max(1, h - 4);
    const int max_scroll = std::max(0, content_h - view_h);
    ctx->active_text_input_scroll_y -= ctx->input.mouse_wheel * 12;
    if (ctx->active_text_input_scroll_y < 0)
      ctx->active_text_input_scroll_y = 0;
    if (ctx->active_text_input_scroll_y > max_scroll)
      ctx->active_text_input_scroll_y = max_scroll;
  }

  if (active && topmost) {
    const bool has_selection =
        ctx->active_text_input_sel_end > ctx->active_text_input_sel_start;
    const int cursor_line = cursor_line_for(ctx->active_text_input_cursor);
    const int cursor_line_start = line_starts[cursor_line];
    const int cursor_line_end = line_end(cursor_line);
    const int cursor_col_chars =
        std::max(0, std::min(ctx->active_text_input_cursor - cursor_line_start,
                             cursor_line_end - cursor_line_start));

    if (ctx->input.key_left) {
      if (has_selection) {
        ctx->active_text_input_cursor = ctx->active_text_input_sel_start;
      } else {
        ctx->active_text_input_cursor -= 1;
      }
      ctx->active_text_input_sel_start = ctx->active_text_input_cursor;
      ctx->active_text_input_sel_end = ctx->active_text_input_cursor;
    }
    if (ctx->input.key_right) {
      if (has_selection) {
        ctx->active_text_input_cursor = ctx->active_text_input_sel_end;
      } else {
        ctx->active_text_input_cursor += 1;
      }
      ctx->active_text_input_sel_start = ctx->active_text_input_cursor;
      ctx->active_text_input_sel_end = ctx->active_text_input_cursor;
    }
    if (ctx->input.key_home) {
      ctx->active_text_input_cursor = cursor_line_start;
      ctx->active_text_input_sel_start = ctx->active_text_input_cursor;
      ctx->active_text_input_sel_end = ctx->active_text_input_cursor;
    }
    if (ctx->input.key_end) {
      ctx->active_text_input_cursor = cursor_line_end;
      ctx->active_text_input_sel_start = ctx->active_text_input_cursor;
      ctx->active_text_input_sel_end = ctx->active_text_input_cursor;
    }
    if (ctx->input.key_up && cursor_line > 0) {
      const int ls = line_starts[cursor_line - 1];
      const int le = line_end(cursor_line - 1);
      ctx->active_text_input_cursor =
          ls + std::min(cursor_col_chars, std::max(0, le - ls));
      ctx->active_text_input_sel_start = ctx->active_text_input_cursor;
      ctx->active_text_input_sel_end = ctx->active_text_input_cursor;
    }
    if (ctx->input.key_down && cursor_line + 1 < line_count) {
      const int ls = line_starts[cursor_line + 1];
      const int le = line_end(cursor_line + 1);
      ctx->active_text_input_cursor =
          ls + std::min(cursor_col_chars, std::max(0, le - ls));
      ctx->active_text_input_sel_start = ctx->active_text_input_cursor;
      ctx->active_text_input_sel_end = ctx->active_text_input_cursor;
    }

    ctx->active_text_input_cursor =
        clamp_text_cursor(ctx->active_text_input_cursor, len);
    const int sel_start = ctx->active_text_input_sel_start;
    const int sel_end = ctx->active_text_input_sel_end;
    const bool has_selection_now = sel_end > sel_start;

    if (ctx->input.key_backspace &&
        (has_selection_now || ctx->active_text_input_cursor > 0)) {
      if (has_selection_now) {
        memmove(buffer + sel_start, buffer + sel_end,
                (size_t)(len - sel_end + 1));
        len -= (sel_end - sel_start);
        ctx->active_text_input_cursor = sel_start;
      } else {
        const int remove_idx = ctx->active_text_input_cursor - 1;
        memmove(buffer + remove_idx, buffer + remove_idx + 1,
                (size_t)(len - remove_idx));
        len -= 1;
        ctx->active_text_input_cursor -= 1;
      }
      ctx->active_text_input_sel_start = ctx->active_text_input_cursor;
      ctx->active_text_input_sel_end = ctx->active_text_input_cursor;
      result_flags |= MDGUI_INPUT_TEXT_CHANGED;
    }
    if (ctx->input.key_delete &&
        (has_selection_now || ctx->active_text_input_cursor < len)) {
      if (has_selection_now) {
        memmove(buffer + sel_start, buffer + sel_end,
                (size_t)(len - sel_end + 1));
        len -= (sel_end - sel_start);
        ctx->active_text_input_cursor = sel_start;
      } else {
        const int remove_idx = ctx->active_text_input_cursor;
        memmove(buffer + remove_idx, buffer + remove_idx + 1,
                (size_t)(len - remove_idx));
        len -= 1;
      }
      ctx->active_text_input_sel_start = ctx->active_text_input_cursor;
      ctx->active_text_input_sel_end = ctx->active_text_input_cursor;
      result_flags |= MDGUI_INPUT_TEXT_CHANGED;
    }

    if (ctx->input.key_enter && len < (buffer_size - 1)) {
      if (ctx->active_text_input_sel_end > ctx->active_text_input_sel_start) {
        const int s0 = ctx->active_text_input_sel_start;
        const int s1 = ctx->active_text_input_sel_end;
        memmove(buffer + s0, buffer + s1, (size_t)(len - s1 + 1));
        len -= (s1 - s0);
        ctx->active_text_input_cursor = s0;
      }
      const int at = ctx->active_text_input_cursor;
      memmove(buffer + at + 1, buffer + at, (size_t)(len - at + 1));
      buffer[at] = '\n';
      len += 1;
      ctx->active_text_input_cursor += 1;
      ctx->active_text_input_sel_start = ctx->active_text_input_cursor;
      ctx->active_text_input_sel_end = ctx->active_text_input_cursor;
      result_flags |= MDGUI_INPUT_TEXT_CHANGED;
    }

    if (ctx->input.text_input) {
      if (ctx->active_text_input_sel_end > ctx->active_text_input_sel_start) {
        const int s0 = ctx->active_text_input_sel_start;
        const int s1 = ctx->active_text_input_sel_end;
        memmove(buffer + s0, buffer + s1, (size_t)(len - s1 + 1));
        len -= (s1 - s0);
        ctx->active_text_input_cursor = s0;
        ctx->active_text_input_sel_start = s0;
        ctx->active_text_input_sel_end = s0;
      }
      const unsigned char *p = (const unsigned char *)ctx->input.text_input;
      while (*p && len < (buffer_size - 1)) {
        const unsigned char ch = *p++;
        if (ch < 32 || ch > 126)
          continue;
        const int insert_at = ctx->active_text_input_cursor;
        memmove(buffer + insert_at + 1, buffer + insert_at,
                (size_t)(len - insert_at + 1));
        buffer[insert_at] = (char)ch;
        len += 1;
        ctx->active_text_input_cursor += 1;
        ctx->active_text_input_sel_start = ctx->active_text_input_cursor;
        ctx->active_text_input_sel_end = ctx->active_text_input_cursor;
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
      if (line_starts[i] <= ctx->active_text_input_cursor)
        caret_line = i;
      else
        break;
    }
    const int caret_y = caret_line * line_h;
    const int view_h = std::max(1, h - 4);
    if (caret_y < ctx->active_text_input_scroll_y)
      ctx->active_text_input_scroll_y = caret_y;
    if (caret_y + line_h > ctx->active_text_input_scroll_y + view_h)
      ctx->active_text_input_scroll_y = (caret_y + line_h) - view_h;
    if (ctx->active_text_input_scroll_y < 0)
      ctx->active_text_input_scroll_y = 0;
  }

  if (mdgui_fonts[1]) {
    const int text_x = ix + 3;
    const int text_y = iy + 2;
    const int scroll_y = active ? ctx->active_text_input_scroll_y : 0;

    if (set_widget_clip_intersect_content(ctx, ix + 1, iy + 1,
                                          std::max(1, w - 2),
                                          std::max(1, h - 2))) {
      const int lc = (int)line_starts.size();
      const int sel_start =
          active ? std::min(ctx->active_text_input_sel_start,
                            ctx->active_text_input_sel_end)
                 : 0;
      const int sel_end =
          active ? std::max(ctx->active_text_input_sel_start,
                            ctx->active_text_input_sel_end)
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
            if (line_starts[i] <= ctx->active_text_input_cursor)
              caret_line = i;
            else
              break;
          }
          const int cls = line_starts[caret_line];
          std::string left(buffer + cls,
                           (size_t)(ctx->active_text_input_cursor - cls));
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
                       ctx->style.spacing_y);
  return result_flags;
}

void mdgui_progress_bar(MDGUI_Context *ctx, float value, int w, int h,
                        const char *overlay_text) {
  if (!ctx || ctx->current_window < 0)
    return;
  ctx->window_has_nonlabel_widget = true;
  const auto &win = ctx->windows[ctx->current_window];
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

  const int ix = ctx->origin_x + local_x;
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
  layout_commit_widget(ctx, local_x, logical_y, w, h, ctx->style.spacing_y + 2);
}

void mdgui_frame_time_graph(MDGUI_Context *ctx, const float *frame_ms_samples,
                            int sample_count, float target_fps,
                            float graph_max_ms, int w, int h) {
  if (!ctx || ctx->current_window < 0)
    return;
  ctx->window_has_nonlabel_widget = true;
  const auto &win = ctx->windows[ctx->current_window];

  const int requested_w = w;
  const int requested_h = h;
  const bool fill_mode = (requested_w == 0 && requested_h == 0);
  int local_x = 0;
  int logical_y = 0;
  layout_prepare_widget(ctx, &local_x, &logical_y);
  if (fill_mode) {
    int avail_w = (win.x + win.w - 2) - (ctx->origin_x + local_x);
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

  const int ix = ctx->origin_x + local_x;
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
    layout_commit_widget(ctx, local_x, logical_y, w, h, ctx->style.spacing_y);
  }
}

