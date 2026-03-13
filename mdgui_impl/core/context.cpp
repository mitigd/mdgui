#include "internal.hpp"

extern "C" {
MDGUI_Context *mdgui_create(void *sdl_renderer) {
  MDGUI_RenderBackend backend = {};
  mdgui_make_sdl_backend(&backend, sdl_renderer);
  return mdgui_create_with_backend(&backend);
}

MDGUI_Context *mdgui_create_with_backend(const MDGUI_RenderBackend *backend) {
  auto *ctx = new MDGUI_Context{};
  memset(&ctx->render.backend, 0, sizeof(ctx->render.backend));
  memset(&ctx->render.input, 0, sizeof(ctx->render.input));
  ctx->window.z_counter = 1;
  ctx->window.dragging_window = -1;
  ctx->window.drag_off_x = 0;
  ctx->window.drag_off_y = 0;
  ctx->window.current_window = -1;
  ctx->window.origin_x = 0;
  ctx->window.origin_y = 0;
  ctx->window.content_y = 0;
  ctx->menus.menu_index = 0;
  ctx->menus.menu_next_x = 0;
  ctx->menus.in_menu_bar = false;
  ctx->menus.in_menu = false;
  ctx->menus.current_menu_index = -1;
  ctx->menus.building_menu_index = -1;
  ctx->menus.current_menu_x = 0;
  ctx->menus.current_menu_y = 0;
  ctx->menus.current_menu_w = 150;
  ctx->menus.current_menu_h = 10;
  ctx->menus.current_menu_item = 0;
  ctx->menus.has_menu_bar = false;
  ctx->menus.menu_bar_x = 0;
  ctx->menus.menu_bar_y = 0;
  ctx->menus.menu_bar_w = 0;
  ctx->menus.menu_bar_h = 0;
  ctx->menus.menu_build_stack.clear();
  ctx->menus.main_menu_index = 0;
  ctx->menus.main_menu_next_x = 0;
  ctx->menus.in_main_menu_bar = false;
  ctx->menus.in_main_menu = false;
  ctx->menus.open_main_menu_index = -1;
  ctx->menus.building_main_menu_index = -1;
  ctx->menus.main_menu_build_stack.clear();
  ctx->menus.open_main_menu_path.clear();
  ctx->menus.open_main_menu_item_path.clear();
  ctx->menus.main_menu_bar_x = 0;
  ctx->menus.main_menu_bar_y = 0;
  ctx->menus.main_menu_bar_w = 0;
  ctx->menus.main_menu_bar_h = 0;
  ctx->menus.status_bar_visible = false;
  ctx->menus.status_bar_h = STATUS_BAR_DEFAULT_H;
  ctx->menus.status_bar_text.clear();

  ctx->window.resizing_window = -1;
  ctx->window.resize_edge_mask = 0;
  ctx->window.m_start_x = 0;
  ctx->window.m_start_y = 0;
  ctx->window.w_start_x = 0;
  ctx->window.w_start_y = 0;
  ctx->window.w_start_w = 0;
  ctx->window.w_start_h = 0;

  for (int i = 0; i < 16; i++)
    ctx->cursor.cursors[i] = nullptr;
  // Only SDL system cursors are used
  // in release-safe builds.
  ctx->cursor.cursors[0] = SDL_CreateSystemCursor((SDL_SystemCursor)0);
  ctx->cursor.cursor_anim_count = ctx->cursor.cursors[0] ? 1 : 0;
  ctx->cursor.cursors[5] = SDL_CreateSystemCursor((SDL_SystemCursor)5); // NWSE
  ctx->cursor.cursors[6] = SDL_CreateSystemCursor((SDL_SystemCursor)6); // NESW
  ctx->cursor.cursors[7] = SDL_CreateSystemCursor((SDL_SystemCursor)7); // EW
  ctx->cursor.cursors[8] = SDL_CreateSystemCursor((SDL_SystemCursor)8); // NS
  ctx->cursor.cursors[9] = SDL_CreateSystemCursor((SDL_SystemCursor)0); // default
                                                                 // arrow
  ctx->cursor.custom_cursor_enabled = false;
  ctx->cursor.cursor_request_idx = 9;
  ctx->cursor.current_cursor_idx = -1;
  ctx->layout.content_req_right = 0;
  ctx->layout.content_req_bottom = 0;
  ctx->layout.style.spacing_x = 6;
  ctx->layout.style.spacing_y = 4;
  ctx->layout.style.section_spacing_y = 6;
  ctx->layout.style.indent_step = 8;
  ctx->layout.style.label_h = 12;
  ctx->layout.style.content_pad_x = 8;
  ctx->layout.style.content_pad_y = 6;
  ctx->layout.same_line = false;
  ctx->layout.has_last_item = false;
  ctx->layout.last_item_x = 0;
  ctx->layout.last_item_y = 0;
  ctx->layout.last_item_w = 0;
  ctx->layout.last_item_h = 0;
  ctx->layout.columns_active = false;
  ctx->layout.columns_count = 0;
  ctx->layout.columns_index = 0;
  ctx->layout.columns_start_x = 0;
  ctx->layout.columns_start_y = 0;
  ctx->layout.columns_width = 0;
  ctx->layout.columns_max_bottom = 0;
  ctx->layout.columns_bottoms.clear();
  ctx->layout.default_font = nullptr;
  ctx->layout.font_stack.clear();
  ctx->browser.path_font = nullptr;
  ctx->demo.font_labels_storage.clear();
  ctx->demo.font_labels.clear();
  ctx->demo.fonts.clear();
  ctx->demo.font_index = 0;
  ctx->layout.window_has_nonlabel_widget = false;
  ctx->window.windows_locked = false;
  ctx->window.tile_manager_enabled = false;
  ctx->window.default_window_alpha = 255;
  ctx->interaction.combo_overlay_pending = false;
  ctx->interaction.combo_overlay_window = -1;
  ctx->interaction.combo_overlay_x = 0;
  ctx->interaction.combo_overlay_y = 0;
  ctx->interaction.combo_overlay_w = 0;
  ctx->interaction.combo_overlay_item_h = 0;
  ctx->interaction.combo_overlay_item_count = 0;
  ctx->interaction.combo_overlay_selected = -1;
  ctx->interaction.combo_overlay_items = nullptr;
  ctx->interaction.combo_capture_active = false;
  ctx->interaction.combo_capture_seen_this_frame = false;
  ctx->interaction.combo_capture_window = -1;
  ctx->interaction.combo_capture_x = 0;
  ctx->interaction.combo_capture_y = 0;
  ctx->interaction.combo_capture_w = 0;
  ctx->interaction.combo_capture_h = 0;
  ctx->interaction.active_text_input_window = -1;
  ctx->interaction.active_text_input_id = 0;
  ctx->interaction.active_text_input_cursor = 0;
  ctx->interaction.active_text_input_sel_anchor = 0;
  ctx->interaction.active_text_input_sel_start = 0;
  ctx->interaction.active_text_input_sel_end = 0;
  ctx->interaction.active_text_input_drag_select = false;
  ctx->interaction.active_text_input_multiline = false;
  ctx->interaction.active_text_input_scroll_y = 0;
  ctx->browser.open = false;
  ctx->browser.select_folders = false;
  ctx->browser.cwd = ".";
  ctx->browser.selected = -1;
  ctx->browser.scroll = 0;
  ctx->browser.scroll_dragging = false;
  ctx->browser.scroll_drag_offset = 0;
  ctx->browser.last_click_idx = -1;
  ctx->browser.last_click_ticks = 0;
  ctx->browser.result.clear();
  ctx->browser.drives = enumerate_file_browser_drives();
  ctx->browser.last_drive_scan_ticks = 0;
  ctx->browser.last_selected_path.clear();
  ctx->browser.restore_scroll_pending = false;
  ctx->browser.center_pending = false;
  ctx->browser.open_x = -1;
  ctx->browser.open_y = -1;
  ctx->browser.path_subpass_enabled = false;
  ctx->layout.indent = ctx->layout.style.content_pad_x;
  ctx->layout.indent_stack.clear();

  mdgui_set_backend(ctx, backend);
  for (int i = 0; i < 10; i++) {
    if (!mdgui_fonts[i]) {
      mdgui_fonts[i] = mdgui_font_create_builtin(1);
    }
  }
  ctx->layout.default_font = fallback_font();
  return ctx;
}

void mdgui_destroy(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  for (int i = 0; i < 16; i++) {
    if (ctx->cursor.cursors[i]) {
      SDL_DestroyCursor(ctx->cursor.cursors[i]);
      ctx->cursor.cursors[i] = nullptr;
    }
  }
  delete ctx;
}

void mdgui_set_default_font(MDGUI_Context *ctx, MDGUI_Font *font) {
  if (!ctx)
    return;
  ctx->layout.default_font = font ? font : fallback_font();
}

MDGUI_Font *mdgui_get_default_font(MDGUI_Context *ctx) {
  if (!ctx)
    return fallback_font();
  return ctx->layout.default_font ? ctx->layout.default_font : fallback_font();
}

void mdgui_push_font(MDGUI_Context *ctx, MDGUI_Font *font) {
  if (!ctx)
    return;
  ctx->layout.font_stack.push_back(font ? font : fallback_font());
}

void mdgui_pop_font(MDGUI_Context *ctx) {
  if (!ctx || ctx->layout.font_stack.empty())
    return;
  ctx->layout.font_stack.pop_back();
}

void mdgui_set_demo_window_font_options(MDGUI_Context *ctx,
                                        const char **labels,
                                        MDGUI_Font **fonts, int font_count,
                                        int selected_index) {
  if (!ctx)
    return;
  ctx->demo.font_labels_storage.clear();
  ctx->demo.font_labels.clear();
  ctx->demo.fonts.clear();
  if (!labels || !fonts || font_count <= 0) {
    ctx->demo.font_index = 0;
    return;
  }

  ctx->demo.font_labels_storage.reserve((size_t)font_count);
  ctx->demo.font_labels.reserve((size_t)font_count);
  ctx->demo.fonts.reserve((size_t)font_count);
  for (int i = 0; i < font_count; ++i) {
    ctx->demo.font_labels_storage.emplace_back(labels[i] ? labels[i] : "");
    ctx->demo.fonts.push_back(fonts[i]);
  }
  for (const std::string &label : ctx->demo.font_labels_storage) {
    ctx->demo.font_labels.push_back(label.c_str());
  }

  if (selected_index < 0 || selected_index >= (int)ctx->demo.fonts.size())
    selected_index = 0;
  ctx->demo.font_index = selected_index;
}

void mdgui_set_demo_window_font_index(MDGUI_Context *ctx, int index) {
  if (!ctx)
    return;
  if (index < 0 || index >= (int)ctx->demo.fonts.size())
    return;
  ctx->demo.font_index = index;
}

int mdgui_get_demo_window_font_index(MDGUI_Context *ctx) {
  if (!ctx)
    return 0;
  return ctx->demo.font_index;
}

void mdgui_set_file_browser_path_font(MDGUI_Context *ctx, MDGUI_Font *font) {
  if (!ctx)
    return;
  ctx->browser.path_font = font;
}

MDGUI_Font *mdgui_get_file_browser_path_font(MDGUI_Context *ctx) {
  if (!ctx)
    return nullptr;
  return ctx->browser.path_font;
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
  ctx->browser.path_subpass_enabled = (enabled != 0);
}

int mdgui_is_file_browser_path_subpass_enabled(MDGUI_Context *ctx) {
  if (!ctx)
    return 0;
  return ctx->browser.path_subpass_enabled ? 1 : 0;
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
    ctx->render.backend = *backend;
    mdgui_bind_backend(&ctx->render.backend);
  } else {
    memset(&ctx->render.backend, 0, sizeof(ctx->render.backend));
    mdgui_bind_backend(nullptr);
  }
}

int mdgui_begin_subpass(MDGUI_Context *ctx, const char *id, int x, int y, int w,
                        int h, float scale, int *out_x, int *out_y,
                        int *out_w, int *out_h) {
  if (!ctx || !id || ctx->window.current_window < 0 || scale <= 0.0f ||
      !ctx->render.backend.begin_subpass || !ctx->render.backend.end_subpass) {
    return 0;
  }
  if (!ctx->window.subpass_stack.empty())
    return 0;

  int local_x = 0;
  int logical_y = 0;
  if (x == 0 && y == 0 && w == 0 && h == 0) {
    layout_prepare_widget(ctx, &local_x, &logical_y);
  } else {
    local_x = x + ctx->layout.indent;
    logical_y = ctx->window.content_y + y;
  }

  if (w <= 0)
    w = layout_resolve_width(ctx, local_x, w, 1);
  if (h <= 0) {
    int viewport_bottom = 0;
    if (const auto *subpass = current_subpass(ctx)) {
      viewport_bottom = subpass->local_h;
    } else {
      const auto &win = ctx->window.windows[ctx->window.current_window];
      viewport_bottom = win.y + win.h - 4 + win.text_scroll;
    }
    int avail_h = viewport_bottom - logical_y;
    if (h < 0)
      avail_h += h;
    h = (avail_h > 0) ? avail_h : 1;
  }

  const int abs_x = ctx->window.origin_x + local_x;
  const int abs_y = logical_y - ctx->window.windows[ctx->window.current_window].text_scroll;
  int local_w = 0;
  int local_h = 0;
  if (!ctx->render.backend.begin_subpass(ctx->render.backend.user_data, id, abs_x, abs_y, w,
                                  h, scale, &local_w, &local_h)) {
    return 0;
  }

  if (local_w < 1)
    local_w = 1;
  if (local_h < 1)
    local_h = 1;

  MDGUI_Context::SubpassState subpass{};
  subpass.parent_input = ctx->render.input;
  subpass.parent_origin_x = ctx->window.origin_x;
  subpass.parent_origin_y = ctx->window.origin_y;
  subpass.parent_content_y = ctx->window.content_y;
  subpass.parent_content_req_right = ctx->layout.content_req_right;
  subpass.parent_content_req_bottom = ctx->layout.content_req_bottom;
  subpass.parent_window_has_nonlabel_widget = ctx->layout.window_has_nonlabel_widget;
  subpass.parent_alpha_mod = mdgui_backend_get_alpha_mod();
  subpass.parent_layout_indent = ctx->layout.indent;
  subpass.parent_indent_stack_size = ctx->layout.indent_stack.size();
  subpass.parent_layout_same_line = ctx->layout.same_line;
  subpass.parent_layout_has_last_item = ctx->layout.has_last_item;
  subpass.parent_layout_last_item_x = ctx->layout.last_item_x;
  subpass.parent_layout_last_item_y = ctx->layout.last_item_y;
  subpass.parent_layout_last_item_w = ctx->layout.last_item_w;
  subpass.parent_layout_last_item_h = ctx->layout.last_item_h;
  subpass.parent_local_x = local_x;
  subpass.parent_logical_y = logical_y;
  subpass.parent_region_w = w;
  subpass.parent_region_h = h;
  subpass.local_w = local_w;
  subpass.local_h = local_h;
  ctx->window.subpass_stack.push_back(subpass);

  const float input_scale_x =
      (w > 0) ? ((float)local_w / (float)w) : 1.0f;
  const float input_scale_y =
      (h > 0) ? ((float)local_h / (float)h) : 1.0f;
  ctx->render.input.mouse_x = (int)(((float)(ctx->render.input.mouse_x - abs_x) * input_scale_x) + 0.5f);
  ctx->render.input.mouse_y = (int)(((float)(ctx->render.input.mouse_y - abs_y) * input_scale_y) + 0.5f);

  ctx->window.origin_x = 0;
  ctx->window.origin_y = 0;
  ctx->window.content_y = 0;
  ctx->layout.content_req_right = 0;
  ctx->layout.content_req_bottom = 0;
  ctx->layout.indent = 0;
  ctx->layout.same_line = false;
  ctx->layout.has_last_item = false;
  ctx->layout.window_has_nonlabel_widget = false;
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
  if (!ctx || ctx->window.subpass_stack.empty())
    return;

  const auto subpass = ctx->window.subpass_stack.back();
  ctx->window.subpass_stack.pop_back();
  ctx->render.backend.end_subpass(ctx->render.backend.user_data);
  ctx->render.input = subpass.parent_input;
  ctx->window.origin_x = subpass.parent_origin_x;
  ctx->window.origin_y = subpass.parent_origin_y;
  ctx->window.content_y = subpass.parent_content_y;
  ctx->layout.content_req_right =
      std::max(ctx->layout.content_req_right, subpass.parent_content_req_right);
  ctx->layout.content_req_bottom =
      std::max(ctx->layout.content_req_bottom, subpass.parent_content_req_bottom);
  ctx->layout.window_has_nonlabel_widget =
      ctx->layout.window_has_nonlabel_widget || subpass.parent_window_has_nonlabel_widget;
  ctx->layout.indent = subpass.parent_layout_indent;
  if (ctx->layout.indent_stack.size() > subpass.parent_indent_stack_size)
    ctx->layout.indent_stack.resize(subpass.parent_indent_stack_size);
  ctx->layout.same_line = subpass.parent_layout_same_line;
  ctx->layout.has_last_item = subpass.parent_layout_has_last_item;
  ctx->layout.last_item_x = subpass.parent_layout_last_item_x;
  ctx->layout.last_item_y = subpass.parent_layout_last_item_y;
  ctx->layout.last_item_w = subpass.parent_layout_last_item_w;
  ctx->layout.last_item_h = subpass.parent_layout_last_item_h;
  mdgui_backend_set_alpha_mod(subpass.parent_alpha_mod);
  set_content_clip(ctx);
}

void mdgui_begin_frame(MDGUI_Context *ctx, const MDGUI_Input *input) {
  if (!ctx || !input)
    return;
  mdgui_bind_backend(&ctx->render.backend);
  mdgui_backend_set_alpha_mod(255);
  mdgui_backend_begin_frame();
  ctx->render.input = *input;
  ctx->interaction.combo_capture_seen_this_frame = false;
  const bool main_menu_modal = !ctx->menus.open_main_menu_path.empty();
  if (ctx->cursor.custom_cursor_enabled && ctx->cursor.cursor_anim_count > 1) {
    const unsigned long long ticks = mdgui_backend_get_ticks_ms();
    const int anim_phase =
        (int)((ticks / 90ull) % (unsigned long long)ctx->cursor.cursor_anim_count);
    ctx->cursor.cursor_request_idx = anim_phase;
  } else if (ctx->cursor.custom_cursor_enabled) {
    ctx->cursor.cursor_request_idx = 0;
  } else {
    ctx->cursor.cursor_request_idx = ctx->cursor.cursors[9] ? 9 : 0;
  }

  // Bring the clicked window to front
  // before any drawing in this frame.
  if (ctx->render.input.mouse_pressed && !main_menu_modal) {
    const int clicked_idx =
        top_window_at_point(ctx, ctx->render.input.mouse_x, ctx->render.input.mouse_y, 3);
    if (clicked_idx >= 0 && clicked_idx < (int)ctx->window.windows.size()) {
      auto &clicked = ctx->window.windows[clicked_idx];
      if (!is_tiled_file_browser_window(ctx, clicked))
        clicked.z = ++ctx->window.z_counter;
    }
  }

  if (main_menu_modal) {
    ctx->window.dragging_window = -1;
    ctx->window.resizing_window = -1;
  }

  if (!ctx->render.input.mouse_down) {
    ctx->window.dragging_window = -1;
    ctx->window.resizing_window = -1;
    ctx->browser.scroll_dragging = false;
    ctx->interaction.active_text_input_drag_select = false;
    for (auto &w : ctx->window.windows) {
      w.text_scroll_dragging = false;
    }
  } else if (ctx->window.resizing_window >= 0 &&
             ctx->window.resizing_window < (int)ctx->window.windows.size() &&
             ctx->window.windows[ctx->window.resizing_window].closed) {
    ctx->window.resizing_window = -1;
  } else if (ctx->window.dragging_window >= 0 &&
             ctx->window.dragging_window < (int)ctx->window.windows.size()) {
    auto &w = ctx->window.windows[ctx->window.dragging_window];
    if (w.closed) {
      ctx->window.dragging_window = -1;
    } else {
      w.x = ctx->render.input.mouse_x - ctx->window.drag_off_x;
      w.y = ctx->render.input.mouse_y - ctx->window.drag_off_y;
      clamp_window_to_work_area(ctx, w);
    }
  }

  if (ctx->window.windows_locked) {
    ctx->window.dragging_window = -1;
    ctx->window.resizing_window = -1;
  }

  if (ctx->interaction.active_text_input_window >= 0 &&
      (ctx->interaction.active_text_input_window >= (int)ctx->window.windows.size() ||
       ctx->window.windows[ctx->interaction.active_text_input_window].closed)) {
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

  if (ctx->window.tile_manager_enabled) {
    tile_windows_internal(ctx);
  }

  ctx->window.current_window = -1;
  ctx->window.origin_x = 0;
  ctx->window.origin_y = 0;
  ctx->window.content_y = 0;
  ctx->menus.in_menu_bar = false;
  ctx->menus.in_menu = false;
  ctx->menus.building_menu_index = -1;
  ctx->menus.has_menu_bar = false;
  ctx->menus.menu_bar_x = 0;
  ctx->menus.menu_bar_y = 0;
  ctx->menus.menu_bar_w = 0;
  ctx->menus.menu_bar_h = 0;
  ctx->menus.menu_defs.clear();
  ctx->menus.menu_build_stack.clear();
  ctx->menus.in_main_menu_bar = false;
  ctx->menus.in_main_menu = false;
  ctx->menus.building_main_menu_index = -1;
  ctx->menus.main_menu_defs.clear();
  ctx->menus.main_menu_build_stack.clear();
  ctx->layout.content_req_right = 0;
  ctx->layout.content_req_bottom = 0;
  ctx->layout.font_stack.clear();
  ctx->layout.window_has_nonlabel_widget = false;
  ctx->interaction.combo_overlay_pending = false;
  ctx->interaction.combo_overlay_window = -1;
  ctx->interaction.combo_overlay_items = nullptr;
}

void mdgui_end_frame(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  mdgui_bind_backend(&ctx->render.backend);
  while (!ctx->window.subpass_stack.empty()) {
    mdgui_end_subpass(ctx);
  }
  if (!ctx->menus.open_main_menu_path.empty() && ctx->render.input.mouse_pressed) {
    bool in_bar = point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y,
                                ctx->menus.main_menu_bar_x, ctx->menus.main_menu_bar_y,
                                ctx->menus.main_menu_bar_w, ctx->menus.main_menu_bar_h);
    const bool in_popup = point_in_menu_popup_chain(
        ctx->menus.main_menu_defs, ctx->menus.open_main_menu_path, MAIN_MENU_ITEM_H,
        ctx->render.input.mouse_x, ctx->render.input.mouse_y);
    if (!in_bar && !in_popup) {
      ctx->menus.open_main_menu_path.clear();
      ctx->menus.open_main_menu_item_path.clear();
      ctx->menus.open_main_menu_index = -1;
    }
  }

  const unsigned long long now_ms = mdgui_backend_get_ticks_ms();
  prune_expired_toasts(ctx, now_ms);
  draw_toasts(ctx);
  for (auto &win : ctx->window.windows) {
    draw_cached_window_menu_overlay(ctx, win);
  }
  draw_open_main_menu_overlay(ctx);
  draw_status_bar(ctx);

  if (ctx->interaction.combo_capture_active && !ctx->interaction.combo_capture_seen_this_frame) {
    ctx->interaction.combo_capture_active = false;
    ctx->interaction.combo_capture_window = -1;
    ctx->interaction.combo_capture_x = 0;
    ctx->interaction.combo_capture_y = 0;
    ctx->interaction.combo_capture_w = 0;
    ctx->interaction.combo_capture_h = 0;
  }

  ctx->window.current_window = -1;
  ctx->menus.in_menu_bar = false;
  ctx->menus.in_menu = false;
  ctx->menus.building_menu_index = -1;
  ctx->menus.has_menu_bar = false;
  ctx->menus.menu_defs.clear();
  ctx->menus.menu_build_stack.clear();
  ctx->menus.in_main_menu_bar = false;
  ctx->menus.in_main_menu = false;
  ctx->menus.building_main_menu_index = -1;
  ctx->menus.main_menu_defs.clear();
  ctx->menus.main_menu_build_stack.clear();

  // Apply final cursor decision once
  // per frame to avoid visible cursor
  // thrash.
  if (ctx->cursor.cursor_request_idx >= 0 && ctx->cursor.cursor_request_idx < 16 &&
      ctx->cursor.cursors[ctx->cursor.cursor_request_idx] &&
      ctx->cursor.current_cursor_idx != ctx->cursor.cursor_request_idx) {
    SDL_SetCursor(ctx->cursor.cursors[ctx->cursor.cursor_request_idx]);
    ctx->cursor.current_cursor_idx = ctx->cursor.cursor_request_idx;
  }
  mdgui_backend_end_frame();
}

int mdgui_begin_window_ex(MDGUI_Context *ctx, const char *title, int x, int y,
                          int w, int h, int flags) {
  if (!ctx)
    return 0;
  bool created = false;
  const int idx = find_or_create_window(ctx, title, x, y, w, h, &created);
  ctx->window.current_window = idx;
  auto &win = ctx->window.windows[idx];

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
    ctx->window.current_window = -1;
    return 0;
  }
  if (is_occluded_by_higher_maximized_window(ctx, idx)) {
    ctx->window.current_window = -1;
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
      top_window_at_point(ctx, ctx->render.input.mouse_x, ctx->render.input.mouse_y, 3);
  const int top_here = (top_idx == idx);
  const bool no_chrome = (flags & MDGUI_WINDOW_FLAG_NO_CHROME) != 0;
  win.no_chrome = no_chrome;
  const bool chrome_input_allowed = ctx->menus.open_main_menu_path.empty();
  const bool move_resize_allowed =
      !no_chrome && chrome_input_allowed && !ctx->window.windows_locked;
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
    if (ctx->render.input.mouse_x >= win.x - margin &&
        ctx->render.input.mouse_x <= win.x + margin)
      edge_mask |= 1; // Left
    if (ctx->render.input.mouse_x >= win.x + win.w - margin &&
        ctx->render.input.mouse_x <= win.x + win.w + margin)
      edge_mask |= 2; // Right
    if (ctx->render.input.mouse_y >= win.y - margin &&
        ctx->render.input.mouse_y <= win.y + margin)
      edge_mask |= 4; // Top
    if (ctx->render.input.mouse_y >= win.y + win.h - margin &&
        ctx->render.input.mouse_y <= win.y + win.h + margin)
      edge_mask |= 8; // Bottom
  }
  win.last_edge_mask = edge_mask;

  // Cursor feedback (Hover OR active
  // resizing)
  if (move_resize_allowed &&
      ((edge_mask != 0 && top_here && ctx->window.resizing_window == -1) ||
       (ctx->window.resizing_window == idx))) {
    int mask =
        (ctx->window.resizing_window == idx) ? ctx->window.resize_edge_mask : edge_mask;
    int cursor_idx = 0;
    if ((mask & 1 && mask & 4) || (mask & 2 && mask & 8))
      cursor_idx = 5; // NWSE
    else if ((mask & 2 && mask & 4) || (mask & 1 && mask & 8))
      cursor_idx = 6; // NESW
    else if (mask & 1 || mask & 2)
      cursor_idx = 7; // EW
    else if (mask & 4 || mask & 8)
      cursor_idx = 8; // NS
    ctx->cursor.cursor_request_idx = cursor_idx;
    if (ctx->cursor.cursors[cursor_idx] && ctx->cursor.current_cursor_idx != cursor_idx) {
      SDL_SetCursor(ctx->cursor.cursors[cursor_idx]);
      ctx->cursor.current_cursor_idx = cursor_idx;
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

  if (!no_chrome && chrome_input_allowed && ctx->render.input.mouse_pressed && top_here) {
    // If a combo popup is open in
    // this window, let combo logic
    // consume this click first; don't
    // treat it as titlebar/chrome
    // interaction.
    if (win.open_combo_id != -1) {
      if (!is_tiled_file_browser_window(ctx, win))
        win.z = ++ctx->window.z_counter;
    } else {
      // Check Close button
      if (point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, close_x,
                        close_y, btn_w, btn_h)) {
        win.closed = true;
        return 0;
      }
      // Check Maximize button
      if (move_resize_allowed && can_maximize &&
          point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, max_x, max_y,
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
        win.z = ++ctx->window.z_counter;

      if (move_resize_allowed && edge_mask != 0) {
        win.fixed_rect = false;
        ctx->window.resizing_window = idx;
        ctx->window.resize_edge_mask = edge_mask;
        ctx->window.m_start_x = ctx->render.input.mouse_x;
        ctx->window.m_start_y = ctx->render.input.mouse_y;
        ctx->window.w_start_x = win.x;
        ctx->window.w_start_y = win.y;
        ctx->window.w_start_w = win.w;
        ctx->window.w_start_h = win.h;
      } else if (move_resize_allowed && !win.is_maximized &&
                 point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, win.x,
                               win.y, win.w, title_h)) {
        win.fixed_rect = false;
        ctx->window.dragging_window = idx;
        ctx->window.drag_off_x = ctx->render.input.mouse_x - win.x;
        ctx->window.drag_off_y = ctx->render.input.mouse_y - win.y;
      }
    }
  }

  // Handle active resizing
  if (ctx->window.resizing_window == idx) {
    if (!ctx->render.input.mouse_down) {
      ctx->window.resizing_window = -1;
    } else {
      int dx = ctx->render.input.mouse_x - ctx->window.m_start_x;
      int dy = ctx->render.input.mouse_y - ctx->window.m_start_y;

      if (ctx->window.resize_edge_mask & 1) { // Left
        int new_w = ctx->window.w_start_w - dx;
        if (new_w < win.min_w)
          dx = ctx->window.w_start_w - win.min_w;
        win.x = ctx->window.w_start_x + dx;
        win.w = ctx->window.w_start_w - dx;
      } else if (ctx->window.resize_edge_mask & 2) { // Right
        win.w = ctx->window.w_start_w + dx;
        if (win.w < win.min_w)
          win.w = win.min_w;
      }

      if (ctx->window.resize_edge_mask & 4) { // Top
        int new_h = ctx->window.w_start_h - dy;
        if (new_h < win.min_h)
          dy = ctx->window.w_start_h - win.min_h;
        win.y = ctx->window.w_start_y + dy;
        win.h = ctx->window.w_start_h - dy;
      } else if (ctx->window.resize_edge_mask & 8) { // Bottom
        win.h = ctx->window.w_start_h + dy;
        if (win.h < win.min_h)
          win.h = win.min_h;
      }
      clamp_window_to_work_area(ctx, win);
    }
  }

  const int focused = (top_window_at_point(ctx, ctx->render.input.mouse_x,
                                           ctx->render.input.mouse_y) == idx) ||
                      (win.z == ctx->window.z_counter);
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

  ctx->window.origin_x = no_chrome ? win.x : (win.x + 2);
  ctx->window.origin_y = no_chrome ? win.y : (win.y + title_h + 2);
  ctx->window.content_y = ctx->window.origin_y + ctx->layout.style.content_pad_y;
  ctx->menus.menu_index = 0;
  ctx->menus.menu_next_x = ctx->window.origin_x;
  ctx->menus.in_menu_bar = false;
  ctx->menus.in_menu = false;
  ctx->menus.current_menu_index = -1;
  ctx->menus.current_menu_item = 0;
  ctx->menus.building_menu_index = -1;
  ctx->menus.has_menu_bar = false;
  ctx->menus.menu_bar_x = 0;
  ctx->menus.menu_bar_y = 0;
  ctx->menus.menu_bar_w = 0;
  ctx->menus.menu_bar_h = 0;
  ctx->menus.menu_defs.clear();
  ctx->menus.menu_build_stack.clear();
  ctx->layout.content_req_right = no_chrome ? (win.x + 1) : (win.x + 6);
  ctx->layout.content_req_bottom = no_chrome ? (win.y + 1) : (win.y + title_h + 6);
  ctx->layout.same_line = false;
  ctx->layout.has_last_item = false;
  ctx->layout.last_item_x = 0;
  ctx->layout.last_item_y = 0;
  ctx->layout.last_item_w = 0;
  ctx->layout.last_item_h = 0;
  ctx->layout.columns_active = false;
  ctx->layout.columns_count = 0;
  ctx->layout.columns_index = 0;
  ctx->layout.columns_start_x = 0;
  ctx->layout.columns_start_y = ctx->window.content_y;
  ctx->layout.columns_width = 0;
  ctx->layout.columns_max_bottom = ctx->window.content_y;
  ctx->layout.columns_bottoms.clear();
  ctx->layout.indent = ctx->layout.style.content_pad_x;
  ctx->layout.indent_stack.clear();
  ctx->layout.window_has_nonlabel_widget = false;
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

  if (!ctx->window.nested_render_stack.empty()) {
    const auto nested = ctx->window.nested_render_stack.back();
    ctx->window.nested_render_stack.pop_back();
    ctx->window.origin_x = nested.parent_origin_x;
    ctx->window.origin_y = nested.parent_origin_y;
    ctx->window.content_y = nested.parent_content_y_after;
    ctx->layout.content_req_right =
        std::max(ctx->layout.content_req_right, nested.parent_content_req_right);
    ctx->layout.content_req_bottom =
        std::max(ctx->layout.content_req_bottom, nested.parent_content_req_bottom);
    ctx->layout.window_has_nonlabel_widget = ctx->layout.window_has_nonlabel_widget ||
                                      nested.parent_window_has_nonlabel_widget;
    ctx->layout.indent = nested.parent_layout_indent;
    if (ctx->layout.indent_stack.size() > nested.parent_indent_stack_size) {
      ctx->layout.indent_stack.resize(nested.parent_indent_stack_size);
    }
    mdgui_backend_set_alpha_mod(nested.parent_alpha_mod);
    set_content_clip(ctx);
    return;
  }

  auto &win = ctx->window.windows[ctx->window.current_window];
  mdgui_backend_set_clip_rect(0, 0, 0, 0, 0);

  if (!win.open_menu_path.empty() && ctx->render.input.mouse_pressed) {
    bool in_menu_bar = false;

    if (ctx->menus.has_menu_bar) {
      in_menu_bar =
          point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, ctx->menus.menu_bar_x,
                        ctx->menus.menu_bar_y, ctx->menus.menu_bar_w, ctx->menus.menu_bar_h);
    }

    const bool in_menu_popup = point_in_menu_popup_chain(
        ctx->menus.menu_defs, win.open_menu_path, ctx->menus.current_menu_h,
        ctx->render.input.mouse_x, ctx->render.input.mouse_y);

    if (!in_menu_bar && !in_menu_popup) {
      clear_window_menu_path(win);
    }
  }

  draw_open_menu_overlay(ctx);
  win.menu_overlay_defs.clear();
  win.menu_overlay_defs.reserve(ctx->menus.menu_defs.size());
  for (const auto &def : ctx->menus.menu_defs) {
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
  win.menu_overlay_item_h = ctx->menus.current_menu_h;

  if (ctx->interaction.combo_overlay_pending &&
      ctx->interaction.combo_overlay_window == ctx->window.current_window &&
      ctx->interaction.combo_overlay_items && ctx->interaction.combo_overlay_item_count > 0) {
    const int ix = ctx->interaction.combo_overlay_x;
    const int popup_y = ctx->interaction.combo_overlay_y;
    const int w = ctx->interaction.combo_overlay_w;
    const int item_h = ctx->interaction.combo_overlay_item_h;
    const int item_count = ctx->interaction.combo_overlay_item_count;
    const int popup_h = item_count * item_h;
    ctx->interaction.combo_capture_active = true;
    ctx->interaction.combo_capture_seen_this_frame = true;
    ctx->interaction.combo_capture_window = ctx->window.current_window;
    ctx->interaction.combo_capture_x = ix;
    ctx->interaction.combo_capture_y = popup_y;
    ctx->interaction.combo_capture_w = w;
    ctx->interaction.combo_capture_h = popup_h;
    mdgui_draw_frame_idx(nullptr, CLR_TEXT_DARK, ix - 1, popup_y - 1,
                         ix + w + 1, popup_y + popup_h + 1);
    mdgui_fill_rect_idx(nullptr, CLR_MENU_BG, ix, popup_y, w, popup_h);
    for (int i = 0; i < item_count; i++) {
      const int ry = popup_y + (i * item_h);
      const int row_hover = point_in_rect(
          ctx->render.input.mouse_x, ctx->render.input.mouse_y, ix, ry, w, item_h);
      if (i == ctx->interaction.combo_overlay_selected || row_hover)
        mdgui_fill_rect_idx(nullptr, CLR_ACCENT, ix, ry, w, item_h);
      if (mdgui_fonts[1] && ctx->interaction.combo_overlay_items[i]) {
        const int text_max_w = std::max(1, w - 4);
        const std::string row_text =
            ellipsize_text_to_width(ctx->interaction.combo_overlay_items[i], text_max_w);
        if (set_widget_clip_intersect_content(ctx, ix + 1, ry,
                                              std::max(1, w - 2), item_h)) {
          mdgui_fonts[1]->drawText(row_text.c_str(), nullptr, ix + 2, ry + 1,
                                   CLR_MENU_TEXT);
          set_content_clip(ctx);
        }
      }
    }
    ctx->interaction.combo_overlay_pending = false;
    ctx->interaction.combo_overlay_window = -1;
    ctx->interaction.combo_overlay_items = nullptr;
  }

  // All windows can scroll when
  // content exceeds viewport.
  {
    const int viewport_top = ctx->window.origin_y;
    const int viewport_h = (win.y + win.h - 4) - viewport_top;
    int total_h = ctx->window.content_y - ctx->window.origin_y;
    if (total_h < 0)
      total_h = 0;
    const int max_scroll =
        (viewport_h > 0 && total_h > viewport_h) ? (total_h - viewport_h) : 0;

    const int content_x = win.x + 2;
    const int content_w = win.w - 4;
    const int over_content = point_in_rect(
        ctx->render.input.mouse_x, ctx->render.input.mouse_y, content_x, viewport_top,
        content_w, (viewport_h > 0) ? viewport_h : 1);
    if (over_content && ctx->render.input.mouse_wheel != 0 && max_scroll > 0 &&
        !win.text_scroll_dragging) {
      const int prev = win.text_scroll;
      win.text_scroll -= ctx->render.input.mouse_wheel * 12;
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
          ctx->render.input.mouse_x, ctx->render.input.mouse_y, sb_x, sb_y, sb_w, viewport_h);
      if (ctx->render.input.mouse_pressed && over_scrollbar &&
          is_current_window_topmost(ctx)) {
        if (ctx->render.input.mouse_y >= thumb_y &&
            ctx->render.input.mouse_y <= thumb_y + thumb_h) {
          win.text_scroll_dragging = true;
          win.text_scroll_drag_offset = ctx->render.input.mouse_y - thumb_y;
        } else if (ctx->render.input.mouse_y < thumb_y) {
          win.text_scroll -= 12 * 3;
        } else {
          win.text_scroll += 12 * 3;
        }
      }
      if (win.text_scroll_dragging && ctx->render.input.mouse_down) {
        int desired_thumb_y = ctx->render.input.mouse_y - win.text_scroll_drag_offset;
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
  const int measured_min_w = (ctx->layout.content_req_right - win.x) + 4;
  if (ctx->layout.window_has_nonlabel_widget) {
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

  ctx->window.current_window = -1;
  ctx->menus.in_menu_bar = false;
  ctx->menus.in_menu = false;
  ctx->menus.building_menu_index = -1;
  ctx->menus.has_menu_bar = false;
  ctx->menus.menu_defs.clear();
  ctx->menus.menu_build_stack.clear();
  mdgui_backend_set_alpha_mod(255);
}

}



