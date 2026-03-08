#include "mdgui_c.h"
#include "mdgui_backends.h"
#include "mdgui_primitives.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>
#include <string.h>
#include <algorithm>
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
  int clip_h = (win.y + win.h - 2) - ctx->origin_y;
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

  mdgui_draw_frame_idx(nullptr, 0, def.x - 1, def.y - 1, def.x + def.w + 1,
          def.y + total_h + 1);
  mdgui_fill_rect_idx(nullptr, CLR_MENU_BG, def.x, def.y, def.w, total_h);

  for (int i = 0; i < (int)def.items.size(); ++i) {
    const int iy = def.y + (i * item_h);
    const int hovered = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                      def.x, iy, def.w, item_h);
    if (hovered) {
      mdgui_fill_rect_idx(nullptr, CLR_MENU_SEL, def.x, iy, def.w, item_h);
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

  mdgui_draw_frame_idx(nullptr, 0, def.x - 1, def.y - 1, def.x + def.w + 1,
          def.y + total_h + 1);
  mdgui_fill_rect_idx(nullptr, CLR_MENU_BG, def.x, def.y, def.w, total_h);

  for (int i = 0; i < (int)def.items.size(); ++i) {
    const int iy = def.y + (i * item_h);
    const int hovered = point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                      def.x, iy, def.w, item_h);
    if (hovered)
      mdgui_fill_rect_idx(nullptr, CLR_MENU_SEL, def.x, iy, def.w, item_h);
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
  const int title_h = 12;
  if (win.min_w < 50)
    win.min_w = 50;
  if (win.min_h < 30)
    win.min_h = 30;

  // Edge detection for hover AND interaction
  const int margin = 3;
  int edge_mask = 0;
  if (strcmp(title, "MESSAGE") != 0 && !win.is_maximized) {
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
  const int btn_w = 10;
  const int btn_h = 10;
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
    if (move_resize_allowed && title && strcmp(title, "MESSAGE") != 0 &&
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

  // Slim gray border around the whole window
  mdgui_draw_frame_idx(nullptr, CLR_TEXT_LIGHT, win.x - 1, win.y - 1, win.x + win.w + 1,
          win.y + win.h + 1);
  mdgui_fill_rect_idx(nullptr, CLR_BOX_TITLE, win.x, win.y, win.w, title_h);
  mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, win.x, win.y + title_h, win.w,
           win.h - title_h - 2);
  mdgui_fill_rect_idx(nullptr, CLR_BOX_TITLE, win.x, win.y + win.h - 2, win.w, 2);

  // Draw 3D-style Close button
  mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, close_x, close_y, btn_w, btn_h);
  mdgui_draw_hline_idx(nullptr, CLR_BUTTON_LIGHT, close_x, close_y, close_x + btn_w);
  mdgui_draw_vline_idx(nullptr, CLR_BUTTON_LIGHT, close_x, close_y, close_y + btn_h);
  mdgui_draw_hline_idx(nullptr, CLR_BUTTON_DARK, close_x, close_y + btn_h - 1,
            close_x + btn_w);
  mdgui_draw_vline_idx(nullptr, CLR_BUTTON_DARK, close_x + btn_w - 1, close_y,
            close_y + btn_h);
  // Draw X as two diagonal lines with margin
  mdgui_draw_line_idx(nullptr, CLR_TEXT_LIGHT, close_x + 2, close_y + 2,
             close_x + 7, close_y + 7);
  mdgui_draw_line_idx(nullptr, CLR_TEXT_LIGHT, close_x + 7, close_y + 2,
             close_x + 2, close_y + 7);

  // Draw 3D-style Maximize button (if not "MESSAGE")
  if (title && strcmp(title, "MESSAGE") != 0) {
    mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, max_x, max_y, btn_w, btn_h);
    mdgui_draw_hline_idx(nullptr, CLR_BUTTON_LIGHT, max_x, max_y, max_x + btn_w);
    mdgui_draw_vline_idx(nullptr, CLR_BUTTON_LIGHT, max_x, max_y, max_y + btn_h);
    mdgui_draw_hline_idx(nullptr, CLR_BUTTON_DARK, max_x, max_y + btn_h - 1,
              max_x + btn_w);
    mdgui_draw_vline_idx(nullptr, CLR_BUTTON_DARK, max_x + btn_w - 1, max_y,
              max_y + btn_h);
    mdgui_fill_rect_idx(nullptr, CLR_TEXT_LIGHT, max_x + 2, max_y + 2, btn_w - 4,
             btn_h - 4);
    mdgui_fill_rect_idx(nullptr, CLR_BUTTON_SURFACE, max_x + 3, max_y + 4, btn_w - 6,
             btn_h - 7);
  }

  if (title && mdgui_fonts[1]) {
    mdgui_fonts[1]->drawText(title, nullptr, win.x + 5, win.y + 1, CLR_TEXT_LIGHT);
  }

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
        mdgui_fill_rect_idx(nullptr, CLR_MENU_SEL, ix, ry, w, item_h);
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
    const int viewport_h = (win.y + win.h - 2) - viewport_top;
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
      mdgui_fill_rect_idx(nullptr, CLR_MENU_SEL, sb_x + 1, thumb_y, sb_w - 2, thumb_h);
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
    mdgui_draw_hline_idx(nullptr, 29, abs_x, abs_y, abs_x + w);
    mdgui_draw_vline_idx(nullptr, 29, abs_x + w - 1, abs_y, abs_y + h);
    mdgui_draw_vline_idx(nullptr, 23, abs_x, abs_y, abs_y + h);
    mdgui_draw_hline_idx(nullptr, 23, abs_x, abs_y + h - 1, abs_x + w);
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
  mdgui_draw_hline_idx(nullptr, CLR_BUTTON_DARK, ix, iy, ix + box_size);
  mdgui_draw_vline_idx(nullptr, CLR_BUTTON_DARK, ix, iy, iy + box_size);
  mdgui_draw_hline_idx(nullptr, CLR_BUTTON_LIGHT, ix, iy + box_size - 1, ix + box_size);
  mdgui_draw_vline_idx(nullptr, CLR_BUTTON_LIGHT, ix + box_size - 1, iy, iy + box_size);

  if (*checked && mdgui_fonts[1]) {
    mdgui_fonts[1]->drawText("x", nullptr, ix + 2, iy + 0, CLR_TEXT_LIGHT);
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
  mdgui_draw_hline_idx(nullptr, CLR_BUTTON_DARK, ix, iy, ix + w);
  mdgui_draw_hline_idx(nullptr, CLR_BUTTON_LIGHT, ix, iy + 1, ix + w);
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

  mdgui_draw_frame_idx(nullptr, CLR_TEXT_DARK, ix - 1, iy - 1, ix + w + 1, iy + box_h + 1);
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
      mdgui_fill_rect_idx(nullptr, CLR_MENU_SEL, ix, ry, w, row_h);
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

  mdgui_draw_frame_idx(nullptr, CLR_TEXT_DARK, ix - 1, iy - 1, ix + w + 1, iy + box_h + 1);
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
  const int logical_y = ctx->content_y + y;
  const int iy = logical_y - win.text_scroll;
  const int fill_w = (int)((float)w * value);

  mdgui_draw_frame_idx(nullptr, CLR_TEXT_DARK, ix - 1, iy - 1, ix + w + 1, iy + h + 1);
  mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, ix, iy, w, h);
  if (fill_w > 0)
    mdgui_fill_rect_idx(nullptr, CLR_MENU_SEL, ix, iy, fill_w, h);

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
  ctx->content_y += h + 4;
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
  def.y = y + 10;
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
    mdgui_fill_rect_idx(nullptr, CLR_MENU_SEL, x, y, item_w, 10);
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
  def.y = y + ctx->main_menu_bar_h;
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
    mdgui_fill_rect_idx(nullptr, CLR_MENU_SEL, x, y, item_w, ctx->main_menu_bar_h);
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
    ctx->windows[ctx->current_window].z = ++ctx->z_counter;
  }

  const auto &win = ctx->windows[ctx->current_window];
  mdgui_fill_rect_idx(nullptr, CLR_MSG_BG, win.x, win.y + 12, win.w, win.h - 24);
  mdgui_fill_rect_idx(nullptr, CLR_MSG_BAR, win.x, win.y, win.w, 12);
  mdgui_fill_rect_idx(nullptr, CLR_MSG_BAR, win.x, win.y + win.h - 12, win.w, 12);
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
  if (style == MDGUI_MSGBOX_TWO_BUTTON) {
    const int b1x = box_w / 4 - 35;
    const int b2x = (box_w * 3) / 4 - 35;
    if (mdgui_button(ctx, button1, b1x, box_h - 24, 70, 12))
      result = 1;
    if (mdgui_button(ctx, button2, b2x, box_h - 24, 70, 12))
      result = 2;
  } else {
    const int bx = box_w / 2 - 35;
    if (mdgui_button(ctx, button1, bx, box_h - 24, 70, 12))
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

void mdgui_set_window_fullscreen(MDGUI_Context *ctx, const char *title,
                                int fullscreen) {
  if (!ctx || !title)
    return;
  for (int i = 0; i < (int)ctx->windows.size(); ++i) {
    auto &w = ctx->windows[i];
    if (w.id != title)
      continue;
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
  int ch = (win.y + win.h - 2) - cy;
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
          ch = (win.y + win.h - 2) - cy;
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
      mdgui_fill_rect_idx(nullptr, CLR_MENU_SEL, list_x, ry, content_w, row_h);
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
    mdgui_fill_rect_idx(nullptr, CLR_MENU_SEL, sb_x + 1, thumb_y, scrollbar_w - 2,
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
    const int viewport_h = (win.y + win.h - 2) - ctx->origin_y;
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

