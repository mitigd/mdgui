#pragma once

#include "mdgui_c.h"
#include "mdgui_backends.h"
#include "mdgui_primitives.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <string.h>
#include <string>
#include <utility>
#include <vector>

extern MDGuiFont *mdgui_fonts[10];

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

  struct RenderState {
    MDGUI_RenderBackend backend;
    MDGUI_Input input;
  } render;

  struct WindowState {
    std::vector<MDGUI_Window> windows;
    int z_counter;
    int dragging_window;
    int drag_off_x;
    int drag_off_y;
    int current_window;
    int origin_x;
    int origin_y;
    int content_y;
    int resizing_window;
    int resize_edge_mask;
    int m_start_x;
    int m_start_y;
    int w_start_x;
    int w_start_y;
    int w_start_w;
    int w_start_h;
    std::vector<NestedRenderState> nested_render_stack;
    std::vector<SubpassState> subpass_stack;
    bool windows_locked;
    bool tile_manager_enabled;
    unsigned char default_window_alpha;
  } window;

  struct MenuState {
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
  } menus;

  struct CursorState {
    SDL_Cursor *cursors[16];
    int cursor_anim_count;
    bool custom_cursor_enabled;
    int cursor_request_idx;
    int current_cursor_idx;
  } cursor;

  struct LayoutState {
    int content_req_right;
    int content_req_bottom;
    struct Style {
      int spacing_x;
      int spacing_y;
      int section_spacing_y;
      int indent_step;
      int label_h;
      int content_pad_x;
      int content_pad_y;
    } style;
    bool same_line;
    bool has_last_item;
    int last_item_x;
    int last_item_y;
    int last_item_w;
    int last_item_h;
    bool columns_active;
    int columns_count;
    int columns_index;
    int columns_start_x;
    int columns_start_y;
    int columns_width;
    int columns_max_bottom;
    std::vector<int> columns_bottoms;
    int indent;
    std::vector<int> indent_stack;
    MDGUI_Font *default_font;
    std::vector<MDGUI_Font *> font_stack;
    bool window_has_nonlabel_widget;
  } layout;

  struct InteractionState {
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
  } interaction;

  struct BrowserState {
    MDGUI_Font *path_font;
    bool open;
    bool select_folders;
    std::string cwd;
    std::vector<FileBrowserEntry> entries;
    int selected;
    int scroll;
    bool scroll_dragging;
    int scroll_drag_offset;
    std::vector<std::string> pending_tile_excluded_titles;
    std::vector<PendingWindowMinSize> pending_window_min_sizes;
    std::vector<PendingWindowScrollbarVisibility> pending_window_scrollbars;
    int last_click_idx;
    Uint64 last_click_ticks;
    std::string result;
    std::vector<std::string> ext_filters;
    std::vector<std::string> drives;
    Uint64 last_drive_scan_ticks;
    std::string last_selected_path;
    bool restore_scroll_pending;
    bool center_pending;
    int open_x;
    int open_y;
    bool path_subpass_enabled;
  } browser;

  struct DemoState {
    std::vector<std::string> font_labels_storage;
    std::vector<const char *> font_labels;
    std::vector<MDGUI_Font *> fonts;
    int font_index;
  } demo;
};

