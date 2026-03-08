#ifndef MGUI_C_H
#define MGUI_C_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MGUI_Context MGUI_Context;

typedef struct MGUI_RenderBackend {
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
} MGUI_RenderBackend;

typedef struct MGUI_Input {
  int mouse_x;
  int mouse_y;
  int mouse_down;
  int mouse_pressed;
  int mouse_wheel;
} MGUI_Input;

typedef void (*MGUI_WindowDrawFn)(MGUI_Context *ctx, int content_x,
                                  int content_y, int content_w, int content_h,
                                  void *user_data);

typedef struct MGUI_WindowPassItem {
  const char *title;
  int x;
  int y;
  int w;
  int h;
  int use_render_window;
  int show_menu;
  MGUI_WindowDrawFn draw_fn;
  void *user_data;
} MGUI_WindowPassItem;

enum {
  MGUI_MSGBOX_ONE_BUTTON = 1,
  MGUI_MSGBOX_TWO_BUTTON = 2,
};

enum {
  MGUI_TEXT_ALIGN_LEFT = 0,
  MGUI_TEXT_ALIGN_CENTER = 1,
};

enum {
  MGUI_THEME_DEFAULT = 0,
  MGUI_THEME_DARK = 1,
  MGUI_THEME_AMBER = 2,
  MGUI_THEME_GRAPHITE = 3,
  MGUI_THEME_MIDNIGHT = 4,
  MGUI_THEME_OLIVE = 5,
};

MGUI_Context *mgui_create(void *sdl_renderer);
MGUI_Context *mgui_create_with_backend(const MGUI_RenderBackend *backend);
void mgui_destroy(MGUI_Context *ctx);
void mgui_set_renderer(MGUI_Context *ctx, void *sdl_renderer);
void mgui_set_backend(MGUI_Context *ctx, const MGUI_RenderBackend *backend);

void mgui_begin_frame(MGUI_Context *ctx, const MGUI_Input *input);
void mgui_end_frame(MGUI_Context *ctx);

int mgui_begin_window(MGUI_Context *ctx, const char *title, int x, int y, int w,
                      int h);
void mgui_end_window(MGUI_Context *ctx);
int mgui_button(MGUI_Context *ctx, const char *text, int x, int y, int w,
                int h);
void mgui_label(MGUI_Context *ctx, const char *text, int x, int y);
void mgui_spacer(MGUI_Context *ctx, int pixels);
int mgui_checkbox(MGUI_Context *ctx, const char *text, bool *checked, int x,
                  int y);
int mgui_slider(MGUI_Context *ctx, const char *text, float *val, float min,
                float max, int x, int y, int w);
void mgui_separator(MGUI_Context *ctx, int x, int y, int w);
int mgui_listbox(MGUI_Context *ctx, const char **items, int item_count,
                 int *selected, int x, int y, int w, int rows);
int mgui_combo(MGUI_Context *ctx, const char *label, const char **items,
               int item_count, int *selected, int x, int y, int w);
void mgui_progress_bar(MGUI_Context *ctx, float value, int x, int y, int w,
                       int h, const char *overlay_text);

void mgui_show_demo_window(MGUI_Context *ctx);
void mgui_open_file_browser(MGUI_Context *ctx);
const char *mgui_show_file_browser(MGUI_Context *ctx);
void mgui_set_file_browser_filters(MGUI_Context *ctx, const char **extensions,
                                   int extension_count);
void mgui_set_window_open(MGUI_Context *ctx, const char *title, int open);
int mgui_is_window_open(MGUI_Context *ctx, const char *title);
void mgui_focus_window(MGUI_Context *ctx, const char *title);
void mgui_set_window_rect(MGUI_Context *ctx, const char *title, int x, int y,
                          int w, int h);
void mgui_set_windows_locked(MGUI_Context *ctx, int locked);
int mgui_is_windows_locked(MGUI_Context *ctx);

void mgui_begin_menu_bar(MGUI_Context *ctx);
int mgui_begin_menu(MGUI_Context *ctx, const char *text);
int mgui_menu_item(MGUI_Context *ctx, const char *text);
void mgui_end_menu(MGUI_Context *ctx);
void mgui_end_menu_bar(MGUI_Context *ctx);

void mgui_begin_main_menu_bar(MGUI_Context *ctx);
int mgui_begin_main_menu(MGUI_Context *ctx, const char *text);
int mgui_main_menu_item(MGUI_Context *ctx, const char *text);
void mgui_end_main_menu(MGUI_Context *ctx);
void mgui_end_main_menu_bar(MGUI_Context *ctx);

// Returns 0 while open/no click, 1 when first button clicked, 2 when second
// button clicked.
int mgui_message_box(MGUI_Context *ctx, const char *id, const char *title,
                     const char *text, int style, const char *button1,
                     const char *button2);
int mgui_message_box_ex(MGUI_Context *ctx, const char *id, const char *title,
                        const char *text, int style, const char *button1,
                        const char *button2, int text_align);
int mgui_get_window_z(MGUI_Context *ctx, const char *title);
void mgui_set_theme(int theme_id);
int mgui_get_theme(void);
void mgui_set_theme_color(int palette_index, unsigned char r, unsigned char g,
                          unsigned char b, unsigned char a);
void mgui_clear_theme_color(int palette_index);
void mgui_clear_all_theme_colors(void);

int mgui_begin_render_window(MGUI_Context *ctx, const char *title, int x, int y,
                             int w, int h, int show_menu, int *out_x,
                             int *out_y, int *out_w, int *out_h);
void mgui_run_window_pass(MGUI_Context *ctx, const MGUI_WindowPassItem *items,
                          int item_count);
void mgui_set_window_fullscreen(MGUI_Context *ctx, const char *title,
                                int fullscreen);
int mgui_is_window_fullscreen(MGUI_Context *ctx, const char *title);

void mgui_set_custom_cursor_enabled(MGUI_Context *ctx, int enabled);
int mgui_is_custom_cursor_enabled(MGUI_Context *ctx);

#ifdef __cplusplus
}
#endif

#endif
