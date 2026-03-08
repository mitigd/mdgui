#ifndef MDGUI_C_H
#define MDGUI_C_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MDGUI_Context MDGUI_Context;

typedef struct MDGUI_RenderBackend {
  void *user_data;

  // Optional frame hooks for explicit backend state setup/flush.
  void (*begin_frame)(void *user_data);
  void (*end_frame)(void *user_data);

  // Required for rendering widgets.
  void (*set_clip_rect)(void *user_data, int enabled, int x, int y, int w,
                        int h);
  void (*fill_rect_rgba)(void *user_data, unsigned char r, unsigned char g,
                         unsigned char b, unsigned char a, int x, int y,
                         int w, int h);
  void (*draw_line_rgba)(void *user_data, unsigned char r, unsigned char g,
                         unsigned char b, unsigned char a, int x1, int y1,
                         int x2, int y2);
  void (*draw_point_rgba)(void *user_data, unsigned char r, unsigned char g,
                          unsigned char b, unsigned char a, int x, int y);

  // Optional fast path for bitmap text glyph draws (returns non-zero if drawn).
  int (*draw_glyph_rgba)(void *user_data, unsigned char glyph, int x, int y,
                         unsigned char r, unsigned char g, unsigned char b,
                         unsigned char a);

  // Required for layout and timing behavior.
  int (*get_render_size)(void *user_data, int *out_w, int *out_h);
  unsigned long long (*get_ticks_ms)(void *user_data);
} MDGUI_RenderBackend;

typedef struct MDGUI_Input {
  int mouse_x;
  int mouse_y;
  int mouse_down;
  int mouse_pressed;
  int mouse_wheel;
} MDGUI_Input;

typedef void (*MDGUI_WindowDrawFn)(MDGUI_Context *ctx, int content_x,
                                  int content_y, int content_w, int content_h,
                                  void *user_data);

typedef struct MDGUI_WindowPassItem {
  const char *title;
  int x;
  int y;
  int w;
  int h;
  int use_render_window;
  int show_menu;
  MDGUI_WindowDrawFn draw_fn;
  void *user_data;
} MDGUI_WindowPassItem;

enum {
  MDGUI_MSGBOX_ONE_BUTTON = 1,
  MDGUI_MSGBOX_TWO_BUTTON = 2,
};

enum {
  MDGUI_TEXT_ALIGN_LEFT = 0,
  MDGUI_TEXT_ALIGN_CENTER = 1,
};

enum {
  MDGUI_THEME_DEFAULT = 0,
  MDGUI_THEME_DARK = 1,
  MDGUI_THEME_AMBER = 2,
  MDGUI_THEME_GRAPHITE = 3,
  MDGUI_THEME_MIDNIGHT = 4,
  MDGUI_THEME_OLIVE = 5,
};

enum {
  MDGUI_TILE_SIDE_AUTO = 0,
  MDGUI_TILE_SIDE_LEFT = 1,
  MDGUI_TILE_SIDE_RIGHT = 2,
  MDGUI_TILE_SIDE_TOP = 3,
  MDGUI_TILE_SIDE_BOTTOM = 4,
  MDGUI_TILE_SIDE_WEST = MDGUI_TILE_SIDE_LEFT,
  MDGUI_TILE_SIDE_EAST = MDGUI_TILE_SIDE_RIGHT,
};

MDGUI_Context *mdgui_create(void *sdl_renderer);
MDGUI_Context *mdgui_create_with_backend(const MDGUI_RenderBackend *backend);
void mdgui_destroy(MDGUI_Context *ctx);
void mdgui_set_renderer(MDGUI_Context *ctx, void *sdl_renderer);
void mdgui_set_backend(MDGUI_Context *ctx, const MDGUI_RenderBackend *backend);

void mdgui_begin_frame(MDGUI_Context *ctx, const MDGUI_Input *input);
void mdgui_end_frame(MDGUI_Context *ctx);

int mdgui_begin_window(MDGUI_Context *ctx, const char *title, int x, int y, int w,
                      int h);
void mdgui_end_window(MDGUI_Context *ctx);
int mdgui_button(MDGUI_Context *ctx, const char *text, int x, int y, int w,
                int h);
void mdgui_label(MDGUI_Context *ctx, const char *text, int x, int y);
void mdgui_spacer(MDGUI_Context *ctx, int pixels);
int mdgui_checkbox(MDGUI_Context *ctx, const char *text, bool *checked, int x,
                  int y);
int mdgui_slider(MDGUI_Context *ctx, const char *text, float *val, float min,
                float max, int x, int y, int w);
void mdgui_separator(MDGUI_Context *ctx, int x, int y, int w);
int mdgui_listbox(MDGUI_Context *ctx, const char **items, int item_count,
                 int *selected, int x, int y, int w, int rows);
int mdgui_combo(MDGUI_Context *ctx, const char *label, const char **items,
               int item_count, int *selected, int x, int y, int w);
void mdgui_progress_bar(MDGUI_Context *ctx, float value, int x, int y, int w,
                       int h, const char *overlay_text);
// For frame graph sizing, w/h follow widget conventions:
// w == 0 uses available width, w < 0 subtracts from available width.
// h == 0 uses remaining window content height, h < 0 subtracts from it.
void mdgui_frame_time_graph(MDGUI_Context *ctx, const float *frame_ms_samples,
                           int sample_count, float target_fps,
                           float graph_max_ms, int x, int y, int w, int h);

void mdgui_show_demo_window(MDGUI_Context *ctx);
void mdgui_open_file_browser(MDGUI_Context *ctx);
const char *mdgui_show_file_browser(MDGUI_Context *ctx);
void mdgui_set_file_browser_filters(MDGUI_Context *ctx, const char **extensions,
                                   int extension_count);
void mdgui_set_window_open(MDGUI_Context *ctx, const char *title, int open);
int mdgui_is_window_open(MDGUI_Context *ctx, const char *title);
void mdgui_focus_window(MDGUI_Context *ctx, const char *title);
void mdgui_set_window_rect(MDGUI_Context *ctx, const char *title, int x, int y,
                          int w, int h);
void mdgui_set_windows_locked(MDGUI_Context *ctx, int locked);
int mdgui_is_windows_locked(MDGUI_Context *ctx);
void mdgui_set_tile_manager_enabled(MDGUI_Context *ctx, int enabled);
int mdgui_is_tile_manager_enabled(MDGUI_Context *ctx);
void mdgui_tile_windows(MDGUI_Context *ctx);
void mdgui_set_window_tile_weight(MDGUI_Context *ctx, const char *title,
                                  int weight);
int mdgui_get_window_tile_weight(MDGUI_Context *ctx, const char *title);
void mdgui_set_window_tile_side(MDGUI_Context *ctx, const char *title,
                                int side);
int mdgui_get_window_tile_side(MDGUI_Context *ctx, const char *title);

void mdgui_begin_menu_bar(MDGUI_Context *ctx);
int mdgui_begin_menu(MDGUI_Context *ctx, const char *text);
int mdgui_menu_item(MDGUI_Context *ctx, const char *text);
void mdgui_end_menu(MDGUI_Context *ctx);
void mdgui_end_menu_bar(MDGUI_Context *ctx);

void mdgui_begin_main_menu_bar(MDGUI_Context *ctx);
int mdgui_begin_main_menu(MDGUI_Context *ctx, const char *text);
int mdgui_main_menu_item(MDGUI_Context *ctx, const char *text);
void mdgui_end_main_menu(MDGUI_Context *ctx);
void mdgui_end_main_menu_bar(MDGUI_Context *ctx);

// Returns 0 while open/no click, 1 when first button clicked, 2 when second
// button clicked.
int mdgui_message_box(MDGUI_Context *ctx, const char *id, const char *title,
                     const char *text, int style, const char *button1,
                     const char *button2);
int mdgui_message_box_ex(MDGUI_Context *ctx, const char *id, const char *title,
                        const char *text, int style, const char *button1,
                        const char *button2, int text_align);
int mdgui_get_window_z(MDGUI_Context *ctx, const char *title);
void mdgui_set_theme(int theme_id);
int mdgui_get_theme(void);
void mdgui_get_accent_color(unsigned char *out_r, unsigned char *out_g,
                            unsigned char *out_b);
void mdgui_set_theme_color(int palette_index, unsigned char r, unsigned char g,
                          unsigned char b, unsigned char a);
void mdgui_clear_theme_color(int palette_index);
void mdgui_clear_all_theme_colors(void);

int mdgui_begin_render_window(MDGUI_Context *ctx, const char *title, int x, int y,
                             int w, int h, int show_menu, int *out_x,
                             int *out_y, int *out_w, int *out_h);
void mdgui_run_window_pass(MDGUI_Context *ctx, const MDGUI_WindowPassItem *items,
                          int item_count);
void mdgui_set_window_fullscreen(MDGUI_Context *ctx, const char *title,
                                int fullscreen);
int mdgui_is_window_fullscreen(MDGUI_Context *ctx, const char *title);

void mdgui_set_custom_cursor_enabled(MDGUI_Context *ctx, int enabled);
int mdgui_is_custom_cursor_enabled(MDGUI_Context *ctx);

#ifdef __cplusplus
}
#endif

#endif
