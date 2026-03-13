#include "file_browser_internal.hpp"

extern "C" {
static void open_browser_at_mode(MDGUI_Context *ctx, int x, int y,
                                 bool select_folders) {
  if (!ctx)
    return;
  const char *browser_title = select_folders ? "Folder Browser" : "File Browser";
  const char *other_title = select_folders ? "File Browser" : "Folder Browser";
  for (auto &w : ctx->window.windows) {
    if (w.id == other_title) {
      w.closed = true;
    }
  }
  // Prevent same-frame click-through
  // from the opener button/menu item.
  ctx->render.input.mouse_pressed = 0;
  const int idx = find_or_create_window(ctx, browser_title, 28, 20, 280, 240);
  if (idx >= 0 && idx < (int)ctx->window.windows.size()) {
    auto &win = ctx->window.windows[idx];
    win.closed = false;
    if (!is_tiled_file_browser_window(ctx, win))
      win.z = ++ctx->window.z_counter;
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
  ctx->window.dragging_window = -1;
  ctx->window.resizing_window = -1;
  ctx->browser.open = true;
  ctx->browser.select_folders = select_folders;
  ctx->browser.open_x = x;
  ctx->browser.open_y = y;
  ctx->browser.center_pending = (x < 0 && y < 0);
  ctx->browser.result.clear();
  ctx->browser.last_click_idx = -1;
  ctx->browser.last_click_ticks = 0;
  file_browser_refresh_drives(ctx, true);
  if (!file_browser_path_exists(ctx->browser.cwd))
    ctx->browser.cwd = file_browser_default_open_path(ctx);
  file_browser_open_path(ctx, ctx->browser.cwd.c_str());
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
  ctx->browser.ext_filters.clear();
  if (!extensions || extension_count <= 0)
    return;

  for (int i = 0; i < extension_count; ++i) {
    const std::string norm = normalize_extension_filter(extensions[i]);
    if (norm.empty())
      continue;
    bool duplicate = false;
    for (const auto &existing : ctx->browser.ext_filters) {
      if (existing == norm) {
        duplicate = true;
        break;
      }
    }
    if (!duplicate)
      ctx->browser.ext_filters.push_back(norm);
  }
}

static const char *show_browser_mode(MDGUI_Context *ctx, bool select_folders) {
  if (!ctx || !ctx->browser.open ||
      ctx->browser.select_folders != select_folders)
    return nullptr;
  const char *browser_title = select_folders ? "Folder Browser" : "File Browser";
  ctx->browser.result.clear();

  const int open_x = (ctx->browser.open_x >= 0) ? ctx->browser.open_x : 28;
  const int open_y = (ctx->browser.open_y >= 0) ? ctx->browser.open_y : 20;
  if (!mdgui_begin_window(ctx, browser_title, open_x, open_y, 280, 240)) {
    for (int i = 0; i < (int)ctx->window.windows.size(); ++i) {
      if (ctx->window.windows[i].id == browser_title && !ctx->window.windows[i].closed) {
        // Hidden behind a higher z fullscreen window; keep browser state open.
        return nullptr;
      }
    }
    ctx->browser.open = false;
    return nullptr;
  }
  auto &win = ctx->window.windows[ctx->window.current_window];
  if (ctx->browser.center_pending) {
    center_window_rect_menu_aware(ctx, win);
    win.restored_x = win.x;
    win.restored_y = win.y;
    win.restored_w = win.w;
    win.restored_h = win.h;
    ctx->browser.center_pending = false;
  }
  if (win.min_w < 220)
    win.min_w = 220;
  if (win.min_h < 180)
    win.min_h = 180;

  file_browser_refresh_drives(ctx, false);
  if (!file_browser_path_exists(ctx->browser.cwd)) {
    const std::string fallback = file_browser_default_open_path(ctx);
    file_browser_open_path(ctx, fallback.c_str());
  }

  if (ctx->browser.path_subpass_enabled) {
    int path_local_x = 0;
    int path_logical_y = 0;
    layout_prepare_widget(ctx, &path_local_x, &path_logical_y);
    const int path_parent_w = layout_resolve_width(ctx, path_local_x, 0, 20);
    const int path_line_h =
        std::max(ctx->layout.style.label_h,
                 font_line_height(ctx, ctx->browser.path_font));
    const int path_top_inset = std::max(1, ctx->layout.style.spacing_y / 2);
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
      mdgui_label_font(ctx, ctx->browser.cwd.c_str(),
                       ctx->browser.path_font);
      mdgui_end_subpass(ctx);
      layout_commit_widget(ctx, path_local_x, path_logical_y, path_parent_w,
                           path_parent_h, ctx->layout.style.spacing_y);
    } else {
      mdgui_label_font(ctx, ctx->browser.cwd.c_str(),
                       ctx->browser.path_font);
    }
  } else {
    mdgui_label_font(ctx, ctx->browser.cwd.c_str(),
                     ctx->browser.path_font);
  }
  const int drive_button_w = 64;
  const int drive_stride = drive_button_w + ctx->layout.style.spacing_x;
  const int drive_avail_w = resolve_dynamic_width(ctx, 0, 0, drive_button_w);
  int drive_columns = (drive_avail_w + ctx->layout.style.spacing_x) / drive_stride;
  if (drive_columns < 1)
    drive_columns = 1;
  for (int i = 0; i < (int)ctx->browser.drives.size(); ++i) {
    const std::string &drive = ctx->browser.drives[(size_t)i];
    if (mdgui_button(ctx, drive.c_str(), drive_button_w, 12)) {
      file_browser_open_path(ctx, drive.c_str());
    }
    if (((i + 1) % drive_columns) != 0 &&
        (i + 1) < (int)ctx->browser.drives.size())
      mdgui_same_line(ctx);
  }
  if (ctx->browser.drives.empty()) {
    mdgui_label(ctx, "(no roots found)");
  }
  mdgui_spacing(ctx, 2);

  const int row_h = 10;
  const int button_h = 12;
  const int list_x = ctx->window.origin_x + 8;
  const int list_y = ctx->window.content_y + 4;
  const int list_w = resolve_dynamic_width(ctx, 8, -16, 40);
  const int bottom_buttons_y = win.y + win.h - button_h - 4;
  int list_h = bottom_buttons_y - 4 - list_y;
  if (list_h < row_h)
    list_h = row_h;
  const int rows = list_h / row_h;
  list_h = rows * row_h;
  const int scrollbar_w = 9;
  const int content_w = list_w - scrollbar_w - 1;
  const int total = (int)ctx->browser.entries.size();
  const int max_scroll = (total > rows) ? (total - rows) : 0;
  if (ctx->browser.restore_scroll_pending && ctx->browser.selected >= 0 &&
      total > 0) {
    int target = ctx->browser.selected - (rows / 2);
    if (target < 0)
      target = 0;
    const int max_target = std::max(0, total - rows);
    if (target > max_target)
      target = max_target;
    ctx->browser.scroll = target;
    ctx->browser.restore_scroll_pending = false;
  }
  const int over_list = point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y,
                                      list_x, list_y, content_w, list_h);
  const int over_scrollbar =
      point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y,
                    list_x + content_w + 1, list_y, scrollbar_w, list_h);
  const int over_browser_pane = point_in_rect(
      ctx->render.input.mouse_x, ctx->render.input.mouse_y, list_x, list_y, list_w, list_h);
  if (ctx->browser.scroll < 0)
    ctx->browser.scroll = 0;
  if (ctx->browser.scroll > max_scroll)
    ctx->browser.scroll = max_scroll;
  if (ctx->render.input.mouse_wheel != 0 &&
      (over_list || over_scrollbar || over_browser_pane)) {
    ctx->browser.scroll -= ctx->render.input.mouse_wheel;
    if (ctx->browser.scroll < 0)
      ctx->browser.scroll = 0;
    if (ctx->browser.scroll > max_scroll)
      ctx->browser.scroll = max_scroll;
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
    const int item_idx = ctx->browser.scroll + i;
    const int ry = list_y + (i * row_h);
    if (item_idx >= total)
      break;
    const int hovered = point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y,
                                      list_x, ry, content_w, row_h);
    if (item_idx == ctx->browser.selected)
      mdgui_fill_rect_idx(nullptr, CLR_ACCENT, list_x, ry, content_w, row_h);
    else if (hovered)
      mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, list_x, ry, content_w,
                          row_h);
    if (mdgui_fonts[1]) {
      mdgui_fonts[1]->drawText(
          ctx->browser.entries[item_idx].label.c_str(), nullptr,
          list_x + 2, ry + 1, CLR_MENU_TEXT);
    }
    if (hovered && ctx->render.input.mouse_pressed) {
      ctx->browser.selected = item_idx;
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
        list_y + ((ctx->browser.scroll * travel) / max_scroll);
    mdgui_fill_rect_idx(nullptr, CLR_ACCENT, sb_x + 1, thumb_y, scrollbar_w - 2,
                        thumb_h);
    if (ctx->render.input.mouse_pressed && over_scrollbar) {
      if (ctx->render.input.mouse_y >= thumb_y &&
          ctx->render.input.mouse_y <= thumb_y + thumb_h) {
        ctx->browser.scroll_dragging = true;
        ctx->browser.scroll_drag_offset = ctx->render.input.mouse_y - thumb_y;
      } else if (ctx->render.input.mouse_y < thumb_y)
        ctx->browser.scroll -= rows;
      else if (ctx->render.input.mouse_y > thumb_y + thumb_h)
        ctx->browser.scroll += rows;
      if (ctx->browser.scroll < 0)
        ctx->browser.scroll = 0;
      if (ctx->browser.scroll > max_scroll)
        ctx->browser.scroll = max_scroll;
    }
    if (ctx->browser.scroll_dragging && ctx->render.input.mouse_down) {
      int desired_thumb_y =
          ctx->render.input.mouse_y - ctx->browser.scroll_drag_offset;
      if (desired_thumb_y < list_y)
        desired_thumb_y = list_y;
      if (desired_thumb_y > list_y + travel)
        desired_thumb_y = list_y + travel;
      ctx->browser.scroll =
          ((desired_thumb_y - list_y) * max_scroll) / (travel > 0 ? travel : 1);
      if (ctx->browser.scroll < 0)
        ctx->browser.scroll = 0;
      if (ctx->browser.scroll > max_scroll)
        ctx->browser.scroll = max_scroll;
    }
  }
  note_content_bounds(ctx, list_x + list_w, list_y + list_h);
  ctx->window.content_y += 4 + list_h + 4;

  bool trigger_open = false;
  bool trigger_select = false;
  if (clicked && ctx->browser.selected >= 0 &&
      ctx->browser.selected < (int)ctx->browser.entries.size()) {
    const int idx = ctx->browser.selected;
    const unsigned long long now = mdgui_backend_get_ticks_ms();
    if (ctx->browser.last_click_idx == idx &&
        (now - ctx->browser.last_click_ticks) <= 350) {
      trigger_open = true;
    }
    ctx->browser.last_click_idx = idx;
    ctx->browser.last_click_ticks = now;
  }

  const int saved_content_y = ctx->window.content_y;
  const int saved_indent = ctx->layout.indent;
  const bool saved_has_last = ctx->layout.has_last_item;
  const bool saved_same_line = ctx->layout.same_line;
  const int button_logical_y = bottom_buttons_y + win.text_scroll;

  ctx->window.content_y = button_logical_y;
  ctx->layout.indent = 8;
  ctx->layout.has_last_item = false;
  ctx->layout.same_line = false;
  if (mdgui_button(ctx, "Up", 48, button_h)) {
    file_browser_open_path(ctx, "..");
  }
  ctx->window.content_y = button_logical_y;
  ctx->layout.indent = 60;
  ctx->layout.has_last_item = false;
  ctx->layout.same_line = false;
  if (mdgui_button(ctx, select_folders ? "Select" : "Open", 60, button_h)) {
    if (select_folders) {
      trigger_select = true;
    } else {
      trigger_open = true;
    }
  }
  ctx->window.content_y = button_logical_y;
  ctx->layout.indent = 124;
  ctx->layout.has_last_item = false;
  ctx->layout.same_line = false;
  if (mdgui_button(ctx, "Cancel", 60, button_h)) {
    ctx->browser.open = false;
    if (ctx->window.current_window >= 0 &&
        ctx->window.current_window < (int)ctx->window.windows.size()) {
      ctx->window.windows[ctx->window.current_window].closed = true;
    }
  }
  ctx->window.content_y = saved_content_y;
  ctx->layout.indent = saved_indent;
  ctx->layout.has_last_item = saved_has_last;
  ctx->layout.same_line = saved_same_line;

  if (trigger_select) {
    std::string selected_path = ctx->browser.cwd;
    if (ctx->browser.selected >= 0 &&
        ctx->browser.selected < (int)ctx->browser.entries.size()) {
      const auto &entry = ctx->browser.entries[ctx->browser.selected];
      if (entry.is_dir && entry.label != "..")
        selected_path = entry.full_path;
    }
    ctx->browser.result = selected_path;
    ctx->browser.open = false;
    if (ctx->window.current_window >= 0 &&
        ctx->window.current_window < (int)ctx->window.windows.size()) {
      ctx->window.windows[ctx->window.current_window].closed = true;
    }
  }

  if (trigger_open && ctx->browser.selected >= 0 &&
      ctx->browser.selected < (int)ctx->browser.entries.size()) {
    const auto &entry = ctx->browser.entries[ctx->browser.selected];
    if (entry.is_dir) {
      file_browser_open_path(ctx, entry.full_path.c_str());
    } else if (!select_folders) {
      ctx->browser.last_selected_path = entry.full_path;
      ctx->browser.result = entry.full_path;
      ctx->browser.open = false;
      if (ctx->window.current_window >= 0 &&
          ctx->window.current_window < (int)ctx->window.windows.size()) {
        ctx->window.windows[ctx->window.current_window].closed = true;
      }
    }
  }

  mdgui_end_window(ctx);
  if (!ctx->browser.result.empty())
    return ctx->browser.result.c_str();
  return nullptr;
}

const char *mdgui_show_file_browser(MDGUI_Context *ctx) {
  return show_browser_mode(ctx, false);
}

const char *mdgui_show_folder_browser(MDGUI_Context *ctx) {
  return show_browser_mode(ctx, true);
}

}


