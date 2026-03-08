#include "mdgui_c.h"
#include "mdgui_backends.h"
#include "mdgui_primitives.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>
#include <string.h>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <string>
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

struct MDGUI_Window {
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
  int open_combo_id;
  int text_scroll;
  bool text_scroll_dragging;
  int text_scroll_drag_offset;
  bool fixed_rect;
  bool disallow_maximize;
  int tile_weight;
  int tile_side;
};

struct FileBrowserEntry {
  std::string label;
  std::string full_path;
  bool is_dir;
};
} // namespace

struct MDGUI_Context {
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
    int x;
    int y;
    int w;
    std::vector<std::string> items;
  };
  std::vector<MenuDef> menu_defs;

  int main_menu_index;
  int main_menu_next_x;
  bool in_main_menu_bar;
  bool in_main_menu;
  int open_main_menu_index;
  int building_main_menu_index;
  std::vector<MenuDef> main_menu_defs;
  int main_menu_bar_x;
  int main_menu_bar_y;
  int main_menu_bar_w;
  int main_menu_bar_h;

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
  bool window_has_nonlabel_widget;
  bool windows_locked;
  bool tile_manager_enabled;

  bool combo_overlay_pending;
  int combo_overlay_window;
  int combo_overlay_x;
  int combo_overlay_y;
  int combo_overlay_w;
  int combo_overlay_item_h;
  int combo_overlay_item_count;
  int combo_overlay_selected;
  const char **combo_overlay_items;

  bool file_browser_open;
  std::string file_browser_cwd;
  std::vector<FileBrowserEntry> file_browser_entries;
  int file_browser_selected;
  int file_browser_scroll;
  bool file_browser_scroll_dragging;
  int file_browser_scroll_drag_offset;
  int file_browser_last_click_idx;
  Uint64 file_browser_last_click_ticks;
  std::string file_browser_result;
  std::vector<std::string> file_browser_ext_filters;
};

static void note_content_bounds(MDGUI_Context *ctx, int right, int bottom) {
  if (!ctx || ctx->current_window < 0)
    return;
  if (right > ctx->content_req_right)
    ctx->content_req_right = right;
  if (bottom > ctx->content_req_bottom)
    ctx->content_req_bottom = bottom;
}

static void set_content_clip(MDGUI_Context *ctx) {
  if (!ctx || ctx->current_window < 0 ||
      ctx->current_window >= (int)ctx->windows.size())
    return;
  const auto &win = ctx->windows[ctx->current_window];
  int clip_w = win.w - 4;
  int clip_h = (win.y + win.h - 4) - ctx->origin_y;
  if (clip_w < 1)
    clip_w = 1;
  if (clip_h < 1)
    clip_h = 1;
  mdgui_backend_set_clip_rect(1, win.x + 2, ctx->origin_y, clip_w, clip_h);
}

static int resolve_dynamic_width(MDGUI_Context *ctx, int local_x, int w,
                                 int min_w = 1) {
  if (!ctx || ctx->current_window < 0)
    return (w > 0) ? w : min_w;
  const auto &win = ctx->windows[ctx->current_window];
  // Reserve a right gutter so vertical scrollbars never overlap widgets.
  const int right_pad = 14;
  int avail = (win.x + win.w - right_pad) - (ctx->origin_x + local_x);
  if (avail < min_w)
    avail = min_w;
  if (w == 0)
    return avail;
  if (w < 0) {
    int adjusted = avail + w;
    return (adjusted < min_w) ? min_w : adjusted;
  }
  return w;
}

static void drawrect_rgb(MDGUI_Context *ctx, uint8_t r, uint8_t g, uint8_t b,
                         int x, int y, int w, int h) {
  if (!ctx)
    return;
  if (!ctx->backend.fill_rect_rgba)
    return;
  ctx->backend.fill_rect_rgba(ctx->backend.user_data, r, g, b, 255, x, y, w,
                              h);
}

static int point_in_rect(int px, int py, int x, int y, int w, int h) {
  return px >= x && py >= y && px < (x + w) && py < (y + h);
}

static int top_window_at_point(const MDGUI_Context *ctx, int px, int py,
                               int margin = 0) {
  int best = -1;
  int best_z = -2147483647;
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    const auto &w = ctx->windows[i];
    if (w.closed)
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

static int get_logical_render_w(MDGUI_Context *ctx);
static int get_logical_render_h(MDGUI_Context *ctx);

static int clampi(int v, int lo, int hi) {
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

static int normalize_tile_side(int side) {
  if (side < MDGUI_TILE_SIDE_AUTO || side > MDGUI_TILE_SIDE_BOTTOM)
    return MDGUI_TILE_SIDE_AUTO;
  return side;
}

static int normalize_tile_weight(int weight) {
  return (weight < 1) ? 1 : weight;
}

static void assign_tiled_window_rect(MDGUI_Window &w, int x, int y, int ww,
                                     int hh) {
  w.x = x;
  w.y = y;
  w.w = ww;
  w.h = hh;
  w.restored_x = x;
  w.restored_y = y;
  w.restored_w = ww;
  w.restored_h = hh;
  w.is_maximized = false;
  w.fixed_rect = true;
}

static void tile_windows_grid(MDGUI_Context *ctx, const std::vector<int> &order,
                              int left, int top, int content_w, int content_h,
                              int gap) {
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

  int y = top;
  int idx = 0;
  for (int row = 0; row < rows && idx < count; ++row) {
    const int rows_left = rows - row;
    const int remaining = count - idx;
    const int cols_this_row = (remaining + rows_left - 1) / rows_left;
    const int h_remaining = (top + content_h) - y - (rows_left - 1) * gap;
    const int row_h = std::max(80, h_remaining / rows_left);

    int x = left;
    for (int col = 0; col < cols_this_row && idx < count; ++col) {
      const int cols_left = cols_this_row - col;
      const int w_remaining = (left + content_w) - x - (cols_left - 1) * gap;
      const int cell_w = std::max(120, w_remaining / cols_left);
      auto &w = ctx->windows[order[idx]];
      assign_tiled_window_rect(w, x, y, cell_w, row_h);
      ++idx;
      x += cell_w + gap;
    }
    y += row_h + gap;
  }
}

static int sum_tile_weights(MDGUI_Context *ctx, const std::vector<int> &indices) {
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

static void sort_by_weight_then_z(MDGUI_Context *ctx, std::vector<int> &indices) {
  if (!ctx)
    return;
  std::stable_sort(indices.begin(), indices.end(),
                   [ctx](int a, int b) {
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
                                           const std::vector<int> &order,
                                           int x, int y, int w, int h,
                                           int gap) {
  if (!ctx || order.empty() || w <= 0 || h <= 0)
    return;
  if ((int)order.size() == 1) {
    assign_tiled_window_rect(ctx->windows[order[0]], x, y, w, h);
    return;
  }

  const int min_h = 80;
  int total_weight = sum_tile_weights(ctx, order);
  if (total_weight < 1)
    total_weight = (int)order.size();
  int remaining_weight = total_weight;
  int cy = y;

  for (int i = 0; i < (int)order.size(); ++i) {
    const int idx = order[i];
    const int items_left = (int)order.size() - i;
    int available_h = (y + h) - cy - (items_left - 1) * gap;
    if (available_h < min_h)
      available_h = min_h;

    int cell_h = available_h;
    if (i < (int)order.size() - 1 && remaining_weight > 0) {
      const int wgt = normalize_tile_weight(ctx->windows[idx].tile_weight);
      int proportional = (int)((double)available_h * (double)wgt /
                               (double)remaining_weight);
      int max_for_this = available_h - (items_left - 1) * min_h;
      if (max_for_this < min_h)
        max_for_this = min_h;
      cell_h = clampi(proportional, min_h, max_for_this);
    }

    assign_tiled_window_rect(ctx->windows[idx], x, cy, w, cell_h);
    cy += cell_h + gap;
    remaining_weight -= normalize_tile_weight(ctx->windows[idx].tile_weight);
    if (remaining_weight < 0)
      remaining_weight = 0;
  }
}

static void tile_windows_horizontal_weighted(MDGUI_Context *ctx,
                                             const std::vector<int> &order,
                                             int x, int y, int w, int h,
                                             int gap) {
  if (!ctx || order.empty() || w <= 0 || h <= 0)
    return;
  if ((int)order.size() == 1) {
    assign_tiled_window_rect(ctx->windows[order[0]], x, y, w, h);
    return;
  }

  const int min_w = 120;
  int total_weight = sum_tile_weights(ctx, order);
  if (total_weight < 1)
    total_weight = (int)order.size();
  int remaining_weight = total_weight;
  int cx = x;

  for (int i = 0; i < (int)order.size(); ++i) {
    const int idx = order[i];
    const int items_left = (int)order.size() - i;
    int available_w = (x + w) - cx - (items_left - 1) * gap;
    if (available_w < min_w)
      available_w = min_w;

    int cell_w = available_w;
    if (i < (int)order.size() - 1 && remaining_weight > 0) {
      const int wgt = normalize_tile_weight(ctx->windows[idx].tile_weight);
      int proportional = (int)((double)available_w * (double)wgt /
                               (double)remaining_weight);
      int max_for_this = available_w - (items_left - 1) * min_w;
      if (max_for_this < min_w)
        max_for_this = min_w;
      cell_w = clampi(proportional, min_w, max_for_this);
    }

    assign_tiled_window_rect(ctx->windows[idx], cx, y, cell_w, h);
    cx += cell_w + gap;
    remaining_weight -= normalize_tile_weight(ctx->windows[idx].tile_weight);
    if (remaining_weight < 0)
      remaining_weight = 0;
  }
}

static void tile_windows_weighted_cluster(MDGUI_Context *ctx,
                                          const std::vector<int> &order,
                                          int left, int top, int content_w,
                                          int content_h, int gap) {
  if (!ctx || order.empty() || content_w <= 0 || content_h <= 0)
    return;
  const int count = (int)order.size();
  if (count == 1) {
    assign_tiled_window_rect(ctx->windows[order[0]], left, top, content_w,
                             content_h);
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

  if (best_weight <= 1) {
    tile_windows_grid(ctx, order, left, top, content_w, content_h, gap);
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
    secondary_weight_sum += normalize_tile_weight(ctx->windows[idx].tile_weight);
  }
  if (secondary_weight_sum < 1)
    secondary_weight_sum = (int)secondary.size();

  const float split_raw =
      (float)best_weight / (float)(best_weight + secondary_weight_sum);
  const float split_ratio = std::max(0.34f, std::min(0.75f, split_raw));
  const int side = (content_w >= content_h) ? MDGUI_TILE_SIDE_LEFT
                                             : MDGUI_TILE_SIDE_TOP;

  int px = left;
  int py = top;
  int pw = content_w;
  int ph = content_h;
  int sx = left;
  int sy = top;
  int sw = content_w;
  int sh = content_h;

  const int min_primary_w = 160;
  const int min_secondary_w = 120;
  const int min_primary_h = 100;
  const int min_secondary_h = 80;

  if (side == MDGUI_TILE_SIDE_LEFT || side == MDGUI_TILE_SIDE_RIGHT) {
    if (content_w < (min_primary_w + min_secondary_w + gap)) {
      tile_windows_grid(ctx, order, left, top, content_w, content_h, gap);
      return;
    }
    int p_w = (int)((float)content_w * split_ratio);
    p_w = clampi(p_w, min_primary_w, content_w - min_secondary_w - gap);
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
  } else {
    if (content_h < (min_primary_h + min_secondary_h + gap)) {
      tile_windows_grid(ctx, order, left, top, content_w, content_h, gap);
      return;
    }
    int p_h = (int)((float)content_h * split_ratio);
    p_h = clampi(p_h, min_primary_h, content_h - min_secondary_h - gap);
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
  }

  assign_tiled_window_rect(ctx->windows[order[primary_pos]], px, py, pw, ph);
  tile_windows_grid(ctx, secondary, sx, sy, sw, sh, gap);
}

static void tile_windows_internal(MDGUI_Context *ctx) {
  if (!ctx)
    return;

  const int rw = get_logical_render_w(ctx);
  const int rh = get_logical_render_h(ctx);

  const int gap = 6;
  int top = ctx->main_menu_bar_h + 6;
  if (top < 18)
    top = 18;
  const int bottom = 6;
  const int left = gap;
  const int right = gap;

  const int content_w = rw - left - right;
  const int content_h = rh - top - bottom;
  if (content_w <= 0 || content_h <= 0)
    return;

  std::vector<int> order;
  order.reserve(ctx->windows.size());
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    const auto &w = ctx->windows[i];
    if (w.closed || w.is_maximized || w.disallow_maximize)
      continue;
    order.push_back(i);
  }
  if (order.empty())
    return;

  const int count = (int)order.size();
  if (count == 1) {
    auto &only = ctx->windows[order[0]];
    assign_tiled_window_rect(only, left, top, content_w, content_h);
    return;
  }

  std::vector<int> left_group;
  std::vector<int> right_group;
  std::vector<int> top_group;
  std::vector<int> bottom_group;
  std::vector<int> auto_group;
  left_group.reserve(order.size());
  right_group.reserve(order.size());
  top_group.reserve(order.size());
  bottom_group.reserve(order.size());
  auto_group.reserve(order.size());

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

    int center_x = left;
    int center_y = top;
    int center_w = content_w;
    int center_h = content_h;

    const int min_center_w = 180;
    const int min_center_h = 120;
    const int min_side_w = 140;
    const int min_side_h = 90;

    int left_w_px = 0;
    if (!left_group.empty()) {
      int total_w_weight = left_sum + right_sum + auto_sum;
      if (total_w_weight < 1)
        total_w_weight = 1;
      int reserve_right = right_group.empty() ? 0 : (min_side_w + gap);
      int max_left_w = center_w - min_center_w - reserve_right;
      if (max_left_w >= min_side_w) {
        int raw = (int)((double)center_w * (double)left_sum /
                        (double)total_w_weight);
        left_w_px = clampi(raw, min_side_w, max_left_w);
      }
    }
    if (left_w_px > 0) {
      tile_windows_vertical_weighted(ctx, left_group, center_x, top, left_w_px,
                                     content_h, gap);
      center_x += left_w_px + gap;
      center_w -= left_w_px + gap;
    } else {
      auto_group.insert(auto_group.end(), left_group.begin(), left_group.end());
      left_group.clear();
      auto_sum = sum_tile_weights(ctx, auto_group);
    }

    int right_w_px = 0;
    if (!right_group.empty()) {
      int total_w_weight = right_sum + auto_sum;
      if (total_w_weight < 1)
        total_w_weight = 1;
      int max_right_w = center_w - min_center_w;
      if (max_right_w >= min_side_w) {
        int raw = (int)((double)center_w * (double)right_sum /
                        (double)total_w_weight);
        right_w_px = clampi(raw, min_side_w, max_right_w);
      }
    }
    if (right_w_px > 0) {
      const int rx = center_x + center_w - right_w_px;
      tile_windows_vertical_weighted(ctx, right_group, rx, top, right_w_px,
                                     content_h, gap);
      center_w -= right_w_px + gap;
    } else {
      auto_group.insert(auto_group.end(), right_group.begin(), right_group.end());
      right_group.clear();
      auto_sum = sum_tile_weights(ctx, auto_group);
    }

    int top_h_px = 0;
    if (!top_group.empty()) {
      int total_h_weight = top_sum + bottom_sum + auto_sum;
      if (total_h_weight < 1)
        total_h_weight = 1;
      int reserve_bottom = bottom_group.empty() ? 0 : (min_side_h + gap);
      int max_top_h = center_h - min_center_h - reserve_bottom;
      if (max_top_h >= min_side_h) {
        int raw = (int)((double)center_h * (double)top_sum /
                        (double)total_h_weight);
        top_h_px = clampi(raw, min_side_h, max_top_h);
      }
    }
    if (top_h_px > 0) {
      tile_windows_horizontal_weighted(ctx, top_group, center_x, center_y,
                                       center_w, top_h_px, gap);
      center_y += top_h_px + gap;
      center_h -= top_h_px + gap;
    } else {
      auto_group.insert(auto_group.end(), top_group.begin(), top_group.end());
      top_group.clear();
    }

    int bottom_h_px = 0;
    if (!bottom_group.empty()) {
      int total_h_weight = bottom_sum + sum_tile_weights(ctx, auto_group);
      if (total_h_weight < 1)
        total_h_weight = 1;
      int max_bottom_h = center_h - min_center_h;
      if (max_bottom_h >= min_side_h) {
        int raw = (int)((double)center_h * (double)bottom_sum /
                        (double)total_h_weight);
        bottom_h_px = clampi(raw, min_side_h, max_bottom_h);
      }
    }
    if (bottom_h_px > 0) {
      const int by = center_y + center_h - bottom_h_px;
      tile_windows_horizontal_weighted(ctx, bottom_group, center_x, by, center_w,
                                       bottom_h_px, gap);
      center_h -= bottom_h_px + gap;
    } else {
      auto_group.insert(auto_group.end(), bottom_group.begin(),
                        bottom_group.end());
      bottom_group.clear();
    }

    if (!auto_group.empty()) {
      sort_by_weight_then_z(ctx, auto_group);
      tile_windows_weighted_cluster(ctx, auto_group, center_x, center_y, center_w,
                                    center_h, gap);
    }
    return;
  }
  tile_windows_weighted_cluster(ctx, order, left, top, content_w, content_h,
                                gap);
}

static int find_or_create_window(MDGUI_Context *ctx, const char *title, int x,
                                 int y, int w, int h) {
  const char *key = title ? title : "window";
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    if (ctx->windows[i].id == key) {
      ctx->windows[i].title = key;
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
  nw.is_maximized = false;
  nw.closed = false;
  nw.restored_x = x;
  nw.restored_y = y;
  nw.restored_w = w;
  nw.restored_h = h;
  nw.last_edge_mask = 0;
  nw.min_w = 50;
  nw.min_h = 30;
  nw.open_combo_id = -1;
  nw.text_scroll = 0;
  nw.text_scroll_dragging = false;
  nw.text_scroll_drag_offset = 0;
  nw.fixed_rect = false;
  nw.disallow_maximize = false;
  nw.tile_weight = 1;
  nw.tile_side = MDGUI_TILE_SIDE_AUTO;
  ctx->windows.push_back(nw);
  return (int)ctx->windows.size() - 1;
}

static int is_current_window_topmost(MDGUI_Context *ctx, int margin = 0) {
  if (ctx->current_window < 0)
    return 0;
  const auto &w = ctx->windows[ctx->current_window];
  return top_window_at_point(ctx, ctx->input.mouse_x, ctx->input.mouse_y,
                             margin) == ctx->current_window ||
         w.z >= ctx->z_counter;
}

static void draw_open_menu_overlay(MDGUI_Context *ctx) {
  if (!ctx || ctx->current_window < 0)
    return;
  auto &win = ctx->windows[ctx->current_window];
  if (win.open_menu_index < 0 ||
      win.open_menu_index >= (int)ctx->menu_defs.size())
    return;

  auto &def = ctx->menu_defs[win.open_menu_index];
  const int item_h = ctx->current_menu_h;
  const int total_h = (int)def.items.size() * item_h;
  if (total_h <= 0)
    return;

  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, def.x - 1, def.y - 1, def.x + def.w + 1);
  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, def.x - 1, def.y + total_h, def.x + def.w + 1);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, def.x - 1, def.y - 1, def.y + total_h + 1);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, def.x + def.w, def.y - 1, def.y + total_h + 1);
  mdgui_fill_rect_idx(nullptr, CLR_MENU_BG, def.x, def.y, def.w, total_h);

  for (int i = 0; i < (int)def.items.size(); ++i) {
    const int iy = def.y + (i * item_h);
    const int hovered = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                      def.x, iy, def.w, item_h);
    if (hovered) {
      mdgui_fill_rect_idx(nullptr, CLR_ACCENT, def.x, iy, def.w, item_h);
    }
    if (mdgui_fonts[1]) {
      mdgui_fonts[1]->drawText(def.items[i].c_str(), nullptr, def.x + 4, iy + 1,
                    CLR_MENU_TEXT);
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
    return true; // default: show all regular files
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
    } else if (entry.is_regular_file(st_ec) && !st_ec &&
               file_matches_filters(ctx, entry.path())) {
      row.label = name;
      files.push_back(row);
    }
  }

  std::sort(dirs.begin(), dirs.end(), [](const FileBrowserEntry &a,
                                         const FileBrowserEntry &b) {
    return ci_less(a.label, b.label);
  });
  std::sort(files.begin(), files.end(), [](const FileBrowserEntry &a,
                                           const FileBrowserEntry &b) {
    return ci_less(a.label, b.label);
  });

  ctx->file_browser_entries.insert(ctx->file_browser_entries.end(), dirs.begin(),
                                   dirs.end());
  ctx->file_browser_entries.insert(ctx->file_browser_entries.end(), files.begin(),
                                   files.end());
  if (!ctx->file_browser_entries.empty())
    ctx->file_browser_selected = 0;
}

static void draw_open_main_menu_overlay(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  if (ctx->open_main_menu_index < 0 ||
      ctx->open_main_menu_index >= (int)ctx->main_menu_defs.size())
    return;

  auto &def = ctx->main_menu_defs[ctx->open_main_menu_index];
  const int item_h = ctx->current_menu_h;
  const int total_h = (int)def.items.size() * item_h;
  if (total_h <= 0)
    return;

  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, def.x - 1, def.y - 1, def.x + def.w + 1);
  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, def.x - 1, def.y + total_h, def.x + def.w + 1);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, def.x - 1, def.y - 1, def.y + total_h + 1);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, def.x + def.w, def.y - 1, def.y + total_h + 1);
  mdgui_fill_rect_idx(nullptr, CLR_MENU_BG, def.x, def.y, def.w, total_h);

  for (int i = 0; i < (int)def.items.size(); ++i) {
    const int iy = def.y + (i * item_h);
    const int hovered = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                      def.x, iy, def.w, item_h);
    if (hovered)
      mdgui_fill_rect_idx(nullptr, CLR_ACCENT, def.x, iy, def.w, item_h);
    if (mdgui_fonts[1])
      mdgui_fonts[1]->drawText(def.items[i].c_str(), nullptr, def.x + 4, iy + 1,
                    CLR_MENU_TEXT);
  }
}

extern "C" {
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
  ctx->main_menu_index = 0;
  ctx->main_menu_next_x = 0;
  ctx->in_main_menu_bar = false;
  ctx->in_main_menu = false;
  ctx->open_main_menu_index = -1;
  ctx->building_main_menu_index = -1;
  ctx->main_menu_bar_x = 0;
  ctx->main_menu_bar_y = 0;
  ctx->main_menu_bar_w = 0;
  ctx->main_menu_bar_h = 0;

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
  // Only SDL system cursors are used in release-safe builds.
  ctx->cursors[0] = SDL_CreateSystemCursor((SDL_SystemCursor)0);
  ctx->cursor_anim_count = ctx->cursors[0] ? 1 : 0;
  ctx->cursors[5] = SDL_CreateSystemCursor((SDL_SystemCursor)5); // NWSE
  ctx->cursors[6] = SDL_CreateSystemCursor((SDL_SystemCursor)6); // NESW
  ctx->cursors[7] = SDL_CreateSystemCursor((SDL_SystemCursor)7); // EW
  ctx->cursors[8] = SDL_CreateSystemCursor((SDL_SystemCursor)8); // NS
  ctx->cursors[9] = SDL_CreateSystemCursor((SDL_SystemCursor)0); // default arrow
  ctx->custom_cursor_enabled = false;
  ctx->cursor_request_idx = 9;
  ctx->current_cursor_idx = -1;
  ctx->content_req_right = 0;
  ctx->content_req_bottom = 0;
  ctx->window_has_nonlabel_widget = false;
  ctx->windows_locked = false;
  ctx->tile_manager_enabled = false;
  ctx->combo_overlay_pending = false;
  ctx->combo_overlay_window = -1;
  ctx->combo_overlay_x = 0;
  ctx->combo_overlay_y = 0;
  ctx->combo_overlay_w = 0;
  ctx->combo_overlay_item_h = 0;
  ctx->combo_overlay_item_count = 0;
  ctx->combo_overlay_selected = -1;
  ctx->combo_overlay_items = nullptr;
  ctx->file_browser_open = false;
  ctx->file_browser_cwd = ".";
  ctx->file_browser_selected = -1;
  ctx->file_browser_scroll = 0;
  ctx->file_browser_scroll_dragging = false;
  ctx->file_browser_scroll_drag_offset = 0;
  ctx->file_browser_last_click_idx = -1;
  ctx->file_browser_last_click_ticks = 0;
  ctx->file_browser_result.clear();

  mdgui_set_backend(ctx, backend);
  for (int i = 0; i < 10; i++) {
    if (!mdgui_fonts[i]) {
      mdgui_fonts[i] = new MDGuiFont();
    }
  }
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

void mdgui_begin_frame(MDGUI_Context *ctx, const MDGUI_Input *input) {
  if (!ctx || !input)
    return;
  mdgui_bind_backend(&ctx->backend);
  mdgui_backend_begin_frame();
  ctx->input = *input;
  const bool main_menu_modal = (ctx->open_main_menu_index != -1);
  if (ctx->custom_cursor_enabled && ctx->cursor_anim_count > 1) {
    const unsigned long long ticks = mdgui_backend_get_ticks_ms();
    const int anim_phase = (int)((ticks / 90ull) %
                                 (unsigned long long)ctx->cursor_anim_count);
    ctx->cursor_request_idx = anim_phase;
  } else if (ctx->custom_cursor_enabled) {
    ctx->cursor_request_idx = 0;
  } else {
    ctx->cursor_request_idx = ctx->cursors[9] ? 9 : 0;
  }

  // Bring the clicked window to front before any drawing in this frame.
  if (ctx->input.mouse_pressed && !main_menu_modal) {
    const int clicked_idx =
        top_window_at_point(ctx, ctx->input.mouse_x, ctx->input.mouse_y, 3);
    if (clicked_idx >= 0 && clicked_idx < (int)ctx->windows.size()) {
      ctx->windows[clicked_idx].z = ++ctx->z_counter;
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
    }
  }

  if (ctx->windows_locked) {
    ctx->dragging_window = -1;
    ctx->resizing_window = -1;
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
  ctx->in_main_menu_bar = false;
  ctx->in_main_menu = false;
  ctx->building_main_menu_index = -1;
  ctx->main_menu_defs.clear();
  ctx->content_req_right = 0;
  ctx->content_req_bottom = 0;
  ctx->window_has_nonlabel_widget = false;
  ctx->combo_overlay_pending = false;
  ctx->combo_overlay_window = -1;
  ctx->combo_overlay_items = nullptr;
}

void mdgui_end_frame(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  mdgui_bind_backend(&ctx->backend);
  if (ctx->open_main_menu_index != -1 && ctx->input.mouse_pressed) {
    bool in_bar = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                ctx->main_menu_bar_x, ctx->main_menu_bar_y,
                                ctx->main_menu_bar_w, ctx->main_menu_bar_h);
    bool in_popup = false;
    if (ctx->open_main_menu_index >= 0 &&
        ctx->open_main_menu_index < (int)ctx->main_menu_defs.size()) {
      const auto &def = ctx->main_menu_defs[ctx->open_main_menu_index];
      const int popup_h = (int)def.items.size() * ctx->current_menu_h;
      in_popup = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, def.x,
                               def.y, def.w, popup_h);
    }
    if (!in_bar && !in_popup) {
      ctx->open_main_menu_index = -1;
    }
  }

  draw_open_main_menu_overlay(ctx);

  ctx->current_window = -1;
  ctx->in_menu_bar = false;
  ctx->in_menu = false;
  ctx->building_menu_index = -1;
  ctx->has_menu_bar = false;
  ctx->menu_defs.clear();
  ctx->in_main_menu_bar = false;
  ctx->in_main_menu = false;
  ctx->building_main_menu_index = -1;
  ctx->main_menu_defs.clear();

  // Apply final cursor decision once per frame to avoid visible cursor thrash.
  if (ctx->cursor_request_idx >= 0 && ctx->cursor_request_idx < 16 &&
      ctx->cursors[ctx->cursor_request_idx] &&
      ctx->current_cursor_idx != ctx->cursor_request_idx) {
    SDL_SetCursor(ctx->cursors[ctx->cursor_request_idx]);
    ctx->current_cursor_idx = ctx->cursor_request_idx;
  }
  mdgui_backend_end_frame();
}

int mdgui_begin_window(MDGUI_Context *ctx, const char *title, int x, int y, int w,
                      int h) {
  if (!ctx)
    return 0;
  const int idx = find_or_create_window(ctx, title, x, y, w, h);
  ctx->current_window = idx;
  auto &win = ctx->windows[idx];

  if (win.closed)
    return 0;

  const int screen_w = get_logical_render_w(ctx);
  const int screen_h = get_logical_render_h(ctx);

  if (win.is_maximized) {
    win.x = 0;
    win.y = ctx->main_menu_bar_h;
    win.w = screen_w;
    win.h = screen_h - ctx->main_menu_bar_h;
  }

  const int top_idx =
      top_window_at_point(ctx, ctx->input.mouse_x, ctx->input.mouse_y, 3);
  const int top_here = (top_idx == idx);
  const bool chrome_input_allowed = (ctx->open_main_menu_index == -1);
  const bool move_resize_allowed = chrome_input_allowed && !ctx->windows_locked;
  const bool can_maximize = !win.disallow_maximize;
  const int title_h = 12;
  if (win.min_w < 50)
    win.min_w = 50;
  if (win.min_h < 30)
    win.min_h = 30;

  // Edge detection for hover AND interaction
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

  // Cursor feedback (Hover OR active resizing)
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

  // Handle Close and Maximize button interaction
  const int btn_w = 9;
  const int btn_h = 9;
  const int close_x = win.x + win.w - btn_w - 1;
  const int close_y = win.y + 1;
  const int max_x = close_x - btn_w - 2;
  const int max_y = win.y + 1;
  const int title_text_w = (title && mdgui_fonts[1]) ? mdgui_fonts[1]->measureTextWidth(title) : 0;
  int chrome_min_w = 26;
  if (title_text_w > 0) {
    const int title_need = 5 + title_text_w + 6 + btn_w + 2 + btn_w + 2;
    if (title_need > chrome_min_w)
      chrome_min_w = title_need;
  }
  if (chrome_min_w > win.min_w)
    win.min_w = chrome_min_w;

  if (chrome_input_allowed && ctx->input.mouse_pressed && top_here) {
    // If a combo popup is open in this window, let combo logic consume this
    // click first; don't treat it as titlebar/chrome interaction.
    if (win.open_combo_id != -1) {
      win.z = ++ctx->z_counter;
    } else {
    // Check Close button
    if (point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, close_x, close_y,
                      btn_w, btn_h)) {
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
    }
  }

  const int focused = (top_window_at_point(ctx, ctx->input.mouse_x,
                                           ctx->input.mouse_y) == idx) ||
                      (win.z == ctx->z_counter);

  mdgui_fill_rect_idx(nullptr, CLR_BOX_TITLE, win.x, win.y, win.w, title_h);
  mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, win.x, win.y + title_h, win.w,
           win.h - title_h - 2);
  mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, win.x, win.y + win.h - 2, win.w, 2);

  // Draw 3D-style Close button
  mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, close_x, close_y, btn_w, btn_h);
  // Draw X as two diagonal lines with margin
  mdgui_draw_line_idx(nullptr, CLR_TEXT_LIGHT, close_x + 2, close_y + 2,
             close_x + 6, close_y + 6);
  mdgui_draw_line_idx(nullptr, CLR_TEXT_LIGHT, close_x + 6, close_y + 2,
             close_x + 2, close_y + 6);

  // Draw 3D-style Maximize button when allowed for this window.
  if (can_maximize) {
    mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, max_x, max_y, btn_w, btn_h);
    mdgui_fill_rect_idx(nullptr, CLR_TEXT_LIGHT, max_x + 2, max_y + 2, btn_w - 4,
             btn_h - 4);
    mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, max_x + 3, max_y + 4, btn_w - 6,
             btn_h - 7);
  }

  if (title && mdgui_fonts[1]) {
    const int title_text_y = win.y + ((title_h - 8) / 2);
    mdgui_fonts[1]->drawText(title, nullptr, win.x + 5, title_text_y, CLR_TEXT_LIGHT);
  }

  // Draw border after fills so it renders on top
  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, win.x - 1, win.y - 1, win.x + win.w + 1);
  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, win.x - 1, win.y + win.h - 1, win.x + win.w + 1);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, win.x - 1, win.y - 1, win.y + win.h);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, win.x + win.w, win.y - 1, win.y + win.h);

  ctx->origin_x = win.x + 2;
  ctx->origin_y = win.y + title_h + 2;
  ctx->content_y = ctx->origin_y;
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
  ctx->content_req_right = win.x + 6;
  ctx->content_req_bottom = win.y + title_h + 6;
  ctx->window_has_nonlabel_widget = false;
  note_content_bounds(ctx, win.x + chrome_min_w, win.y + title_h + 2);

  set_content_clip(ctx);
  return 1;
}

void mdgui_end_window(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  auto &win = ctx->windows[ctx->current_window];
  mdgui_backend_set_clip_rect(0, 0, 0, 0, 0);

  if (win.open_menu_index != -1 && ctx->input.mouse_pressed &&
      win.z == ctx->z_counter) {
    bool in_menu_bar = false;
    bool in_menu_popup = false;

    if (ctx->has_menu_bar) {
      in_menu_bar =
          point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, ctx->menu_bar_x,
                        ctx->menu_bar_y, ctx->menu_bar_w, ctx->menu_bar_h);
    }

    if (win.open_menu_index >= 0 &&
        win.open_menu_index < (int)ctx->menu_defs.size()) {
      const auto &def = ctx->menu_defs[win.open_menu_index];
      const int popup_h = (int)def.items.size() * ctx->current_menu_h;
      in_menu_popup = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                    def.x, def.y, def.w, popup_h);
    }

    if (!in_menu_bar && !in_menu_popup) {
      win.open_menu_index = -1;
    }
  }

  draw_open_menu_overlay(ctx);

  if (ctx->combo_overlay_pending &&
      ctx->combo_overlay_window == ctx->current_window &&
      ctx->combo_overlay_items && ctx->combo_overlay_item_count > 0) {
    const int ix = ctx->combo_overlay_x;
    const int popup_y = ctx->combo_overlay_y;
    const int w = ctx->combo_overlay_w;
    const int item_h = ctx->combo_overlay_item_h;
    const int item_count = ctx->combo_overlay_item_count;
    const int popup_h = item_count * item_h;
    mdgui_draw_frame_idx(nullptr, CLR_TEXT_DARK, ix - 1, popup_y - 1, ix + w + 1,
            popup_y + popup_h + 1);
    mdgui_fill_rect_idx(nullptr, CLR_MENU_BG, ix, popup_y, w, popup_h);
    for (int i = 0; i < item_count; i++) {
      const int ry = popup_y + (i * item_h);
      const int row_hover = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                          ix, ry, w, item_h);
      if (i == ctx->combo_overlay_selected || row_hover)
        mdgui_fill_rect_idx(nullptr, CLR_ACCENT, ix, ry, w, item_h);
      if (mdgui_fonts[1] && ctx->combo_overlay_items[i]) {
        mdgui_fonts[1]->drawText(ctx->combo_overlay_items[i], nullptr, ix + 2, ry + 1,
                      CLR_MENU_TEXT);
      }
    }
    ctx->combo_overlay_pending = false;
    ctx->combo_overlay_window = -1;
    ctx->combo_overlay_items = nullptr;
  }

  // All windows can scroll when content exceeds viewport.
  {
    const int viewport_top = ctx->origin_y;
    const int viewport_h = (win.y + win.h - 4) - viewport_top;
    int total_h = ctx->content_y - ctx->origin_y;
    if (total_h < 0)
      total_h = 0;
    const int max_scroll = (viewport_h > 0 && total_h > viewport_h)
                               ? (total_h - viewport_h)
                               : 0;

    const int content_x = win.x + 2;
    const int content_w = win.w - 4;
    const int over_content = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                           content_x, viewport_top, content_w,
                                           (viewport_h > 0) ? viewport_h : 1);
    if (over_content && ctx->input.mouse_wheel != 0 && max_scroll > 0 &&
        !win.text_scroll_dragging) {
      const int prev = win.text_scroll;
      win.text_scroll -= ctx->input.mouse_wheel * 12;
      if (win.text_scroll < 0)
        win.text_scroll = 0;
      if (win.text_scroll > max_scroll)
        win.text_scroll = max_scroll;
      if (win.text_scroll == prev) {
        // At edge: keep stable (prevents visual jitter while wheel continues).
        win.text_scroll = prev;
      }
    }
    if (win.text_scroll < 0)
      win.text_scroll = 0;
    if (win.text_scroll > max_scroll)
      win.text_scroll = max_scroll;

    if (max_scroll > 0 && viewport_h > 8) {
      const int sb_w = 8;
      const int sb_x = win.x + win.w - sb_w - 2;
      const int sb_y = viewport_top;
      mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, sb_x, sb_y, sb_w, viewport_h);
      int thumb_h = (viewport_h * viewport_h) / (total_h > 0 ? total_h : 1);
      if (thumb_h < 10)
        thumb_h = 10;
      if (thumb_h > viewport_h)
        thumb_h = viewport_h;
      const int travel = viewport_h - thumb_h;
      const int thumb_y = sb_y + ((travel > 0)
                                      ? ((win.text_scroll * travel) / max_scroll)
                                      : 0);
      mdgui_fill_rect_idx(nullptr, CLR_ACCENT, sb_x + 1, thumb_y, sb_w - 2, thumb_h);
      const int over_scrollbar = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                               sb_x, sb_y, sb_w, viewport_h);
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
      // Scrollbar is chrome anchored to the current window edge and should not
      // participate in content-driven minimum width calculations.
    }
  }

  // Dynamic minimum size based on hard content only (widgets/interactive UI).
  if (!win.fixed_rect) {
    const int measured_min_w = (ctx->content_req_right - win.x) + 4;
    const int measured_min_h = (ctx->content_req_bottom - win.y) + 4;
    if (ctx->window_has_nonlabel_widget) {
      if (measured_min_w > win.min_w)
        win.min_w = measured_min_w;
      if (measured_min_h > win.min_h)
        win.min_h = measured_min_h;
    }
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
}

int mdgui_button(MDGUI_Context *ctx, const char *text, int x, int y, int w,
                int h) {
  if (!ctx || ctx->current_window < 0)
    return 0;
  ctx->window_has_nonlabel_widget = true;
  const auto &win = ctx->windows[ctx->current_window];

  const int topmost = is_current_window_topmost(ctx);
  const int requested_w = w;
  w = resolve_dynamic_width(ctx, x, w, 12);
  const int abs_x = ctx->origin_x + x;
  const int logical_y = ctx->content_y + y;
  const int abs_y = logical_y - win.text_scroll;
  const int hovered =
      topmost &&
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, abs_x, abs_y, w, h);
  const int depressed = hovered && ctx->input.mouse_down;

  if (depressed) {
    mdgui_fill_rect_idx(nullptr, CLR_BUTTON_PRESSED, abs_x, abs_y, w, h);
    mdgui_draw_hline_idx(nullptr, CLR_BUTTON_DARK, abs_x, abs_y, abs_x + w);
    mdgui_draw_vline_idx(nullptr, CLR_BUTTON_DARK, abs_x + w - 1, abs_y, abs_y + h);
    mdgui_draw_vline_idx(nullptr, CLR_BUTTON_LIGHT, abs_x, abs_y, abs_y + h);
    mdgui_draw_hline_idx(nullptr, CLR_BUTTON_LIGHT, abs_x, abs_y + h - 1, abs_x + w);
  } else {
    mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, abs_x, abs_y, w, h);
    mdgui_draw_hline_idx(nullptr, CLR_BUTTON_LIGHT, abs_x, abs_y, abs_x + w);
    mdgui_draw_vline_idx(nullptr, CLR_BUTTON_LIGHT, abs_x + w - 1, abs_y, abs_y + h);
    mdgui_draw_vline_idx(nullptr, CLR_BUTTON_DARK, abs_x, abs_y, abs_y + h);
    mdgui_draw_hline_idx(nullptr, CLR_BUTTON_DARK, abs_x, abs_y + h - 1, abs_x + w);
  }

  if (text && mdgui_fonts[1]) {
    const int tx = abs_x + (w - mdgui_fonts[1]->measureTextWidth(text)) / 2;
    const int ty = abs_y + (h - 8) / 2;
    mdgui_fonts[1]->drawText(text, nullptr, tx, ty, CLR_TEXT_LIGHT);
  }
  int intrinsic_w = w;
  if (requested_w <= 0) {
    intrinsic_w = 12;
    if (text && mdgui_fonts[1]) {
      int tw = mdgui_fonts[1]->measureTextWidth(text) + 8;
      if (tw > intrinsic_w)
        intrinsic_w = tw;
    }
  }
  note_content_bounds(ctx, abs_x + intrinsic_w, logical_y + h);

  return hovered && ctx->input.mouse_pressed && win.z == ctx->z_counter;
}

void mdgui_label(MDGUI_Context *ctx, const char *text, int x, int y) {
  if (!ctx || !text || !mdgui_fonts[1] || ctx->current_window < 0)
    return;
  const auto &win = ctx->windows[ctx->current_window];
  const int abs_x = ctx->origin_x + x;
  const int logical_y = ctx->content_y + y;
  const int abs_y = logical_y - win.text_scroll;
  mdgui_fonts[1]->drawText(text, nullptr, abs_x, abs_y, CLR_TEXT_LIGHT);
  note_content_bounds(ctx, abs_x + mdgui_fonts[1]->measureTextWidth(text), logical_y + 12);
  ctx->content_y += 12; // Advance content Y
}

void mdgui_label_wrapped(MDGUI_Context *ctx, const char *text, int x, int y,
                         int w) {
  if (!ctx || !text || !mdgui_fonts[1] || ctx->current_window < 0)
    return;
  const auto &win = ctx->windows[ctx->current_window];
  const int top_margin = ((y > 0) ? y : 0) + 4;
  const int abs_x = ctx->origin_x + x;
  const int logical_y = ctx->content_y + top_margin;
  const int wrap_w = resolve_dynamic_width(ctx, x, w, 20);
  const int margin_l = 2;
  const int margin_r = 2;
  const int draw_x = abs_x + margin_l;
  const int inner_wrap_w = std::max(10, wrap_w - (margin_l + margin_r));
  const int line_h = 12;

  int lines_drawn = 0;
  int max_right = draw_x;

  auto draw_line = [&](const std::string &line) {
    const int abs_y = (logical_y + lines_drawn * line_h) - win.text_scroll;
    mdgui_fonts[1]->drawText(line.c_str(), nullptr, draw_x, abs_y, CLR_TEXT_LIGHT);
    const int line_w = mdgui_fonts[1]->measureTextWidth(line.c_str());
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
        if (mdgui_fonts[1]->measureTextWidth(candidate.c_str()) <= inner_wrap_w) {
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

        if (mdgui_fonts[1]->measureTextWidth(candidate.c_str()) <= inner_wrap_w) {
          current = candidate;
          continue;
        }

        if (!current.empty()) {
          draw_line(current);
          current.clear();
          if (mdgui_fonts[1]->measureTextWidth(word.c_str()) <= inner_wrap_w) {
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
  note_content_bounds(ctx, max_right + margin_r,
                      logical_y + lines_drawn * line_h + bottom_margin);
  ctx->content_y += top_margin + lines_drawn * line_h + bottom_margin;
}

void mdgui_spacer(MDGUI_Context *ctx, int pixels) {
  if (!ctx || ctx->current_window < 0)
    return;
  if (pixels < 0)
    pixels = 0;
  const auto &win = ctx->windows[ctx->current_window];
  const int bottom = ctx->content_y + pixels;
  note_content_bounds(ctx, win.x + 6, bottom);
  ctx->content_y = bottom;
}

int mdgui_checkbox(MDGUI_Context *ctx, const char *text, bool *checked, int x,
                  int y) {
  if (!ctx || !checked || ctx->current_window < 0)
    return 0;
  ctx->window_has_nonlabel_widget = true;
  const auto &win = ctx->windows[ctx->current_window];
  const int ix = ctx->origin_x + x;
  const int logical_y = ctx->content_y + y;
  const int iy = logical_y - win.text_scroll;
  const int box_size = 10;

  // 3D-drawn checkbox
  mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, ix, iy, box_size, box_size);
  mdgui_draw_hline_idx(nullptr, CLR_ACCENT, ix, iy, ix + box_size);
  mdgui_draw_hline_idx(nullptr, CLR_ACCENT, ix, iy + box_size - 1, ix + box_size);
  mdgui_draw_vline_idx(nullptr, CLR_ACCENT, ix, iy, iy + box_size);
  mdgui_draw_vline_idx(nullptr, CLR_ACCENT, ix + box_size - 1, iy, iy + box_size);

  if (*checked) {
    mdgui_draw_line_idx(nullptr, CLR_TEXT_LIGHT, ix + 2, iy + 2,
               ix + 7, iy + 7);
    mdgui_draw_line_idx(nullptr, CLR_TEXT_LIGHT, ix + 7, iy + 2,
               ix + 2, iy + 7);
  }

  if (text && mdgui_fonts[1]) {
    mdgui_fonts[1]->drawText(text, nullptr, ix + box_size + 4, iy + 1, CLR_TEXT_LIGHT);
  }
  const int label_w = (text && mdgui_fonts[1]) ? mdgui_fonts[1]->measureTextWidth(text) : 0;
  note_content_bounds(ctx, ix + box_size + 4 + label_w, logical_y + box_size);

  int result = 0;
  if (ctx->input.mouse_pressed && is_current_window_topmost(ctx) &&
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, ix, iy,
                    box_size + (text ? 50 : 0), box_size)) {
    *checked = !*checked;
    result = 1;
  }

  ctx->content_y += box_size + 4;
  return result;
}

int mdgui_slider(MDGUI_Context *ctx, const char *text, float *val, float min,
                float max, int x, int y, int w) {
  if (!ctx || !val || ctx->current_window < 0)
    return 0;
  ctx->window_has_nonlabel_widget = true;
  const auto &win = ctx->windows[ctx->current_window];
  const int requested_w = w;
  w = resolve_dynamic_width(ctx, x, w, 24);
  const int ix = ctx->origin_x + x;
  const int top_margin = 10;
  const int logical_y = ctx->content_y + std::max(y, top_margin);
  const int iy = logical_y - win.text_scroll;
  const int track_h = 4;
  const int thumb_w = 8;
  const int thumb_h = 10;

  // Track (inset 3D)
  mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, ix, iy + 3, w, track_h);
  mdgui_draw_hline_idx(nullptr, CLR_BUTTON_DARK, ix, iy + 3, ix + w);
  mdgui_draw_vline_idx(nullptr, CLR_BUTTON_DARK, ix, iy + 3, iy + 3 + track_h);
  mdgui_draw_hline_idx(nullptr, CLR_BUTTON_LIGHT, ix, iy + 3 + track_h - 1, ix + w);
  mdgui_draw_vline_idx(nullptr, CLR_BUTTON_LIGHT, ix + w - 1, iy + 3, iy + 3 + track_h);

  float ratio = (*val - min) / (max - min);
  if (ratio < 0)
    ratio = 0;
  if (ratio > 1)
    ratio = 1;

  int tx = ix + (int)(ratio * (float)(w - thumb_w));

  // Thumb (outset 3D)
  mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, tx, iy, thumb_w, thumb_h);
  mdgui_draw_hline_idx(nullptr, CLR_BUTTON_LIGHT, tx, iy, tx + thumb_w);
  mdgui_draw_vline_idx(nullptr, CLR_BUTTON_LIGHT, tx, iy, iy + thumb_h);
  mdgui_draw_hline_idx(nullptr, CLR_BUTTON_DARK, tx, iy + thumb_h - 1, tx + thumb_w);
  mdgui_draw_vline_idx(nullptr, CLR_BUTTON_DARK, tx + thumb_w - 1, iy, iy + thumb_h);

  int result = 0;
  if (ctx->input.mouse_down && is_current_window_topmost(ctx) &&
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, ix, iy, w,
                    thumb_h)) {
    float new_ratio = (float)(ctx->input.mouse_x - ix) / (float)w;
    if (new_ratio < 0)
      new_ratio = 0;
    if (new_ratio > 1)
      new_ratio = 1;
    *val = min + new_ratio * (max - min);
    result = 1;
  }

  if (text && mdgui_fonts[1]) {
    mdgui_fonts[1]->drawText(text, nullptr, ix + w + 4, iy + 1, CLR_TEXT_LIGHT);
  }
  const int text_w = (text && mdgui_fonts[1]) ? mdgui_fonts[1]->measureTextWidth(text) : 0;
  int intrinsic_slider_w = w;
  if (requested_w <= 0)
    intrinsic_slider_w = 24;
  note_content_bounds(ctx, ix + intrinsic_slider_w + 4 + text_w,
                      logical_y + thumb_h);

  const int bottom_margin = 4;
  ctx->content_y += std::max(y, top_margin) + thumb_h + bottom_margin;
  return result;
}

void mdgui_separator(MDGUI_Context *ctx, int x, int y, int w) {
  if (!ctx || ctx->current_window < 0)
    return;
  ctx->window_has_nonlabel_widget = true;
  const auto &win = ctx->windows[ctx->current_window];
  const int requested_w = w;
  w = resolve_dynamic_width(ctx, x, w, 4);
  const int ix = ctx->origin_x + x;
  const int logical_y = ctx->content_y + y;
  const int iy = logical_y - win.text_scroll;
  mdgui_draw_hline_idx(nullptr, CLR_ACCENT, ix, iy, ix + w);
  mdgui_draw_hline_idx(nullptr, CLR_ACCENT, ix, iy + 1, ix + w);
  if (requested_w > 0)
    note_content_bounds(ctx, ix + w, logical_y + 2);
  ctx->content_y += 4;
}

int mdgui_listbox(MDGUI_Context *ctx, const char **items, int item_count,
                 int *selected, int x, int y, int w, int rows) {
  if (!ctx || !selected || !items || item_count <= 0)
    return 0;
  ctx->window_has_nonlabel_widget = true;
  const auto &win = ctx->windows[ctx->current_window];
  if (rows < 1)
    rows = 1;
  const int requested_w = w;
  w = resolve_dynamic_width(ctx, x, w, 24);
  const int row_h = 10;
  const int box_h = rows * row_h;
  const int ix = ctx->origin_x + x;
  const int top_margin = 4;
  const int logical_y = ctx->content_y + std::max(y, top_margin);
  const int iy = logical_y - win.text_scroll;
  const int topmost = is_current_window_topmost(ctx);

  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy - 1, ix + w + 1);
  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy + box_h + 1, ix + w + 1);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy - 1, iy + box_h + 1);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, ix + w + 1, iy - 1, iy + box_h + 1);
  mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, ix, iy, w, box_h);

  int clicked = 0;
  for (int i = 0; i < rows; i++) {
    const int item_idx = i;
    if (item_idx >= item_count)
      break;
    const int ry = iy + (i * row_h);
    const int hovered = topmost && point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                                 ix, ry, w, row_h);
    if (item_idx == *selected)
      mdgui_fill_rect_idx(nullptr, CLR_ACCENT, ix, ry, w, row_h);
    else if (hovered)
      mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, ix, ry, w, row_h);
    if (mdgui_fonts[1] && items[item_idx]) {
      mdgui_fonts[1]->drawText(items[item_idx], nullptr, ix + 2, ry + 1, CLR_MENU_TEXT);
    }
    if (hovered && ctx->input.mouse_pressed) {
      *selected = item_idx;
      clicked = 1;
    }
  }

  int intrinsic_w = w;
  if (requested_w <= 0)
    intrinsic_w = 24;
  note_content_bounds(ctx, ix + intrinsic_w, logical_y + box_h);
  const int bottom_margin = 4;
  ctx->content_y += std::max(y, top_margin) + box_h + bottom_margin;
  return clicked;
}

int mdgui_combo(MDGUI_Context *ctx, const char *label, const char **items,
               int item_count, int *selected, int x, int y, int w) {
  if (!ctx || !selected || !items || item_count <= 0 || ctx->current_window < 0)
    return 0;
  ctx->window_has_nonlabel_widget = true;
  auto &win = ctx->windows[ctx->current_window];
  const int topmost = is_current_window_topmost(ctx);
  const int item_h = 10;
  const int box_h = 12;
  const int requested_w = w;
  w = resolve_dynamic_width(ctx, x, w, 40);
  if (*selected < 0)
    *selected = 0;
  if (*selected >= item_count)
    *selected = item_count - 1;

  const int ix = ctx->origin_x + x;
  const int top_margin = 10;
  const int logical_y = ctx->content_y + std::max(y, top_margin);
  const int iy = logical_y - win.text_scroll;
  const int combo_id = ((ix & 0xffff) << 16) ^ (iy & 0xffff) ^ (w << 2);
  const int open = (win.open_combo_id == combo_id);

  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy - 1, ix + w + 1);
  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy + box_h + 1, ix + w + 1);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy - 1, iy + box_h + 1);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, ix + w + 1, iy - 1, iy + box_h + 1);
  mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, ix, iy, w, box_h);
  mdgui_draw_vline_idx(nullptr, CLR_BUTTON_DARK, ix + w - 12, iy, iy + box_h);
  if (mdgui_fonts[1] && items[*selected]) {
    mdgui_fonts[1]->drawText(items[*selected], nullptr, ix + 2, iy + 2, CLR_MENU_TEXT);
    mdgui_fonts[1]->drawText("v", nullptr, ix + w - 9, iy + 2, CLR_MENU_TEXT);
  }
  if (label && mdgui_fonts[1]) {
    mdgui_fonts[1]->drawText(label, nullptr, ix, iy - 10, CLR_TEXT_LIGHT);
  }

  const int hovered = topmost &&
                      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, ix, iy,
                                    w, box_h);
  if (hovered && ctx->input.mouse_pressed) {
    win.open_combo_id = open ? -1 : combo_id;
    win.z = ++ctx->z_counter;
    ctx->input.mouse_pressed = 0; // consume click so underlying widgets don't also fire
  } else if (ctx->input.mouse_pressed && open) {
    const int popup_h = item_count * item_h;
    const int in_popup =
        point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, ix, iy + box_h, w,
                      popup_h);
    if (!in_popup)
      win.open_combo_id = -1;
  }

  int changed = 0;
  int intrinsic_w = w;
  if (requested_w <= 0) {
    intrinsic_w = 40;
    if (mdgui_fonts[1] && items[*selected]) {
      const int text_need = mdgui_fonts[1]->measureTextWidth(items[*selected]) + 14;
      if (text_need > intrinsic_w)
        intrinsic_w = text_need;
    }
  }

  if (win.open_combo_id == combo_id) {
    const int popup_y = iy + box_h;
    const int popup_h = item_count * item_h;
    for (int i = 0; i < item_count; i++) {
      const int ry = popup_y + (i * item_h);
      const int row_hover = topmost && point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                                     ix, ry, w, item_h);
      if (row_hover && ctx->input.mouse_pressed) {
        if (*selected != i) {
          *selected = i;
          changed = 1;
        }
        win.open_combo_id = -1;
        win.z = ++ctx->z_counter;
        ctx->input.mouse_pressed = 0; // consume popup selection click
      }
    }
    note_content_bounds(ctx, ix + intrinsic_w, logical_y + box_h + popup_h);
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

  note_content_bounds(ctx, ix + intrinsic_w, logical_y + box_h);
  const int bottom_margin = 4;
  ctx->content_y += std::max(y, top_margin) + box_h + bottom_margin;
  return changed;
}

void mdgui_progress_bar(MDGUI_Context *ctx, float value, int x, int y, int w,
                       int h, const char *overlay_text) {
  if (!ctx || ctx->current_window < 0)
    return;
  ctx->window_has_nonlabel_widget = true;
  const auto &win = ctx->windows[ctx->current_window];
  const int requested_w = w;
  w = resolve_dynamic_width(ctx, x, w, 24);
  if (h < 6)
    h = 6;
  if (value < 0.0f)
    value = 0.0f;
  if (value > 1.0f)
    value = 1.0f;

  const int ix = ctx->origin_x + x;
  const int top_margin = 4;
  const int logical_y = ctx->content_y + std::max(y, top_margin);
  const int iy = logical_y - win.text_scroll;
  const int fill_w = (int)((float)w * value);

  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy - 1, ix + w + 1);
  mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy + h + 1, ix + w + 1);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy - 1, iy + h + 1);
  mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, ix + w + 1, iy - 1, iy + h + 1);
  mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, ix, iy, w, h);
  if (fill_w > 0)
    mdgui_fill_rect_idx(nullptr, CLR_ACCENT, ix, iy, fill_w, h);

  if (overlay_text && mdgui_fonts[1]) {
    const int tw = mdgui_fonts[1]->measureTextWidth(overlay_text);
    mdgui_fonts[1]->drawText(overlay_text, nullptr, ix + (w - tw) / 2, iy + 1,
                  CLR_TEXT_LIGHT);
  }
  int intrinsic_w = w;
  if (requested_w <= 0) {
    intrinsic_w = 24;
    if (overlay_text && mdgui_fonts[1]) {
      const int text_need = mdgui_fonts[1]->measureTextWidth(overlay_text) + 6;
      if (text_need > intrinsic_w)
        intrinsic_w = text_need;
    }
  }
  note_content_bounds(ctx, ix + intrinsic_w, logical_y + h);
  const int bottom_margin = 4;
  ctx->content_y += std::max(y, top_margin) + h + bottom_margin;
}

void mdgui_frame_time_graph(MDGUI_Context *ctx, const float *frame_ms_samples,
                           int sample_count, float target_fps,
                           float graph_max_ms, int x, int y, int w, int h) {
  if (!ctx || ctx->current_window < 0)
    return;
  ctx->window_has_nonlabel_widget = true;
  const auto &win = ctx->windows[ctx->current_window];

  const int requested_w = w;
  const int requested_h = h;
  const bool fill_mode = (requested_w == 0 && requested_h == 0);
  if (fill_mode) {
    int avail_w = (win.x + win.w - 2) - (ctx->origin_x + x);
    if (avail_w < 24)
      avail_w = 24;
    w = avail_w;
  } else {
    w = resolve_dynamic_width(ctx, x, w, 24);
  }
  if (target_fps < 1.0f)
    target_fps = 1.0f;
  if (graph_max_ms < 1.0f)
    graph_max_ms = 1.0f;

  const int ix = ctx->origin_x + x;
  const int top_margin = 4;
  const int logical_y = ctx->content_y + std::max(y, top_margin);
  const int iy = logical_y - win.text_scroll;

  // Height 0/negative mirrors dynamic width behavior: fill remaining viewport.
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
    mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy - 1, ix + w + 1);
    mdgui_draw_hline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy + h + 1, ix + w + 1);
    mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, ix - 1, iy - 1, iy + h + 1);
    mdgui_draw_vline_idx(nullptr, CLR_WINDOW_BORDER, ix + w + 1, iy - 1, iy + h + 1);
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
    mdgui_draw_hline_idx(nullptr, CLR_ACCENT, plot_x, target_y, plot_x + plot_w);

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

  // In fill mode, the graph tracks current window height. Reporting that full
  // height as required content would ratchet min_h upward and prevent shrinking.
  // Report only a minimal intrinsic height for min-size calculations.
  const int intrinsic_h = fill_mode ? 12 : h;
  note_content_bounds(ctx, ix + intrinsic_w, logical_y + intrinsic_h);
  if (fill_mode) {
    ctx->content_y = logical_y + h;
  } else {
    const int bottom_margin = 4;
    ctx->content_y += std::max(y, top_margin) + h + bottom_margin;
  }
}

void mdgui_begin_menu_bar(MDGUI_Context *ctx) {
  if (!ctx || ctx->current_window < 0)
    return;
  ctx->window_has_nonlabel_widget = true;
  const auto &win = ctx->windows[ctx->current_window];
  const int bar_x = win.x + 1;
  const int bar_y = ctx->content_y;
  const int bar_w = win.w - 2;
  const int bar_h = 10;
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
  ctx->content_y += 12;
}

int mdgui_begin_menu(MDGUI_Context *ctx, const char *text) {
  if (!ctx || !ctx->in_menu_bar || ctx->current_window < 0 || !text)
    return 0;
  auto &win = ctx->windows[ctx->current_window];

  const int tw = mdgui_fonts[1] ? mdgui_fonts[1]->measureTextWidth(text) : 40;
  const int item_w = tw + 10;
  const int x = ctx->menu_next_x;
  const int y = ctx->content_y - 12;
  const int hovered =
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, x, y, item_w, 10);

  if ((int)ctx->menu_defs.size() <= ctx->menu_index) {
    ctx->menu_defs.resize(ctx->menu_index + 1);
  }
  auto &def = ctx->menu_defs[ctx->menu_index];
  def.x = x;
  def.y = y + 10 + MENU_POPUP_GAP_Y;
  def.w = (tw + 40 < 120) ? 120 : (tw + 40);
  def.items.clear();

  if (win.open_menu_index != -1 && hovered && win.z == ctx->z_counter) {
    win.open_menu_index = ctx->menu_index;
  }

  if (ctx->input.mouse_pressed && hovered && win.z == ctx->z_counter) {
    if (win.open_menu_index == ctx->menu_index)
      win.open_menu_index = -1;
    else
      win.open_menu_index = ctx->menu_index;
  }

  const int open = (win.open_menu_index == ctx->menu_index);
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
  }

  ctx->menu_next_x += item_w + 2;
  ctx->menu_index += 1;
  return open;
}

int mdgui_menu_item(MDGUI_Context *ctx, const char *text) {
  if (!ctx || !ctx->in_menu || !text || ctx->building_menu_index < 0)
    return 0;
  auto &win = ctx->windows[ctx->current_window];
  auto &def = ctx->menu_defs[ctx->building_menu_index];
  const int item_text_w = mdgui_fonts[1] ? mdgui_fonts[1]->measureTextWidth(text) : 40;
  const int item_needed_w = item_text_w + 12;
  if (item_needed_w > def.w)
    def.w = item_needed_w;
  def.items.push_back(text);
  const int item_index = (int)def.items.size() - 1;
  const int item_y = def.y + (item_index * ctx->current_menu_h);

  const int hovered = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                    def.x, item_y, def.w, ctx->current_menu_h);
  if (hovered && ctx->input.mouse_pressed && win.z == ctx->z_counter) {
    win.open_menu_index = -1;
    return 1;
  }
  return 0;
}

void mdgui_end_menu(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  ctx->in_menu = false;
  ctx->building_menu_index = -1;
}

void mdgui_end_menu_bar(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  ctx->in_menu_bar = false;
  ctx->in_menu = false;
}

void mdgui_begin_main_menu_bar(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  const int rw = get_logical_render_w(ctx);
  ctx->main_menu_bar_x = 0;
  ctx->main_menu_bar_y = 0;
  ctx->main_menu_bar_w = rw;
  ctx->main_menu_bar_h = 10;

  mdgui_fill_rect_idx(nullptr, CLR_MENU_BG, ctx->main_menu_bar_x, ctx->main_menu_bar_y,
           ctx->main_menu_bar_w, ctx->main_menu_bar_h);
  ctx->main_menu_index = 0;
  ctx->main_menu_next_x = 3;
  ctx->in_main_menu_bar = true;
  ctx->in_main_menu = false;
  ctx->building_main_menu_index = -1;
  ctx->main_menu_defs.clear();

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
  def.items.clear();

  if (ctx->open_main_menu_index != -1 && hovered) {
    ctx->open_main_menu_index = ctx->main_menu_index;
  }
  if (ctx->input.mouse_pressed && hovered) {
    if (ctx->open_main_menu_index == ctx->main_menu_index)
      ctx->open_main_menu_index = -1;
    else
      ctx->open_main_menu_index = ctx->main_menu_index;
  }

  const int open = (ctx->open_main_menu_index == ctx->main_menu_index);
  if (open || hovered) {
    mdgui_fill_rect_idx(nullptr, CLR_ACCENT, x, y, item_w, ctx->main_menu_bar_h);
  }
  if (mdgui_fonts[1]) {
    mdgui_fonts[1]->drawText(text, nullptr, x + 3, y + 1, CLR_MENU_TEXT);
  }

  if (open) {
    ctx->in_main_menu = true;
    ctx->building_main_menu_index = ctx->main_menu_index;
  }

  ctx->main_menu_next_x += item_w + 2;
  ctx->main_menu_index += 1;
  return open;
}

int mdgui_main_menu_item(MDGUI_Context *ctx, const char *text) {
  if (!ctx || !ctx->in_main_menu || !text || ctx->building_main_menu_index < 0)
    return 0;

  auto &def = ctx->main_menu_defs[ctx->building_main_menu_index];
  const int item_text_w = mdgui_fonts[1] ? mdgui_fonts[1]->measureTextWidth(text) : 40;
  const int item_needed_w = item_text_w + 12;
  if (item_needed_w > def.w)
    def.w = item_needed_w;
  def.items.push_back(text);
  const int item_index = (int)def.items.size() - 1;
  const int item_y = def.y + (item_index * ctx->current_menu_h);
  const int hovered = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                    def.x, item_y, def.w, ctx->current_menu_h);

  if (hovered && ctx->input.mouse_pressed) {
    ctx->open_main_menu_index = -1;
    return 1;
  }
  return 0;
}

void mdgui_end_main_menu(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  ctx->in_main_menu = false;
  ctx->building_main_menu_index = -1;
}

void mdgui_end_main_menu_bar(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  ctx->in_main_menu_bar = false;
  ctx->in_main_menu = false;
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
    return 2; // Return "Cancel" if closed via X
  }
  if (ctx->current_window >= 0) {
    auto &msg_win = ctx->windows[ctx->current_window];
    msg_win.z = ++ctx->z_counter;
    msg_win.disallow_maximize = true;
    msg_win.is_maximized = false;
    // Message boxes should remain fixed-size; avoid feedback with auto min-size
    // tracking when footer/button geometry is anchored to bottom chrome.
    msg_win.fixed_rect = true;
    msg_win.w = box_w;
    msg_win.h = box_h;
    msg_win.min_w = box_w;
    msg_win.min_h = box_h;
  }

  const auto &win = ctx->windows[ctx->current_window];
  const int title_h = 12;
  const int footer_h = 16;
  // Content is clipped to (win.y + win.h - 4); keep footer fully inside that.
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
  // mdgui_button applies window text_scroll to y; compensate so footer button
  // stays visually centered regardless of scroll state.
  const int btn_y = btn_abs_y - ctx->content_y + win.text_scroll;

  if (style == MDGUI_MSGBOX_TWO_BUTTON) {
    const int b1_abs_x = win.x + (win.w / 4) - (btn_w / 2);
    const int b2_abs_x = win.x + ((win.w * 3) / 4) - (btn_w / 2);
    if (mdgui_button(ctx, button1, b1_abs_x - ctx->origin_x, btn_y, btn_w, btn_h))
      result = 1;
    if (mdgui_button(ctx, button2, b2_abs_x - ctx->origin_x, btn_y, btn_w, btn_h))
      result = 2;
  } else {
    const int b_abs_x = win.x + (win.w / 2) - (btn_w / 2);
    if (mdgui_button(ctx, button1, b_abs_x - ctx->origin_x, btn_y, btn_w, btn_h))
      result = 1;
  }

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
      ctx->windows[i].closed = (open == 0);
      if (open) {
        ctx->windows[i].z = ++ctx->z_counter;
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

void mdgui_set_tile_manager_enabled(MDGUI_Context *ctx, int enabled) {
  if (!ctx)
    return;
  ctx->tile_manager_enabled = (enabled != 0);
  if (ctx->tile_manager_enabled) {
    tile_windows_internal(ctx);
  }
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

int mdgui_begin_render_window(MDGUI_Context *ctx, const char *title, int x, int y,
                             int w, int h, int show_menu, int *out_x,
                             int *out_y, int *out_w, int *out_h) {
  if (!ctx)
    return 0;
  if (!mdgui_begin_window(ctx, title, x, y, w, h))
    return 0;
  if (show_menu) {
    mdgui_begin_menu_bar(ctx);
    mdgui_end_menu_bar(ctx);
  }
  if (ctx->current_window < 0)
    return 0;
  const auto &win = ctx->windows[ctx->current_window];
  const int cx = ctx->origin_x;
  const int cy = ctx->content_y;
  int cw = win.w - 4;
  int ch = (win.y + win.h - 4) - cy;
  if (cw < 1)
    cw = 1;
  if (ch < 1)
    ch = 1;
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

void mdgui_run_window_pass(MDGUI_Context *ctx, const MDGUI_WindowPassItem *items,
                          int item_count) {
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
    const int idx = find_or_create_window(ctx, it.title, it.x, it.y, it.w, it.h);
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

void mdgui_open_file_browser(MDGUI_Context *ctx) {
  if (!ctx)
    return;
  // Prevent same-frame click-through from the opener button/menu item.
  ctx->input.mouse_pressed = 0;
  const int idx = find_or_create_window(ctx, "File Browser", 28, 20, 280, 180);
  if (idx >= 0 && idx < (int)ctx->windows.size()) {
    ctx->windows[idx].closed = false;
    ctx->windows[idx].z = ++ctx->z_counter;
  }
  ctx->dragging_window = -1;
  ctx->resizing_window = -1;
  ctx->file_browser_open = true;
  ctx->file_browser_result.clear();
  ctx->file_browser_last_click_idx = -1;
  ctx->file_browser_last_click_ticks = 0;
  file_browser_open_path(ctx, ctx->file_browser_cwd.c_str());
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

const char *mdgui_show_file_browser(MDGUI_Context *ctx) {
  if (!ctx || !ctx->file_browser_open)
    return nullptr;
  ctx->file_browser_result.clear();

  if (!mdgui_begin_window(ctx, "File Browser", 28, 20, 280, 180)) {
    ctx->file_browser_open = false;
    return nullptr;
  }
  auto &win = ctx->windows[ctx->current_window];
  if (win.min_w < 220)
    win.min_w = 220;
  if (win.min_h < 130)
    win.min_h = 130;

  mdgui_label(ctx, "Directory:", 8, 2);
  mdgui_label(ctx, ctx->file_browser_cwd.c_str(), 8, 0);

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
  const int over_list =
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, list_x, list_y,
                    content_w, list_h);
  const int over_scrollbar =
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, list_x + content_w + 1,
                    list_y, scrollbar_w, list_h);
  const int over_browser_pane =
      point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, list_x, list_y, list_w,
                    list_h);
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

  mdgui_draw_frame_idx(nullptr, CLR_TEXT_DARK, list_x - 1, list_y - 1, list_x + list_w + 1,
          list_y + list_h + 1);
  mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, list_x, list_y, list_w, list_h);

  // Clip row text to the virtual list viewport so long names can't bleed out.
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
      mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, list_x, ry, content_w, row_h);
    if (mdgui_fonts[1]) {
      mdgui_fonts[1]->drawText(ctx->file_browser_entries[item_idx].label.c_str(), nullptr,
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
  mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, sb_x, list_y, scrollbar_w, list_h);
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
      if (ctx->input.mouse_y >= thumb_y && ctx->input.mouse_y <= thumb_y + thumb_h) {
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
      int desired_thumb_y = ctx->input.mouse_y - ctx->file_browser_scroll_drag_offset;
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

  const int button_local_y = bottom_buttons_y - ctx->content_y;
  if (mdgui_button(ctx, "Up", 8, button_local_y, 48, button_h)) {
    file_browser_open_path(ctx, "..");
  }
  if (mdgui_button(ctx, "Open", 60, button_local_y, 60, button_h)) {
    trigger_open = true;
  }
  if (mdgui_button(ctx, "Cancel", 124, button_local_y, 60, button_h)) {
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
    } else {
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

void mdgui_show_demo_window(MDGUI_Context *ctx) {
  static bool show_demo = true;
  static float progress = 0.35f;
  static int quality_idx = 0;
  static int theme_idx = MDGUI_THEME_DEFAULT;
  static const char *quality_items[] = {"Nearest", "Bilinear", "CRT Mask"};
  static const char *theme_items[] = {"Default", "Dark", "Amber", "Graphite",
                                      "Midnight", "Olive"};
  if (!show_demo)
    return;

  if (mdgui_begin_window(ctx, "MDGUI Demo Window", 20, 20, 220, 220)) {

    mdgui_separator(ctx, 10, 5, 0);
    mdgui_label(ctx, "Aesthetics & Widgets", 10, 5);
    mdgui_spacer(ctx, 4);
    mdgui_separator(ctx, 10, 0, 0);

    static bool check1 = true;
    static bool check2 = false;
    mdgui_checkbox(ctx, "Enable Sound", &check1, 10, 5);
    mdgui_checkbox(ctx, "Turbo Mode", &check2, 10, 5);

    mdgui_separator(ctx, 10, 5, 0);
    mdgui_label(ctx, "Volumes", 10, 5);
    mdgui_separator(ctx, 10, 5, 0);

    static float vol1 = 0.5f;
    static float vol2 = 0.8f;
    mdgui_slider(ctx, "Master", &vol1, 0.0f, 1.0f, 10, 5, -54);
    mdgui_slider(ctx, "Music", &vol2, 0.0f, 1.0f, 10, 5, -46);

    mdgui_separator(ctx, 10, 5, 0);
    mdgui_label(ctx, "Renderer", 10, 5);
    mdgui_separator(ctx, 10, 5, 0);
    mdgui_spacer(ctx, 4);

    mdgui_listbox(ctx, quality_items, 3, &quality_idx, 10, 3, -16, 3);

    //ctx->content_y += 8; // hard spacer: keep combo clear of renderer listbox

    mdgui_separator(ctx, 10, 5, 0);
    mdgui_label(ctx, "Filter Combo", 10, 5);
    mdgui_separator(ctx, 10, 5, 0);

    mdgui_combo(ctx, nullptr, quality_items, 3, &quality_idx, 10, 2, -16);

    mdgui_separator(ctx, 10, 5, 0);
    mdgui_label(ctx, "Theme", 10, 5);
    mdgui_separator(ctx, 10, 5, 0);

    theme_idx = mdgui_get_theme();
    if (mdgui_combo(ctx, nullptr, theme_items, 6, &theme_idx, 10, 2, -16)) {
      mdgui_set_theme(theme_idx);
    }

    mdgui_label(ctx, "Frame Progress", 10, 5);
    mdgui_progress_bar(ctx, progress, 10, 3, -16, 10, nullptr);
    if (mdgui_button(ctx, "Step", 10, 5, 56, 12)) {
      progress += 0.1f;
      if (progress > 1.0f)
        progress = 0.0f;
    }

    // Reserve footer space, but only show the close action when scrolled near
    // the bottom of content.
    const auto &win = ctx->windows[ctx->current_window];
    const int viewport_h = (win.y + win.h - 4) - ctx->origin_y;
    const int content_h_before_footer = ctx->content_y - ctx->origin_y;
    const int footer_h = 6 + 12 + 4; // spacer + button + widget gap
    const int total_h_with_footer = content_h_before_footer + footer_h;
    const int max_scroll_with_footer =
        (viewport_h > 0 && total_h_with_footer > viewport_h)
            ? (total_h_with_footer - viewport_h)
            : 0;
    const bool show_close =
        (max_scroll_with_footer == 0) ||
        (win.text_scroll >= (max_scroll_with_footer - 2));

    const int footer_start_y = ctx->content_y;
    mdgui_spacer(ctx, footer_h);
    if (show_close) {
      const int button_local_y = (footer_start_y + 6) - ctx->content_y;
      if (mdgui_button(ctx, "Close Demo", 10, button_local_y, -12, 12)) {
        ctx->windows[ctx->current_window].closed = true;
      }
    }

    mdgui_end_window(ctx);
  }
}
}

