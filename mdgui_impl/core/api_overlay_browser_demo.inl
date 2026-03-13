int mdgui_begin_render_window_ex(MDGUI_Context *ctx, const char *title, int x,
                                 int y, int w, int h, int show_menu, int flags,
                                 int *out_x, int *out_y, int *out_w,
                                 int *out_h) {
  if (!ctx)
    return 0;

  // Nested mode: inside an existing
  // parent window, treat render
  // window as a clipped child
  // viewport anchored in parent-local
  // coordinates.
  if (ctx->current_window >= 0 &&
      ctx->current_window < (int)ctx->windows.size()) {
    auto &parent = ctx->windows[ctx->current_window];
    const auto *subpass = current_subpass(ctx);
    const bool no_chrome = (flags & MDGUI_WINDOW_FLAG_NO_CHROME) != 0;
    const int requested_h = h;
    w = resolve_dynamic_width(ctx, x, w, 8);
    const int ix = ctx->origin_x + x + ctx->layout_indent;
    const int logical_y = ctx->content_y + y;
    const int iy = subpass ? logical_y : (logical_y - parent.text_scroll);

    if (requested_h == 0 || requested_h < 0) {
      const int viewport_bottom =
          subpass ? subpass->local_h : (parent.y + parent.h - 4);
      int avail_h = viewport_bottom - logical_y;
      if (requested_h < 0)
        avail_h += requested_h;
      h = avail_h;
    }
    if (h < 8)
      h = 8;

    // Parent content clip (active
    // while drawing nested
    // frame/body) so border can never
    // bleed outside tiled/visible
    // parent content.
    const int parent_clip_x = current_viewport_x(ctx);
    const int parent_clip_y = ctx->origin_y;
    int parent_clip_w = current_viewport_width(ctx);
    int parent_clip_h = 0;
    if (subpass) {
      parent_clip_h = subpass->local_h - parent_clip_y;
    } else {
      parent_clip_h = (parent.y + parent.h - 4) - parent_clip_y;
    }
    if (parent_clip_w < 1)
      parent_clip_w = 1;
    if (parent_clip_h < 1)
      parent_clip_h = 1;
    mdgui_backend_set_clip_rect(1, parent_clip_x, parent_clip_y, parent_clip_w,
                                parent_clip_h);

    const int cx = no_chrome ? ix : (ix + 1);
    const int cy = no_chrome ? iy : (iy + 1);
    int cw = no_chrome ? w : (w - 2);
    int ch = no_chrome ? h : (h - 2);
    if (cw < 1)
      cw = 1;
    if (ch < 1)
      ch = 1;

    // Intersect child content clip
    // with parent content clip.
    int clip_x1 = cx;
    int clip_y1 = cy;
    int clip_x2 = cx + cw;
    int clip_y2 = cy + ch;
    const int parent_x2 = parent_clip_x + parent_clip_w;
    const int parent_y2 = parent_clip_y + parent_clip_h;
    if (clip_x1 < parent_clip_x)
      clip_x1 = parent_clip_x;
    if (clip_y1 < parent_clip_y)
      clip_y1 = parent_clip_y;
    if (clip_x2 > parent_x2)
      clip_x2 = parent_x2;
    if (clip_y2 > parent_y2)
      clip_y2 = parent_y2;
    int clip_w = clip_x2 - clip_x1;
    int clip_h = clip_y2 - clip_y1;
    if (clip_w <= 0 || clip_h <= 0) {
      note_content_bounds(ctx, ix + w, logical_y + h);
      ctx->content_y = logical_y + h + 4;
      if (out_x)
        *out_x = 0;
      if (out_y)
        *out_y = 0;
      if (out_w)
        *out_w = 0;
      if (out_h)
        *out_h = 0;
      (void)title;
      (void)show_menu;
      (void)flags;
      return 0;
    }

    MDGUI_Context::NestedRenderState nested{};
    nested.parent_origin_x = ctx->origin_x;
    nested.parent_origin_y = ctx->origin_y;
    nested.parent_content_y = ctx->content_y;
    nested.parent_content_req_right = ctx->content_req_right;
    nested.parent_content_req_bottom = ctx->content_req_bottom;
    nested.parent_window_has_nonlabel_widget = ctx->window_has_nonlabel_widget;
    nested.parent_alpha_mod = mdgui_backend_get_alpha_mod();
    nested.parent_content_y_after = logical_y + h + 4;
    nested.parent_layout_indent = ctx->layout_indent;
    nested.parent_indent_stack_size = ctx->indent_stack.size();
    ctx->nested_render_stack.push_back(nested);
    const bool transparent_bg = (flags & MDGUI_WINDOW_FLAG_TRANSPARENT_BG) != 0;
    const int frame_x1 = ix - 1;
    const int frame_y1 = iy - 1;
    const int frame_x2 = ix + w + 1;
    const int frame_y2 = iy + h + 1;
    if (!no_chrome) {
      mdgui_draw_frame_idx(nullptr, CLR_WINDOW_BORDER, frame_x1, frame_y1,
                           frame_x2, frame_y2);
    }
    if (!transparent_bg) {
      mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, ix, iy, w, h);
    }
    note_content_bounds(ctx, ix + w, logical_y + h);

    ctx->origin_x = cx;
    ctx->origin_y = cy;
    ctx->content_y = cy;
    ctx->layout_indent = 0;
    mdgui_backend_set_clip_rect(1, clip_x1, clip_y1, clip_w, clip_h);

    if (out_x)
      *out_x = cx;
    if (out_y)
      *out_y = cy;
    if (out_w)
      *out_w = cw;
    if (out_h)
      *out_h = ch;
    (void)title;
    (void)show_menu;
    (void)flags;
    return 1;
  }

  if (!mdgui_begin_window_ex(ctx, title, x, y, w, h, flags))
    return 0;
  if (show_menu) {
    mdgui_begin_menu_bar(ctx);
    mdgui_end_menu_bar(ctx);
  }
  if (ctx->current_window < 0)
    return 0;
  const auto &win = ctx->windows[ctx->current_window];
  const int cx = ctx->origin_x;
  int cy = ctx->origin_y;
  // Render-window content is raw viewport content; it should not inherit flow
  // layout padding. If menu exists, anchor directly below it.
  if (show_menu && ctx->has_menu_bar)
    cy = ctx->menu_bar_y + ctx->menu_bar_h + 2;
  int cw = win.w - 4;
  int ch = (win.y + win.h - 4) - cy;
  if (cw < 1)
    cw = 1;
  if (ch < 1)
    ch = 1;
  ctx->content_y = cy;
  ctx->layout_indent = 0;
  if (out_x)
    *out_x = cx;
  if (out_y)
    *out_y = cy;
  if (out_w)
    *out_w = cw;
  if (out_h)
    *out_h = ch;
  return 1;
}

int mdgui_begin_render_window(MDGUI_Context *ctx, const char *title, int x,
                              int y, int w, int h, int show_menu, int *out_x,
                              int *out_y, int *out_w, int *out_h) {
  return mdgui_begin_render_window_ex(ctx, title, x, y, w, h, show_menu,
                                      MDGUI_WINDOW_FLAG_NONE, out_x, out_y,
                                      out_w, out_h);
}

void mdgui_overlay_init_config(MDGUI_OverlayConfig *config) {
  if (!config)
    return;
  memset(config, 0, sizeof(*config));
  config->subpass_scale = 1.0f;
  config->clamp_to_bounds = 1;
  config->consume_mouse_input_while_dragging = 1;
  config->window_flags =
      MDGUI_WINDOW_FLAG_NO_CHROME | MDGUI_WINDOW_FLAG_EXCLUDE_FROM_TILING;
}

void mdgui_overlay_init_state(MDGUI_OverlayState *state) {
  if (!state)
    return;
  memset(state, 0, sizeof(*state));
  state->use_subpass = 1;
  state->w = 240;
  state->h = 78;
  state->alpha = 255;
}

void mdgui_overlay_handle_drag(MDGUI_Input *input,
                               const MDGUI_OverlayConfig *config,
                               MDGUI_OverlayState *state, int raw_mouse_down,
                               int bounds_w, int bounds_h) {
  if (!input || !state) {
    return;
  }
  if (!state->visible || !state->allow_mouse_drag || state->click_through) {
    state->dragging = 0;
    return;
  }

  sanitize_overlay_state(state);

  if (input->mouse_pressed &&
      point_in_rect(input->mouse_x, input->mouse_y, state->x, state->y,
                    state->w, state->h)) {
    state->dragging = 1;
    state->drag_offset_x = input->mouse_x - state->x;
    state->drag_offset_y = input->mouse_y - state->y;
    if (!config || config->consume_mouse_input_while_dragging) {
      input->mouse_pressed = 0;
    }
  }

  if (state->dragging && raw_mouse_down) {
    int next_x = input->mouse_x - state->drag_offset_x;
    int next_y = input->mouse_y - state->drag_offset_y;
    if (!config || config->clamp_to_bounds) {
      if (bounds_w > 0)
        next_x = clampi(next_x, 0, std::max(0, bounds_w - state->w));
      if (bounds_h > 0)
        next_y = clampi(next_y, 0, std::max(0, bounds_h - state->h));
    }
    state->x = next_x;
    state->y = next_y;
    if (!config || config->consume_mouse_input_while_dragging) {
      input->mouse_pressed = 0;
      input->mouse_down = 0;
    }
  }

  if (!raw_mouse_down)
    state->dragging = 0;
}

int mdgui_begin_overlay(MDGUI_Context *ctx, const MDGUI_OverlayConfig *config,
                        MDGUI_OverlayState *state,
                        MDGUI_OverlayLayout *out_layout) {
  if (out_layout)
    memset(out_layout, 0, sizeof(*out_layout));
  if (!ctx || !config || !config->title || !state)
    return 0;

  sanitize_overlay_state(state);
  const bool click_through = state->click_through != 0;

  if (!state->visible) {
    mdgui_set_window_open(ctx, config->title, 0);
    return 0;
  }

  mdgui_set_window_rect(ctx, config->title, state->x, state->y, state->w,
                        state->h);
  mdgui_set_window_open(ctx, config->title, 1);
  mdgui_set_window_alpha(ctx, config->title, state->alpha);

  int view_x = 0;
  int view_y = 0;
  int view_w = 0;
  int view_h = 0;
  if (!mdgui_begin_render_window_ex(ctx, config->title, state->x, state->y,
                                    state->w, state->h, 0, config->window_flags,
                                    &view_x, &view_y, &view_w, &view_h)) {
    return 0;
  }

  if (ctx->current_window >= 0 &&
      ctx->current_window < (int)ctx->windows.size()) {
    ctx->windows[ctx->current_window].input_passthrough = click_through;
  }

  int base_x = view_x;
  int base_y = view_y;
  int base_w = view_w;
  int base_h = view_h;
  if (state->use_subpass && config->subpass_scale > 0.0f) {
    int sx = 0;
    int sy = 0;
    int sw = 0;
    int sh = 0;
    if (mdgui_begin_subpass(ctx, config->title, 0, 0, 0, 0, config->subpass_scale,
                            &sx, &sy, &sw, &sh)) {
      base_x = sx;
      base_y = sy;
      base_w = sw;
      base_h = sh;
    }
  }

  if (state->margin_top > 0)
    mdgui_spacing(ctx, state->margin_top);
  if (state->margin_left > 0)
    mdgui_indent(ctx, state->margin_left);

  if (out_layout) {
    out_layout->view_x = base_x;
    out_layout->view_y = base_y;
    out_layout->view_w = base_w;
    out_layout->view_h = base_h;
    out_layout->content_x = base_x + state->margin_left;
    out_layout->content_y = base_y + state->margin_top;
    out_layout->content_w =
        std::max(1, base_w - state->margin_left - state->margin_right);
    out_layout->content_h =
        std::max(1, base_h - state->margin_top - state->margin_bottom);
  }

  return 1;
}

void mdgui_end_overlay(MDGUI_Context *ctx, const MDGUI_OverlayConfig *config,
                       const MDGUI_OverlayState *state) {
  if (!ctx || !config || !state)
    return;
  if (state->margin_left > 0)
    mdgui_unindent(ctx);
  if (state->use_subpass && !ctx->subpass_stack.empty())
    mdgui_end_subpass(ctx);
  mdgui_end_window(ctx);
}

void mdgui_run_window_pass(MDGUI_Context *ctx,
                           const MDGUI_WindowPassItem *items, int item_count) {
  if (!ctx || !items || item_count <= 0)
    return;

  struct PassOrderEntry {
    int item_index;
    int z;
  };
  std::vector<PassOrderEntry> order;
  order.reserve((size_t)item_count);

  for (int i = 0; i < item_count; ++i) {
    const auto &it = items[i];
    if (!it.title)
      continue;
    const int idx =
        find_or_create_window(ctx, it.title, it.x, it.y, it.w, it.h);
    if (idx < 0 || idx >= (int)ctx->windows.size())
      continue;
    order.push_back({i, ctx->windows[idx].z});
  }

  std::sort(order.begin(), order.end(),
            [](const PassOrderEntry &a, const PassOrderEntry &b) {
              return a.z < b.z;
            });

  for (const auto &entry : order) {
    const auto &it = items[entry.item_index];
    if (it.use_render_window) {
      int cx = 0, cy = 0, cw = 0, ch = 0;
      if (mdgui_begin_render_window(ctx, it.title, it.x, it.y, it.w, it.h,
                                    it.show_menu, &cx, &cy, &cw, &ch)) {
        if (it.draw_fn)
          it.draw_fn(ctx, cx, cy, cw, ch, it.user_data);
        mdgui_end_window(ctx);
      }
    } else {
      if (mdgui_begin_window(ctx, it.title, it.x, it.y, it.w, it.h)) {
        int cx = 0, cy = 0, cw = 1, ch = 1;
        if (ctx->current_window >= 0 &&
            ctx->current_window < (int)ctx->windows.size()) {
          const auto &win = ctx->windows[ctx->current_window];
          cx = ctx->origin_x;
          cy = ctx->content_y;
          cw = win.w - 4;
          ch = (win.y + win.h - 4) - cy;
          if (cw < 1)
            cw = 1;
          if (ch < 1)
            ch = 1;
        }
        if (it.draw_fn)
          it.draw_fn(ctx, cx, cy, cw, ch, it.user_data);
        mdgui_end_window(ctx);
      }
    }
  }
}

static void open_browser_at_mode(MDGUI_Context *ctx, int x, int y,
                                 bool select_folders) {
  if (!ctx)
    return;
  const char *browser_title = select_folders ? "Folder Browser" : "File Browser";
  const char *other_title = select_folders ? "File Browser" : "Folder Browser";
  for (auto &w : ctx->windows) {
    if (w.id == other_title) {
      w.closed = true;
    }
  }
  // Prevent same-frame click-through
  // from the opener button/menu item.
  ctx->input.mouse_pressed = 0;
  const int idx = find_or_create_window(ctx, browser_title, 28, 20, 280, 240);
  if (idx >= 0 && idx < (int)ctx->windows.size()) {
    auto &win = ctx->windows[idx];
    win.closed = false;
    if (!is_tiled_file_browser_window(ctx, win))
      win.z = ++ctx->z_counter;
    if (x >= 0 || y >= 0) {
      if (x >= 0)
        win.x = x;
      if (y >= 0)
        win.y = y;
      clamp_window_to_work_area(ctx, win);
      win.restored_x = win.x;
      win.restored_y = win.y;
      win.restored_w = win.w;
      win.restored_h = win.h;
    } else {
      center_window_rect_menu_aware(ctx, win);
      win.restored_x = win.x;
      win.restored_y = win.y;
      win.restored_w = win.w;
      win.restored_h = win.h;
    }
  }
  ctx->dragging_window = -1;
  ctx->resizing_window = -1;
  ctx->file_browser_open = true;
  ctx->file_browser_select_folders = select_folders;
  ctx->file_browser_open_x = x;
  ctx->file_browser_open_y = y;
  ctx->file_browser_center_pending = (x < 0 && y < 0);
  ctx->file_browser_result.clear();
  ctx->file_browser_last_click_idx = -1;
  ctx->file_browser_last_click_ticks = 0;
  file_browser_refresh_drives(ctx, true);
  if (!file_browser_path_exists(ctx->file_browser_cwd))
    ctx->file_browser_cwd = file_browser_default_open_path(ctx);
  file_browser_open_path(ctx, ctx->file_browser_cwd.c_str());
}

void mdgui_open_file_browser_at(MDGUI_Context *ctx, int x, int y) {
  open_browser_at_mode(ctx, x, y, false);
}

void mdgui_open_file_browser(MDGUI_Context *ctx) {
  mdgui_open_file_browser_at(ctx, -1, -1);
}

void mdgui_open_folder_browser_at(MDGUI_Context *ctx, int x, int y) {
  open_browser_at_mode(ctx, x, y, true);
}

void mdgui_open_folder_browser(MDGUI_Context *ctx) {
  mdgui_open_folder_browser_at(ctx, -1, -1);
}

void mdgui_set_file_browser_filters(MDGUI_Context *ctx, const char **extensions,
                                    int extension_count) {
  if (!ctx)
    return;
  ctx->file_browser_ext_filters.clear();
  if (!extensions || extension_count <= 0)
    return;

  for (int i = 0; i < extension_count; ++i) {
    const std::string norm = normalize_extension_filter(extensions[i]);
    if (norm.empty())
      continue;
    bool duplicate = false;
    for (const auto &existing : ctx->file_browser_ext_filters) {
      if (existing == norm) {
        duplicate = true;
        break;
      }
    }
    if (!duplicate)
      ctx->file_browser_ext_filters.push_back(norm);
  }
}

static const char *show_browser_mode(MDGUI_Context *ctx, bool select_folders) {
  if (!ctx || !ctx->file_browser_open ||
      ctx->file_browser_select_folders != select_folders)
    return nullptr;
  const char *browser_title = select_folders ? "Folder Browser" : "File Browser";
  ctx->file_browser_result.clear();

  const int open_x = (ctx->file_browser_open_x >= 0) ? ctx->file_browser_open_x : 28;
  const int open_y = (ctx->file_browser_open_y >= 0) ? ctx->file_browser_open_y : 20;
  if (!mdgui_begin_window(ctx, browser_title, open_x, open_y, 280, 240)) {
    for (int i = 0; i < (int)ctx->windows.size(); ++i) {
      if (ctx->windows[i].id == browser_title && !ctx->windows[i].closed) {
        // Hidden behind a higher z fullscreen window; keep browser state open.
        return nullptr;
      }
    }
    ctx->file_browser_open = false;
    return nullptr;
  }
  auto &win = ctx->windows[ctx->current_window];
  if (ctx->file_browser_center_pending) {
    center_window_rect_menu_aware(ctx, win);
    win.restored_x = win.x;
    win.restored_y = win.y;
    win.restored_w = win.w;
    win.restored_h = win.h;
    ctx->file_browser_center_pending = false;
  }
  if (win.min_w < 220)
    win.min_w = 220;
  if (win.min_h < 180)
    win.min_h = 180;

  file_browser_refresh_drives(ctx, false);
  if (!file_browser_path_exists(ctx->file_browser_cwd)) {
    const std::string fallback = file_browser_default_open_path(ctx);
    file_browser_open_path(ctx, fallback.c_str());
  }

  if (ctx->file_browser_path_subpass_enabled) {
    int path_local_x = 0;
    int path_logical_y = 0;
    layout_prepare_widget(ctx, &path_local_x, &path_logical_y);
    const int path_parent_w = layout_resolve_width(ctx, path_local_x, 0, 20);
    const int path_line_h =
        std::max(ctx->style.label_h,
                 font_line_height(ctx, ctx->file_browser_path_font));
    const int path_top_inset = std::max(1, ctx->style.spacing_y / 2);
    float parent_scale_x = 1.0f;
    float parent_scale_y = 1.0f;
    get_current_render_scale(ctx, &parent_scale_x, &parent_scale_y);
    const float subpass_scale = 1.0f;
    int path_parent_h =
        (int)(((float)(path_line_h + path_top_inset) * subpass_scale) /
              parent_scale_y +
              0.999f);
    if (path_parent_h < 1)
      path_parent_h = 1;

    if (mdgui_begin_subpass(ctx, "file_browser_path", 0, 0, path_parent_w,
                            path_parent_h, subpass_scale, nullptr, nullptr,
                            nullptr, nullptr) != 0) {
      mdgui_label_font(ctx, ctx->file_browser_cwd.c_str(),
                       ctx->file_browser_path_font);
      mdgui_end_subpass(ctx);
      layout_commit_widget(ctx, path_local_x, path_logical_y, path_parent_w,
                           path_parent_h, ctx->style.spacing_y);
    } else {
      mdgui_label_font(ctx, ctx->file_browser_cwd.c_str(),
                       ctx->file_browser_path_font);
    }
  } else {
    mdgui_label_font(ctx, ctx->file_browser_cwd.c_str(),
                     ctx->file_browser_path_font);
  }
  const int drive_button_w = 64;
  const int drive_stride = drive_button_w + ctx->style.spacing_x;
  const int drive_avail_w = resolve_dynamic_width(ctx, 0, 0, drive_button_w);
  int drive_columns = (drive_avail_w + ctx->style.spacing_x) / drive_stride;
  if (drive_columns < 1)
    drive_columns = 1;
  for (int i = 0; i < (int)ctx->file_browser_drives.size(); ++i) {
    const std::string &drive = ctx->file_browser_drives[(size_t)i];
    if (mdgui_button(ctx, drive.c_str(), drive_button_w, 12)) {
      file_browser_open_path(ctx, drive.c_str());
    }
    if (((i + 1) % drive_columns) != 0 &&
        (i + 1) < (int)ctx->file_browser_drives.size())
      mdgui_same_line(ctx);
  }
  if (ctx->file_browser_drives.empty()) {
    mdgui_label(ctx, "(no roots found)");
  }
  mdgui_spacing(ctx, 2);

  const int row_h = 10;
  const int button_h = 12;
  const int list_x = ctx->origin_x + 8;
  const int list_y = ctx->content_y + 4;
  const int list_w = resolve_dynamic_width(ctx, 8, -16, 40);
  const int bottom_buttons_y = win.y + win.h - button_h - 4;
  int list_h = bottom_buttons_y - 4 - list_y;
  if (list_h < row_h)
    list_h = row_h;
  const int rows = list_h / row_h;
  list_h = rows * row_h;
  const int scrollbar_w = 9;
  const int content_w = list_w - scrollbar_w - 1;
  const int total = (int)ctx->file_browser_entries.size();
  const int max_scroll = (total > rows) ? (total - rows) : 0;
  if (ctx->file_browser_restore_scroll_pending && ctx->file_browser_selected >= 0 &&
      total > 0) {
    int target = ctx->file_browser_selected - (rows / 2);
    if (target < 0)
      target = 0;
    const int max_target = std::max(0, total - rows);
    if (target > max_target)
      target = max_target;
    ctx->file_browser_scroll = target;
    ctx->file_browser_restore_scroll_pending = false;
  }
  const int over_list = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                      list_x, list_y, content_w, list_h);
  const int over_scrollbar =
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                    list_x + content_w + 1, list_y, scrollbar_w, list_h);
  const int over_browser_pane = point_in_rect(
      ctx->input.mouse_x, ctx->input.mouse_y, list_x, list_y, list_w, list_h);
  if (ctx->file_browser_scroll < 0)
    ctx->file_browser_scroll = 0;
  if (ctx->file_browser_scroll > max_scroll)
    ctx->file_browser_scroll = max_scroll;
  if (ctx->input.mouse_wheel != 0 &&
      (over_list || over_scrollbar || over_browser_pane)) {
    ctx->file_browser_scroll -= ctx->input.mouse_wheel;
    if (ctx->file_browser_scroll < 0)
      ctx->file_browser_scroll = 0;
    if (ctx->file_browser_scroll > max_scroll)
      ctx->file_browser_scroll = max_scroll;
  }

  mdgui_draw_frame_idx(nullptr, CLR_TEXT_DARK, list_x - 1, list_y - 1,
                       list_x + list_w + 1, list_y + list_h + 1);
  mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, list_x, list_y, list_w, list_h);

  // Clip row text to the virtual list
  // viewport so long names can't
  // bleed out.
  mdgui_backend_set_clip_rect(1, list_x, list_y, content_w, list_h);

  int clicked = 0;
  for (int i = 0; i < rows; i++) {
    const int item_idx = ctx->file_browser_scroll + i;
    const int ry = list_y + (i * row_h);
    if (item_idx >= total)
      break;
    const int hovered = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                      list_x, ry, content_w, row_h);
    if (item_idx == ctx->file_browser_selected)
      mdgui_fill_rect_idx(nullptr, CLR_ACCENT, list_x, ry, content_w, row_h);
    else if (hovered)
      mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, list_x, ry, content_w,
                          row_h);
    if (mdgui_fonts[1]) {
      mdgui_fonts[1]->drawText(
          ctx->file_browser_entries[item_idx].label.c_str(), nullptr,
          list_x + 2, ry + 1, CLR_MENU_TEXT);
    }
    if (hovered && ctx->input.mouse_pressed) {
      ctx->file_browser_selected = item_idx;
      clicked = 1;
    }
  }
  mdgui_backend_set_clip_rect(0, 0, 0, 0, 0);
  set_content_clip(ctx);

  const int sb_x = list_x + content_w + 1;
  mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, sb_x, list_y, scrollbar_w,
                      list_h);
  if (max_scroll > 0) {
    int thumb_h = (rows * list_h) / total;
    if (thumb_h < 10)
      thumb_h = 10;
    if (thumb_h > list_h)
      thumb_h = list_h;
    const int travel = list_h - thumb_h;
    const int thumb_y =
        list_y + ((ctx->file_browser_scroll * travel) / max_scroll);
    mdgui_fill_rect_idx(nullptr, CLR_ACCENT, sb_x + 1, thumb_y, scrollbar_w - 2,
                        thumb_h);
    if (ctx->input.mouse_pressed && over_scrollbar) {
      if (ctx->input.mouse_y >= thumb_y &&
          ctx->input.mouse_y <= thumb_y + thumb_h) {
        ctx->file_browser_scroll_dragging = true;
        ctx->file_browser_scroll_drag_offset = ctx->input.mouse_y - thumb_y;
      } else if (ctx->input.mouse_y < thumb_y)
        ctx->file_browser_scroll -= rows;
      else if (ctx->input.mouse_y > thumb_y + thumb_h)
        ctx->file_browser_scroll += rows;
      if (ctx->file_browser_scroll < 0)
        ctx->file_browser_scroll = 0;
      if (ctx->file_browser_scroll > max_scroll)
        ctx->file_browser_scroll = max_scroll;
    }
    if (ctx->file_browser_scroll_dragging && ctx->input.mouse_down) {
      int desired_thumb_y =
          ctx->input.mouse_y - ctx->file_browser_scroll_drag_offset;
      if (desired_thumb_y < list_y)
        desired_thumb_y = list_y;
      if (desired_thumb_y > list_y + travel)
        desired_thumb_y = list_y + travel;
      ctx->file_browser_scroll =
          ((desired_thumb_y - list_y) * max_scroll) / (travel > 0 ? travel : 1);
      if (ctx->file_browser_scroll < 0)
        ctx->file_browser_scroll = 0;
      if (ctx->file_browser_scroll > max_scroll)
        ctx->file_browser_scroll = max_scroll;
    }
  }
  note_content_bounds(ctx, list_x + list_w, list_y + list_h);
  ctx->content_y += 4 + list_h + 4;

  bool trigger_open = false;
  bool trigger_select = false;
  if (clicked && ctx->file_browser_selected >= 0 &&
      ctx->file_browser_selected < (int)ctx->file_browser_entries.size()) {
    const int idx = ctx->file_browser_selected;
    const unsigned long long now = mdgui_backend_get_ticks_ms();
    if (ctx->file_browser_last_click_idx == idx &&
        (now - ctx->file_browser_last_click_ticks) <= 350) {
      trigger_open = true;
    }
    ctx->file_browser_last_click_idx = idx;
    ctx->file_browser_last_click_ticks = now;
  }

  const int saved_content_y = ctx->content_y;
  const int saved_indent = ctx->layout_indent;
  const bool saved_has_last = ctx->layout_has_last_item;
  const bool saved_same_line = ctx->layout_same_line;
  const int button_logical_y = bottom_buttons_y + win.text_scroll;

  ctx->content_y = button_logical_y;
  ctx->layout_indent = 8;
  ctx->layout_has_last_item = false;
  ctx->layout_same_line = false;
  if (mdgui_button(ctx, "Up", 48, button_h)) {
    file_browser_open_path(ctx, "..");
  }
  ctx->content_y = button_logical_y;
  ctx->layout_indent = 60;
  ctx->layout_has_last_item = false;
  ctx->layout_same_line = false;
  if (mdgui_button(ctx, select_folders ? "Select" : "Open", 60, button_h)) {
    if (select_folders) {
      trigger_select = true;
    } else {
      trigger_open = true;
    }
  }
  ctx->content_y = button_logical_y;
  ctx->layout_indent = 124;
  ctx->layout_has_last_item = false;
  ctx->layout_same_line = false;
  if (mdgui_button(ctx, "Cancel", 60, button_h)) {
    ctx->file_browser_open = false;
    if (ctx->current_window >= 0 &&
        ctx->current_window < (int)ctx->windows.size()) {
      ctx->windows[ctx->current_window].closed = true;
    }
  }
  ctx->content_y = saved_content_y;
  ctx->layout_indent = saved_indent;
  ctx->layout_has_last_item = saved_has_last;
  ctx->layout_same_line = saved_same_line;

  if (trigger_select) {
    std::string selected_path = ctx->file_browser_cwd;
    if (ctx->file_browser_selected >= 0 &&
        ctx->file_browser_selected < (int)ctx->file_browser_entries.size()) {
      const auto &entry = ctx->file_browser_entries[ctx->file_browser_selected];
      if (entry.is_dir && entry.label != "..")
        selected_path = entry.full_path;
    }
    ctx->file_browser_result = selected_path;
    ctx->file_browser_open = false;
    if (ctx->current_window >= 0 &&
        ctx->current_window < (int)ctx->windows.size()) {
      ctx->windows[ctx->current_window].closed = true;
    }
  }

  if (trigger_open && ctx->file_browser_selected >= 0 &&
      ctx->file_browser_selected < (int)ctx->file_browser_entries.size()) {
    const auto &entry = ctx->file_browser_entries[ctx->file_browser_selected];
    if (entry.is_dir) {
      file_browser_open_path(ctx, entry.full_path.c_str());
    } else if (!select_folders) {
      ctx->file_browser_last_selected_path = entry.full_path;
      ctx->file_browser_result = entry.full_path;
      ctx->file_browser_open = false;
      if (ctx->current_window >= 0 &&
          ctx->current_window < (int)ctx->windows.size()) {
        ctx->windows[ctx->current_window].closed = true;
      }
    }
  }

  mdgui_end_window(ctx);
  if (!ctx->file_browser_result.empty())
    return ctx->file_browser_result.c_str();
  return nullptr;
}

const char *mdgui_show_file_browser(MDGUI_Context *ctx) {
  return show_browser_mode(ctx, false);
}

const char *mdgui_show_folder_browser(MDGUI_Context *ctx) {
  return show_browser_mode(ctx, true);
}

void mdgui_show_demo_window(MDGUI_Context *ctx) {
  static bool show_demo = true;
  static float progress = 0.35f;
  static int quality_idx = 0;
  static int theme_idx = MDGUI_THEME_DEFAULT;
  static int pane_tab = 0;
  static const char *quality_items[] = {"Nearest", "Bilinear", "CRT Mask"};
  static const char *theme_items[] = {"Default",  "Dark",     "Amber",
                                      "Graphite", "Midnight", "Olive"};
  static const char *pane_tabs[] = {"Scene",   "Inspector", "Console",
                                    "Profiler","Assets",    "Settings",
                                    "History"};
  if (!show_demo)
    return;

  if (mdgui_begin_window(ctx, "MDGUI Demo Window", 20, 20, 220, 220)) {
    static bool check1 = true;
    static bool check2 = false;
    static float vol1 = 0.5f;
    static float vol2 = 0.8f;
    MDGUI_Font *demo_font = nullptr;

    if (!ctx->demo_window_font_labels.empty() &&
        ctx->demo_window_font_labels.size() == ctx->demo_window_fonts.size()) {
      mdgui_label(ctx, "Window Font");
      if (ctx->demo_window_font_index < 0 ||
          ctx->demo_window_font_index >= (int)ctx->demo_window_fonts.size()) {
        ctx->demo_window_font_index = 0;
      }
      mdgui_combo(ctx, nullptr, ctx->demo_window_font_labels.data(),
                  (int)ctx->demo_window_font_labels.size(),
                  &ctx->demo_window_font_index, -16);
      if (ctx->demo_window_font_index >= 0 &&
          ctx->demo_window_font_index < (int)ctx->demo_window_fonts.size()) {
        demo_font = ctx->demo_window_fonts[(size_t)ctx->demo_window_font_index];
      }
      if (demo_font)
        mdgui_push_font(ctx, demo_font);
    }

    if (mdgui_collapsing_header(ctx, "demo.widgets", "Aesthetics & Widgets",
                                -16, 1)) {
      mdgui_indent(ctx, 8);
      mdgui_checkbox(ctx, "Enable Sound", &check1);
      mdgui_checkbox(ctx, "Turbo Mode", &check2);
      mdgui_unindent(ctx);
    }

    if (mdgui_collapsing_header(ctx, "demo.volumes", "Audio Volumes", -16, 1)) {
      mdgui_indent(ctx, 8);
      mdgui_slider(ctx, "Master", &vol1, 0.0f, 1.0f, -54);
      mdgui_slider(ctx, "Music", &vol2, 0.0f, 1.0f, -46);
      mdgui_unindent(ctx);
    }

    if (mdgui_collapsing_header(ctx, "demo.renderer", "Renderer Options", -16,
                                1)) {
      mdgui_indent(ctx, 8);
      mdgui_listbox(ctx, quality_items, 3, &quality_idx, -16, 3);

      mdgui_label(ctx, "Filter Combo");
      mdgui_combo(ctx, nullptr, quality_items, 3, &quality_idx, -16);

      mdgui_label(ctx, "Theme");
      theme_idx = mdgui_get_theme();
      if (mdgui_combo(ctx, nullptr, theme_items, 6, &theme_idx, -16)) {
        mdgui_set_theme(theme_idx);
      }

      mdgui_label(ctx, "Frame Progress");
      mdgui_progress_bar(ctx, progress, -16, 10, nullptr);
      if (mdgui_button(ctx, "Step", 56, 12)) {
        progress += 0.1f;
        if (progress > 1.0f)
          progress = 0.0f;
      }
      mdgui_unindent(ctx);
    }

    if (mdgui_collapsing_header(ctx, "demo.tabs", "Pane Tabs", -16, 1)) {
      mdgui_indent(ctx, 8);
      mdgui_tab_bar(ctx, "demo.panes", pane_tabs,
                    (int)(sizeof(pane_tabs) / sizeof(pane_tabs[0])), &pane_tab,
                    -16);
      if (pane_tab == 0) {
        mdgui_label(ctx, "Scene pane: viewport controls.");
      } else if (pane_tab == 1) {
        mdgui_label(ctx, "Inspector pane: entity properties.");
      } else if (pane_tab == 2) {
        mdgui_label(ctx, "Console pane: logs + commands.");
      } else if (pane_tab == 3) {
        mdgui_label(ctx, "Profiler pane: frame metrics.");
      } else if (pane_tab == 4) {
        mdgui_label(ctx, "Assets pane: project files.");
      } else if (pane_tab == 5) {
        mdgui_label(ctx, "Settings pane: runtime options.");
      } else {
        mdgui_label(ctx, "History pane: undo / redo timeline.");
      }
      mdgui_unindent(ctx);
    }

    // Reserve footer space, but only
    // show the close action when
    // scrolled near the bottom of
    // content.
    const auto &win = ctx->windows[ctx->current_window];
    const int viewport_h = (win.y + win.h - 4) - ctx->origin_y;
    const int content_h_before_footer = ctx->content_y - ctx->origin_y;
    const int footer_h = 6 + 12 + 4; // spacer + button
                                     // + widget gap
    const int total_h_with_footer = content_h_before_footer + footer_h;
    const int max_scroll_with_footer =
        (viewport_h > 0 && total_h_with_footer > viewport_h)
            ? (total_h_with_footer - viewport_h)
            : 0;
    const bool show_close = (max_scroll_with_footer == 0) ||
                            (win.text_scroll >= (max_scroll_with_footer - 2));

    const int footer_start_y = ctx->content_y;
    mdgui_spacing(ctx, footer_h);
    if (show_close) {
      const int saved_demo_content_y = ctx->content_y;
      const int saved_demo_indent = ctx->layout_indent;
      const bool saved_demo_has_last = ctx->layout_has_last_item;
      const bool saved_demo_same_line = ctx->layout_same_line;
      ctx->content_y = footer_start_y + 6 + win.text_scroll;
      ctx->layout_indent = 10;
      ctx->layout_has_last_item = false;
      ctx->layout_same_line = false;
      if (mdgui_button(ctx, "Close Demo", -12, 12)) {
        ctx->windows[ctx->current_window].closed = true;
      }
      ctx->content_y = saved_demo_content_y;
      ctx->layout_indent = saved_demo_indent;
      ctx->layout_has_last_item = saved_demo_has_last;
      ctx->layout_same_line = saved_demo_same_line;
    }

    if (demo_font)
      mdgui_pop_font(ctx);
    mdgui_end_window(ctx);
  }
}
}
