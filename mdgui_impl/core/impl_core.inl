#include <filesystem>
#include <string.h>
#include <string>
#include <utility>
#include <vector>

extern MDGuiFont *mdgui_fonts[10];

namespace {
constexpr int CLR_BUTTON_SURFACE = 25;
constexpr int CLR_BUTTON_LIGHT = 23;
constexpr int CLR_BUTTON_DARK = 27;
constexpr int CLR_BUTTON_PRESSED = 27;
constexpr int CLR_BOX_BODY = 141;
constexpr int CLR_BOX_TITLE = 143;
constexpr int CLR_BOX_FOCUS_BORDER = 139;
constexpr int CLR_MENU_BG = 241;
constexpr int CLR_MENU_TEXT = 246;
constexpr int CLR_MENU_SEL = 2;
constexpr int CLR_TEXT_LIGHT = 15;
constexpr int CLR_TEXT_DARK = 0;
constexpr int CLR_MSG_BG = 243;
constexpr int CLR_MSG_BAR = 244;
constexpr int CLR_ACCENT = 247;
constexpr int CLR_WINDOW_BORDER = 248;
constexpr int MENU_POPUP_GAP_Y = 2;
constexpr int RESERVED_TOP_LEVEL_MENU_SLOTS = 32;
constexpr int STATUS_BAR_DEFAULT_H = 12;
constexpr int MAIN_MENU_ITEM_H = 10;
constexpr int TOAST_DEFAULT_DURATION_MS = 2400;
constexpr int TOAST_MAX_VISIBLE = 6;

struct MDGUI_Window {
  struct MenuOverlayDef {
    struct ItemDef {
      std::string text;
      int child_menu_index;
      bool is_separator;
    };
    int x;
    int y;
    int w;
    int parent_menu_index;
    int parent_item_index;
    std::vector<ItemDef> items;
  };

  struct CollapsingState {
    std::string id;
    bool open;
  };

  struct TabBarState {
    std::string id;
    int scroll_x;
    int last_selected;
  };

  std::string id;
  std::string title;
  int x;
  int y;
  int w;
  int h;
  int z;
  int open_menu_index;
  bool is_maximized;
  bool closed;
  int restored_x, restored_y, restored_w, restored_h;
  int last_edge_mask;
  int min_w;
  int min_h;
  int user_min_w;
  int user_min_h;
  bool user_min_from_percent;
  float user_min_w_percent;
  float user_min_h_percent;
  int open_combo_id;
  std::vector<int> open_menu_path;
  std::vector<MenuOverlayDef> menu_overlay_defs;
  int menu_overlay_item_h;
  int text_scroll;
  bool text_scroll_dragging;
  int text_scroll_drag_offset;
  bool fixed_rect;
  bool disallow_maximize;
  bool is_message_box;
  bool scrollbar_visible;
  bool scrollbar_overflow_active;
  int tile_weight;
  int tile_side;
  bool tile_excluded;
  unsigned char alpha;
  bool no_chrome;
  bool input_passthrough;
  int chrome_min_w;
  int chrome_min_h;
  std::vector<CollapsingState> collapsing_states;
  std::vector<TabBarState> tab_bar_states;
};

struct FileBrowserEntry {
  std::string label;
  std::string full_path;
  bool is_dir;
};

struct PendingWindowMinSize {
  std::string title;
  int min_w;
  int min_h;
  bool use_percent;
  float min_w_percent;
  float min_h_percent;
};

struct PendingWindowScrollbarVisibility {
  std::string title;
  bool visible;
};
} // namespace

struct MDGUI_Context {
  struct ToastItem {
    std::string text;
    unsigned long long expires_at_ms;
  };

  struct NestedRenderState {
    int parent_origin_x;
    int parent_origin_y;
    int parent_content_y;
    int parent_content_req_right;
    int parent_content_req_bottom;
    bool parent_window_has_nonlabel_widget;
    unsigned char parent_alpha_mod;
    int parent_content_y_after;
    int parent_layout_indent;
    size_t parent_indent_stack_size;
  };

  struct SubpassState {
    MDGUI_Input parent_input;
    int parent_origin_x;
    int parent_origin_y;
    int parent_content_y;
    int parent_content_req_right;
    int parent_content_req_bottom;
    bool parent_window_has_nonlabel_widget;
    unsigned char parent_alpha_mod;
    int parent_layout_indent;
    size_t parent_indent_stack_size;
    bool parent_layout_same_line;
    bool parent_layout_has_last_item;
    int parent_layout_last_item_x;
    int parent_layout_last_item_y;
    int parent_layout_last_item_w;
    int parent_layout_last_item_h;
    int parent_local_x;
    int parent_logical_y;
    int parent_region_w;
    int parent_region_h;
    int local_w;
    int local_h;
  };

  MDGUI_RenderBackend backend;
  MDGUI_Input input;

  std::vector<MDGUI_Window> windows;
  int z_counter;
  int dragging_window;
  int drag_off_x;
  int drag_off_y;

  int current_window;
  int origin_x;
  int origin_y;
  int content_y;

  int menu_index;
  int menu_next_x;
  bool in_menu_bar;
  bool in_menu;
  int current_menu_index;
  int building_menu_index;
  int current_menu_x;
  int current_menu_y;
  int current_menu_w;
  int current_menu_h;
  int current_menu_item;
  bool has_menu_bar;
  int menu_bar_x;
  int menu_bar_y;
  int menu_bar_w;
  int menu_bar_h;

  struct MenuDef {
    struct ItemDef {
      std::string text;
      int child_menu_index;
      bool is_separator;
    };
    int x;
    int y;
    int w;
    int parent_menu_index;
    int parent_item_index;
    std::vector<ItemDef> items;
  };
  std::vector<MenuDef> menu_defs;
  std::vector<int> menu_build_stack;

  int main_menu_index;
  int main_menu_next_x;
  bool in_main_menu_bar;
  bool in_main_menu;
  int open_main_menu_index;
  int building_main_menu_index;
  std::vector<MenuDef> main_menu_defs;
  std::vector<int> main_menu_build_stack;
  std::vector<int> open_main_menu_path;
  std::vector<int> open_main_menu_item_path;
  int main_menu_bar_x;
  int main_menu_bar_y;
  int main_menu_bar_w;
  int main_menu_bar_h;
  bool status_bar_visible;
  int status_bar_h;
  std::string status_bar_text;
  std::vector<ToastItem> toasts;

  // Resizing state
  int resizing_window; // -1 if none
  int resize_edge_mask;
  int m_start_x, m_start_y;
  int w_start_x, w_start_y, w_start_w, w_start_h;

  // Cursor cache
  SDL_Cursor *cursors[16];
  int cursor_anim_count;
  bool custom_cursor_enabled;
  int cursor_request_idx;
  int current_cursor_idx;

  int content_req_right;
  int content_req_bottom;

  struct LayoutStyle {
    int spacing_x;
    int spacing_y;
    int section_spacing_y;
    int indent_step;
    int label_h;
    int content_pad_x;
    int content_pad_y;
  } style;

  bool layout_same_line;
  bool layout_has_last_item;
  int layout_last_item_x;
  int layout_last_item_y;
  int layout_last_item_w;
  int layout_last_item_h;
  bool layout_columns_active;
  int layout_columns_count;
  int layout_columns_index;
  int layout_columns_start_x;
  int layout_columns_start_y;
  int layout_columns_width;
  int layout_columns_max_bottom;
  std::vector<int> layout_columns_bottoms;
  int layout_indent;
  std::vector<int> indent_stack;
  MDGUI_Font *default_font;
  std::vector<MDGUI_Font *> font_stack;
  MDGUI_Font *file_browser_path_font;
  std::vector<std::string> demo_window_font_labels_storage;
  std::vector<const char *> demo_window_font_labels;
  std::vector<MDGUI_Font *> demo_window_fonts;
  int demo_window_font_index;
  bool window_has_nonlabel_widget;
  bool windows_locked;
  bool tile_manager_enabled;
  unsigned char default_window_alpha;

  bool combo_overlay_pending;
  int combo_overlay_window;
  int combo_overlay_x;
  int combo_overlay_y;
  int combo_overlay_w;
  int combo_overlay_item_h;
  int combo_overlay_item_count;
  int combo_overlay_selected;
  const char **combo_overlay_items;
  bool combo_capture_active;
  bool combo_capture_seen_this_frame;
  int combo_capture_window;
  int combo_capture_x;
  int combo_capture_y;
  int combo_capture_w;
  int combo_capture_h;
  int active_text_input_window;
  int active_text_input_id;
  int active_text_input_cursor;
  int active_text_input_sel_anchor;
  int active_text_input_sel_start;
  int active_text_input_sel_end;
  bool active_text_input_drag_select;
  bool active_text_input_multiline;
  int active_text_input_scroll_y;

  bool file_browser_open;
  bool file_browser_select_folders;
  std::string file_browser_cwd;
  std::vector<FileBrowserEntry> file_browser_entries;
  int file_browser_selected;
  int file_browser_scroll;
  bool file_browser_scroll_dragging;
  int file_browser_scroll_drag_offset;
  std::vector<std::string> pending_tile_excluded_titles;
  std::vector<PendingWindowMinSize> pending_window_min_sizes;
  std::vector<PendingWindowScrollbarVisibility> pending_window_scrollbars;
  int file_browser_last_click_idx;
  Uint64 file_browser_last_click_ticks;
  std::string file_browser_result;
  std::vector<std::string> file_browser_ext_filters;
  std::vector<std::string> file_browser_drives;
  Uint64 file_browser_last_drive_scan_ticks;
  std::string file_browser_last_selected_path;
  bool file_browser_restore_scroll_pending;
  bool file_browser_center_pending;
  int file_browser_open_x;
  int file_browser_open_y;
  std::vector<NestedRenderState> nested_render_stack;
  std::vector<SubpassState> subpass_stack;
  bool file_browser_path_subpass_enabled;
};

static void note_content_bounds(MDGUI_Context *ctx, int right, int bottom) {
  if (!ctx || ctx->current_window < 0)
    return;
  if (right > ctx->content_req_right)
    ctx->content_req_right = right;
  if (bottom > ctx->content_req_bottom)
    ctx->content_req_bottom = bottom;
}

static MDGUI_Font *fallback_font() { return mdgui_fonts[1]; }

static MDGUI_Font *resolve_font(MDGUI_Context *ctx,
                                MDGUI_Font *override_font = nullptr) {
  if (override_font)
    return override_font;
  if (ctx && !ctx->font_stack.empty() && ctx->font_stack.back())
    return ctx->font_stack.back();
  if (ctx && ctx->default_font)
    return ctx->default_font;
  return fallback_font();
}

static int font_line_height(MDGUI_Context *ctx,
                            MDGUI_Font *override_font = nullptr) {
  MDGUI_Font *font = resolve_font(ctx, override_font);
  const int h = mdgui_font_get_line_height(font);
  return (h > 0) ? h : 8;
}

static int font_measure_text(MDGUI_Context *ctx, const char *text,
                             MDGUI_Font *override_font = nullptr) {
  MDGUI_Font *font = resolve_font(ctx, override_font);
  return mdgui_font_measure_text(font, text);
}

static void font_draw_text(MDGUI_Context *ctx, const char *text, int x, int y,
                           int color_idx,
                           MDGUI_Font *override_font = nullptr) {
  MDGUI_Font *font = resolve_font(ctx, override_font);
  if (!font || !text)
    return;
  font->drawText(text, nullptr, x, y, color_idx);
}

static const MDGUI_Context::SubpassState *current_subpass(
    const MDGUI_Context *ctx) {
  if (!ctx || ctx->subpass_stack.empty())
    return nullptr;
  return &ctx->subpass_stack.back();
}

static int current_viewport_x(const MDGUI_Context *ctx) {
  if (const auto *subpass = current_subpass(ctx))
    return 0;
  if (!ctx || ctx->current_window < 0 ||
      ctx->current_window >= (int)ctx->windows.size())
    return 0;
  const auto &win = ctx->windows[ctx->current_window];
  return win.no_chrome ? win.x : (win.x + 2);
}

static int current_viewport_width(const MDGUI_Context *ctx) {
  if (const auto *subpass = current_subpass(ctx))
    return subpass->local_w;
  if (!ctx || ctx->current_window < 0 ||
      ctx->current_window >= (int)ctx->windows.size())
    return 0;
  const auto &win = ctx->windows[ctx->current_window];
  return win.no_chrome ? win.w : (win.w - 4);
}

static void layout_prepare_widget(MDGUI_Context *ctx, int *out_local_x,
                                  int *out_logical_y);
static int layout_resolve_width(MDGUI_Context *ctx, int local_x, int requested_w,
                                int min_w);

static void set_content_clip(MDGUI_Context *ctx) {
  if (!ctx || ctx->current_window < 0 ||
      ctx->current_window >= (int)ctx->windows.size())
    return;
  int clip_x = current_viewport_x(ctx);
  int clip_w = current_viewport_width(ctx);
  int clip_h = 0;
  if (const auto *subpass = current_subpass(ctx)) {
    clip_h = subpass->local_h - ctx->origin_y;
  } else {
    const auto &win = ctx->windows[ctx->current_window];
    clip_h = (win.y + win.h - 4) - ctx->origin_y;
  }
  if (clip_w < 1)
    clip_w = 1;
  if (clip_h < 1)
    clip_h = 1;
  mdgui_backend_set_clip_rect(1, clip_x, ctx->origin_y, clip_w, clip_h);
}

// Clips a widget drawing region to the current window content area.
// Returns false when there is no visible intersection.
static bool set_widget_clip_intersect_content(MDGUI_Context *ctx, int x, int y,
                                              int w, int h) {
  if (!ctx || ctx->current_window < 0 ||
      ctx->current_window >= (int)ctx->windows.size())
    return false;

  const auto &win = ctx->windows[ctx->current_window];
  const int content_x1 = current_viewport_x(ctx);
  const int content_y1 = ctx->origin_y;
  int content_x2 = content_x1 + current_viewport_width(ctx);
  int content_y2 = 0;
  if (const auto *subpass = current_subpass(ctx)) {
    content_y2 = content_y1 + (subpass->local_h - ctx->origin_y);
  } else {
    content_y2 = content_y1 + ((win.y + win.h - 4) - ctx->origin_y);
  }

  const int widget_x1 = x;
  const int widget_y1 = y;
  const int widget_x2 = x + w;
  const int widget_y2 = y + h;

  const int clip_x1 = std::max(content_x1, widget_x1);
  const int clip_y1 = std::max(content_y1, widget_y1);
  content_x2 = std::max(content_x1, content_x2);
  content_y2 = std::max(content_y1, content_y2);
  const int clip_x2 = std::min(content_x2, widget_x2);
  const int clip_y2 = std::min(content_y2, widget_y2);

  if (clip_x2 <= clip_x1 || clip_y2 <= clip_y1)
    return false;

  mdgui_backend_set_clip_rect(1, clip_x1, clip_y1, clip_x2 - clip_x1,
                              clip_y2 - clip_y1);
  return true;
}

static int resolve_dynamic_width(MDGUI_Context *ctx, int local_x, int w,
                                 int min_w = 1) {
  if (!ctx || ctx->current_window < 0)
    return (w > 0) ? w : min_w;
  const int effective_local_x = local_x + ctx->layout_indent;
  // Reserve right gutter only when an active scrollbar is visible.
  int right_pad = 2;
  if (!current_subpass(ctx) && ctx->current_window >= 0 &&
      ctx->current_window < (int)ctx->windows.size()) {
    const auto &win = ctx->windows[ctx->current_window];
    if (win.scrollbar_visible && !win.is_message_box &&
        win.scrollbar_overflow_active)
      right_pad = 14;
  }
  const int viewport_x = current_viewport_x(ctx);
  const int viewport_w = current_viewport_width(ctx);
  int avail = (viewport_x + viewport_w - right_pad) -
              (ctx->origin_x + effective_local_x);
  if (avail < min_w)
    avail = min_w;
  if (w == 0)
    return avail;
  if (w < 0) {
    int adjusted = avail + w;
    return (adjusted < min_w) ? min_w : adjusted;
  }
  if (w > avail)
    return avail;
  return w;
}

static void drawrect_rgb(MDGUI_Context *ctx, uint8_t r, uint8_t g, uint8_t b,
                         int x, int y, int w, int h) {
  if (!ctx)
    return;
  if (!ctx->backend.fill_rect_rgba)
    return;
  ctx->backend.fill_rect_rgba(ctx->backend.user_data, r, g, b, 255, x, y, w, h);
}

static int point_in_rect(int px, int py, int x, int y, int w, int h) {
  return px >= x && py >= y && px < (x + w) && py < (y + h);
}

static bool menu_path_prefix_matches(const std::vector<int> &path,
                                     const std::vector<int> &prefix) {
  if (path.size() < prefix.size())
    return false;
  for (size_t i = 0; i < prefix.size(); ++i) {
    if (path[i] != prefix[i])
      return false;
  }
  return true;
}

static void clear_window_menu_path(MDGUI_Window &win) {
  win.open_menu_index = -1;
  win.open_menu_path.clear();
  win.menu_overlay_defs.clear();
}

static const char *parse_menu_check_prefix(const char *text, bool *out_has_check,
                                           bool *out_checked) {
  if (out_has_check)
    *out_has_check = false;
  if (out_checked)
    *out_checked = false;
  if (!text)
    return "";
  const size_t len = strlen(text);
  if (len >= 4 && text[0] == '[' && text[2] == ']' && text[3] == ' ') {
    const char mark = text[1];
    if (mark == 'x' || mark == 'X' || mark == ' ') {
      if (out_has_check)
        *out_has_check = true;
      if (out_checked)
        *out_checked = (mark == 'x' || mark == 'X');
      return text + 4;
    }
  }
  return text;
}

static int menu_check_indicator_width() {
  if (!mdgui_fonts[1])
    return 16;
  const int w = mdgui_fonts[1]->measureTextWidth("[ ] ");
  return (w > 0) ? w : 16;
}

static void draw_menu_check_indicator(int x, int y, bool checked) {
  if (!mdgui_fonts[1])
    return;
  int left_w = mdgui_fonts[1]->measureTextWidth("[");
  int space_w = mdgui_fonts[1]->measureTextWidth(" ");
  if (left_w < 1)
    left_w = 1;
  if (space_w < 1)
    space_w = 1;
  mdgui_fonts[1]->drawText("[", nullptr, x, y, CLR_MENU_TEXT);
  mdgui_fonts[1]->drawText("]", nullptr, x + left_w + space_w, y, CLR_MENU_TEXT);
  if (checked) {
    const int mark_x = x + left_w + std::max(0, (space_w - 4) / 2);
    const int mark_y = y + 2;
    mdgui_draw_line_idx(nullptr, CLR_TEXT_LIGHT, mark_x, mark_y, mark_x + 3,
                        mark_y + 3);
    mdgui_draw_line_idx(nullptr, CLR_TEXT_LIGHT, mark_x + 3, mark_y, mark_x,
                        mark_y + 3);
  }
}

static bool *find_or_create_collapsing_state(MDGUI_Window &win, const char *id,
                                             bool default_open) {
  const char *key = (id && id[0]) ? id : "__default";
  for (auto &st : win.collapsing_states) {
    if (st.id == key)
      return &st.open;
  }
  win.collapsing_states.push_back({key, default_open});
  return &win.collapsing_states.back().open;
}

static MDGUI_Window::TabBarState *find_or_create_tab_bar_state(MDGUI_Window &win,
                                                               const char *id) {
  const char *key = (id && id[0]) ? id : "__default";
  for (auto &st : win.tab_bar_states) {
    if (st.id == key)
      return &st;
  }
  win.tab_bar_states.push_back({key, 0, -1});
  return &win.tab_bar_states.back();
}

static bool
point_in_menu_popup_chain(const std::vector<MDGUI_Context::MenuDef> &defs,
                          const std::vector<int> &path, int item_h, int px,
                          int py) {
  for (int menu_idx : path) {
    if (menu_idx < 0 || menu_idx >= (int)defs.size())
      continue;
    const auto &def = defs[menu_idx];
    const int popup_h = (int)def.items.size() * item_h;
    if (popup_h <= 0)
      continue;
    if (point_in_rect(px, py, def.x, def.y, def.w, popup_h))
      return true;
  }
  return false;
}

static void
reposition_child_menu_chain(std::vector<MDGUI_Context::MenuDef> &defs,
                            int menu_index, int item_h) {
  if (menu_index < 0 || menu_index >= (int)defs.size() || item_h <= 0)
    return;
  auto &parent = defs[menu_index];
  for (int i = 0; i < (int)parent.items.size(); ++i) {
    const int child_idx = parent.items[i].child_menu_index;
    if (child_idx < 0 || child_idx >= (int)defs.size())
      continue;
    auto &child = defs[child_idx];
    child.x = parent.x + parent.w + MENU_POPUP_GAP_Y;
    child.y = parent.y + (i * item_h);
    reposition_child_menu_chain(defs, child_idx, item_h);
  }
}

static bool window_accepts_input(const MDGUI_Window &w);

static int top_window_at_point(const MDGUI_Context *ctx, int px, int py,
                               int margin = 0) {
  if (ctx->combo_capture_active && ctx->combo_capture_window >= 0 &&
      ctx->combo_capture_window < (int)ctx->windows.size()) {
    const auto &ow = ctx->windows[ctx->combo_capture_window];
    if (!ow.closed &&
        point_in_rect(px, py, ctx->combo_capture_x, ctx->combo_capture_y,
                      ctx->combo_capture_w, ctx->combo_capture_h)) {
      return ctx->combo_capture_window;
    }
  }

  int best = -1;
  int best_z = -2147483647;
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    const auto &w = ctx->windows[i];
    if (w.closed)
      continue;
    if (!window_accepts_input(w))
      continue;
    if (w.w <= 0 || w.h <= 0)
      continue;
    if (point_in_rect(px, py, w.x - margin, w.y - margin, w.w + 2 * margin,
                      w.h + 2 * margin) &&
        w.z > best_z) {
      best_z = w.z;
      best = i;
    }
  }
  return best;
}

static bool is_tiled_file_browser_window(const MDGUI_Context *ctx,
                                         const MDGUI_Window &win) {
  if (!ctx)
    return false;
  return ctx->tile_manager_enabled && !win.tile_excluded &&
         win.id == "File Browser";
}

static bool is_occluded_by_higher_maximized_window(const MDGUI_Context *ctx,
                                                   int window_idx) {
  if (!ctx || window_idx < 0 || window_idx >= (int)ctx->windows.size())
    return false;
  const auto &target = ctx->windows[window_idx];
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    if (i == window_idx)
      continue;
    const auto &other = ctx->windows[i];
    if (other.closed || !other.is_maximized)
      continue;
    if (other.z > target.z)
      return true;
  }
  return false;
}

static int get_logical_render_w(MDGUI_Context *ctx);
static int get_logical_render_h(MDGUI_Context *ctx);
static int get_status_bar_h(const MDGUI_Context *ctx);
static int get_work_area_top(MDGUI_Context *ctx);
static int get_work_area_bottom(MDGUI_Context *ctx);
static void clamp_window_to_work_area(MDGUI_Context *ctx, MDGUI_Window &win);

static int clampi(int v, int lo, int hi) {
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

static void sanitize_overlay_state(MDGUI_OverlayState *state) {
  if (!state)
    return;
  if (state->w < 1)
    state->w = 1;
  if (state->h < 1)
    state->h = 1;
  if (state->margin_left < 0)
    state->margin_left = 0;
  if (state->margin_top < 0)
    state->margin_top = 0;
  if (state->margin_right < 0)
    state->margin_right = 0;
  if (state->margin_bottom < 0)
    state->margin_bottom = 0;
}

static bool window_accepts_input(const MDGUI_Window &w) {
  return !w.input_passthrough;
}

static bool has_pending_tile_excluded_title(const MDGUI_Context *ctx,
                                            const char *title) {
  if (!ctx || !title)
    return false;
  for (const auto &it : ctx->pending_tile_excluded_titles) {
    if (it == title)
      return true;
  }
  return false;
}

static int normalize_window_min_w(int min_w) { return (min_w < 50) ? 50 : min_w; }

static int normalize_window_min_h(int min_h) { return (min_h < 30) ? 30 : min_h; }

static float normalize_window_min_percent(float percent) {
  if (percent < 0.0f)
    return 0.0f;
  if (percent > 100.0f)
    return 100.0f;
  return percent;
}

static int min_from_percent(int total, float percent, int floor_px) {
  if (total < 1)
    total = 1;
  const float p = normalize_window_min_percent(percent);
  const int px = (int)std::lround(((double)total * (double)p) / 100.0);
  return (px < floor_px) ? floor_px : px;
}

static void set_pending_tile_excluded_title(MDGUI_Context *ctx,
                                            const char *title, bool excluded) {
  if (!ctx || !title)
    return;
  for (size_t i = 0; i < ctx->pending_tile_excluded_titles.size(); ++i) {
    if (ctx->pending_tile_excluded_titles[i] != title)
      continue;
    if (!excluded) {
      ctx->pending_tile_excluded_titles.erase(
          ctx->pending_tile_excluded_titles.begin() + (int)i);
    }
    return;
  }
  if (excluded)
    ctx->pending_tile_excluded_titles.push_back(title);
}

static bool get_pending_window_min_size(const MDGUI_Context *ctx,
                                        const char *title, int *out_min_w,
                                        int *out_min_h) {
  if (!ctx || !title)
    return false;
  for (const auto &it : ctx->pending_window_min_sizes) {
    if (it.title != title)
      continue;
    if (it.use_percent) {
      if (out_min_w)
        *out_min_w = min_from_percent(get_logical_render_w(const_cast<MDGUI_Context *>(ctx)),
                                      it.min_w_percent, 50);
      if (out_min_h)
        *out_min_h = min_from_percent(get_logical_render_h(const_cast<MDGUI_Context *>(ctx)),
                                      it.min_h_percent, 30);
      return true;
    }
    if (out_min_w)
      *out_min_w = it.min_w;
    if (out_min_h)
      *out_min_h = it.min_h;
    return true;
  }
  return false;
}

static void set_pending_window_min_size(MDGUI_Context *ctx, const char *title,
                                        int min_w, int min_h) {
  if (!ctx || !title)
    return;
  const int nw = normalize_window_min_w(min_w);
  const int nh = normalize_window_min_h(min_h);
  for (auto &it : ctx->pending_window_min_sizes) {
    if (it.title != title)
      continue;
    it.min_w = nw;
    it.min_h = nh;
    it.use_percent = false;
    it.min_w_percent = 0.0f;
    it.min_h_percent = 0.0f;
    return;
  }
  ctx->pending_window_min_sizes.push_back({title, nw, nh, false, 0.0f, 0.0f});
}

static void set_pending_window_min_size_percent(MDGUI_Context *ctx,
                                                const char *title,
                                                float min_w_percent,
                                                float min_h_percent) {
  if (!ctx || !title)
    return;
  const float wp = normalize_window_min_percent(min_w_percent);
  const float hp = normalize_window_min_percent(min_h_percent);
  for (auto &it : ctx->pending_window_min_sizes) {
    if (it.title != title)
      continue;
    it.use_percent = true;
    it.min_w_percent = wp;
    it.min_h_percent = hp;
    it.min_w = min_from_percent(get_logical_render_w(ctx), wp, 50);
    it.min_h = min_from_percent(get_logical_render_h(ctx), hp, 30);
    return;
  }
  ctx->pending_window_min_sizes.push_back(
      {title,
       min_from_percent(get_logical_render_w(ctx), wp, 50),
       min_from_percent(get_logical_render_h(ctx), hp, 30), true, wp, hp});
}

static bool get_pending_window_min_size_percent(const MDGUI_Context *ctx,
                                                const char *title,
                                                float *out_min_w_percent,
                                                float *out_min_h_percent) {
  if (!ctx || !title)
    return false;
  for (const auto &it : ctx->pending_window_min_sizes) {
    if (it.title != title)
      continue;
    if (!it.use_percent)
      return false;
    if (out_min_w_percent)
      *out_min_w_percent = it.min_w_percent;
    if (out_min_h_percent)
      *out_min_h_percent = it.min_h_percent;
    return true;
  }
  return false;
}

static bool get_pending_window_scrollbar_visibility(const MDGUI_Context *ctx,
                                                    const char *title,
                                                    bool *out_visible) {
  if (!ctx || !title)
    return false;
  for (const auto &it : ctx->pending_window_scrollbars) {
    if (it.title != title)
      continue;
    if (out_visible)
      *out_visible = it.visible;
    return true;
  }
  return false;
}

static void set_pending_window_scrollbar_visibility(MDGUI_Context *ctx,
                                                    const char *title,
                                                    bool visible) {
  if (!ctx || !title)
    return;
  for (auto &it : ctx->pending_window_scrollbars) {
    if (it.title != title)
      continue;
    it.visible = visible;
    return;
  }
  ctx->pending_window_scrollbars.push_back({title, visible});
}

static int normalize_tile_side(int side) {
  if (side < MDGUI_TILE_SIDE_AUTO || side > MDGUI_TILE_SIDE_BOTTOM)
    return MDGUI_TILE_SIDE_AUTO;
  return side;
}

static int normalize_tile_weight(int weight) {
  return (weight < 1) ? 1 : weight;
}

// Fit a desired per-tile minimum into the available span so tiles + gaps never
// exceed the container when the host window is very small.
static int fit_tile_min_size(int total_size, int count, int gap,
                             int desired_min) {
  if (count <= 0)
    return std::max(1, desired_min);
  if (gap < 0)
    gap = 0;
  if (desired_min < 1)
    desired_min = 1;

  const int total_gap = (count - 1) * gap;
  int usable = total_size - total_gap;
  if (usable < count)
    return 1;
  const int max_equal = usable / count;
  if (max_equal < 1)
    return 1;
  return std::min(desired_min, max_equal);
}

static void assign_tiled_window_rect(MDGUI_Window &w, int x, int y, int ww,
                                     int hh) {
  w.x = x;
  w.y = y;
  w.w = std::max(1, ww);
  w.h = std::max(1, hh);
  w.restored_x = x;
  w.restored_y = y;
  w.restored_w = w.w;
  w.restored_h = w.h;
  w.is_maximized = false;
  w.fixed_rect = true;
}

static void tile_windows_grid(MDGUI_Context *ctx, const std::vector<int> &order,
                              int left, int top, int content_w, int content_h,
                              int gap, int *out_total_min_w = nullptr,
                              int *out_total_min_h = nullptr) {
  if (!ctx || order.empty() || content_w <= 0 || content_h <= 0)
    return;

  const int count = (int)order.size();
  const float aspect =
      (float)content_w / (float)((content_h > 0) ? content_h : 1);
  int cols = (int)std::ceil(std::sqrt((float)count * aspect));
  if (cols < 1)
    cols = 1;
  if (cols > count)
    cols = count;
  int rows = (count + cols - 1) / cols;
  if (rows < 1)
    rows = 1;

  int total_min_w = 0;
  int total_min_h = 0;

  int y = top;
  int idx = 0;
  for (int row = 0; row < rows && idx < count; ++row) {
    const int rows_left = rows - row;
    const int remaining = count - idx;
    const int cols_this_row = (remaining + rows_left - 1) / rows_left;

    int max_row_min_h = 80; // Default minimum height
    for (int i = 0; i < cols_this_row && (idx + i) < count; ++i) {
      if (ctx->windows[order[idx + i]].min_h > max_row_min_h)
        max_row_min_h = ctx->windows[order[idx + i]].min_h;
    }

    const int span_h = (top + content_h) - y;
    const int row_min_h =
        fit_tile_min_size(span_h, rows_left, gap, max_row_min_h);
    const int h_remaining = span_h - (rows_left - 1) * gap;
    int row_h = std::max(1, h_remaining);
    if (row < rows - 1) {
      int target = h_remaining / rows_left;
      int max_for_this = h_remaining - (rows_left - 1) * row_min_h;
      if (max_for_this < row_min_h)
        max_for_this = row_min_h;
      row_h = clampi(target, row_min_h, max_for_this);
    }

    total_min_h += max_row_min_h;
    if (row < rows - 1)
      total_min_h += gap;

    int current_row_min_w = 0;
    int x = left;
    for (int col = 0; col < cols_this_row && idx < count; ++col) {
      const int cols_left = cols_this_row - col;
      const int span_w = (left + content_w) - x;
      const int col_min_w = fit_tile_min_size(span_w, cols_left, gap,
                                              ctx->windows[order[idx]].min_w);
      const int w_remaining = span_w - (cols_left - 1) * gap;
      int cell_w = std::max(1, w_remaining);
      if (col < cols_this_row - 1) {
        int target = w_remaining / cols_left;
        int max_for_this = w_remaining - (cols_left - 1) * col_min_w;
        if (max_for_this < col_min_w)
          max_for_this = col_min_w;
        cell_w = clampi(target, col_min_w, max_for_this);
      }
      auto &w = ctx->windows[order[idx]];
      assign_tiled_window_rect(w, x, y, cell_w, row_h);

      current_row_min_w += w.min_w;
      if (col < cols_this_row - 1)
        current_row_min_w += gap;

      ++idx;
      x += cell_w + gap;
    }
    if (current_row_min_w > total_min_w)
      total_min_w = current_row_min_w;
    y += row_h + gap;
  }

  if (out_total_min_w)
    *out_total_min_w = total_min_w;
  if (out_total_min_h)
    *out_total_min_h = total_min_h;
}

static int sum_tile_weights(MDGUI_Context *ctx,
                            const std::vector<int> &indices) {
  if (!ctx)
    return 0;
  int sum = 0;
  for (int idx : indices) {
    if (idx < 0 || idx >= (int)ctx->windows.size())
      continue;
    sum += normalize_tile_weight(ctx->windows[idx].tile_weight);
  }
  return sum;
}

static void sort_by_weight_then_z(MDGUI_Context *ctx,
                                  std::vector<int> &indices) {
  if (!ctx)
    return;
  std::stable_sort(indices.begin(), indices.end(), [ctx](int a, int b) {
    const auto &wa = ctx->windows[a];
    const auto &wb = ctx->windows[b];
    const int wgt_a = normalize_tile_weight(wa.tile_weight);
    const int wgt_b = normalize_tile_weight(wb.tile_weight);
    if (wgt_a != wgt_b)
      return wgt_a > wgt_b;
    return wa.z > wb.z;
  });
}

static void tile_windows_vertical_weighted(MDGUI_Context *ctx,
                                           const std::vector<int> &order, int x,
                                           int y, int w, int h, int gap,
                                           int *out_total_min_w = nullptr,
                                           int *out_total_min_h = nullptr) {
  if (!ctx || order.empty() || w <= 0 || h <= 0)
    return;
  if ((int)order.size() == 1) {
    auto &win = ctx->windows[order[0]];
    assign_tiled_window_rect(win, x, y, w, h);
    if (out_total_min_w)
      *out_total_min_w = win.min_w;
    if (out_total_min_h)
      *out_total_min_h = win.min_h;
    return;
  }

  int total_min_h = 0;
  int max_min_w = 0;

  int total_weight = sum_tile_weights(ctx, order);
  if (total_weight < 1)
    total_weight = (int)order.size();
  int remaining_weight = total_weight;
  int cy = y;

  for (int i = 0; i < (int)order.size(); ++i) {
    const int idx = order[i];
    const int items_left = (int)order.size() - i;
    const int span_h = (y + h) - cy;
    const int item_min_h =
        fit_tile_min_size(span_h, items_left, gap, ctx->windows[idx].min_h);
    int available_h = span_h - (items_left - 1) * gap;
    if (available_h < item_min_h)
      available_h = item_min_h;

    int cell_h = available_h;
    if (i < (int)order.size() - 1 && remaining_weight > 0) {
      const int wgt = normalize_tile_weight(ctx->windows[idx].tile_weight);
      int proportional =
          (int)((double)available_h * (double)wgt / (double)remaining_weight);
      int max_for_this = available_h - (items_left - 1) * item_min_h;
      if (max_for_this < item_min_h)
        max_for_this = item_min_h;
      cell_h = clampi(proportional, item_min_h, max_for_this);
    }

    assign_tiled_window_rect(ctx->windows[idx], x, cy, w, cell_h);

    total_min_h += ctx->windows[idx].min_h;
    if (i < (int)order.size() - 1)
      total_min_h += gap;
    if (ctx->windows[idx].min_w > max_min_w)
      max_min_w = ctx->windows[idx].min_w;

    cy += cell_h + gap;
    remaining_weight -= normalize_tile_weight(ctx->windows[idx].tile_weight);
    if (remaining_weight < 0)
      remaining_weight = 0;
  }

  if (out_total_min_w)
    *out_total_min_w = max_min_w;
  if (out_total_min_h)
    *out_total_min_h = total_min_h;
}

static void tile_windows_horizontal_weighted(MDGUI_Context *ctx,
                                             const std::vector<int> &order,
                                             int x, int y, int w, int h,
                                             int gap,
                                             int *out_total_min_w = nullptr,
                                             int *out_total_min_h = nullptr) {
  if (!ctx || order.empty() || w <= 0 || h <= 0)
    return;
  if ((int)order.size() == 1) {
    auto &win = ctx->windows[order[0]];
    assign_tiled_window_rect(win, x, y, w, h);
    if (out_total_min_w)
      *out_total_min_w = win.min_w;
    if (out_total_min_h)
      *out_total_min_h = win.min_h;
    return;
  }

  int total_min_w = 0, max_min_h = 0;
  int total_weight = sum_tile_weights(ctx, order);
  if (total_weight < 1)
    total_weight = (int)order.size();
  int remaining_weight = total_weight;
  int cx = x;

  for (int i = 0; i < (int)order.size(); ++i) {
    const int idx = order[i];
    const int items_left = (int)order.size() - i;
    const int span_w = (x + w) - cx;
    const int item_min_w =
        fit_tile_min_size(span_w, items_left, gap, ctx->windows[idx].min_w);
    int available_w = span_w - (items_left - 1) * gap;
    if (available_w < item_min_w)
      available_w = item_min_w;

    int cell_w = available_w;
    if (i < (int)order.size() - 1 && remaining_weight > 0) {
      const int wgt = normalize_tile_weight(ctx->windows[idx].tile_weight);
      int proportional =
          (int)((double)available_w * (double)wgt / (double)remaining_weight);
      int max_for_this = available_w - (items_left - 1) * item_min_w;
      if (max_for_this < item_min_w)
        max_for_this = item_min_w;
      cell_w = clampi(proportional, item_min_w, max_for_this);
    }

    assign_tiled_window_rect(ctx->windows[idx], cx, y, cell_w, h);

    total_min_w += ctx->windows[idx].min_w;
    if (i < (int)order.size() - 1)
      total_min_w += gap;
    if (ctx->windows[idx].min_h > max_min_h)
      max_min_h = ctx->windows[idx].min_h;

    cx += cell_w + gap;
    remaining_weight -= normalize_tile_weight(ctx->windows[idx].tile_weight);
    if (remaining_weight < 0)
      remaining_weight = 0;
  }

  if (out_total_min_w)
    *out_total_min_w = total_min_w;
  if (out_total_min_h)
    *out_total_min_h = max_min_h;
}

static void tile_windows_weighted_cluster(MDGUI_Context *ctx,
                                          const std::vector<int> &order,
                                          int left, int top, int content_w,
                                          int content_h, int gap,
                                          int *out_total_min_w = nullptr,
                                          int *out_total_min_h = nullptr) {
  if (!ctx || order.empty() || content_w <= 0 || content_h <= 0)
    return;
  const int count = (int)order.size();
  if (count == 1) {
    auto &win = ctx->windows[order[0]];
    assign_tiled_window_rect(win, left, top, content_w, content_h);
    if (out_total_min_w)
      *out_total_min_w = win.min_w;
    if (out_total_min_h)
      *out_total_min_h = win.min_h;
    return;
  }

  int primary_pos = -1;
  int best_weight = 1;
  int best_z = -2147483647;
  for (int i = 0; i < count; ++i) {
    const auto &w = ctx->windows[order[i]];
    const int weight = normalize_tile_weight(w.tile_weight);
    if (primary_pos < 0 || weight > best_weight ||
        (weight == best_weight && w.z > best_z)) {
      primary_pos = i;
      best_weight = weight;
      best_z = w.z;
    }
  }

  if (primary_pos == -1 || best_weight <= 1) {
    tile_windows_grid(ctx, order, left, top, content_w, content_h, gap,
                      out_total_min_w, out_total_min_h);
    return;
  }

  std::vector<int> secondary;
  secondary.reserve((size_t)count - 1);
  for (int i = 0; i < count; ++i) {
    if (i == primary_pos)
      continue;
    secondary.push_back(order[i]);
  }

  int secondary_weight_sum = 0;
  for (int idx : secondary) {
    secondary_weight_sum +=
        normalize_tile_weight(ctx->windows[idx].tile_weight);
  }
  if (secondary_weight_sum < 1)
    secondary_weight_sum = (int)secondary.size();

  const float split_raw =
      (float)best_weight / (float)(best_weight + secondary_weight_sum);
  const float split_ratio = std::max(0.34f, std::min(0.75f, split_raw));
  const int side =
      (content_w >= content_h) ? MDGUI_TILE_SIDE_LEFT : MDGUI_TILE_SIDE_TOP;

  int px = left, py = top, pw = content_w, ph = content_h;
  int sx = left, sy = top, sw = content_w, sh = content_h;

  const int min_primary_w = ctx->windows[order[primary_pos]].min_w;
  const int min_primary_h = ctx->windows[order[primary_pos]].min_h;

  int min_sec_w = 0, min_sec_h = 0;
  tile_windows_grid(ctx, secondary, left, top, content_w, content_h, gap,
                    &min_sec_w, &min_sec_h);

  if (side == MDGUI_TILE_SIDE_LEFT || side == MDGUI_TILE_SIDE_RIGHT) {
    if (content_w < (min_primary_w + min_sec_w + gap)) {
      tile_windows_grid(ctx, order, left, top, content_w, content_h, gap,
                        out_total_min_w, out_total_min_h);
      return;
    }
    int p_w = (int)((float)content_w * split_ratio);
    p_w = clampi(p_w, min_primary_w, content_w - min_sec_w - gap);
    const int s_w = content_w - p_w - gap;
    if (side == MDGUI_TILE_SIDE_LEFT) {
      px = left;
      sx = left + p_w + gap;
    } else {
      px = left + s_w + gap;
      sx = left;
    }
    pw = p_w;
    sw = s_w;
    if (out_total_min_w)
      *out_total_min_w = min_primary_w + gap + min_sec_w;
    if (out_total_min_h)
      *out_total_min_h = std::max(min_primary_h, min_sec_h);
  } else {
    if (content_h < (min_primary_h + min_sec_h + gap)) {
      tile_windows_grid(ctx, order, left, top, content_w, content_h, gap,
                        out_total_min_w, out_total_min_h);
      return;
    }
    int p_h = (int)((float)content_h * split_ratio);
    p_h = clampi(p_h, min_primary_h, content_h - min_sec_h - gap);
    const int s_h = content_h - p_h - gap;
    if (side == MDGUI_TILE_SIDE_TOP) {
      py = top;
      sy = top + p_h + gap;
    } else {
      py = top + s_h + gap;
      sy = top;
    }
    ph = p_h;
    sh = s_h;
    if (out_total_min_w)
      *out_total_min_w = std::max(min_primary_w, min_sec_w);
    if (out_total_min_h)
      *out_total_min_h = min_primary_h + gap + min_sec_h;
  }

  assign_tiled_window_rect(ctx->windows[order[primary_pos]], px, py, pw, ph);
  tile_windows_grid(ctx, secondary, sx, sy, sw, sh, gap);
}

static void tile_windows_internal(MDGUI_Context *ctx,
                                  int *out_total_min_w = nullptr,
                                  int *out_total_min_h = nullptr) {
  if (!ctx)
    return;

  const int rw = get_logical_render_w(ctx);
  const int rh = get_logical_render_h(ctx);
  const int gap = 6;
  int top = get_work_area_top(ctx) + 6;
  if (top < 18)
    top = 18;
  const int bottom = 6 + get_status_bar_h(ctx);
  const int left = gap, right = gap;

  const int content_w = rw - left - right;
  const int content_h = rh - top - bottom;
  if (content_w <= 0 || content_h <= 0)
    return;

  std::vector<int> order;
  order.reserve(ctx->windows.size());
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    const auto &w = ctx->windows[i];
    if (w.closed || w.is_maximized || w.disallow_maximize || w.tile_excluded)
      continue;
    order.push_back(i);
  }
  if (order.empty())
    return;

  const int count = (int)order.size();
  if (count == 1) {
    auto &only = ctx->windows[order[0]];
    assign_tiled_window_rect(only, left, top, content_w, content_h);
    if (out_total_min_w)
      *out_total_min_w = left + only.min_w + right;
    if (out_total_min_h)
      *out_total_min_h = top + only.min_h + bottom;
    return;
  }

  std::vector<int> left_group, right_group, top_group, bottom_group, auto_group;
  for (int idx : order) {
    const auto &w = ctx->windows[idx];
    const int side = normalize_tile_side(w.tile_side);
    if (side == MDGUI_TILE_SIDE_LEFT)
      left_group.push_back(idx);
    else if (side == MDGUI_TILE_SIDE_RIGHT)
      right_group.push_back(idx);
    else if (side == MDGUI_TILE_SIDE_TOP)
      top_group.push_back(idx);
    else if (side == MDGUI_TILE_SIDE_BOTTOM)
      bottom_group.push_back(idx);
    else
      auto_group.push_back(idx);
  }

  const bool has_directional_windows =
      !left_group.empty() || !right_group.empty() || !top_group.empty() ||
      !bottom_group.empty();
  if (has_directional_windows) {
    sort_by_weight_then_z(ctx, left_group);
    sort_by_weight_then_z(ctx, right_group);
    sort_by_weight_then_z(ctx, top_group);
    sort_by_weight_then_z(ctx, bottom_group);
    sort_by_weight_then_z(ctx, auto_group);

    int left_sum = sum_tile_weights(ctx, left_group);
    int right_sum = sum_tile_weights(ctx, right_group);
    int top_sum = sum_tile_weights(ctx, top_group);
    int bottom_sum = sum_tile_weights(ctx, bottom_group);
    int auto_sum = sum_tile_weights(ctx, auto_group);
    if (auto_sum < 1 && !auto_group.empty())
      auto_sum = (int)auto_group.size();

    int center_x = left, center_y = top, center_w = content_w,
        center_h = content_h;

    int min_l_w = 0, min_l_h = 0, min_r_w = 0, min_r_h = 0;
    int min_t_w = 0, min_t_h = 0, min_b_w = 0, min_b_h = 0;
    int min_a_w = 0, min_a_h = 0;

    if (!left_group.empty())
      tile_windows_vertical_weighted(ctx, left_group, 0, 0, 100, 100, gap,
                                     &min_l_w, &min_l_h);
    if (!right_group.empty())
      tile_windows_vertical_weighted(ctx, right_group, 0, 0, 100, 100, gap,
                                     &min_r_w, &min_r_h);
    if (!top_group.empty())
      tile_windows_horizontal_weighted(ctx, top_group, 0, 0, 100, 100, gap,
                                       &min_t_w, &min_t_h);
    if (!bottom_group.empty())
      tile_windows_horizontal_weighted(ctx, bottom_group, 0, 0, 100, 100, gap,
                                       &min_b_w, &min_b_h);
    if (!auto_group.empty())
      tile_windows_weighted_cluster(ctx, auto_group, 0, 0, 100, 100, gap,
                                    &min_a_w, &min_a_h);

    const int min_center_w = std::max(min_a_w, std::max(min_t_w, min_b_w));
    const int min_center_h = std::max(min_a_h, std::max(min_l_h, min_r_h));

    if (out_total_min_w)
      *out_total_min_w = left + min_l_w + (min_l_w > 0 ? gap : 0) +
                         min_center_w + (min_r_w > 0 ? gap : 0) + min_r_w +
                         right;
    if (out_total_min_h)
      *out_total_min_h = top + min_t_h + (min_t_h > 0 ? gap : 0) +
                         min_center_h + (min_b_h > 0 ? gap : 0) + min_b_h +
                         bottom;

    if (!left_group.empty()) {
      int total_w_weight = left_sum + right_sum + auto_sum;
      int max_left_w =
          center_w - min_center_w - (right_group.empty() ? 0 : (min_r_w + gap));
      int left_w_px =
          clampi((int)((double)center_w * (double)left_sum / total_w_weight),
                 min_l_w, max_left_w);
      tile_windows_vertical_weighted(ctx, left_group, center_x, top, left_w_px,
                                     content_h, gap);
      center_x += left_w_px + gap;
      center_w -= left_w_px + gap;
    }
    if (!right_group.empty()) {
      int total_w_weight = right_sum + auto_sum;
      int max_right_w = center_w - min_center_w;
      int right_w_px =
          clampi((int)((double)center_w * (double)right_sum / total_w_weight),
                 min_r_w, max_right_w);
      tile_windows_vertical_weighted(ctx, right_group,
                                     center_x + center_w - right_w_px, top,
                                     right_w_px, content_h, gap);
      center_w -= right_w_px + gap;
    }
    if (!top_group.empty()) {
      int total_h_weight = top_sum + bottom_sum + auto_sum;
      int max_top_h = center_h - min_center_h -
                      (bottom_group.empty() ? 0 : (min_b_h + gap));
      int top_h_px =
          clampi((int)((double)center_h * (double)top_sum / total_h_weight),
                 min_t_h, max_top_h);
      tile_windows_horizontal_weighted(ctx, top_group, center_x, center_y,
                                       center_w, top_h_px, gap);
      center_y += top_h_px + gap;
      center_h -= top_h_px + gap;
    }
    if (!bottom_group.empty()) {
      int total_h_weight = bottom_sum + auto_sum;
      int max_bottom_h = center_h - min_center_h;
      int bottom_h_px =
          clampi((int)((double)center_h * (double)bottom_sum / total_h_weight),
                 min_b_h, max_bottom_h);
      tile_windows_horizontal_weighted(ctx, bottom_group, center_x,
                                       center_y + center_h - bottom_h_px,
                                       center_w, bottom_h_px, gap);
      center_h -= bottom_h_px + gap;
    }
    if (!auto_group.empty())
      tile_windows_weighted_cluster(ctx, auto_group, center_x, center_y,
                                    center_w, center_h, gap);
  } else {
    tile_windows_weighted_cluster(ctx, order, left, top, content_w, content_h,
                                  gap, out_total_min_w, out_total_min_h);
  }
}

static int find_or_create_window(MDGUI_Context *ctx, const char *title, int x,
                                 int y, int w, int h,
                                 bool *out_created = nullptr) {
  const char *key = title ? title : "window";
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    if (ctx->windows[i].id == key) {
      ctx->windows[i].title = key;
      if (out_created)
        *out_created = false;
      return i;
    }
  }
  MDGUI_Window nw{};
  nw.id = key;
  nw.title = key;
  nw.x = x;
  nw.y = y;
  nw.w = w;
  nw.h = h;
  nw.z = ++ctx->z_counter;
  nw.open_menu_index = -1;
  nw.open_menu_path.clear();
  nw.is_maximized = false;
  nw.closed = false;
  nw.restored_x = x;
  nw.restored_y = y;
  nw.restored_w = w;
  nw.restored_h = h;
  nw.last_edge_mask = 0;
  nw.min_w = 50;
  nw.min_h = 30;
  nw.user_min_w = 50;
  nw.user_min_h = 30;
  nw.user_min_from_percent = false;
  nw.user_min_w_percent = 0.0f;
  nw.user_min_h_percent = 0.0f;
  nw.open_combo_id = -1;
  nw.menu_overlay_defs.clear();
  nw.menu_overlay_item_h = 10;
  nw.text_scroll = 0;
  nw.text_scroll_dragging = false;
  nw.text_scroll_drag_offset = 0;
  nw.fixed_rect = false;
  nw.disallow_maximize = false;
  nw.is_message_box = false;
  nw.scrollbar_visible = true;
  nw.scrollbar_overflow_active = false;
  nw.tile_weight = 1;
  nw.tile_side = MDGUI_TILE_SIDE_AUTO;
  nw.tile_excluded = has_pending_tile_excluded_title(ctx, key);
  nw.no_chrome = false;
  int pending_min_w = 0;
  int pending_min_h = 0;
  if (get_pending_window_min_size(ctx, key, &pending_min_w, &pending_min_h)) {
    nw.user_min_w = pending_min_w;
    nw.user_min_h = pending_min_h;
    nw.min_w = pending_min_w;
    nw.min_h = pending_min_h;
    float wp = 0.0f, hp = 0.0f;
    if (get_pending_window_min_size_percent(ctx, key, &wp, &hp)) {
      nw.user_min_from_percent = true;
      nw.user_min_w_percent = wp;
      nw.user_min_h_percent = hp;
    }
  }
  bool pending_scrollbar_visible = true;
  if (get_pending_window_scrollbar_visibility(ctx, key,
                                              &pending_scrollbar_visible)) {
    nw.scrollbar_visible = pending_scrollbar_visible;
  }
  nw.alpha = ctx->default_window_alpha;
  nw.input_passthrough = false;
  ctx->windows.push_back(nw);
  if (out_created)
    *out_created = true;
  return (int)ctx->windows.size() - 1;
}

static int is_current_window_topmost(MDGUI_Context *ctx, int margin = 0) {
  if (ctx->current_window < 0)
    return 0;
  if (current_subpass(ctx))
    return 1;
  const auto &w = ctx->windows[ctx->current_window];
  if (!window_accepts_input(w))
    return 0;
  return top_window_at_point(ctx, ctx->input.mouse_x, ctx->input.mouse_y,
                             margin) == ctx->current_window ||
         w.z >= ctx->z_counter;
}

static void draw_open_menu_overlay(MDGUI_Context *ctx) {
  if (!ctx || ctx->current_window < 0)
    return;
  auto &win = ctx->windows[ctx->current_window];
  if (win.open_menu_path.empty())
    return;

  const int item_h = ctx->current_menu_h;
  for (int depth = 0; depth < (int)win.open_menu_path.size(); ++depth) {
    const int menu_idx = win.open_menu_path[depth];
    if (menu_idx < 0 || menu_idx >= (int)ctx->menu_defs.size())
      continue;
    auto &def = ctx->menu_defs[menu_idx];
    const int total_h = (int)def.items.size() * item_h;
    if (total_h <= 0)
      continue;

    mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, def.x - 1, def.y - 1,
                         def.x + def.w + 1);
    mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, def.x - 1, def.y + total_h,
                         def.x + def.w + 1);
    mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, def.x - 1, def.y - 1,
                         def.y + total_h + 1);
    mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, def.x + def.w, def.y - 1,
                         def.y + total_h + 1);
    mdgui_fill_rect_idx(nullptr, CLR_MENU_BG, def.x, def.y, def.w, total_h);

    for (int i = 0; i < (int)def.items.size(); ++i) {
      const int iy = def.y + (i * item_h);
      if (def.items[i].is_separator) {
        const int sep_y = iy + (item_h / 2);
        mdgui_draw_hline_idx(nullptr, CLR_BUTTON_LIGHT, def.x + 4, sep_y,
                             def.x + def.w - 4);
        mdgui_draw_hline_idx(nullptr, CLR_BUTTON_DARK, def.x + 4, sep_y + 1,
                             def.x + def.w - 4);
        continue;
      }
      const int hovered = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                        def.x, iy, def.w, item_h);
      if (hovered) {
        mdgui_fill_rect_idx(nullptr, CLR_ACCENT, def.x, iy, def.w, item_h);
        if (def.items[i].child_menu_index >= 0) {
          const int child_idx = def.items[i].child_menu_index;
          const int want_size = depth + 2;
          if ((int)win.open_menu_path.size() < want_size) {
            win.open_menu_path.resize((size_t)want_size);
          }
          win.open_menu_path[depth + 1] = child_idx;
          win.open_menu_index =
              win.open_menu_path.empty() ? -1 : win.open_menu_path[0];
        } else if ((int)win.open_menu_path.size() > depth + 1) {
          win.open_menu_path.resize((size_t)depth + 1);
          win.open_menu_index =
              win.open_menu_path.empty() ? -1 : win.open_menu_path[0];
        }
      }
      if (mdgui_fonts[1]) {
        bool has_check = false;
        bool checked = false;
        const char *label = parse_menu_check_prefix(def.items[i].text.c_str(),
                                                    &has_check, &checked);
        const int check_w = has_check ? menu_check_indicator_width() : 0;
        const int text_x = def.x + 4 + check_w;
        if (has_check)
          draw_menu_check_indicator(def.x + 4, iy + 1, checked);
        mdgui_fonts[1]->drawText(label, nullptr, text_x, iy + 1, CLR_MENU_TEXT);
        if (def.items[i].child_menu_index >= 0) {
          mdgui_fonts[1]->drawText(">", nullptr, def.x + def.w - 8, iy + 1,
                                   CLR_MENU_TEXT);
        }
      }
    }
  }
}

static void draw_cached_window_menu_overlay(MDGUI_Context *ctx,
                                            MDGUI_Window &win) {
  if (!ctx || win.closed || win.open_menu_path.empty() ||
      win.menu_overlay_defs.empty())
    return;
  const int item_h = (win.menu_overlay_item_h > 0) ? win.menu_overlay_item_h : 10;

  // Overlay menus should ignore existing content clip/alpha.
  mdgui_backend_set_clip_rect(0, 0, 0, 0, 0);
  mdgui_backend_set_alpha_mod(255);

  for (int depth = 0; depth < (int)win.open_menu_path.size(); ++depth) {
    const int menu_idx = win.open_menu_path[depth];
    if (menu_idx < 0 || menu_idx >= (int)win.menu_overlay_defs.size())
      continue;
    const auto &def = win.menu_overlay_defs[menu_idx];
    const int total_h = (int)def.items.size() * item_h;
    if (total_h <= 0)
      continue;

    mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, def.x - 1, def.y - 1,
                         def.x + def.w + 1);
    mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, def.x - 1, def.y + total_h,
                         def.x + def.w + 1);
    mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, def.x - 1, def.y - 1,
                         def.y + total_h + 1);
    mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, def.x + def.w, def.y - 1,
                         def.y + total_h + 1);
    mdgui_fill_rect_idx(nullptr, CLR_MENU_BG, def.x, def.y, def.w, total_h);

    for (int i = 0; i < (int)def.items.size(); ++i) {
      const int iy = def.y + (i * item_h);
      if (def.items[i].is_separator) {
        const int sep_y = iy + (item_h / 2);
        mdgui_draw_hline_idx(nullptr, CLR_BUTTON_LIGHT, def.x + 4, sep_y,
                             def.x + def.w - 4);
        mdgui_draw_hline_idx(nullptr, CLR_BUTTON_DARK, def.x + 4, sep_y + 1,
                             def.x + def.w - 4);
        continue;
      }
      const int hovered = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                        def.x, iy, def.w, item_h);
      if (hovered)
        mdgui_fill_rect_idx(nullptr, CLR_ACCENT, def.x, iy, def.w, item_h);
      if (mdgui_fonts[1]) {
        bool has_check = false;
        bool checked = false;
        const char *label = parse_menu_check_prefix(def.items[i].text.c_str(),
                                                    &has_check, &checked);
        const int check_w = has_check ? menu_check_indicator_width() : 0;
        const int text_x = def.x + 4 + check_w;
        if (has_check)
          draw_menu_check_indicator(def.x + 4, iy + 1, checked);
        mdgui_fonts[1]->drawText(label, nullptr, text_x, iy + 1, CLR_MENU_TEXT);
        if (def.items[i].child_menu_index >= 0) {
          mdgui_fonts[1]->drawText(">", nullptr, def.x + def.w - 8, iy + 1,
                                   CLR_MENU_TEXT);
        }
      }
    }
  }
}

static int get_logical_render_w(MDGUI_Context *ctx) {
  int rw = 320;
  int rh = 240;
  if (ctx)
    mdgui_backend_get_render_size(&rw, &rh);
  if (rw <= 0)
    rw = 320;
  return rw;
}

static int get_logical_render_h(MDGUI_Context *ctx) {
  int rw = 320;
  int rh = 240;
  if (ctx)
    mdgui_backend_get_render_size(&rw, &rh);
  if (rh <= 0)
    rh = 240;
  return rh;
}

static int get_status_bar_h(const MDGUI_Context *ctx) {
  if (!ctx || !ctx->status_bar_visible)
    return 0;
  if (ctx->status_bar_h > 0)
    return ctx->status_bar_h;
  return STATUS_BAR_DEFAULT_H;
}

static int get_work_area_top(MDGUI_Context *ctx) {
  return ctx ? ctx->main_menu_bar_h : 0;
}

static int get_work_area_bottom(MDGUI_Context *ctx) {
  int bottom = get_logical_render_h(ctx) - get_status_bar_h(ctx);
  if (bottom < 0)
    bottom = 0;
  return bottom;
}

static void clamp_window_to_work_area(MDGUI_Context *ctx, MDGUI_Window &win) {
  const int screen_w = get_logical_render_w(ctx);
  const int top = get_work_area_top(ctx);
  int bottom = get_work_area_bottom(ctx);
  if (bottom < top)
    bottom = top;

  if (win.w < 1)
    win.w = 1;
  if (win.h < 1)
    win.h = 1;
  if (win.w > screen_w)
    win.w = screen_w;
  const int max_h = std::max(1, bottom - top);
  if (win.h > max_h)
    win.h = max_h;

  int max_x = screen_w - win.w;
  if (max_x < 0)
    max_x = 0;
  int max_y = bottom - win.h;
  if (max_y < top)
    max_y = top;

  if (win.x < 0)
    win.x = 0;
  if (win.x > max_x)
    win.x = max_x;
  if (win.y < top)
    win.y = top;
  if (win.y > max_y)
    win.y = max_y;
}

static void apply_window_user_min_size(MDGUI_Context *ctx, MDGUI_Window &win) {
  if (!ctx)
    return;
  if (win.user_min_from_percent) {
    win.user_min_w =
        min_from_percent(get_logical_render_w(ctx), win.user_min_w_percent, 50);
    win.user_min_h =
        min_from_percent(get_logical_render_h(ctx), win.user_min_h_percent, 30);
  }
  apply_window_user_min_size(ctx, win);
}

static void draw_status_bar(MDGUI_Context *ctx) {
  if (!ctx || !ctx->status_bar_visible)
    return;
  const int rh = get_logical_render_h(ctx);
  const int rw = get_logical_render_w(ctx);
  const int bar_h = get_status_bar_h(ctx);
  if (rw <= 0 || bar_h <= 0 || rh <= 0)
    return;
  const int bar_y = rh - bar_h;
  if (bar_y < 0)
    return;

  mdgui_backend_set_clip_rect(0, 0, 0, 0, 0);
  mdgui_backend_set_alpha_mod(255);
  mdgui_fill_rect_idx(nullptr, CLR_MENU_BG, 0, bar_y, rw, bar_h);
  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, 0, bar_y, rw - 1);
  if (mdgui_fonts[1] && !ctx->status_bar_text.empty()) {
    mdgui_fonts[1]->drawText(ctx->status_bar_text.c_str(), nullptr, 3,
                             bar_y + 2, CLR_MENU_TEXT);
  }
}

static std::string ellipsize_text_to_width(const std::string &text,
                                           int max_w) {
  if (max_w <= 0 || !mdgui_fonts[1])
    return {};
  if (mdgui_fonts[1]->measureTextWidth(text.c_str()) <= max_w)
    return text;
  static const char *kEllipsis = "...";
  const int ellipsis_w = mdgui_fonts[1]->measureTextWidth(kEllipsis);
  if (ellipsis_w > max_w)
    return {};

  std::string out;
  out.reserve(text.size());
  for (char c : text) {
    out.push_back(c);
    std::string candidate = out + kEllipsis;
    if (mdgui_fonts[1]->measureTextWidth(candidate.c_str()) > max_w) {
      out.pop_back();
      break;
    }
  }
  return out + kEllipsis;
}

static void prune_expired_toasts(MDGUI_Context *ctx, unsigned long long now_ms) {
  if (!ctx)
    return;
  auto &toasts = ctx->toasts;
  toasts.erase(std::remove_if(toasts.begin(), toasts.end(),
                              [now_ms](const MDGUI_Context::ToastItem &t) {
                                return now_ms >= t.expires_at_ms;
                              }),
               toasts.end());
}

static void draw_toasts(MDGUI_Context *ctx) {
  if (!ctx || ctx->toasts.empty() || !mdgui_fonts[1])
    return;
  const int rw = get_logical_render_w(ctx);
  const int bottom = get_work_area_bottom(ctx);
  if (rw <= 0 || bottom <= 0)
    return;

  const int top = get_work_area_top(ctx) + 6;
  const int margin = 6;
  const int pad_x = 6;
  const int pad_y = 4;
  const int toast_h = 16;
  const int step = toast_h + 4;
  const int max_toast_w = std::max(80, rw - (margin * 2));
  const int text_max_w = std::max(20, max_toast_w - (pad_x * 2));

  mdgui_backend_set_clip_rect(0, 0, 0, 0, 0);
  mdgui_backend_set_alpha_mod(255);

  int y = top;
  const int total = (int)ctx->toasts.size();
  const int first = (total > TOAST_MAX_VISIBLE) ? (total - TOAST_MAX_VISIBLE) : 0;
  for (int i = first; i < total; ++i) {
    if (y + toast_h > bottom)
      break;
    const auto &toast = ctx->toasts[(size_t)i];
    std::string text = ellipsize_text_to_width(toast.text, text_max_w);
    const int text_w = text.empty() ? 0 : mdgui_fonts[1]->measureTextWidth(text.c_str());
    int toast_w = text_w + (pad_x * 2);
    if (toast_w < 80)
      toast_w = 80;
    if (toast_w > max_toast_w)
      toast_w = max_toast_w;
    const int x = rw - margin - toast_w;

    mdgui_fill_rect_idx(nullptr, CLR_MSG_BG, x, y, toast_w, toast_h);
    mdgui_draw_frame_idx(nullptr, CLR_BUTTON_LIGHT, x, y, x + toast_w,
                         y + toast_h);
    if (!text.empty()) {
      mdgui_fonts[1]->drawText(text.c_str(), nullptr, x + pad_x, y + pad_y,
                               CLR_TEXT_LIGHT);
    }
    y += step;
  }
}

static void center_window_rect_menu_aware(MDGUI_Context *ctx,
                                          MDGUI_Window &win) {
  const int screen_w = get_logical_render_w(ctx);
  const int top = get_work_area_top(ctx);
  const int bottom = get_work_area_bottom(ctx);
  const int avail_h = std::max(1, bottom - top);
  const int max_x = std::max(0, screen_w - win.w);
  const int max_y = top + std::max(0, avail_h - win.h);

  int x = (screen_w - win.w) / 2;
  int y = top + ((avail_h - win.h) / 2);
  if (x < 0)
    x = 0;
  if (x > max_x)
    x = max_x;
  if (y < top)
    y = top;
  if (y > max_y)
    y = max_y;

  win.x = x;
  win.y = y;
}

static bool ci_less(const std::string &a, const std::string &b) {
  size_t i = 0;
  const size_t n = std::min(a.size(), b.size());
  while (i < n) {
    const char ca = (char)std::tolower((unsigned char)a[i]);
    const char cb = (char)std::tolower((unsigned char)b[i]);
    if (ca < cb)
      return true;
    if (ca > cb)
      return false;
    i++;
  }
  return a.size() < b.size();
}

static std::vector<std::string> enumerate_file_browser_drives() {
  std::vector<std::string> roots;
#ifdef _WIN32
  const DWORD drive_mask = GetLogicalDrives();
  for (int i = 0; i < 26; ++i) {
    if ((drive_mask & (1u << i)) == 0)
      continue;
    std::string root;
    root.push_back((char)('A' + i));
    root += ":\\";
    std::error_code ec;
    if (std::filesystem::exists(root, ec) && !ec)
      roots.push_back(root);
  }
#else
  auto try_add_root = [&roots](const std::filesystem::path &p) {
    const std::string s = p.string();
    if (s.empty())
      return;
    for (const auto &existing : roots) {
      if (existing == s)
        return;
    }
    std::error_code ec;
    if (std::filesystem::is_directory(p, ec) && !ec)
      roots.push_back(s);
  };

  try_add_root("/");
#if defined(__APPLE__)
  std::error_code ec;
  for (const auto &entry : std::filesystem::directory_iterator(
           "/Volumes", std::filesystem::directory_options::skip_permission_denied,
           ec)) {
    if (ec)
      break;
    try_add_root(entry.path());
  }
#else
  std::error_code mnt_ec;
  for (const auto &entry : std::filesystem::directory_iterator(
           "/mnt", std::filesystem::directory_options::skip_permission_denied,
           mnt_ec)) {
    if (mnt_ec)
      break;
    try_add_root(entry.path());
  }

  const char *user_env = std::getenv("USER");
  if (user_env && user_env[0]) {
    const std::filesystem::path media_user =
        std::filesystem::path("/media") / user_env;
    std::error_code media_ec;
    for (const auto &entry : std::filesystem::directory_iterator(
             media_user, std::filesystem::directory_options::skip_permission_denied,
             media_ec)) {
      if (media_ec)
        break;
      try_add_root(entry.path());
    }

    const std::filesystem::path run_media_user =
        std::filesystem::path("/run/media") / user_env;
    std::error_code run_media_ec;
    for (const auto &entry : std::filesystem::directory_iterator(
             run_media_user, std::filesystem::directory_options::skip_permission_denied,
             run_media_ec)) {
      if (run_media_ec)
        break;
      try_add_root(entry.path());
    }
  }
#endif
#endif
  std::sort(roots.begin(), roots.end(), ci_less);
  return roots;
}

static bool file_browser_path_exists(const std::string &path) {
  if (path.empty())
    return false;
  std::error_code ec;
  return std::filesystem::exists(path, ec) && !ec;
}

static std::string file_browser_default_open_path(const MDGUI_Context *ctx) {
  if (!ctx || ctx->file_browser_drives.empty())
    return ".";
  return ctx->file_browser_drives.front();
}

static void file_browser_refresh_drives(MDGUI_Context *ctx, bool force_scan) {
  if (!ctx)
    return;
  const Uint64 now = mdgui_backend_get_ticks_ms();
  if (!force_scan && ctx->file_browser_last_drive_scan_ticks > 0 &&
      (now - ctx->file_browser_last_drive_scan_ticks) < 500) {
    return;
  }
  ctx->file_browser_last_drive_scan_ticks = now;
  ctx->file_browser_drives = enumerate_file_browser_drives();
}

static std::string normalize_extension_filter(const char *ext) {
  if (!ext)
    return {};
  std::string s(ext);
  if (s.empty())
    return {};
  for (char &c : s)
    c = (char)std::tolower((unsigned char)c);
  if (s == "*" || s == "*.*")
    return {};
  if (!s.empty() && s[0] != '.')
    s.insert(s.begin(), '.');
  return s;
}

static std::string path_extension_lower(const std::filesystem::path &p) {
  if (!p.has_extension())
    return {};
  std::string ext = p.extension().string();
  for (char &c : ext)
    c = (char)std::tolower((unsigned char)c);
  return ext;
}

static bool file_matches_filters(const MDGUI_Context *ctx,
                                 const std::filesystem::path &p) {
  if (!ctx || ctx->file_browser_ext_filters.empty())
    return true; // default: show all
                 // regular files
  const std::string ext = path_extension_lower(p);
  if (ext.empty())
    return false;
  for (const auto &f : ctx->file_browser_ext_filters) {
    if (ext == f)
      return true;
  }
  return false;
}

static void file_browser_open_path(MDGUI_Context *ctx, const char *path) {
  if (!ctx)
    return;
  namespace fs = std::filesystem;
  std::error_code ec;
  fs::path next(path ? path : ".");
  if (next.is_relative()) {
    const fs::path base = ctx->file_browser_cwd.empty()
                              ? fs::path(".")
                              : fs::path(ctx->file_browser_cwd);
    next = base / next;
  }

  fs::path norm = fs::weakly_canonical(next, ec);
  if (ec) {
    norm = fs::absolute(next, ec);
    if (ec)
      norm = next;
  }

  ctx->file_browser_entries.clear();
  ctx->file_browser_cwd = norm.string();
  ctx->file_browser_selected = -1;
  ctx->file_browser_scroll = 0;
  ctx->file_browser_restore_scroll_pending = false;
  ctx->file_browser_scroll_dragging = false;
  ctx->file_browser_scroll_drag_offset = 0;

  fs::path cwd_path(ctx->file_browser_cwd);
  if (cwd_path.has_parent_path()) {
    FileBrowserEntry up{};
    up.label = "..";
    up.full_path = cwd_path.parent_path().string();
    up.is_dir = true;
    ctx->file_browser_entries.push_back(up);
  }

  std::vector<FileBrowserEntry> dirs;
  std::vector<FileBrowserEntry> files;
  for (const auto &entry : fs::directory_iterator(
           cwd_path, fs::directory_options::skip_permission_denied, ec)) {
    if (ec)
      break;
    std::error_code st_ec;
    const bool is_dir = entry.is_directory(st_ec);
    if (st_ec)
      continue;

    FileBrowserEntry row{};
    row.full_path = entry.path().string();
    row.is_dir = is_dir;
    const std::string name = entry.path().filename().string();
    if (is_dir) {
      row.label = std::string("[DIR] ") + name;
      dirs.push_back(row);
    } else if (!ctx->file_browser_select_folders &&
               entry.is_regular_file(st_ec) && !st_ec &&
               file_matches_filters(ctx, entry.path())) {
      row.label = name;
      files.push_back(row);
    }
  }

  std::sort(dirs.begin(), dirs.end(),
            [](const FileBrowserEntry &a, const FileBrowserEntry &b) {
              return ci_less(a.label, b.label);
            });
  std::sort(files.begin(), files.end(),
            [](const FileBrowserEntry &a, const FileBrowserEntry &b) {
              return ci_less(a.label, b.label);
            });

  ctx->file_browser_entries.insert(ctx->file_browser_entries.end(),
                                   dirs.begin(), dirs.end());
  if (!ctx->file_browser_select_folders) {
    ctx->file_browser_entries.insert(ctx->file_browser_entries.end(),
                                     files.begin(), files.end());
  }
  if (!ctx->file_browser_entries.empty()) {
    int restore_idx = -1;
    if (!ctx->file_browser_last_selected_path.empty()) {
      for (int i = 0; i < (int)ctx->file_browser_entries.size(); ++i) {
        if (!ctx->file_browser_select_folders &&
            !ctx->file_browser_entries[(size_t)i].is_dir &&
            ctx->file_browser_entries[(size_t)i].full_path ==
                ctx->file_browser_last_selected_path) {
          restore_idx = i;
          break;
        }
      }
    }
    ctx->file_browser_selected = (restore_idx >= 0) ? restore_idx : 0;
    ctx->file_browser_restore_scroll_pending = (restore_idx >= 0);
  }
}

static void draw_open_main_menu_overlay(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  if (ctx->open_main_menu_path.empty())
    return;

  // Main-menu popups are an overlay and should not inherit clip or alpha
  // state from any previously drawn window/content region.
  mdgui_backend_set_clip_rect(0, 0, 0, 0, 0);
  mdgui_backend_set_alpha_mod(255);

  const int item_h = MAIN_MENU_ITEM_H;
  for (int depth = 0; depth < (int)ctx->open_main_menu_path.size(); ++depth) {
    const int menu_idx = ctx->open_main_menu_path[depth];
    if (menu_idx < 0 || menu_idx >= (int)ctx->main_menu_defs.size())
      continue;
    auto &def = ctx->main_menu_defs[menu_idx];
    const int total_h = (int)def.items.size() * item_h;
    if (total_h <= 0)
      continue;

    mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, def.x - 1, def.y - 1,
                         def.x + def.w + 1);
    mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, def.x - 1, def.y + total_h,
                         def.x + def.w + 1);
    mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, def.x - 1, def.y - 1,
                         def.y + total_h + 1);
    mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, def.x + def.w, def.y - 1,
                         def.y + total_h + 1);
    mdgui_fill_rect_idx(nullptr, CLR_MENU_BG, def.x, def.y, def.w, total_h);

    for (int i = 0; i < (int)def.items.size(); ++i) {
      const int iy = def.y + (i * item_h);
      if (def.items[i].is_separator) {
        const int sep_y = iy + (item_h / 2);
        mdgui_draw_hline_idx(nullptr, CLR_BUTTON_LIGHT, def.x + 4, sep_y,
                             def.x + def.w - 4);
        mdgui_draw_hline_idx(nullptr, CLR_BUTTON_DARK, def.x + 4, sep_y + 1,
                             def.x + def.w - 4);
        continue;
      }
      const int hovered = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                        def.x, iy, def.w, item_h);
      if (hovered) {
        mdgui_fill_rect_idx(nullptr, CLR_ACCENT, def.x, iy, def.w, item_h);
        if (def.items[i].child_menu_index >= 0) {
          const int child_idx = def.items[i].child_menu_index;
          const int want_size = depth + 2;
          if ((int)ctx->open_main_menu_path.size() < want_size) {
            ctx->open_main_menu_path.resize((size_t)want_size);
          }
          if ((int)ctx->open_main_menu_item_path.size() < want_size) {
            ctx->open_main_menu_item_path.resize((size_t)want_size, -1);
          }
          ctx->open_main_menu_path[depth + 1] = child_idx;
          ctx->open_main_menu_item_path[depth + 1] = i;
          ctx->open_main_menu_index = ctx->open_main_menu_path.empty()
                                          ? -1
                                          : ctx->open_main_menu_path[0];
        } else if ((int)ctx->open_main_menu_path.size() > depth + 1) {
          ctx->open_main_menu_path.resize((size_t)depth + 1);
          if ((int)ctx->open_main_menu_item_path.size() > depth + 1) {
            ctx->open_main_menu_item_path.resize((size_t)depth + 1);
          }
          ctx->open_main_menu_index = ctx->open_main_menu_path.empty()
                                          ? -1
                                          : ctx->open_main_menu_path[0];
        }
      }
      if (mdgui_fonts[1]) {
        bool has_check = false;
        bool checked = false;
        const char *label = parse_menu_check_prefix(def.items[i].text.c_str(),
                                                    &has_check, &checked);
        const int check_w = has_check ? menu_check_indicator_width() : 0;
        const int text_x = def.x + 4 + check_w;
        if (has_check)
          draw_menu_check_indicator(def.x + 4, iy + 1, checked);
        mdgui_fonts[1]->drawText(label, nullptr, text_x, iy + 1, CLR_MENU_TEXT);
        if (def.items[i].child_menu_index >= 0) {
          mdgui_fonts[1]->drawText(">", nullptr, def.x + def.w - 8, iy + 1,
                                   CLR_MENU_TEXT);
        }
      }
    }
  }
}

extern "C" {
