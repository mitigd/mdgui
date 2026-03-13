#include "../core/windowing_internal.hpp"

extern "C" {
void mdgui_begin_menu_bar(MDGUI_Context *ctx) {
  if (!ctx || ctx->window.current_window < 0)
    return;
  ctx->layout.window_has_nonlabel_widget = true;
  const auto &win = ctx->window.windows[ctx->window.current_window];
  const bool had_menu_bar = ctx->menus.has_menu_bar;
  const int bar_x = win.x + 1;
  const int bar_y = ctx->window.origin_y;
  const int bar_w = win.w - 2;
  const int bar_h = 10;
  mdgui_backend_set_clip_rect(0, 0, 0, 0, 0);
  mdgui_fill_rect_idx(nullptr, CLR_MENU_BG, bar_x, bar_y, bar_w, bar_h);
  ctx->menus.menu_index = 0;
  ctx->menus.menu_next_x = win.x + 4;
  ctx->menus.in_menu_bar = true;
  ctx->menus.in_menu = false;
  ctx->menus.building_menu_index = -1;
  ctx->menus.has_menu_bar = true;
  ctx->menus.menu_bar_x = bar_x;
  ctx->menus.menu_bar_y = bar_y;
  ctx->menus.menu_bar_w = bar_w;
  ctx->menus.menu_bar_h = bar_h;
  ctx->menus.menu_defs.clear();
  ctx->menus.menu_defs.resize(RESERVED_TOP_LEVEL_MENU_SLOTS);
  ctx->menus.menu_build_stack.clear();
  if (!had_menu_bar) {
    const int content_top = bar_y + bar_h + 2 + ctx->layout.style.content_pad_y;
    ctx->window.origin_y = content_top;
    if (ctx->window.content_y < content_top)
      ctx->window.content_y = content_top;
  }
}

int mdgui_begin_menu(MDGUI_Context *ctx, const char *text) {
  if (!ctx || !ctx->menus.in_menu_bar || ctx->window.current_window < 0 || !text)
    return 0;
  auto &win = ctx->window.windows[ctx->window.current_window];

  const int tw = mdgui_fonts[1] ? mdgui_fonts[1]->measureTextWidth(text) : 40;
  const int item_w = tw + 10;
  const int x = ctx->menus.menu_next_x;
  const int y = ctx->menus.menu_bar_y;
  const int hovered =
      point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, x, y, item_w, 10);
  const bool menu_interactable =
      (win.z == ctx->window.z_counter) || !win.open_menu_path.empty();

  if ((int)ctx->menus.menu_defs.size() <= ctx->menus.menu_index) {
    ctx->menus.menu_defs.resize(ctx->menus.menu_index + 1);
  }
  auto &def = ctx->menus.menu_defs[ctx->menus.menu_index];
  def.x = x;
  def.y = y + 10 + MENU_POPUP_GAP_Y;
  def.w = (tw + 40 < 120) ? 120 : (tw + 40);
  def.parent_menu_index = -1;
  def.parent_item_index = -1;
  def.items.clear();

  if (!win.open_menu_path.empty() && hovered && menu_interactable) {
    win.open_menu_path.resize(1);
    win.open_menu_path[0] = ctx->menus.menu_index;
    win.open_menu_index = ctx->menus.menu_index;
  }

  if (ctx->render.input.mouse_pressed && hovered && menu_interactable) {
    win.z = ++ctx->window.z_counter;
    if (win.open_menu_path.size() == 1 &&
        win.open_menu_path[0] == ctx->menus.menu_index) {
      clear_window_menu_path(win);
    } else {
      win.open_menu_path.resize(1);
      win.open_menu_path[0] = ctx->menus.menu_index;
      win.open_menu_index = ctx->menus.menu_index;
    }
    ctx->render.input.mouse_pressed = 0;
  }

  const int open =
      (!win.open_menu_path.empty() && win.open_menu_path[0] == ctx->menus.menu_index);
  if (open || hovered) {
    mdgui_fill_rect_idx(nullptr, CLR_ACCENT, x, y, item_w, 10);
  }
  if (mdgui_fonts[1]) {
    mdgui_fonts[1]->drawText(text, nullptr, x + 3, y + 1, CLR_MENU_TEXT);
  }

  if (open) {
    ctx->menus.in_menu = true;
    ctx->menus.current_menu_index = ctx->menus.menu_index;
    ctx->menus.building_menu_index = ctx->menus.menu_index;
    ctx->menus.current_menu_x = def.x;
    ctx->menus.current_menu_y = def.y;
    ctx->menus.current_menu_w = def.w;
    ctx->menus.current_menu_h = 10;
    ctx->menus.current_menu_item = 0;
    ctx->menus.menu_build_stack.clear();
    ctx->menus.menu_build_stack.push_back(ctx->menus.menu_index);
  }

  ctx->menus.menu_next_x += item_w + 2;
  ctx->menus.menu_index += 1;
  return open;
}

int mdgui_menu_item(MDGUI_Context *ctx, const char *text) {
  if (!ctx || !ctx->menus.in_menu || !text || ctx->menus.menu_build_stack.empty())
    return 0;
  auto &win = ctx->window.windows[ctx->window.current_window];
  const bool menu_interactable =
      (win.z == ctx->window.z_counter) || !win.open_menu_path.empty();
  const int menu_idx = ctx->menus.menu_build_stack.back();
  auto &def = ctx->menus.menu_defs[menu_idx];
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
    reposition_child_menu_chain(ctx->menus.menu_defs, menu_idx, ctx->menus.current_menu_h);
  }
  def.items.push_back({text, -1, false});
  const int item_index = (int)def.items.size() - 1;
  const int item_y = def.y + (item_index * ctx->menus.current_menu_h);

  const int hovered = point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y,
                                    def.x, item_y, def.w, ctx->menus.current_menu_h);
  const int submenu_depth = (int)ctx->menus.menu_build_stack.size();
  if (hovered && menu_interactable &&
      menu_path_prefix_matches(win.open_menu_path, ctx->menus.menu_build_stack) &&
      (int)win.open_menu_path.size() > submenu_depth) {
    win.open_menu_path.resize(submenu_depth);
    win.open_menu_index =
        win.open_menu_path.empty() ? -1 : win.open_menu_path[0];
  }
  if (hovered && ctx->render.input.mouse_pressed && menu_interactable) {
    win.z = ++ctx->window.z_counter;
    clear_window_menu_path(win);
    ctx->render.input.mouse_pressed = 0;
    return 1;
  }
  return 0;
}

void mdgui_menu_separator(MDGUI_Context *ctx) {
  if (!ctx || !ctx->menus.in_menu || ctx->menus.menu_build_stack.empty())
    return;
  auto &def = ctx->menus.menu_defs[ctx->menus.menu_build_stack.back()];
  def.items.push_back({"", -1, true});
}

int mdgui_begin_submenu(MDGUI_Context *ctx, const char *text) {
  if (!ctx || !ctx->menus.in_menu || !text || ctx->menus.menu_build_stack.empty() ||
      ctx->window.current_window < 0)
    return 0;
  auto &win = ctx->window.windows[ctx->window.current_window];
  const bool menu_interactable =
      (win.z == ctx->window.z_counter) || !win.open_menu_path.empty();
  const int parent_menu_index = ctx->menus.menu_build_stack.back();
  auto &parent = ctx->menus.menu_defs[parent_menu_index];
  bool has_check = false;
  bool checked = false;
  const char *label = parse_menu_check_prefix(text, &has_check, &checked);
  (void)checked;
  const int check_w = has_check ? menu_check_indicator_width() : 0;
  const int item_h = ctx->menus.current_menu_h;
  const int item_text_w =
      mdgui_fonts[1] ? mdgui_fonts[1]->measureTextWidth(label) : 40;
  const int item_needed_w = item_text_w + 20 + check_w;
  if (item_needed_w > parent.w) {
    parent.w = item_needed_w;
    reposition_child_menu_chain(ctx->menus.menu_defs, parent_menu_index, item_h);
  }
  parent.items.push_back({text, -1, false});
  const int item_index = (int)parent.items.size() - 1;
  const int item_y = parent.y + (item_index * item_h);

  const int child_menu_index = (int)ctx->menus.menu_defs.size();
  parent.items[item_index].child_menu_index = child_menu_index;
  const int parent_x = parent.x;
  const int parent_w = parent.w;
  ctx->menus.menu_defs.push_back({});
  auto &child = ctx->menus.menu_defs.back();
  child.x = parent_x + parent_w + MENU_POPUP_GAP_Y;
  child.y = item_y;
  child.w = (item_text_w + 40 < 120) ? 120 : (item_text_w + 40);
  child.parent_menu_index = parent_menu_index;
  child.parent_item_index = item_index;
  child.items.clear();

  const auto &parent_after = ctx->menus.menu_defs[parent_menu_index];

  const int hovered =
      point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, parent_after.x,
                    item_y, parent_after.w, item_h);
  const int submenu_depth = (int)ctx->menus.menu_build_stack.size();
  const bool was_open =
      menu_path_prefix_matches(win.open_menu_path, ctx->menus.menu_build_stack) &&
      (int)win.open_menu_path.size() > submenu_depth &&
      win.open_menu_path[submenu_depth] == child_menu_index;
  if (ctx->render.input.mouse_pressed && hovered && menu_interactable) {
    win.z = ++ctx->window.z_counter;
    if (was_open && (int)win.open_menu_path.size() == submenu_depth + 1) {
      win.open_menu_path.resize(submenu_depth);
      win.open_menu_index =
          win.open_menu_path.empty() ? -1 : win.open_menu_path[0];
    } else {
      win.open_menu_path.resize((size_t)submenu_depth + 1);
      for (int i = 0; i < submenu_depth; ++i)
        win.open_menu_path[i] = ctx->menus.menu_build_stack[i];
      win.open_menu_path[submenu_depth] = child_menu_index;
      win.open_menu_index =
          win.open_menu_path.empty() ? -1 : win.open_menu_path[0];
    }
    ctx->render.input.mouse_pressed = 0;
  } else if (hovered && menu_interactable) {
    win.open_menu_path.resize((size_t)submenu_depth + 1);
    for (int i = 0; i < submenu_depth; ++i)
      win.open_menu_path[i] = ctx->menus.menu_build_stack[i];
    win.open_menu_path[submenu_depth] = child_menu_index;
    win.open_menu_index =
        win.open_menu_path.empty() ? -1 : win.open_menu_path[0];
  }

  const bool open =
      menu_path_prefix_matches(win.open_menu_path, ctx->menus.menu_build_stack) &&
      (int)win.open_menu_path.size() > submenu_depth &&
      win.open_menu_path[submenu_depth] == child_menu_index;
  if (open) {
    ctx->menus.menu_build_stack.push_back(child_menu_index);
    ctx->menus.building_menu_index = child_menu_index;
  }
  return open ? 1 : 0;
}

void mdgui_end_submenu(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  if (ctx->menus.menu_build_stack.size() > 1) {
    ctx->menus.menu_build_stack.pop_back();
    ctx->menus.building_menu_index =
        ctx->menus.menu_build_stack.empty() ? -1 : ctx->menus.menu_build_stack.back();
  }
}

void mdgui_end_menu(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  ctx->menus.in_menu = false;
  ctx->menus.building_menu_index = -1;
  ctx->menus.menu_build_stack.clear();
}

void mdgui_end_menu_bar(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  ctx->menus.in_menu_bar = false;
  ctx->menus.in_menu = false;
  ctx->menus.menu_build_stack.clear();
  set_content_clip(ctx);
}

void mdgui_begin_main_menu_bar(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  const int rw = get_logical_render_w(ctx);
  ctx->menus.main_menu_bar_x = 0;
  ctx->menus.main_menu_bar_y = 0;
  ctx->menus.main_menu_bar_w = rw;
  ctx->menus.main_menu_bar_h = 10;

  mdgui_fill_rect_idx(nullptr, CLR_MENU_BG, ctx->menus.main_menu_bar_x,
                      ctx->menus.main_menu_bar_y, ctx->menus.main_menu_bar_w,
                      ctx->menus.main_menu_bar_h);
  ctx->menus.main_menu_index = 0;
  ctx->menus.main_menu_next_x = 3;
  ctx->menus.in_main_menu_bar = true;
  ctx->menus.in_main_menu = false;
  ctx->menus.building_main_menu_index = -1;
  ctx->menus.main_menu_defs.clear();
  ctx->menus.main_menu_defs.resize(RESERVED_TOP_LEVEL_MENU_SLOTS);
  ctx->menus.main_menu_build_stack.clear();
}

int mdgui_begin_main_menu(MDGUI_Context *ctx, const char *text) {
  if (!ctx || !ctx->menus.in_main_menu_bar || !text)
    return 0;

  const int tw = mdgui_fonts[1] ? mdgui_fonts[1]->measureTextWidth(text) : 40;
  const int item_w = tw + 10;
  const int x = ctx->menus.main_menu_next_x;
  const int y = ctx->menus.main_menu_bar_y;
  const int hovered = point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, x,
                                    y, item_w, ctx->menus.main_menu_bar_h);

  if ((int)ctx->menus.main_menu_defs.size() <= ctx->menus.main_menu_index) {
    ctx->menus.main_menu_defs.resize(ctx->menus.main_menu_index + 1);
  }
  auto &def = ctx->menus.main_menu_defs[ctx->menus.main_menu_index];
  def.x = x;
  def.y = y + ctx->menus.main_menu_bar_h + MENU_POPUP_GAP_Y;
  def.w = (tw + 40 < 120) ? 120 : (tw + 40);
  def.parent_menu_index = -1;
  def.parent_item_index = -1;
  def.items.clear();

  if (!ctx->menus.open_main_menu_path.empty() && hovered) {
    ctx->menus.open_main_menu_path.resize(1);
    ctx->menus.open_main_menu_path[0] = ctx->menus.main_menu_index;
    ctx->menus.open_main_menu_item_path.resize(1);
    ctx->menus.open_main_menu_item_path[0] = ctx->menus.main_menu_index;
    ctx->menus.open_main_menu_index = ctx->menus.main_menu_index;
  }
  if (ctx->render.input.mouse_pressed && hovered) {
    if (ctx->menus.open_main_menu_path.size() == 1 &&
        ctx->menus.open_main_menu_path[0] == ctx->menus.main_menu_index) {
      ctx->menus.open_main_menu_path.clear();
      ctx->menus.open_main_menu_item_path.clear();
      ctx->menus.open_main_menu_index = -1;
    } else {
      ctx->menus.open_main_menu_path.resize(1);
      ctx->menus.open_main_menu_path[0] = ctx->menus.main_menu_index;
      ctx->menus.open_main_menu_item_path.resize(1);
      ctx->menus.open_main_menu_item_path[0] = ctx->menus.main_menu_index;
      ctx->menus.open_main_menu_index = ctx->menus.main_menu_index;
    }
    ctx->render.input.mouse_pressed = 0;
  }

  const int open = (!ctx->menus.open_main_menu_path.empty() &&
                    ctx->menus.open_main_menu_path[0] == ctx->menus.main_menu_index);
  if (open || hovered) {
    mdgui_fill_rect_idx(nullptr, CLR_ACCENT, x, y, item_w,
                        ctx->menus.main_menu_bar_h);
  }
  if (mdgui_fonts[1]) {
    mdgui_fonts[1]->drawText(text, nullptr, x + 3, y + 1, CLR_MENU_TEXT);
  }

  if (open) {
    ctx->menus.in_main_menu = true;
    ctx->menus.building_main_menu_index = ctx->menus.main_menu_index;
    ctx->menus.current_menu_x = def.x;
    ctx->menus.current_menu_y = def.y;
    ctx->menus.current_menu_w = def.w;
    ctx->menus.current_menu_h = 10;
    ctx->menus.current_menu_item = 0;
    ctx->menus.main_menu_build_stack.clear();
    ctx->menus.main_menu_build_stack.push_back(ctx->menus.main_menu_index);
  }

  ctx->menus.main_menu_next_x += item_w + 2;
  ctx->menus.main_menu_index += 1;
  return open;
}

int mdgui_main_menu_item(MDGUI_Context *ctx, const char *text) {
  if (!ctx || !ctx->menus.in_main_menu || !text || ctx->menus.main_menu_build_stack.empty())
    return 0;

  bool has_check = false;
  bool checked = false;
  const char *label = parse_menu_check_prefix(text, &has_check, &checked);
  (void)checked;
  const int check_w = has_check ? menu_check_indicator_width() : 0;
  const int menu_idx = ctx->menus.main_menu_build_stack.back();
  auto &def = ctx->menus.main_menu_defs[menu_idx];
  const int item_text_w =
      mdgui_fonts[1] ? mdgui_fonts[1]->measureTextWidth(label) : 40;
  const int item_needed_w = item_text_w + 12 + check_w;
  if (item_needed_w > def.w) {
    def.w = item_needed_w;
    reposition_child_menu_chain(ctx->menus.main_menu_defs, menu_idx,
                                MAIN_MENU_ITEM_H);
  }
  def.items.push_back({text, -1, false});
  const int item_index = (int)def.items.size() - 1;
  const int item_y = def.y + (item_index * MAIN_MENU_ITEM_H);
  const int hovered = point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y,
                                    def.x, item_y, def.w, MAIN_MENU_ITEM_H);
  const int submenu_depth = (int)ctx->menus.main_menu_build_stack.size();
  if (hovered &&
      menu_path_prefix_matches(ctx->menus.open_main_menu_path,
                               ctx->menus.main_menu_build_stack) &&
      (int)ctx->menus.open_main_menu_path.size() > submenu_depth) {
    ctx->menus.open_main_menu_path.resize(submenu_depth);
    if ((int)ctx->menus.open_main_menu_item_path.size() > submenu_depth) {
      ctx->menus.open_main_menu_item_path.resize(submenu_depth);
    }
    ctx->menus.open_main_menu_index =
        ctx->menus.open_main_menu_path.empty() ? -1 : ctx->menus.open_main_menu_path[0];
  }

  if (hovered && ctx->render.input.mouse_pressed) {
    ctx->menus.open_main_menu_path.clear();
    ctx->menus.open_main_menu_item_path.clear();
    ctx->menus.open_main_menu_index = -1;
    ctx->render.input.mouse_pressed = 0;
    return 1;
  }
  return 0;
}

void mdgui_main_menu_separator(MDGUI_Context *ctx) {
  if (!ctx || !ctx->menus.in_main_menu || ctx->menus.main_menu_build_stack.empty())
    return;
  auto &def = ctx->menus.main_menu_defs[ctx->menus.main_menu_build_stack.back()];
  def.items.push_back({"", -1, true});
}

int mdgui_begin_main_submenu(MDGUI_Context *ctx, const char *text) {
  if (!ctx || !ctx->menus.in_main_menu || !text || ctx->menus.main_menu_build_stack.empty())
    return 0;
  bool has_check = false;
  bool checked = false;
  const char *label = parse_menu_check_prefix(text, &has_check, &checked);
  (void)checked;
  const int check_w = has_check ? menu_check_indicator_width() : 0;
  const int parent_menu_index = ctx->menus.main_menu_build_stack.back();
  auto &parent = ctx->menus.main_menu_defs[parent_menu_index];
  const int item_h = MAIN_MENU_ITEM_H;
  const int item_text_w =
      mdgui_fonts[1] ? mdgui_fonts[1]->measureTextWidth(label) : 40;
  const int item_needed_w = item_text_w + 20 + check_w;
  if (item_needed_w > parent.w) {
    parent.w = item_needed_w;
    reposition_child_menu_chain(ctx->menus.main_menu_defs, parent_menu_index, item_h);
  }
  parent.items.push_back({text, -1, false});
  const int item_index = (int)parent.items.size() - 1;
  const int item_y = parent.y + (item_index * item_h);

  const int child_menu_index = (int)ctx->menus.main_menu_defs.size();
  parent.items[item_index].child_menu_index = child_menu_index;
  const int parent_x = parent.x;
  const int parent_w = parent.w;
  ctx->menus.main_menu_defs.push_back({});
  auto &child = ctx->menus.main_menu_defs.back();
  child.x = parent_x + parent_w + MENU_POPUP_GAP_Y;
  child.y = item_y;
  child.w = (item_text_w + 40 < 120) ? 120 : (item_text_w + 40);
  child.parent_menu_index = parent_menu_index;
  child.parent_item_index = item_index;
  child.items.clear();

  const auto &parent_after = ctx->menus.main_menu_defs[parent_menu_index];

  const int hovered =
      point_in_rect(ctx->render.input.mouse_x, ctx->render.input.mouse_y, parent_after.x,
                    item_y, parent_after.w, item_h);
  const int submenu_depth = (int)ctx->menus.main_menu_build_stack.size();
  auto ensure_open_paths_for_item = [&](int child_idx) {
    if ((int)ctx->menus.open_main_menu_path.size() < submenu_depth + 1) {
      ctx->menus.open_main_menu_path.resize((size_t)submenu_depth + 1);
    }
    for (int i = 0; i < submenu_depth; ++i) {
      ctx->menus.open_main_menu_path[i] = ctx->menus.main_menu_build_stack[i];
    }
    ctx->menus.open_main_menu_path[submenu_depth] = child_idx;

    if ((int)ctx->menus.open_main_menu_item_path.size() < submenu_depth + 1) {
      ctx->menus.open_main_menu_item_path.resize((size_t)submenu_depth + 1, -1);
    }
    for (int i = 0; i < submenu_depth; ++i) {
      if (i < (int)ctx->menus.open_main_menu_item_path.size() &&
          ctx->menus.open_main_menu_item_path[i] >= 0) {
        continue;
      }
      ctx->menus.open_main_menu_item_path[i] = ctx->menus.main_menu_build_stack[i];
    }
    ctx->menus.open_main_menu_item_path[submenu_depth] = item_index;
    ctx->menus.open_main_menu_index =
        ctx->menus.open_main_menu_path.empty() ? -1 : ctx->menus.open_main_menu_path[0];
  };

  const bool was_open =
      menu_path_prefix_matches(ctx->menus.open_main_menu_item_path,
                               ctx->menus.main_menu_build_stack) &&
      (int)ctx->menus.open_main_menu_item_path.size() > submenu_depth &&
      ctx->menus.open_main_menu_item_path[submenu_depth] == item_index;
  if (ctx->render.input.mouse_pressed && hovered) {
    if (was_open &&
        (int)ctx->menus.open_main_menu_item_path.size() == submenu_depth + 1) {
      ctx->menus.open_main_menu_path.resize(submenu_depth);
      if ((int)ctx->menus.open_main_menu_item_path.size() > submenu_depth) {
        ctx->menus.open_main_menu_item_path.resize(submenu_depth);
      }
      ctx->menus.open_main_menu_index =
          ctx->menus.open_main_menu_path.empty() ? -1 : ctx->menus.open_main_menu_path[0];
    } else {
      ensure_open_paths_for_item(child_menu_index);
    }
    ctx->render.input.mouse_pressed = 0;
  } else if (hovered) {
    ensure_open_paths_for_item(child_menu_index);
  }

  const bool open =
      menu_path_prefix_matches(ctx->menus.open_main_menu_item_path,
                               ctx->menus.main_menu_build_stack) &&
      (int)ctx->menus.open_main_menu_item_path.size() > submenu_depth &&
      ctx->menus.open_main_menu_item_path[submenu_depth] == item_index;
  if (open) {
    ensure_open_paths_for_item(child_menu_index);
    ctx->menus.main_menu_build_stack.push_back(child_menu_index);
    ctx->menus.building_main_menu_index = child_menu_index;
  }
  return open ? 1 : 0;
}

void mdgui_end_main_submenu(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  if (ctx->menus.main_menu_build_stack.size() > 1) {
    ctx->menus.main_menu_build_stack.pop_back();
    ctx->menus.building_main_menu_index = ctx->menus.main_menu_build_stack.empty()
                                        ? -1
                                        : ctx->menus.main_menu_build_stack.back();
  }
}

void mdgui_end_main_menu(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  ctx->menus.in_main_menu = false;
  ctx->menus.building_main_menu_index = -1;
  ctx->menus.main_menu_build_stack.clear();
}

void mdgui_end_main_menu_bar(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  ctx->menus.in_main_menu_bar = false;
  ctx->menus.in_main_menu = false;
  ctx->menus.main_menu_build_stack.clear();
}

void mdgui_set_status_bar_visible(MDGUI_Context *ctx, int visible) {
  if (!ctx)
    return;
  ctx->menus.status_bar_visible = (visible != 0);
}

int mdgui_is_status_bar_visible(MDGUI_Context *ctx) {
  if (!ctx)
    return 0;
  return ctx->menus.status_bar_visible ? 1 : 0;
}

void mdgui_set_status_bar_text(MDGUI_Context *ctx, const char *text) {
  if (!ctx)
    return;
  ctx->menus.status_bar_text = text ? text : "";
}

const char *mdgui_get_status_bar_text(MDGUI_Context *ctx) {
  if (!ctx)
    return "";
  return ctx->menus.status_bar_text.c_str();
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
  ctx->menus.toasts.push_back(std::move(toast));
}

void mdgui_clear_toasts(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  ctx->menus.toasts.clear();
}

}


