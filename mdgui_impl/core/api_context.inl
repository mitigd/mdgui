MDGUI_Context *mdgui_create(void *sdl_renderer) {
  MDGUI_RenderBackend backend = {};
  mdgui_make_sdl_backend(&backend, sdl_renderer);
  return mdgui_create_with_backend(&backend);
}

MDGUI_Context *mdgui_create_with_backend(const MDGUI_RenderBackend *backend) {
  auto *ctx = new MDGUI_Context{};
  memset(&ctx->backend, 0, sizeof(ctx->backend));
  memset(&ctx->input, 0, sizeof(ctx->input));
  ctx->z_counter = 1;
  ctx->dragging_window = -1;
  ctx->drag_off_x = 0;
  ctx->drag_off_y = 0;
  ctx->current_window = -1;
  ctx->origin_x = 0;
  ctx->origin_y = 0;
  ctx->content_y = 0;
  ctx->menu_index = 0;
  ctx->menu_next_x = 0;
  ctx->in_menu_bar = false;
  ctx->in_menu = false;
  ctx->current_menu_index = -1;
  ctx->building_menu_index = -1;
  ctx->current_menu_x = 0;
  ctx->current_menu_y = 0;
  ctx->current_menu_w = 150;
  ctx->current_menu_h = 10;
  ctx->current_menu_item = 0;
  ctx->has_menu_bar = false;
  ctx->menu_bar_x = 0;
  ctx->menu_bar_y = 0;
  ctx->menu_bar_w = 0;
  ctx->menu_bar_h = 0;
  ctx->menu_build_stack.clear();
  ctx->main_menu_index = 0;
  ctx->main_menu_next_x = 0;
  ctx->in_main_menu_bar = false;
  ctx->in_main_menu = false;
  ctx->open_main_menu_index = -1;
  ctx->building_main_menu_index = -1;
  ctx->main_menu_build_stack.clear();
  ctx->open_main_menu_path.clear();
  ctx->open_main_menu_item_path.clear();
  ctx->main_menu_bar_x = 0;
  ctx->main_menu_bar_y = 0;
  ctx->main_menu_bar_w = 0;
  ctx->main_menu_bar_h = 0;
  ctx->status_bar_visible = false;
  ctx->status_bar_h = STATUS_BAR_DEFAULT_H;
  ctx->status_bar_text.clear();

  ctx->resizing_window = -1;
  ctx->resize_edge_mask = 0;
  ctx->m_start_x = 0;
  ctx->m_start_y = 0;
  ctx->w_start_x = 0;
  ctx->w_start_y = 0;
  ctx->w_start_w = 0;
  ctx->w_start_h = 0;

  for (int i = 0; i < 16; i++)
    ctx->cursors[i] = nullptr;
  // Only SDL system cursors are used
  // in release-safe builds.
  ctx->cursors[0] = SDL_CreateSystemCursor((SDL_SystemCursor)0);
  ctx->cursor_anim_count = ctx->cursors[0] ? 1 : 0;
  ctx->cursors[5] = SDL_CreateSystemCursor((SDL_SystemCursor)5); // NWSE
  ctx->cursors[6] = SDL_CreateSystemCursor((SDL_SystemCursor)6); // NESW
  ctx->cursors[7] = SDL_CreateSystemCursor((SDL_SystemCursor)7); // EW
  ctx->cursors[8] = SDL_CreateSystemCursor((SDL_SystemCursor)8); // NS
  ctx->cursors[9] = SDL_CreateSystemCursor((SDL_SystemCursor)0); // default
                                                                 // arrow
  ctx->custom_cursor_enabled = false;
  ctx->cursor_request_idx = 9;
  ctx->current_cursor_idx = -1;
  ctx->content_req_right = 0;
  ctx->content_req_bottom = 0;
  ctx->style.spacing_x = 6;
  ctx->style.spacing_y = 4;
  ctx->style.section_spacing_y = 6;
  ctx->style.indent_step = 8;
  ctx->style.label_h = 12;
  ctx->style.content_pad_x = 8;
  ctx->style.content_pad_y = 6;
  ctx->layout_same_line = false;
  ctx->layout_has_last_item = false;
  ctx->layout_last_item_x = 0;
  ctx->layout_last_item_y = 0;
  ctx->layout_last_item_w = 0;
  ctx->layout_last_item_h = 0;
  ctx->layout_columns_active = false;
  ctx->layout_columns_count = 0;
  ctx->layout_columns_index = 0;
  ctx->layout_columns_start_x = 0;
  ctx->layout_columns_start_y = 0;
  ctx->layout_columns_width = 0;
  ctx->layout_columns_max_bottom = 0;
  ctx->layout_columns_bottoms.clear();
  ctx->default_font = nullptr;
  ctx->font_stack.clear();
  ctx->file_browser_path_font = nullptr;
  ctx->demo_window_font_labels_storage.clear();
  ctx->demo_window_font_labels.clear();
  ctx->demo_window_fonts.clear();
  ctx->demo_window_font_index = 0;
  ctx->window_has_nonlabel_widget = false;
  ctx->windows_locked = false;
  ctx->tile_manager_enabled = false;
  ctx->default_window_alpha = 255;
  ctx->combo_overlay_pending = false;
  ctx->combo_overlay_window = -1;
  ctx->combo_overlay_x = 0;
  ctx->combo_overlay_y = 0;
  ctx->combo_overlay_w = 0;
  ctx->combo_overlay_item_h = 0;
  ctx->combo_overlay_item_count = 0;
  ctx->combo_overlay_selected = -1;
  ctx->combo_overlay_items = nullptr;
  ctx->combo_capture_active = false;
  ctx->combo_capture_seen_this_frame = false;
  ctx->combo_capture_window = -1;
  ctx->combo_capture_x = 0;
  ctx->combo_capture_y = 0;
  ctx->combo_capture_w = 0;
  ctx->combo_capture_h = 0;
  ctx->active_text_input_window = -1;
  ctx->active_text_input_id = 0;
  ctx->active_text_input_cursor = 0;
  ctx->active_text_input_sel_anchor = 0;
  ctx->active_text_input_sel_start = 0;
  ctx->active_text_input_sel_end = 0;
  ctx->active_text_input_drag_select = false;
  ctx->active_text_input_multiline = false;
  ctx->active_text_input_scroll_y = 0;
  ctx->file_browser_open = false;
  ctx->file_browser_select_folders = false;
  ctx->file_browser_cwd = ".";
  ctx->file_browser_selected = -1;
  ctx->file_browser_scroll = 0;
  ctx->file_browser_scroll_dragging = false;
  ctx->file_browser_scroll_drag_offset = 0;
  ctx->file_browser_last_click_idx = -1;
  ctx->file_browser_last_click_ticks = 0;
  ctx->file_browser_result.clear();
  ctx->file_browser_drives = enumerate_file_browser_drives();
  ctx->file_browser_last_drive_scan_ticks = 0;
  ctx->file_browser_last_selected_path.clear();
  ctx->file_browser_restore_scroll_pending = false;
  ctx->file_browser_center_pending = false;
  ctx->file_browser_open_x = -1;
  ctx->file_browser_open_y = -1;
  ctx->file_browser_path_subpass_enabled = false;
  ctx->layout_indent = ctx->style.content_pad_x;
  ctx->indent_stack.clear();

  mdgui_set_backend(ctx, backend);
  for (int i = 0; i < 10; i++) {
    if (!mdgui_fonts[i]) {
      mdgui_fonts[i] = mdgui_font_create_builtin(1);
    }
  }
  ctx->default_font = fallback_font();
  return ctx;
}

void mdgui_destroy(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  for (int i = 0; i < 16; i++) {
    if (ctx->cursors[i]) {
      SDL_DestroyCursor(ctx->cursors[i]);
      ctx->cursors[i] = nullptr;
    }
  }
  delete ctx;
}

void mdgui_set_default_font(MDGUI_Context *ctx, MDGUI_Font *font) {
  if (!ctx)
    return;
  ctx->default_font = font ? font : fallback_font();
}

MDGUI_Font *mdgui_get_default_font(MDGUI_Context *ctx) {
  if (!ctx)
    return fallback_font();
  return ctx->default_font ? ctx->default_font : fallback_font();
}

void mdgui_push_font(MDGUI_Context *ctx, MDGUI_Font *font) {
  if (!ctx)
    return;
  ctx->font_stack.push_back(font ? font : fallback_font());
}

void mdgui_pop_font(MDGUI_Context *ctx) {
  if (!ctx || ctx->font_stack.empty())
    return;
  ctx->font_stack.pop_back();
}

void mdgui_set_demo_window_font_options(MDGUI_Context *ctx,
                                        const char **labels,
                                        MDGUI_Font **fonts, int font_count,
                                        int selected_index) {
  if (!ctx)
    return;
  ctx->demo_window_font_labels_storage.clear();
  ctx->demo_window_font_labels.clear();
  ctx->demo_window_fonts.clear();
  if (!labels || !fonts || font_count <= 0) {
    ctx->demo_window_font_index = 0;
    return;
  }

  ctx->demo_window_font_labels_storage.reserve((size_t)font_count);
  ctx->demo_window_font_labels.reserve((size_t)font_count);
  ctx->demo_window_fonts.reserve((size_t)font_count);
  for (int i = 0; i < font_count; ++i) {
    ctx->demo_window_font_labels_storage.emplace_back(labels[i] ? labels[i] : "");
    ctx->demo_window_fonts.push_back(fonts[i]);
  }
  for (const std::string &label : ctx->demo_window_font_labels_storage) {
    ctx->demo_window_font_labels.push_back(label.c_str());
  }

  if (selected_index < 0 || selected_index >= (int)ctx->demo_window_fonts.size())
    selected_index = 0;
  ctx->demo_window_font_index = selected_index;
}

void mdgui_set_demo_window_font_index(MDGUI_Context *ctx, int index) {
  if (!ctx)
    return;
  if (index < 0 || index >= (int)ctx->demo_window_fonts.size())
    return;
  ctx->demo_window_font_index = index;
}

int mdgui_get_demo_window_font_index(MDGUI_Context *ctx) {
  if (!ctx)
    return 0;
  return ctx->demo_window_font_index;
}

void mdgui_set_file_browser_path_font(MDGUI_Context *ctx, MDGUI_Font *font) {
  if (!ctx)
    return;
  ctx->file_browser_path_font = font;
}

MDGUI_Font *mdgui_get_file_browser_path_font(MDGUI_Context *ctx) {
  if (!ctx)
    return nullptr;
  return ctx->file_browser_path_font;
}

void mdgui_set_file_browser_path(MDGUI_Context *ctx, const char *path) {
  if (!ctx || !path || !*path)
    return;
  file_browser_open_path(ctx, path);
}

void mdgui_set_file_browser_path_subpass_enabled(MDGUI_Context *ctx,
                                                 int enabled) {
  if (!ctx)
    return;
  ctx->file_browser_path_subpass_enabled = (enabled != 0);
}

int mdgui_is_file_browser_path_subpass_enabled(MDGUI_Context *ctx) {
  if (!ctx)
    return 0;
  return ctx->file_browser_path_subpass_enabled ? 1 : 0;
}

static void get_current_render_scale(MDGUI_Context *ctx, float *out_sx,
                                     float *out_sy) {
  float sx = 1.0f;
  float sy = 1.0f;
  if (ctx && ctx->backend.get_render_scale) {
    ctx->backend.get_render_scale(ctx->backend.user_data, &sx, &sy);
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

void mdgui_set_renderer(MDGUI_Context *ctx, void *sdl_renderer) {
  MDGUI_RenderBackend backend = {};
  mdgui_make_sdl_backend(&backend, sdl_renderer);
  mdgui_set_backend(ctx, &backend);
}

void mdgui_set_backend(MDGUI_Context *ctx, const MDGUI_RenderBackend *backend) {
  if (!ctx)
    return;
  if (backend) {
    ctx->backend = *backend;
    mdgui_bind_backend(&ctx->backend);
  } else {
    memset(&ctx->backend, 0, sizeof(ctx->backend));
    mdgui_bind_backend(nullptr);
  }
}

int mdgui_begin_subpass(MDGUI_Context *ctx, const char *id, int x, int y, int w,
                        int h, float scale, int *out_x, int *out_y,
                        int *out_w, int *out_h) {
  if (!ctx || !id || ctx->current_window < 0 || scale <= 0.0f ||
      !ctx->backend.begin_subpass || !ctx->backend.end_subpass) {
    return 0;
  }
  if (!ctx->subpass_stack.empty())
    return 0;

  int local_x = 0;
  int logical_y = 0;
  if (x == 0 && y == 0 && w == 0 && h == 0) {
    layout_prepare_widget(ctx, &local_x, &logical_y);
  } else {
    local_x = x + ctx->layout_indent;
    logical_y = ctx->content_y + y;
  }

  if (w <= 0)
    w = layout_resolve_width(ctx, local_x, w, 1);
  if (h <= 0) {
    int viewport_bottom = 0;
    if (const auto *subpass = current_subpass(ctx)) {
      viewport_bottom = subpass->local_h;
    } else {
      const auto &win = ctx->windows[ctx->current_window];
      viewport_bottom = win.y + win.h - 4 + win.text_scroll;
    }
    int avail_h = viewport_bottom - logical_y;
    if (h < 0)
      avail_h += h;
    h = (avail_h > 0) ? avail_h : 1;
  }

  const int abs_x = ctx->origin_x + local_x;
  const int abs_y = logical_y - ctx->windows[ctx->current_window].text_scroll;
  int local_w = 0;
  int local_h = 0;
  if (!ctx->backend.begin_subpass(ctx->backend.user_data, id, abs_x, abs_y, w,
                                  h, scale, &local_w, &local_h)) {
    return 0;
  }

  if (local_w < 1)
    local_w = 1;
  if (local_h < 1)
    local_h = 1;

  MDGUI_Context::SubpassState subpass{};
  subpass.parent_input = ctx->input;
  subpass.parent_origin_x = ctx->origin_x;
  subpass.parent_origin_y = ctx->origin_y;
  subpass.parent_content_y = ctx->content_y;
  subpass.parent_content_req_right = ctx->content_req_right;
  subpass.parent_content_req_bottom = ctx->content_req_bottom;
  subpass.parent_window_has_nonlabel_widget = ctx->window_has_nonlabel_widget;
  subpass.parent_alpha_mod = mdgui_backend_get_alpha_mod();
  subpass.parent_layout_indent = ctx->layout_indent;
  subpass.parent_indent_stack_size = ctx->indent_stack.size();
  subpass.parent_layout_same_line = ctx->layout_same_line;
  subpass.parent_layout_has_last_item = ctx->layout_has_last_item;
  subpass.parent_layout_last_item_x = ctx->layout_last_item_x;
  subpass.parent_layout_last_item_y = ctx->layout_last_item_y;
  subpass.parent_layout_last_item_w = ctx->layout_last_item_w;
  subpass.parent_layout_last_item_h = ctx->layout_last_item_h;
  subpass.parent_local_x = local_x;
  subpass.parent_logical_y = logical_y;
  subpass.parent_region_w = w;
  subpass.parent_region_h = h;
  subpass.local_w = local_w;
  subpass.local_h = local_h;
  ctx->subpass_stack.push_back(subpass);

  const float input_scale_x =
      (w > 0) ? ((float)local_w / (float)w) : 1.0f;
  const float input_scale_y =
      (h > 0) ? ((float)local_h / (float)h) : 1.0f;
  ctx->input.mouse_x = (int)(((float)(ctx->input.mouse_x - abs_x) * input_scale_x) + 0.5f);
  ctx->input.mouse_y = (int)(((float)(ctx->input.mouse_y - abs_y) * input_scale_y) + 0.5f);

  ctx->origin_x = 0;
  ctx->origin_y = 0;
  ctx->content_y = 0;
  ctx->content_req_right = 0;
  ctx->content_req_bottom = 0;
  ctx->layout_indent = 0;
  ctx->layout_same_line = false;
  ctx->layout_has_last_item = false;
  ctx->window_has_nonlabel_widget = false;
  mdgui_backend_set_alpha_mod(255);
  set_content_clip(ctx);

  if (out_x)
    *out_x = 0;
  if (out_y)
    *out_y = 0;
  if (out_w)
    *out_w = local_w;
  if (out_h)
    *out_h = local_h;
  return 1;
}

void mdgui_end_subpass(MDGUI_Context *ctx) {
  if (!ctx || ctx->subpass_stack.empty())
    return;

  const auto subpass = ctx->subpass_stack.back();
  ctx->subpass_stack.pop_back();
  ctx->backend.end_subpass(ctx->backend.user_data);
  ctx->input = subpass.parent_input;
  ctx->origin_x = subpass.parent_origin_x;
  ctx->origin_y = subpass.parent_origin_y;
  ctx->content_y = subpass.parent_content_y;
  ctx->content_req_right =
      std::max(ctx->content_req_right, subpass.parent_content_req_right);
  ctx->content_req_bottom =
      std::max(ctx->content_req_bottom, subpass.parent_content_req_bottom);
  ctx->window_has_nonlabel_widget =
      ctx->window_has_nonlabel_widget || subpass.parent_window_has_nonlabel_widget;
  ctx->layout_indent = subpass.parent_layout_indent;
  if (ctx->indent_stack.size() > subpass.parent_indent_stack_size)
    ctx->indent_stack.resize(subpass.parent_indent_stack_size);
  ctx->layout_same_line = subpass.parent_layout_same_line;
  ctx->layout_has_last_item = subpass.parent_layout_has_last_item;
  ctx->layout_last_item_x = subpass.parent_layout_last_item_x;
  ctx->layout_last_item_y = subpass.parent_layout_last_item_y;
  ctx->layout_last_item_w = subpass.parent_layout_last_item_w;
  ctx->layout_last_item_h = subpass.parent_layout_last_item_h;
  mdgui_backend_set_alpha_mod(subpass.parent_alpha_mod);
  set_content_clip(ctx);
}

void mdgui_begin_frame(MDGUI_Context *ctx, const MDGUI_Input *input) {
  if (!ctx || !input)
    return;
  mdgui_bind_backend(&ctx->backend);
  mdgui_backend_set_alpha_mod(255);
  mdgui_backend_begin_frame();
  ctx->input = *input;
  ctx->combo_capture_seen_this_frame = false;
  const bool main_menu_modal = !ctx->open_main_menu_path.empty();
  if (ctx->custom_cursor_enabled && ctx->cursor_anim_count > 1) {
    const unsigned long long ticks = mdgui_backend_get_ticks_ms();
    const int anim_phase =
        (int)((ticks / 90ull) % (unsigned long long)ctx->cursor_anim_count);
    ctx->cursor_request_idx = anim_phase;
  } else if (ctx->custom_cursor_enabled) {
    ctx->cursor_request_idx = 0;
  } else {
    ctx->cursor_request_idx = ctx->cursors[9] ? 9 : 0;
  }

  // Bring the clicked window to front
  // before any drawing in this frame.
  if (ctx->input.mouse_pressed && !main_menu_modal) {
    const int clicked_idx =
        top_window_at_point(ctx, ctx->input.mouse_x, ctx->input.mouse_y, 3);
    if (clicked_idx >= 0 && clicked_idx < (int)ctx->windows.size()) {
      auto &clicked = ctx->windows[clicked_idx];
      if (!is_tiled_file_browser_window(ctx, clicked))
        clicked.z = ++ctx->z_counter;
    }
  }

  if (main_menu_modal) {
    ctx->dragging_window = -1;
    ctx->resizing_window = -1;
  }

  if (!ctx->input.mouse_down) {
    ctx->dragging_window = -1;
    ctx->resizing_window = -1;
    ctx->file_browser_scroll_dragging = false;
    ctx->active_text_input_drag_select = false;
    for (auto &w : ctx->windows) {
      w.text_scroll_dragging = false;
    }
  } else if (ctx->resizing_window >= 0 &&
             ctx->resizing_window < (int)ctx->windows.size() &&
             ctx->windows[ctx->resizing_window].closed) {
    ctx->resizing_window = -1;
  } else if (ctx->dragging_window >= 0 &&
             ctx->dragging_window < (int)ctx->windows.size()) {
    auto &w = ctx->windows[ctx->dragging_window];
    if (w.closed) {
      ctx->dragging_window = -1;
    } else {
      w.x = ctx->input.mouse_x - ctx->drag_off_x;
      w.y = ctx->input.mouse_y - ctx->drag_off_y;
      clamp_window_to_work_area(ctx, w);
    }
  }

  if (ctx->windows_locked) {
    ctx->dragging_window = -1;
    ctx->resizing_window = -1;
  }

  if (ctx->active_text_input_window >= 0 &&
      (ctx->active_text_input_window >= (int)ctx->windows.size() ||
       ctx->windows[ctx->active_text_input_window].closed)) {
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

  if (ctx->tile_manager_enabled) {
    tile_windows_internal(ctx);
  }

  ctx->current_window = -1;
  ctx->origin_x = 0;
  ctx->origin_y = 0;
  ctx->content_y = 0;
  ctx->in_menu_bar = false;
  ctx->in_menu = false;
  ctx->building_menu_index = -1;
  ctx->has_menu_bar = false;
  ctx->menu_bar_x = 0;
  ctx->menu_bar_y = 0;
  ctx->menu_bar_w = 0;
  ctx->menu_bar_h = 0;
  ctx->menu_defs.clear();
  ctx->menu_build_stack.clear();
  ctx->in_main_menu_bar = false;
  ctx->in_main_menu = false;
  ctx->building_main_menu_index = -1;
  ctx->main_menu_defs.clear();
  ctx->main_menu_build_stack.clear();
  ctx->content_req_right = 0;
  ctx->content_req_bottom = 0;
  ctx->font_stack.clear();
  ctx->window_has_nonlabel_widget = false;
  ctx->combo_overlay_pending = false;
  ctx->combo_overlay_window = -1;
  ctx->combo_overlay_items = nullptr;
}

void mdgui_end_frame(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  mdgui_bind_backend(&ctx->backend);
  while (!ctx->subpass_stack.empty()) {
    mdgui_end_subpass(ctx);
  }
  if (!ctx->open_main_menu_path.empty() && ctx->input.mouse_pressed) {
    bool in_bar = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                ctx->main_menu_bar_x, ctx->main_menu_bar_y,
                                ctx->main_menu_bar_w, ctx->main_menu_bar_h);
    const bool in_popup = point_in_menu_popup_chain(
        ctx->main_menu_defs, ctx->open_main_menu_path, MAIN_MENU_ITEM_H,
        ctx->input.mouse_x, ctx->input.mouse_y);
    if (!in_bar && !in_popup) {
      ctx->open_main_menu_path.clear();
      ctx->open_main_menu_item_path.clear();
      ctx->open_main_menu_index = -1;
    }
  }

  const unsigned long long now_ms = mdgui_backend_get_ticks_ms();
  prune_expired_toasts(ctx, now_ms);
  draw_toasts(ctx);
  for (auto &win : ctx->windows) {
    draw_cached_window_menu_overlay(ctx, win);
  }
  draw_open_main_menu_overlay(ctx);
  draw_status_bar(ctx);

  if (ctx->combo_capture_active && !ctx->combo_capture_seen_this_frame) {
    ctx->combo_capture_active = false;
    ctx->combo_capture_window = -1;
    ctx->combo_capture_x = 0;
    ctx->combo_capture_y = 0;
    ctx->combo_capture_w = 0;
    ctx->combo_capture_h = 0;
  }

  ctx->current_window = -1;
  ctx->in_menu_bar = false;
  ctx->in_menu = false;
  ctx->building_menu_index = -1;
  ctx->has_menu_bar = false;
  ctx->menu_defs.clear();
  ctx->menu_build_stack.clear();
  ctx->in_main_menu_bar = false;
  ctx->in_main_menu = false;
  ctx->building_main_menu_index = -1;
  ctx->main_menu_defs.clear();
  ctx->main_menu_build_stack.clear();

  // Apply final cursor decision once
  // per frame to avoid visible cursor
  // thrash.
  if (ctx->cursor_request_idx >= 0 && ctx->cursor_request_idx < 16 &&
      ctx->cursors[ctx->cursor_request_idx] &&
      ctx->current_cursor_idx != ctx->cursor_request_idx) {
    SDL_SetCursor(ctx->cursors[ctx->cursor_request_idx]);
    ctx->current_cursor_idx = ctx->cursor_request_idx;
  }
  mdgui_backend_end_frame();
}

int mdgui_begin_window_ex(MDGUI_Context *ctx, const char *title, int x, int y,
                          int w, int h, int flags) {
  if (!ctx)
    return 0;
  bool created = false;
  const int idx = find_or_create_window(ctx, title, x, y, w, h, &created);
  ctx->current_window = idx;
  auto &win = ctx->windows[idx];

  if (created && (flags & MDGUI_WINDOW_FLAG_CENTER_ON_FIRST_USE)) {
    center_window_rect_menu_aware(ctx, win);
    win.restored_x = win.x;
    win.restored_y = win.y;
    win.restored_w = win.w;
    win.restored_h = win.h;
  }

  if (flags & MDGUI_WINDOW_FLAG_EXCLUDE_FROM_TILING) {
    win.tile_excluded = true;
  }

  if (win.closed) {
    ctx->current_window = -1;
    return 0;
  }
  if (is_occluded_by_higher_maximized_window(ctx, idx)) {
    ctx->current_window = -1;
    return 0;
  }
  win.is_message_box = false;

  const int screen_w = get_logical_render_w(ctx);

  if (win.is_maximized) {
    const int top = get_work_area_top(ctx);
    int bottom = get_work_area_bottom(ctx);
    if (bottom < top)
      bottom = top;
    win.x = 0;
    win.y = top;
    win.w = screen_w;
    win.h = std::max(1, bottom - top);
  }

  const int top_idx =
      top_window_at_point(ctx, ctx->input.mouse_x, ctx->input.mouse_y, 3);
  const int top_here = (top_idx == idx);
  const bool no_chrome = (flags & MDGUI_WINDOW_FLAG_NO_CHROME) != 0;
  win.no_chrome = no_chrome;
  const bool chrome_input_allowed = ctx->open_main_menu_path.empty();
  const bool move_resize_allowed =
      !no_chrome && chrome_input_allowed && !ctx->windows_locked;
  const bool can_maximize = !no_chrome && !win.disallow_maximize;
  const int title_h = no_chrome ? 0 : 12;
  if (win.user_min_w < 50)
    win.user_min_w = 50;
  if (win.user_min_h < 30)
    win.user_min_h = 30;
  if (win.min_w < win.user_min_w)
    win.min_w = win.user_min_w;
  if (win.min_h < win.user_min_h)
    win.min_h = win.user_min_h;
  clamp_window_to_work_area(ctx, win);

  // Edge detection for hover AND
  // interaction
  const int margin = 3;
  int edge_mask = 0;
  if (can_maximize && !win.is_maximized) {
    if (ctx->input.mouse_x >= win.x - margin &&
        ctx->input.mouse_x <= win.x + margin)
      edge_mask |= 1; // Left
    if (ctx->input.mouse_x >= win.x + win.w - margin &&
        ctx->input.mouse_x <= win.x + win.w + margin)
      edge_mask |= 2; // Right
    if (ctx->input.mouse_y >= win.y - margin &&
        ctx->input.mouse_y <= win.y + margin)
      edge_mask |= 4; // Top
    if (ctx->input.mouse_y >= win.y + win.h - margin &&
        ctx->input.mouse_y <= win.y + win.h + margin)
      edge_mask |= 8; // Bottom
  }
  win.last_edge_mask = edge_mask;

  // Cursor feedback (Hover OR active
  // resizing)
  if (move_resize_allowed &&
      ((edge_mask != 0 && top_here && ctx->resizing_window == -1) ||
       (ctx->resizing_window == idx))) {
    int mask =
        (ctx->resizing_window == idx) ? ctx->resize_edge_mask : edge_mask;
    int cursor_idx = 0;
    if ((mask & 1 && mask & 4) || (mask & 2 && mask & 8))
      cursor_idx = 5; // NWSE
    else if ((mask & 2 && mask & 4) || (mask & 1 && mask & 8))
      cursor_idx = 6; // NESW
    else if (mask & 1 || mask & 2)
      cursor_idx = 7; // EW
    else if (mask & 4 || mask & 8)
      cursor_idx = 8; // NS
    ctx->cursor_request_idx = cursor_idx;
    if (ctx->cursors[cursor_idx] && ctx->current_cursor_idx != cursor_idx) {
      SDL_SetCursor(ctx->cursors[cursor_idx]);
      ctx->current_cursor_idx = cursor_idx;
    }
  }

  // Handle Close and Maximize button
  // interaction
  const int btn_w = 9;
  const int btn_h = 9;
  const int close_x = win.x + win.w - btn_w - 1;
  const int close_y = win.y + 1;
  const int max_x = close_x - btn_w - 2;
  const int max_y = win.y + 1;
  const int title_text_w =
      (title && mdgui_fonts[1]) ? mdgui_fonts[1]->measureTextWidth(title) : 0;
  int chrome_min_w = no_chrome ? 1 : 26;
  if (!no_chrome && title_text_w > 0) {
    const int title_need = 5 + title_text_w + 6 + btn_w + 2 + btn_w + 2;
    if (title_need > chrome_min_w)
      chrome_min_w = title_need;
  }
  if (chrome_min_w > win.min_w)
    win.min_w = chrome_min_w;

  if (!no_chrome && chrome_input_allowed && ctx->input.mouse_pressed && top_here) {
    // If a combo popup is open in
    // this window, let combo logic
    // consume this click first; don't
    // treat it as titlebar/chrome
    // interaction.
    if (win.open_combo_id != -1) {
      if (!is_tiled_file_browser_window(ctx, win))
        win.z = ++ctx->z_counter;
    } else {
      // Check Close button
      if (point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, close_x,
                        close_y, btn_w, btn_h)) {
        win.closed = true;
        return 0;
      }
      // Check Maximize button
      if (move_resize_allowed && can_maximize &&
          point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, max_x, max_y,
                        btn_w, btn_h)) {
        win.fixed_rect = false;
        win.is_maximized = !win.is_maximized;
        if (win.is_maximized) {
          win.restored_x = win.x;
          win.restored_y = win.y;
          win.restored_w = win.w;
          win.restored_h = win.h;
        } else {
          win.x = win.restored_x;
          win.y = win.restored_y;
          win.w = win.restored_w;
          win.h = win.restored_h;
        }
      }

      if (!is_tiled_file_browser_window(ctx, win))
        win.z = ++ctx->z_counter;

      if (move_resize_allowed && edge_mask != 0) {
        win.fixed_rect = false;
        ctx->resizing_window = idx;
        ctx->resize_edge_mask = edge_mask;
        ctx->m_start_x = ctx->input.mouse_x;
        ctx->m_start_y = ctx->input.mouse_y;
        ctx->w_start_x = win.x;
        ctx->w_start_y = win.y;
        ctx->w_start_w = win.w;
        ctx->w_start_h = win.h;
      } else if (move_resize_allowed && !win.is_maximized &&
                 point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, win.x,
                               win.y, win.w, title_h)) {
        win.fixed_rect = false;
        ctx->dragging_window = idx;
        ctx->drag_off_x = ctx->input.mouse_x - win.x;
        ctx->drag_off_y = ctx->input.mouse_y - win.y;
      }
    }
  }

  // Handle active resizing
  if (ctx->resizing_window == idx) {
    if (!ctx->input.mouse_down) {
      ctx->resizing_window = -1;
    } else {
      int dx = ctx->input.mouse_x - ctx->m_start_x;
      int dy = ctx->input.mouse_y - ctx->m_start_y;

      if (ctx->resize_edge_mask & 1) { // Left
        int new_w = ctx->w_start_w - dx;
        if (new_w < win.min_w)
          dx = ctx->w_start_w - win.min_w;
        win.x = ctx->w_start_x + dx;
        win.w = ctx->w_start_w - dx;
      } else if (ctx->resize_edge_mask & 2) { // Right
        win.w = ctx->w_start_w + dx;
        if (win.w < win.min_w)
          win.w = win.min_w;
      }

      if (ctx->resize_edge_mask & 4) { // Top
        int new_h = ctx->w_start_h - dy;
        if (new_h < win.min_h)
          dy = ctx->w_start_h - win.min_h;
        win.y = ctx->w_start_y + dy;
        win.h = ctx->w_start_h - dy;
      } else if (ctx->resize_edge_mask & 8) { // Bottom
        win.h = ctx->w_start_h + dy;
        if (win.h < win.min_h)
          win.h = win.min_h;
      }
      clamp_window_to_work_area(ctx, win);
    }
  }

  const int focused = (top_window_at_point(ctx, ctx->input.mouse_x,
                                           ctx->input.mouse_y) == idx) ||
                      (win.z == ctx->z_counter);
  (void)focused;
  const bool transparent_bg = (flags & MDGUI_WINDOW_FLAG_TRANSPARENT_BG) != 0;

  mdgui_backend_set_alpha_mod(win.alpha);

  if (no_chrome) {
    if (!transparent_bg) {
      mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, win.x, win.y, win.w, win.h);
    }
  } else {
    mdgui_fill_rect_idx(nullptr, CLR_BOX_TITLE, win.x, win.y, win.w, title_h);
    if (!transparent_bg) {
      mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, win.x, win.y + title_h, win.w,
                          win.h - title_h - 2);
      mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, win.x, win.y + win.h - 2, win.w,
                          2);
    }

    // Draw 3D-style Close button
    mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, close_x, close_y, btn_w,
                        btn_h);
    // Draw X as two diagonal lines with margin
    mdgui_draw_line_idx(nullptr, CLR_TEXT_LIGHT, close_x + 2, close_y + 2,
                        close_x + 6, close_y + 6);
    mdgui_draw_line_idx(nullptr, CLR_TEXT_LIGHT, close_x + 6, close_y + 2,
                        close_x + 2, close_y + 6);

    // Draw 3D-style Maximize button when allowed for this window.
    if (can_maximize) {
      mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, max_x, max_y, btn_w,
                          btn_h);
      mdgui_fill_rect_idx(nullptr, CLR_TEXT_LIGHT, max_x + 2, max_y + 2,
                          btn_w - 4, btn_h - 4);
      mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, max_x + 3, max_y + 4,
                          btn_w - 6, btn_h - 7);
    }

    if (title && mdgui_fonts[1]) {
      const int title_text_y = win.y + ((title_h - 8) / 2);
      mdgui_fonts[1]->drawText(title, nullptr, win.x + 5, title_text_y,
                               CLR_TEXT_LIGHT);
    }

    // Draw border after fills so it renders on top.
    mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, win.x - 1, win.y - 1,
                         win.x + win.w + 1);
    mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, win.x - 1, win.y + win.h - 1,
                         win.x + win.w + 1);
    mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, win.x - 1, win.y - 1,
                         win.y + win.h);
    mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, win.x + win.w, win.y - 1,
                         win.y + win.h);
  }

  ctx->origin_x = no_chrome ? win.x : (win.x + 2);
  ctx->origin_y = no_chrome ? win.y : (win.y + title_h + 2);
  ctx->content_y = ctx->origin_y + ctx->style.content_pad_y;
  ctx->menu_index = 0;
  ctx->menu_next_x = ctx->origin_x;
  ctx->in_menu_bar = false;
  ctx->in_menu = false;
  ctx->current_menu_index = -1;
  ctx->current_menu_item = 0;
  ctx->building_menu_index = -1;
  ctx->has_menu_bar = false;
  ctx->menu_bar_x = 0;
  ctx->menu_bar_y = 0;
  ctx->menu_bar_w = 0;
  ctx->menu_bar_h = 0;
  ctx->menu_defs.clear();
  ctx->menu_build_stack.clear();
  ctx->content_req_right = no_chrome ? (win.x + 1) : (win.x + 6);
  ctx->content_req_bottom = no_chrome ? (win.y + 1) : (win.y + title_h + 6);
  ctx->layout_same_line = false;
  ctx->layout_has_last_item = false;
  ctx->layout_last_item_x = 0;
  ctx->layout_last_item_y = 0;
  ctx->layout_last_item_w = 0;
  ctx->layout_last_item_h = 0;
  ctx->layout_columns_active = false;
  ctx->layout_columns_count = 0;
  ctx->layout_columns_index = 0;
  ctx->layout_columns_start_x = 0;
  ctx->layout_columns_start_y = ctx->content_y;
  ctx->layout_columns_width = 0;
  ctx->layout_columns_max_bottom = ctx->content_y;
  ctx->layout_columns_bottoms.clear();
  ctx->layout_indent = ctx->style.content_pad_x;
  ctx->indent_stack.clear();
  ctx->window_has_nonlabel_widget = false;
  note_content_bounds(ctx, win.x + chrome_min_w,
                      no_chrome ? (win.y + 1) : (win.y + title_h + 2));

  set_content_clip(ctx);
  return 1;
}

int mdgui_begin_window(MDGUI_Context *ctx, const char *title, int x, int y,
                       int w, int h) {
  return mdgui_begin_window_ex(ctx, title, x, y, w, h, MDGUI_WINDOW_FLAG_NONE);
}

void mdgui_end_window(MDGUI_Context *ctx) {
  if (!ctx)
    return;

  if (!ctx->nested_render_stack.empty()) {
    const auto nested = ctx->nested_render_stack.back();
    ctx->nested_render_stack.pop_back();
    ctx->origin_x = nested.parent_origin_x;
    ctx->origin_y = nested.parent_origin_y;
    ctx->content_y = nested.parent_content_y_after;
    ctx->content_req_right =
        std::max(ctx->content_req_right, nested.parent_content_req_right);
    ctx->content_req_bottom =
        std::max(ctx->content_req_bottom, nested.parent_content_req_bottom);
    ctx->window_has_nonlabel_widget = ctx->window_has_nonlabel_widget ||
                                      nested.parent_window_has_nonlabel_widget;
    ctx->layout_indent = nested.parent_layout_indent;
    if (ctx->indent_stack.size() > nested.parent_indent_stack_size) {
      ctx->indent_stack.resize(nested.parent_indent_stack_size);
    }
    mdgui_backend_set_alpha_mod(nested.parent_alpha_mod);
    set_content_clip(ctx);
    return;
  }

  auto &win = ctx->windows[ctx->current_window];
  mdgui_backend_set_clip_rect(0, 0, 0, 0, 0);

  if (!win.open_menu_path.empty() && ctx->input.mouse_pressed) {
    bool in_menu_bar = false;

    if (ctx->has_menu_bar) {
      in_menu_bar =
          point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, ctx->menu_bar_x,
                        ctx->menu_bar_y, ctx->menu_bar_w, ctx->menu_bar_h);
    }

    const bool in_menu_popup = point_in_menu_popup_chain(
        ctx->menu_defs, win.open_menu_path, ctx->current_menu_h,
        ctx->input.mouse_x, ctx->input.mouse_y);

    if (!in_menu_bar && !in_menu_popup) {
      clear_window_menu_path(win);
    }
  }

  draw_open_menu_overlay(ctx);
  win.menu_overlay_defs.clear();
  win.menu_overlay_defs.reserve(ctx->menu_defs.size());
  for (const auto &def : ctx->menu_defs) {
    MDGUI_Window::MenuOverlayDef cached{};
    cached.x = def.x;
    cached.y = def.y;
    cached.w = def.w;
    cached.parent_menu_index = def.parent_menu_index;
    cached.parent_item_index = def.parent_item_index;
    cached.items.reserve(def.items.size());
    for (const auto &item : def.items) {
      MDGUI_Window::MenuOverlayDef::ItemDef cached_item{};
      cached_item.text = item.text;
      cached_item.child_menu_index = item.child_menu_index;
      cached_item.is_separator = item.is_separator;
      cached.items.push_back(std::move(cached_item));
    }
    win.menu_overlay_defs.push_back(std::move(cached));
  }
  win.menu_overlay_item_h = ctx->current_menu_h;

  if (ctx->combo_overlay_pending &&
      ctx->combo_overlay_window == ctx->current_window &&
      ctx->combo_overlay_items && ctx->combo_overlay_item_count > 0) {
    const int ix = ctx->combo_overlay_x;
    const int popup_y = ctx->combo_overlay_y;
    const int w = ctx->combo_overlay_w;
    const int item_h = ctx->combo_overlay_item_h;
    const int item_count = ctx->combo_overlay_item_count;
    const int popup_h = item_count * item_h;
    ctx->combo_capture_active = true;
    ctx->combo_capture_seen_this_frame = true;
    ctx->combo_capture_window = ctx->current_window;
    ctx->combo_capture_x = ix;
    ctx->combo_capture_y = popup_y;
    ctx->combo_capture_w = w;
    ctx->combo_capture_h = popup_h;
    mdgui_draw_frame_idx(nullptr, CLR_TEXT_DARK, ix - 1, popup_y - 1,
                         ix + w + 1, popup_y + popup_h + 1);
    mdgui_fill_rect_idx(nullptr, CLR_MENU_BG, ix, popup_y, w, popup_h);
    for (int i = 0; i < item_count; i++) {
      const int ry = popup_y + (i * item_h);
      const int row_hover = point_in_rect(
          ctx->input.mouse_x, ctx->input.mouse_y, ix, ry, w, item_h);
      if (i == ctx->combo_overlay_selected || row_hover)
        mdgui_fill_rect_idx(nullptr, CLR_ACCENT, ix, ry, w, item_h);
      if (mdgui_fonts[1] && ctx->combo_overlay_items[i]) {
        const int text_max_w = std::max(1, w - 4);
        const std::string row_text =
            ellipsize_text_to_width(ctx->combo_overlay_items[i], text_max_w);
        if (set_widget_clip_intersect_content(ctx, ix + 1, ry,
                                              std::max(1, w - 2), item_h)) {
          mdgui_fonts[1]->drawText(row_text.c_str(), nullptr, ix + 2, ry + 1,
                                   CLR_MENU_TEXT);
          set_content_clip(ctx);
        }
      }
    }
    ctx->combo_overlay_pending = false;
    ctx->combo_overlay_window = -1;
    ctx->combo_overlay_items = nullptr;
  }

  // All windows can scroll when
  // content exceeds viewport.
  {
    const int viewport_top = ctx->origin_y;
    const int viewport_h = (win.y + win.h - 4) - viewport_top;
    int total_h = ctx->content_y - ctx->origin_y;
    if (total_h < 0)
      total_h = 0;
    const int max_scroll =
        (viewport_h > 0 && total_h > viewport_h) ? (total_h - viewport_h) : 0;

    const int content_x = win.x + 2;
    const int content_w = win.w - 4;
    const int over_content = point_in_rect(
        ctx->input.mouse_x, ctx->input.mouse_y, content_x, viewport_top,
        content_w, (viewport_h > 0) ? viewport_h : 1);
    if (over_content && ctx->input.mouse_wheel != 0 && max_scroll > 0 &&
        !win.text_scroll_dragging) {
      const int prev = win.text_scroll;
      win.text_scroll -= ctx->input.mouse_wheel * 12;
      if (win.text_scroll < 0)
        win.text_scroll = 0;
      if (win.text_scroll > max_scroll)
        win.text_scroll = max_scroll;
      if (win.text_scroll == prev) {
        // At edge: keep stable
        // (prevents visual jitter
        // while wheel continues).
        win.text_scroll = prev;
      }
    }
    if (win.text_scroll < 0)
      win.text_scroll = 0;
    if (win.text_scroll > max_scroll)
      win.text_scroll = max_scroll;
    win.scrollbar_overflow_active = (max_scroll > 0);

    const bool show_scrollbar = (viewport_h > 8) && !win.is_message_box &&
                                win.scrollbar_visible && (max_scroll > 0);
    if (show_scrollbar) {
      const int sb_w = 8;
      const int sb_x = win.x + win.w - sb_w - 2;
      const int sb_y = viewport_top;
      mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, sb_x, sb_y, sb_w,
                          viewport_h);
      int thumb_h = (viewport_h * viewport_h) / (total_h > 0 ? total_h : 1);
      if (thumb_h < 10)
        thumb_h = 10;
      if (thumb_h > viewport_h)
        thumb_h = viewport_h;
      const int travel = viewport_h - thumb_h;
      const int thumb_y = sb_y + ((travel > 0) ? ((win.text_scroll * travel) / max_scroll) : 0);
      mdgui_fill_rect_idx(nullptr, CLR_ACCENT, sb_x + 1, thumb_y, sb_w - 2,
                          thumb_h);
      const int over_scrollbar = point_in_rect(
          ctx->input.mouse_x, ctx->input.mouse_y, sb_x, sb_y, sb_w, viewport_h);
      if (ctx->input.mouse_pressed && over_scrollbar &&
          is_current_window_topmost(ctx)) {
        if (ctx->input.mouse_y >= thumb_y &&
            ctx->input.mouse_y <= thumb_y + thumb_h) {
          win.text_scroll_dragging = true;
          win.text_scroll_drag_offset = ctx->input.mouse_y - thumb_y;
        } else if (ctx->input.mouse_y < thumb_y) {
          win.text_scroll -= 12 * 3;
        } else {
          win.text_scroll += 12 * 3;
        }
      }
      if (win.text_scroll_dragging && ctx->input.mouse_down) {
        int desired_thumb_y = ctx->input.mouse_y - win.text_scroll_drag_offset;
        if (desired_thumb_y < sb_y)
          desired_thumb_y = sb_y;
        if (desired_thumb_y > sb_y + travel)
          desired_thumb_y = sb_y + travel;
        win.text_scroll =
            ((desired_thumb_y - sb_y) * max_scroll) / (travel > 0 ? travel : 1);
      }
      if (win.text_scroll < 0)
        win.text_scroll = 0;
      if (win.text_scroll > max_scroll)
        win.text_scroll = max_scroll;
      // Scrollbar is chrome anchored
      // to the current window edge
      // and should not participate in
      // content-driven minimum width
      // calculations.
    }
  }

  // Dynamic minimum size based on hard content
  const int measured_min_w = (ctx->content_req_right - win.x) + 4;
  if (ctx->window_has_nonlabel_widget) {
    win.min_w = std::max(win.user_min_w, measured_min_w);
  } else {
    win.min_w = win.user_min_w;
  }

  win.min_h = std::max(win.user_min_h, win.chrome_min_h);

  if (!win.fixed_rect) {
    if (win.w < win.min_w)
      win.w = win.min_w;
    if (win.h < win.min_h)
      win.h = win.min_h;
  }

  ctx->current_window = -1;
  ctx->in_menu_bar = false;
  ctx->in_menu = false;
  ctx->building_menu_index = -1;
  ctx->has_menu_bar = false;
  ctx->menu_defs.clear();
  ctx->menu_build_stack.clear();
  mdgui_backend_set_alpha_mod(255);
}

static int clamp_text_cursor(int cursor, int len) {
  if (cursor < 0)
    return 0;
  if (cursor > len)
    return len;
  return cursor;
}

static int text_cursor_from_pixel_x(const char *buffer, int len,
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

static int layout_current_column_base_x(const MDGUI_Context *ctx) {
  if (!ctx)
    return 0;
  if (!ctx->layout_columns_active || ctx->layout_columns_count < 1)
    return ctx->layout_indent;
  return ctx->layout_columns_start_x +
         ctx->layout_columns_index * (ctx->layout_columns_width +
                                      ctx->style.spacing_x);
}

static void layout_prepare_widget(MDGUI_Context *ctx, int *out_local_x,
                                  int *out_logical_y) {
  if (!ctx) {
    if (out_local_x)
      *out_local_x = 0;
    if (out_logical_y)
      *out_logical_y = 0;
    return;
  }
  int local_x = layout_current_column_base_x(ctx);
  int logical_y = ctx->content_y;
  if (ctx->layout_columns_active &&
      ctx->layout_columns_index < (int)ctx->layout_columns_bottoms.size()) {
    logical_y = ctx->layout_columns_bottoms[ctx->layout_columns_index];
    ctx->content_y = logical_y;
  }
  if (ctx->layout_same_line && ctx->layout_has_last_item &&
      !ctx->layout_columns_active) {
    local_x = ctx->layout_last_item_x + ctx->layout_last_item_w +
              ctx->style.spacing_x;
    logical_y = ctx->layout_last_item_y;
  }
  if (out_local_x)
    *out_local_x = local_x;
  if (out_logical_y)
    *out_logical_y = logical_y;
}

static int layout_resolve_width(MDGUI_Context *ctx, int local_x, int requested_w,
                                int min_w) {
  int resolved = resolve_dynamic_width(ctx, local_x, requested_w, min_w);
  if (ctx && ctx->layout_columns_active && requested_w <= 0) {
    if (resolved > ctx->layout_columns_width)
      resolved = ctx->layout_columns_width;
  }
  if (resolved < min_w)
    resolved = min_w;
  return resolved;
}

static void layout_commit_widget(MDGUI_Context *ctx, int local_x, int logical_y,
                                 int w, int h, int spacing_y) {
  if (!ctx)
    return;
  if (spacing_y < 0)
    spacing_y = 0;
  ctx->layout_last_item_x = local_x;
  ctx->layout_last_item_y = logical_y;
  ctx->layout_last_item_w = w;
  ctx->layout_last_item_h = h;
  ctx->layout_has_last_item = true;
  ctx->layout_same_line = false;

  const int bottom = logical_y + h + spacing_y;
  if (ctx->layout_columns_active &&
      ctx->layout_columns_index < (int)ctx->layout_columns_bottoms.size()) {
    int &col_bottom = ctx->layout_columns_bottoms[ctx->layout_columns_index];
    if (bottom > col_bottom)
      col_bottom = bottom;
    if (col_bottom > ctx->layout_columns_max_bottom)
      ctx->layout_columns_max_bottom = col_bottom;
    ctx->content_y = col_bottom;
  } else {
    ctx->content_y = bottom;
  }
}

