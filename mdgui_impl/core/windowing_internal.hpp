#pragma once

#include "layout_internal.hpp"

bool menu_path_prefix_matches(const std::vector<int> &path,
                              const std::vector<int> &prefix);
void clear_window_menu_path(MDGUI_Window &win);
const char *parse_menu_check_prefix(const char *text, bool *out_has_check,
                                    bool *out_checked);
int menu_check_indicator_width();
void draw_menu_check_indicator(int x, int y, bool checked);
bool *find_or_create_collapsing_state(MDGUI_Window &win, const char *id,
                                      bool default_open);
MDGUI_Window::TabBarState *find_or_create_tab_bar_state(MDGUI_Window &win,
                                                        const char *id);
bool point_in_menu_popup_chain(const std::vector<MenuDef> &defs,
                               const std::vector<int> &path, int item_h, int px,
                               int py);
void reposition_child_menu_chain(std::vector<MenuDef> &defs,
                                 int menu_index, int item_h);
bool window_accepts_input(const MDGUI_Window &w);
int top_window_at_point(const MDGUI_Context *ctx, int px, int py,
                        int margin = 0);
bool is_tiled_file_browser_window(const MDGUI_Context *ctx,
                                  const MDGUI_Window &w);
bool is_occluded_by_higher_maximized_window(const MDGUI_Context *ctx,
                                            int window_idx);
int get_logical_render_w(MDGUI_Context *ctx);
int get_logical_render_h(MDGUI_Context *ctx);
int get_status_bar_h(const MDGUI_Context *ctx);
int get_work_area_top(MDGUI_Context *ctx);
int get_work_area_bottom(MDGUI_Context *ctx);
void clamp_window_to_work_area(MDGUI_Context *ctx, MDGUI_Window &win);
int clampi(int v, int lo, int hi);
void sanitize_overlay_state(MDGUI_OverlayState *state);
bool has_pending_tile_excluded_title(const MDGUI_Context *ctx,
                                     const char *title);
int normalize_window_min_w(int min_w);
int normalize_window_min_h(int min_h);
float normalize_window_min_percent(float percent);
int min_from_percent(int total, float percent, int floor_px);
void set_pending_tile_excluded_title(MDGUI_Context *ctx, const char *title,
                                     bool excluded);
bool get_pending_window_min_size(const MDGUI_Context *ctx, const char *title,
                                 int *out_min_w, int *out_min_h);
void set_pending_window_min_size(MDGUI_Context *ctx, const char *title,
                                 int min_w, int min_h);
void set_pending_window_min_size_percent(MDGUI_Context *ctx, const char *title,
                                         float min_w_percent,
                                         float min_h_percent);
bool get_pending_window_min_size_percent(const MDGUI_Context *ctx,
                                         const char *title,
                                         float *out_min_w_percent,
                                         float *out_min_h_percent);
bool get_pending_window_scrollbar_visibility(const MDGUI_Context *ctx,
                                             const char *title,
                                             bool *out_visible);
void set_pending_window_scrollbar_visibility(MDGUI_Context *ctx,
                                             const char *title, bool visible);
int normalize_tile_side(int side);
int normalize_tile_weight(int weight);
int fit_tile_min_size(int total_size, int count, int gap, int min_size);
void assign_tiled_window_rect(MDGUI_Window &w, int x, int y, int ww, int hh);
void tile_windows_grid(MDGUI_Context *ctx, const std::vector<int> &order,
                       int left, int top, int content_w, int content_h,
                       int gap, int *out_total_min_w = nullptr,
                       int *out_total_min_h = nullptr);
int sum_tile_weights(MDGUI_Context *ctx, const std::vector<int> &order);
void sort_by_weight_then_z(MDGUI_Context *ctx, std::vector<int> &order);
void tile_windows_vertical_weighted(MDGUI_Context *ctx,
                                    const std::vector<int> &order, int x,
                                    int y, int w, int h, int gap,
                                    int *out_total_min_w = nullptr,
                                    int *out_total_min_h = nullptr);
void tile_windows_horizontal_weighted(MDGUI_Context *ctx,
                                      const std::vector<int> &order, int x,
                                      int y, int w, int h, int gap,
                                      int *out_total_min_w = nullptr,
                                      int *out_total_min_h = nullptr);
void tile_windows_weighted_cluster(MDGUI_Context *ctx,
                                   const std::vector<int> &order, int x, int y,
                                   int w, int h, int gap,
                                   int *out_total_min_w = nullptr,
                                   int *out_total_min_h = nullptr);
void tile_windows_internal(MDGUI_Context *ctx,
                           int *out_total_min_w = nullptr,
                           int *out_total_min_h = nullptr);
int find_or_create_window(MDGUI_Context *ctx, const char *title, int x, int y,
                          int w, int h, bool *out_created = nullptr);
int is_current_window_topmost(MDGUI_Context *ctx, int margin = 0);
void draw_open_menu_overlay(MDGUI_Context *ctx);
void draw_cached_window_menu_overlay(MDGUI_Context *ctx, MDGUI_Window &win);
void apply_window_user_min_size(MDGUI_Context *ctx, MDGUI_Window &win);
std::string ellipsize_text_to_width(const std::string &text, int max_w);
void prune_expired_toasts(MDGUI_Context *ctx, unsigned long long now_ms);
void draw_toasts(MDGUI_Context *ctx);
void center_window_rect_menu_aware(MDGUI_Context *ctx, MDGUI_Window &win);
void draw_open_main_menu_overlay(MDGUI_Context *ctx);
void draw_status_bar(MDGUI_Context *ctx);

