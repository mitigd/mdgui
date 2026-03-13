void mdgui_begin_menu_bar(MDGUI_Context *ctx) {
  if (!ctx || ctx->current_window < 0)
    return;
  ctx->window_has_nonlabel_widget = true;
  const auto &win = ctx->windows[ctx->current_window];
  const bool had_menu_bar = ctx->has_menu_bar;
  const int bar_x = win.x + 1;
  const int bar_y = ctx->origin_y;
  const int bar_w = win.w - 2;
  const int bar_h = 10;
  mdgui_backend_set_clip_rect(0, 0, 0, 0, 0);
  mdgui_fill_rect_idx(nullptr, CLR_MENU_BG, bar_x, bar_y, bar_w, bar_h);
  ctx->menu_index = 0;
  ctx->menu_next_x = win.x + 4;
  ctx->in_menu_bar = true;
  ctx->in_menu = false;
  ctx->building_menu_index = -1;
  ctx->has_menu_bar = true;
  ctx->menu_bar_x = bar_x;
  ctx->menu_bar_y = bar_y;
  ctx->menu_bar_w = bar_w;
  ctx->menu_bar_h = bar_h;
  ctx->menu_defs.clear();
  ctx->menu_defs.resize(RESERVED_TOP_LEVEL_MENU_SLOTS);
  ctx->menu_build_stack.clear();
  if (!had_menu_bar) {
    const int content_top = bar_y + bar_h + 2 + ctx->style.content_pad_y;
    ctx->origin_y = content_top;
    if (ctx->content_y < content_top)
      ctx->content_y = content_top;
  }
}

int mdgui_begin_menu(MDGUI_Context *ctx, const char *text) {
  if (!ctx || !ctx->in_menu_bar || ctx->current_window < 0 || !text)
    return 0;
  auto &win = ctx->windows[ctx->current_window];

  const int tw = mdgui_fonts[1] ? mdgui_fonts[1]->measureTextWidth(text) : 40;
  const int item_w = tw + 10;
  const int x = ctx->menu_next_x;
  const int y = ctx->menu_bar_y;
  const int hovered =
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, x, y, item_w, 10);
  const bool menu_interactable =
      (win.z == ctx->z_counter) || !win.open_menu_path.empty();

  if ((int)ctx->menu_defs.size() <= ctx->menu_index) {
    ctx->menu_defs.resize(ctx->menu_index + 1);
  }
  auto &def = ctx->menu_defs[ctx->menu_index];
  def.x = x;
  def.y = y + 10 + MENU_POPUP_GAP_Y;
  def.w = (tw + 40 < 120) ? 120 : (tw + 40);
  def.parent_menu_index = -1;
  def.parent_item_index = -1;
  def.items.clear();

  if (!win.open_menu_path.empty() && hovered && menu_interactable) {
    win.open_menu_path.resize(1);
    win.open_menu_path[0] = ctx->menu_index;
    win.open_menu_index = ctx->menu_index;
  }

  if (ctx->input.mouse_pressed && hovered && menu_interactable) {
    win.z = ++ctx->z_counter;
    if (win.open_menu_path.size() == 1 &&
        win.open_menu_path[0] == ctx->menu_index) {
      clear_window_menu_path(win);
    } else {
      win.open_menu_path.resize(1);
      win.open_menu_path[0] = ctx->menu_index;
      win.open_menu_index = ctx->menu_index;
    }
    ctx->input.mouse_pressed = 0;
  }

  const int open =
      (!win.open_menu_path.empty() && win.open_menu_path[0] == ctx->menu_index);
  if (open || hovered) {
    mdgui_fill_rect_idx(nullptr, CLR_ACCENT, x, y, item_w, 10);
  }
  if (mdgui_fonts[1]) {
    mdgui_fonts[1]->drawText(text, nullptr, x + 3, y + 1, CLR_MENU_TEXT);
  }

  if (open) {
    ctx->in_menu = true;
    ctx->current_menu_index = ctx->menu_index;
    ctx->building_menu_index = ctx->menu_index;
    ctx->current_menu_x = def.x;
    ctx->current_menu_y = def.y;
    ctx->current_menu_w = def.w;
    ctx->current_menu_h = 10;
    ctx->current_menu_item = 0;
    ctx->menu_build_stack.clear();
    ctx->menu_build_stack.push_back(ctx->menu_index);
  }

  ctx->menu_next_x += item_w + 2;
  ctx->menu_index += 1;
  return open;
}

int mdgui_menu_item(MDGUI_Context *ctx, const char *text) {
  if (!ctx || !ctx->in_menu || !text || ctx->menu_build_stack.empty())
    return 0;
  auto &win = ctx->windows[ctx->current_window];
  const bool menu_interactable =
      (win.z == ctx->z_counter) || !win.open_menu_path.empty();
  const int menu_idx = ctx->menu_build_stack.back();
  auto &def = ctx->menu_defs[menu_idx];
  bool has_check = false;
  bool checked = false;
  const char *label = parse_menu_check_prefix(text, &has_check, &checked);
  (void)checked;
  const int check_w = has_check ? menu_check_indicator_width() : 0;
  const int item_text_w =
      mdgui_fonts[1] ? mdgui_fonts[1]->measureTextWidth(label) : 40;
  const int item_needed_w = item_text_w + 12 + check_w;
  if (item_needed_w > def.w) {
    def.w = item_needed_w;
    reposition_child_menu_chain(ctx->menu_defs, menu_idx, ctx->current_menu_h);
  }
  def.items.push_back({text, -1, false});
  const int item_index = (int)def.items.size() - 1;
  const int item_y = def.y + (item_index * ctx->current_menu_h);

  const int hovered = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                    def.x, item_y, def.w, ctx->current_menu_h);
  const int submenu_depth = (int)ctx->menu_build_stack.size();
  if (hovered && menu_interactable &&
      menu_path_prefix_matches(win.open_menu_path, ctx->menu_build_stack) &&
      (int)win.open_menu_path.size() > submenu_depth) {
    win.open_menu_path.resize(submenu_depth);
    win.open_menu_index =
        win.open_menu_path.empty() ? -1 : win.open_menu_path[0];
  }
  if (hovered && ctx->input.mouse_pressed && menu_interactable) {
    win.z = ++ctx->z_counter;
    clear_window_menu_path(win);
    ctx->input.mouse_pressed = 0;
    return 1;
  }
  return 0;
}

void mdgui_menu_separator(MDGUI_Context *ctx) {
  if (!ctx || !ctx->in_menu || ctx->menu_build_stack.empty())
    return;
  auto &def = ctx->menu_defs[ctx->menu_build_stack.back()];
  def.items.push_back({"", -1, true});
}

int mdgui_begin_submenu(MDGUI_Context *ctx, const char *text) {
  if (!ctx || !ctx->in_menu || !text || ctx->menu_build_stack.empty() ||
      ctx->current_window < 0)
    return 0;
  auto &win = ctx->windows[ctx->current_window];
  const bool menu_interactable =
      (win.z == ctx->z_counter) || !win.open_menu_path.empty();
  const int parent_menu_index = ctx->menu_build_stack.back();
  auto &parent = ctx->menu_defs[parent_menu_index];
  bool has_check = false;
  bool checked = false;
  const char *label = parse_menu_check_prefix(text, &has_check, &checked);
  (void)checked;
  const int check_w = has_check ? menu_check_indicator_width() : 0;
  const int item_h = ctx->current_menu_h;
  const int item_text_w =
      mdgui_fonts[1] ? mdgui_fonts[1]->measureTextWidth(label) : 40;
  const int item_needed_w = item_text_w + 20 + check_w;
  if (item_needed_w > parent.w) {
    parent.w = item_needed_w;
    reposition_child_menu_chain(ctx->menu_defs, parent_menu_index, item_h);
  }
  parent.items.push_back({text, -1, false});
  const int item_index = (int)parent.items.size() - 1;
  const int item_y = parent.y + (item_index * item_h);

  const int child_menu_index = (int)ctx->menu_defs.size();
  parent.items[item_index].child_menu_index = child_menu_index;
  const int parent_x = parent.x;
  const int parent_w = parent.w;
  ctx->menu_defs.push_back({});
  auto &child = ctx->menu_defs.back();
  child.x = parent_x + parent_w + MENU_POPUP_GAP_Y;
  child.y = item_y;
  child.w = (item_text_w + 40 < 120) ? 120 : (item_text_w + 40);
  child.parent_menu_index = parent_menu_index;
  child.parent_item_index = item_index;
  child.items.clear();

  const auto &parent_after = ctx->menu_defs[parent_menu_index];

  const int hovered =
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, parent_after.x,
                    item_y, parent_after.w, item_h);
  const int submenu_depth = (int)ctx->menu_build_stack.size();
  const bool was_open =
      menu_path_prefix_matches(win.open_menu_path, ctx->menu_build_stack) &&
      (int)win.open_menu_path.size() > submenu_depth &&
      win.open_menu_path[submenu_depth] == child_menu_index;
  if (ctx->input.mouse_pressed && hovered && menu_interactable) {
    win.z = ++ctx->z_counter;
    if (was_open && (int)win.open_menu_path.size() == submenu_depth + 1) {
      win.open_menu_path.resize(submenu_depth);
      win.open_menu_index =
          win.open_menu_path.empty() ? -1 : win.open_menu_path[0];
    } else {
      win.open_menu_path.resize((size_t)submenu_depth + 1);
      for (int i = 0; i < submenu_depth; ++i)
        win.open_menu_path[i] = ctx->menu_build_stack[i];
      win.open_menu_path[submenu_depth] = child_menu_index;
      win.open_menu_index =
          win.open_menu_path.empty() ? -1 : win.open_menu_path[0];
    }
    ctx->input.mouse_pressed = 0;
  } else if (hovered && menu_interactable) {
    win.open_menu_path.resize((size_t)submenu_depth + 1);
    for (int i = 0; i < submenu_depth; ++i)
      win.open_menu_path[i] = ctx->menu_build_stack[i];
    win.open_menu_path[submenu_depth] = child_menu_index;
    win.open_menu_index =
        win.open_menu_path.empty() ? -1 : win.open_menu_path[0];
  }

  const bool open =
      menu_path_prefix_matches(win.open_menu_path, ctx->menu_build_stack) &&
      (int)win.open_menu_path.size() > submenu_depth &&
      win.open_menu_path[submenu_depth] == child_menu_index;
  if (open) {
    ctx->menu_build_stack.push_back(child_menu_index);
    ctx->building_menu_index = child_menu_index;
  }
  return open ? 1 : 0;
}

void mdgui_end_submenu(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  if (ctx->menu_build_stack.size() > 1) {
    ctx->menu_build_stack.pop_back();
    ctx->building_menu_index =
        ctx->menu_build_stack.empty() ? -1 : ctx->menu_build_stack.back();
  }
}

void mdgui_end_menu(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  ctx->in_menu = false;
  ctx->building_menu_index = -1;
  ctx->menu_build_stack.clear();
}

void mdgui_end_menu_bar(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  ctx->in_menu_bar = false;
  ctx->in_menu = false;
  ctx->menu_build_stack.clear();
  set_content_clip(ctx);
}

void mdgui_begin_main_menu_bar(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  const int rw = get_logical_render_w(ctx);
  ctx->main_menu_bar_x = 0;
  ctx->main_menu_bar_y = 0;
  ctx->main_menu_bar_w = rw;
  ctx->main_menu_bar_h = 10;

  mdgui_fill_rect_idx(nullptr, CLR_MENU_BG, ctx->main_menu_bar_x,
                      ctx->main_menu_bar_y, ctx->main_menu_bar_w,
                      ctx->main_menu_bar_h);
  ctx->main_menu_index = 0;
  ctx->main_menu_next_x = 3;
  ctx->in_main_menu_bar = true;
  ctx->in_main_menu = false;
  ctx->building_main_menu_index = -1;
  ctx->main_menu_defs.clear();
  ctx->main_menu_defs.resize(RESERVED_TOP_LEVEL_MENU_SLOTS);
  ctx->main_menu_build_stack.clear();
}

int mdgui_begin_main_menu(MDGUI_Context *ctx, const char *text) {
  if (!ctx || !ctx->in_main_menu_bar || !text)
    return 0;

  const int tw = mdgui_fonts[1] ? mdgui_fonts[1]->measureTextWidth(text) : 40;
  const int item_w = tw + 10;
  const int x = ctx->main_menu_next_x;
  const int y = ctx->main_menu_bar_y;
  const int hovered = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, x,
                                    y, item_w, ctx->main_menu_bar_h);

  if ((int)ctx->main_menu_defs.size() <= ctx->main_menu_index) {
    ctx->main_menu_defs.resize(ctx->main_menu_index + 1);
  }
  auto &def = ctx->main_menu_defs[ctx->main_menu_index];
  def.x = x;
  def.y = y + ctx->main_menu_bar_h + MENU_POPUP_GAP_Y;
  def.w = (tw + 40 < 120) ? 120 : (tw + 40);
  def.parent_menu_index = -1;
  def.parent_item_index = -1;
  def.items.clear();

  if (!ctx->open_main_menu_path.empty() && hovered) {
    ctx->open_main_menu_path.resize(1);
    ctx->open_main_menu_path[0] = ctx->main_menu_index;
    ctx->open_main_menu_item_path.resize(1);
    ctx->open_main_menu_item_path[0] = ctx->main_menu_index;
    ctx->open_main_menu_index = ctx->main_menu_index;
  }
  if (ctx->input.mouse_pressed && hovered) {
    if (ctx->open_main_menu_path.size() == 1 &&
        ctx->open_main_menu_path[0] == ctx->main_menu_index) {
      ctx->open_main_menu_path.clear();
      ctx->open_main_menu_item_path.clear();
      ctx->open_main_menu_index = -1;
    } else {
      ctx->open_main_menu_path.resize(1);
      ctx->open_main_menu_path[0] = ctx->main_menu_index;
      ctx->open_main_menu_item_path.resize(1);
      ctx->open_main_menu_item_path[0] = ctx->main_menu_index;
      ctx->open_main_menu_index = ctx->main_menu_index;
    }
    ctx->input.mouse_pressed = 0;
  }

  const int open = (!ctx->open_main_menu_path.empty() &&
                    ctx->open_main_menu_path[0] == ctx->main_menu_index);
  if (open || hovered) {
    mdgui_fill_rect_idx(nullptr, CLR_ACCENT, x, y, item_w,
                        ctx->main_menu_bar_h);
  }
  if (mdgui_fonts[1]) {
    mdgui_fonts[1]->drawText(text, nullptr, x + 3, y + 1, CLR_MENU_TEXT);
  }

  if (open) {
    ctx->in_main_menu = true;
    ctx->building_main_menu_index = ctx->main_menu_index;
    ctx->current_menu_x = def.x;
    ctx->current_menu_y = def.y;
    ctx->current_menu_w = def.w;
    ctx->current_menu_h = 10;
    ctx->current_menu_item = 0;
    ctx->main_menu_build_stack.clear();
    ctx->main_menu_build_stack.push_back(ctx->main_menu_index);
  }

  ctx->main_menu_next_x += item_w + 2;
  ctx->main_menu_index += 1;
  return open;
}

int mdgui_main_menu_item(MDGUI_Context *ctx, const char *text) {
  if (!ctx || !ctx->in_main_menu || !text || ctx->main_menu_build_stack.empty())
    return 0;

  bool has_check = false;
  bool checked = false;
  const char *label = parse_menu_check_prefix(text, &has_check, &checked);
  (void)checked;
  const int check_w = has_check ? menu_check_indicator_width() : 0;
  const int menu_idx = ctx->main_menu_build_stack.back();
  auto &def = ctx->main_menu_defs[menu_idx];
  const int item_text_w =
      mdgui_fonts[1] ? mdgui_fonts[1]->measureTextWidth(label) : 40;
  const int item_needed_w = item_text_w + 12 + check_w;
  if (item_needed_w > def.w) {
    def.w = item_needed_w;
    reposition_child_menu_chain(ctx->main_menu_defs, menu_idx,
                                MAIN_MENU_ITEM_H);
  }
  def.items.push_back({text, -1, false});
  const int item_index = (int)def.items.size() - 1;
  const int item_y = def.y + (item_index * MAIN_MENU_ITEM_H);
  const int hovered = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                    def.x, item_y, def.w, MAIN_MENU_ITEM_H);
  const int submenu_depth = (int)ctx->main_menu_build_stack.size();
  if (hovered &&
      menu_path_prefix_matches(ctx->open_main_menu_path,
                               ctx->main_menu_build_stack) &&
      (int)ctx->open_main_menu_path.size() > submenu_depth) {
    ctx->open_main_menu_path.resize(submenu_depth);
    if ((int)ctx->open_main_menu_item_path.size() > submenu_depth) {
      ctx->open_main_menu_item_path.resize(submenu_depth);
    }
    ctx->open_main_menu_index =
        ctx->open_main_menu_path.empty() ? -1 : ctx->open_main_menu_path[0];
  }

  if (hovered && ctx->input.mouse_pressed) {
    ctx->open_main_menu_path.clear();
    ctx->open_main_menu_item_path.clear();
    ctx->open_main_menu_index = -1;
    ctx->input.mouse_pressed = 0;
    return 1;
  }
  return 0;
}

void mdgui_main_menu_separator(MDGUI_Context *ctx) {
  if (!ctx || !ctx->in_main_menu || ctx->main_menu_build_stack.empty())
    return;
  auto &def = ctx->main_menu_defs[ctx->main_menu_build_stack.back()];
  def.items.push_back({"", -1, true});
}

int mdgui_begin_main_submenu(MDGUI_Context *ctx, const char *text) {
  if (!ctx || !ctx->in_main_menu || !text || ctx->main_menu_build_stack.empty())
    return 0;
  bool has_check = false;
  bool checked = false;
  const char *label = parse_menu_check_prefix(text, &has_check, &checked);
  (void)checked;
  const int check_w = has_check ? menu_check_indicator_width() : 0;
  const int parent_menu_index = ctx->main_menu_build_stack.back();
  auto &parent = ctx->main_menu_defs[parent_menu_index];
  const int item_h = MAIN_MENU_ITEM_H;
  const int item_text_w =
      mdgui_fonts[1] ? mdgui_fonts[1]->measureTextWidth(label) : 40;
  const int item_needed_w = item_text_w + 20 + check_w;
  if (item_needed_w > parent.w) {
    parent.w = item_needed_w;
    reposition_child_menu_chain(ctx->main_menu_defs, parent_menu_index, item_h);
  }
  parent.items.push_back({text, -1, false});
  const int item_index = (int)parent.items.size() - 1;
  const int item_y = parent.y + (item_index * item_h);

  const int child_menu_index = (int)ctx->main_menu_defs.size();
  parent.items[item_index].child_menu_index = child_menu_index;
  const int parent_x = parent.x;
  const int parent_w = parent.w;
  ctx->main_menu_defs.push_back({});
  auto &child = ctx->main_menu_defs.back();
  child.x = parent_x + parent_w + MENU_POPUP_GAP_Y;
  child.y = item_y;
  child.w = (item_text_w + 40 < 120) ? 120 : (item_text_w + 40);
  child.parent_menu_index = parent_menu_index;
  child.parent_item_index = item_index;
  child.items.clear();

  const auto &parent_after = ctx->main_menu_defs[parent_menu_index];

  const int hovered =
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, parent_after.x,
                    item_y, parent_after.w, item_h);
  const int submenu_depth = (int)ctx->main_menu_build_stack.size();
  auto ensure_open_paths_for_item = [&](int child_idx) {
    if ((int)ctx->open_main_menu_path.size() < submenu_depth + 1) {
      ctx->open_main_menu_path.resize((size_t)submenu_depth + 1);
    }
    for (int i = 0; i < submenu_depth; ++i) {
      ctx->open_main_menu_path[i] = ctx->main_menu_build_stack[i];
    }
    ctx->open_main_menu_path[submenu_depth] = child_idx;

    if ((int)ctx->open_main_menu_item_path.size() < submenu_depth + 1) {
      ctx->open_main_menu_item_path.resize((size_t)submenu_depth + 1, -1);
    }
    for (int i = 0; i < submenu_depth; ++i) {
      if (i < (int)ctx->open_main_menu_item_path.size() &&
          ctx->open_main_menu_item_path[i] >= 0) {
        continue;
      }
      ctx->open_main_menu_item_path[i] = ctx->main_menu_build_stack[i];
    }
    ctx->open_main_menu_item_path[submenu_depth] = item_index;
    ctx->open_main_menu_index =
        ctx->open_main_menu_path.empty() ? -1 : ctx->open_main_menu_path[0];
  };

  const bool was_open =
      menu_path_prefix_matches(ctx->open_main_menu_item_path,
                               ctx->main_menu_build_stack) &&
      (int)ctx->open_main_menu_item_path.size() > submenu_depth &&
      ctx->open_main_menu_item_path[submenu_depth] == item_index;
  if (ctx->input.mouse_pressed && hovered) {
    if (was_open &&
        (int)ctx->open_main_menu_item_path.size() == submenu_depth + 1) {
      ctx->open_main_menu_path.resize(submenu_depth);
      if ((int)ctx->open_main_menu_item_path.size() > submenu_depth) {
        ctx->open_main_menu_item_path.resize(submenu_depth);
      }
      ctx->open_main_menu_index =
          ctx->open_main_menu_path.empty() ? -1 : ctx->open_main_menu_path[0];
    } else {
      ensure_open_paths_for_item(child_menu_index);
    }
    ctx->input.mouse_pressed = 0;
  } else if (hovered) {
    ensure_open_paths_for_item(child_menu_index);
  }

  const bool open =
      menu_path_prefix_matches(ctx->open_main_menu_item_path,
                               ctx->main_menu_build_stack) &&
      (int)ctx->open_main_menu_item_path.size() > submenu_depth &&
      ctx->open_main_menu_item_path[submenu_depth] == item_index;
  if (open) {
    ensure_open_paths_for_item(child_menu_index);
    ctx->main_menu_build_stack.push_back(child_menu_index);
    ctx->building_main_menu_index = child_menu_index;
  }
  return open ? 1 : 0;
}

void mdgui_end_main_submenu(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  if (ctx->main_menu_build_stack.size() > 1) {
    ctx->main_menu_build_stack.pop_back();
    ctx->building_main_menu_index = ctx->main_menu_build_stack.empty()
                                        ? -1
                                        : ctx->main_menu_build_stack.back();
  }
}

void mdgui_end_main_menu(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  ctx->in_main_menu = false;
  ctx->building_main_menu_index = -1;
  ctx->main_menu_build_stack.clear();
}

void mdgui_end_main_menu_bar(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  ctx->in_main_menu_bar = false;
  ctx->in_main_menu = false;
  ctx->main_menu_build_stack.clear();
}

void mdgui_set_status_bar_visible(MDGUI_Context *ctx, int visible) {
  if (!ctx)
    return;
  ctx->status_bar_visible = (visible != 0);
}

int mdgui_is_status_bar_visible(MDGUI_Context *ctx) {
  if (!ctx)
    return 0;
  return ctx->status_bar_visible ? 1 : 0;
}

void mdgui_set_status_bar_text(MDGUI_Context *ctx, const char *text) {
  if (!ctx)
    return;
  ctx->status_bar_text = text ? text : "";
}

const char *mdgui_get_status_bar_text(MDGUI_Context *ctx) {
  if (!ctx)
    return "";
  return ctx->status_bar_text.c_str();
}

void mdgui_show_toast(MDGUI_Context *ctx, const char *text, int duration_ms) {
  if (!ctx || !text || !*text)
    return;
  if (duration_ms <= 0)
    duration_ms = TOAST_DEFAULT_DURATION_MS;
  const unsigned long long now_ms = mdgui_backend_get_ticks_ms();
  prune_expired_toasts(ctx, now_ms);

  MDGUI_Context::ToastItem toast;
  toast.text = text;
  toast.expires_at_ms = now_ms + (unsigned long long)duration_ms;
  ctx->toasts.push_back(std::move(toast));
}

void mdgui_clear_toasts(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  ctx->toasts.clear();
}

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
  if (ctx->current_window >= 0) {
    auto &msg_win = ctx->windows[ctx->current_window];
    msg_win.z = ++ctx->z_counter;
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

  const auto &win = ctx->windows[ctx->current_window];
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
  const int saved_content_y = ctx->content_y;
  const int saved_indent = ctx->layout_indent;
  const bool saved_has_last = ctx->layout_has_last_item;
  const bool saved_same_line = ctx->layout_same_line;
  const int button_logical_y = btn_abs_y + win.text_scroll;
  if (style == MDGUI_MSGBOX_TWO_BUTTON) {
    const int b1_abs_x = win.x + (win.w / 4) - (btn_w / 2);
    const int b2_abs_x = win.x + ((win.w * 3) / 4) - (btn_w / 2);
    ctx->content_y = button_logical_y;
    ctx->layout_indent = b1_abs_x - ctx->origin_x;
    ctx->layout_has_last_item = false;
    ctx->layout_same_line = false;
    if (mdgui_button(ctx, button1, btn_w, btn_h))
      result = 1;
    ctx->content_y = button_logical_y;
    ctx->layout_indent = b2_abs_x - ctx->origin_x;
    ctx->layout_has_last_item = false;
    ctx->layout_same_line = false;
    if (mdgui_button(ctx, button2, btn_w, btn_h))
      result = 2;
  } else {
    const int b_abs_x = win.x + (win.w / 2) - (btn_w / 2);
    ctx->content_y = button_logical_y;
    ctx->layout_indent = b_abs_x - ctx->origin_x;
    ctx->layout_has_last_item = false;
    ctx->layout_same_line = false;
    if (mdgui_button(ctx, button1, btn_w, btn_h))
      result = 1;
  }
  ctx->content_y = saved_content_y;
  ctx->layout_indent = saved_indent;
  ctx->layout_has_last_item = saved_has_last;
  ctx->layout_same_line = saved_same_line;

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
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    if (ctx->windows[i].id == title)
      return ctx->windows[i].z;
  }
  return -1;
}

void mdgui_set_window_open(MDGUI_Context *ctx, const char *title, int open) {
  if (!ctx || !title)
    return;
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    if (ctx->windows[i].id == title) {
      auto &win = ctx->windows[i];
      win.closed = (open == 0);
      if (open) {
        win.z = ++ctx->z_counter;
      } else {
        win.open_menu_index = -1;
        win.open_menu_path.clear();
        win.menu_overlay_defs.clear();
        win.text_scroll_dragging = false;
        if (ctx->current_window == i)
          ctx->current_window = -1;
        if (ctx->dragging_window == i)
          ctx->dragging_window = -1;
        if (ctx->resizing_window == i)
          ctx->resizing_window = -1;
        if (ctx->active_text_input_window == i) {
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
        if (ctx->combo_overlay_window == i) {
          ctx->combo_overlay_pending = false;
          ctx->combo_overlay_window = -1;
          ctx->combo_overlay_items = nullptr;
        }
        if (ctx->combo_capture_window == i) {
          ctx->combo_capture_active = false;
          ctx->combo_capture_seen_this_frame = false;
          ctx->combo_capture_window = -1;
          ctx->combo_capture_x = 0;
          ctx->combo_capture_y = 0;
          ctx->combo_capture_w = 0;
          ctx->combo_capture_h = 0;
        }
      }
      return;
    }
  }
}

int mdgui_is_window_open(MDGUI_Context *ctx, const char *title) {
  if (!ctx || !title)
    return 0;
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    if (ctx->windows[i].id == title)
      return ctx->windows[i].closed ? 0 : 1;
  }
  return 0;
}

void mdgui_focus_window(MDGUI_Context *ctx, const char *title) {
  if (!ctx || !title)
    return;
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    if (ctx->windows[i].id == title && !ctx->windows[i].closed) {
      ctx->windows[i].z = ++ctx->z_counter;
      return;
    }
  }
}

void mdgui_set_window_rect(MDGUI_Context *ctx, const char *title, int x, int y,
                           int w, int h) {
  if (!ctx || !title)
    return;
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    auto &win = ctx->windows[i];
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
  for (auto &win : ctx->windows) {
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
  return (int)ctx->windows.size();
}

int mdgui_get_window_rect_by_index(MDGUI_Context *ctx, int index, int *x, int *y,
                                   int *w, int *h, int *z, int *open) {
  if (!ctx || index < 0 || index >= (int)ctx->windows.size())
    return 0;
  const auto &win = ctx->windows[(size_t)index];
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
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    auto &win = ctx->windows[i];
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
    if (ctx->tile_manager_enabled)
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

  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    auto &win = ctx->windows[i];
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
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    auto &win = ctx->windows[i];
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
    if (ctx->tile_manager_enabled)
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

  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    const auto &win = ctx->windows[i];
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
  ctx->windows_locked = (locked != 0);
  if (ctx->windows_locked) {
    ctx->dragging_window = -1;
    ctx->resizing_window = -1;
  }
}

int mdgui_is_windows_locked(MDGUI_Context *ctx) {
  if (!ctx)
    return 0;
  return ctx->windows_locked ? 1 : 0;
}

void mdgui_set_window_alpha(MDGUI_Context *ctx, const char *title,
                            unsigned char alpha) {
  if (!ctx || !title)
    return;
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    auto &win = ctx->windows[i];
    if (win.id != title)
      continue;
    win.alpha = alpha;
    return;
  }
}

unsigned char mdgui_get_window_alpha(MDGUI_Context *ctx, const char *title) {
  if (!ctx || !title)
    return 255;
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    const auto &win = ctx->windows[i];
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
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    auto &win = ctx->windows[i];
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
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    const auto &win = ctx->windows[i];
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
  ctx->default_window_alpha = alpha;
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    ctx->windows[i].alpha = alpha;
  }
}

unsigned char mdgui_get_windows_alpha(MDGUI_Context *ctx) {
  if (!ctx)
    return 255;
  return ctx->default_window_alpha;
}

void mdgui_set_tile_manager_enabled(MDGUI_Context *ctx, int enabled) {
  if (!ctx)
    return;
  ctx->tile_manager_enabled = (enabled != 0);
  if (ctx->tile_manager_enabled) {
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
  return ctx->tile_manager_enabled ? 1 : 0;
}

void mdgui_tile_windows(MDGUI_Context *ctx) { tile_windows_internal(ctx); }

void mdgui_set_window_tile_weight(MDGUI_Context *ctx, const char *title,
                                  int weight) {
  if (!ctx || !title)
    return;
  const int normalized = normalize_tile_weight(weight);
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    auto &w = ctx->windows[i];
    if (w.id != title)
      continue;
    w.tile_weight = normalized;
    return;
  }
}

int mdgui_get_window_tile_weight(MDGUI_Context *ctx, const char *title) {
  if (!ctx || !title)
    return 1;
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    const auto &w = ctx->windows[i];
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
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    auto &w = ctx->windows[i];
    if (w.id != title)
      continue;
    w.tile_side = normalized;
    return;
  }
}

int mdgui_get_window_tile_side(MDGUI_Context *ctx, const char *title) {
  if (!ctx || !title)
    return MDGUI_TILE_SIDE_AUTO;
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    const auto &w = ctx->windows[i];
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
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    auto &w = ctx->windows[i];
    if (w.id != title)
      continue;
    w.tile_excluded = normalized;
    if (ctx->tile_manager_enabled)
      tile_windows_internal(ctx);
    return;
  }
}

int mdgui_is_window_tile_excluded(MDGUI_Context *ctx, const char *title) {
  if (!ctx || !title)
    return 0;
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    const auto &w = ctx->windows[i];
    if (w.id == title)
      return w.tile_excluded ? 1 : 0;
  }
  return 0;
}

void mdgui_set_window_fullscreen(MDGUI_Context *ctx, const char *title,
                                 int fullscreen) {
  if (!ctx || !title)
    return;
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    auto &w = ctx->windows[i];
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
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    if (ctx->windows[i].id == title)
      return ctx->windows[i].is_maximized ? 1 : 0;
  }
  return 0;
}

void mdgui_set_custom_cursor_enabled(MDGUI_Context *ctx, int enabled) {
  if (!ctx)
    return;
  ctx->custom_cursor_enabled = (enabled != 0);
}

int mdgui_is_custom_cursor_enabled(MDGUI_Context *ctx) {
  if (!ctx)
    return 0;
  return ctx->custom_cursor_enabled ? 1 : 0;
}

