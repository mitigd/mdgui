#include "../core/internal.hpp"

extern "C" {
void mdgui_show_demo_window(MDGUI_Context *ctx) {
  static bool show_demo = true;
  static float progress = 0.35f;
  static int quality_idx = 0;
  static int theme_idx = MDGUI_THEME_DEFAULT;
  static int pane_tab = 0;
  static const char *quality_items[] = {"Nearest", "Bilinear", "CRT Mask"};
  static const char *theme_items[] = {"Default",  "Dark",     "Amber",
                                      "Graphite", "Midnight", "Olive"};
  static const char *pane_tabs[] = {"Scene",   "Inspector", "Console",
                                    "Profiler","Assets",    "Settings",
                                    "History"};
  if (!show_demo)
    return;

  if (mdgui_begin_window(ctx, "MDGUI Demo Window", 20, 20, 220, 220)) {
    static bool check1 = true;
    static bool check2 = false;
    static float vol1 = 0.5f;
    static float vol2 = 0.8f;
    MDGUI_Font *demo_font = nullptr;

    if (!ctx->demo.font_labels.empty() &&
        ctx->demo.font_labels.size() == ctx->demo.fonts.size()) {
      mdgui_label(ctx, "Window Font");
      if (ctx->demo.font_index < 0 ||
          ctx->demo.font_index >= (int)ctx->demo.fonts.size()) {
        ctx->demo.font_index = 0;
      }
      mdgui_combo(ctx, nullptr, ctx->demo.font_labels.data(),
                  (int)ctx->demo.font_labels.size(),
                  &ctx->demo.font_index, -16);
      if (ctx->demo.font_index >= 0 &&
          ctx->demo.font_index < (int)ctx->demo.fonts.size()) {
        demo_font = ctx->demo.fonts[(size_t)ctx->demo.font_index];
      }
      if (demo_font)
        mdgui_push_font(ctx, demo_font);
    }

    if (mdgui_collapsing_header(ctx, "demo.widgets", "Aesthetics & Widgets",
                                -16, 1)) {
      mdgui_indent(ctx, 8);
      mdgui_checkbox(ctx, "Enable Sound", &check1);
      mdgui_checkbox(ctx, "Turbo Mode", &check2);
      mdgui_unindent(ctx);
    }

    if (mdgui_collapsing_header(ctx, "demo.volumes", "Audio Volumes", -16, 1)) {
      mdgui_indent(ctx, 8);
      mdgui_slider(ctx, "Master", &vol1, 0.0f, 1.0f, -54);
      mdgui_slider(ctx, "Music", &vol2, 0.0f, 1.0f, -46);
      mdgui_unindent(ctx);
    }

    if (mdgui_collapsing_header(ctx, "demo.renderer", "Renderer Options", -16,
                                1)) {
      mdgui_indent(ctx, 8);
      mdgui_listbox(ctx, quality_items, 3, &quality_idx, -16, 3);

      mdgui_label(ctx, "Filter Combo");
      mdgui_combo(ctx, nullptr, quality_items, 3, &quality_idx, -16);

      mdgui_label(ctx, "Theme");
      theme_idx = mdgui_get_theme();
      if (mdgui_combo(ctx, nullptr, theme_items, 6, &theme_idx, -16)) {
        mdgui_set_theme(theme_idx);
      }

      mdgui_label(ctx, "Frame Progress");
      mdgui_progress_bar(ctx, progress, -16, 10, nullptr);
      if (mdgui_button(ctx, "Step", 56, 12)) {
        progress += 0.1f;
        if (progress > 1.0f)
          progress = 0.0f;
      }
      mdgui_unindent(ctx);
    }

    if (mdgui_collapsing_header(ctx, "demo.tabs", "Pane Tabs", -16, 1)) {
      mdgui_indent(ctx, 8);
      mdgui_tab_bar(ctx, "demo.panes", pane_tabs,
                    (int)(sizeof(pane_tabs) / sizeof(pane_tabs[0])), &pane_tab,
                    -16);
      if (pane_tab == 0) {
        mdgui_label(ctx, "Scene pane: viewport controls.");
      } else if (pane_tab == 1) {
        mdgui_label(ctx, "Inspector pane: entity properties.");
      } else if (pane_tab == 2) {
        mdgui_label(ctx, "Console pane: logs + commands.");
      } else if (pane_tab == 3) {
        mdgui_label(ctx, "Profiler pane: frame metrics.");
      } else if (pane_tab == 4) {
        mdgui_label(ctx, "Assets pane: project files.");
      } else if (pane_tab == 5) {
        mdgui_label(ctx, "Settings pane: runtime options.");
      } else {
        mdgui_label(ctx, "History pane: undo / redo timeline.");
      }
      mdgui_unindent(ctx);
    }

    // Reserve footer space, but only
    // show the close action when
    // scrolled near the bottom of
    // content.
    const auto &win = ctx->window.windows[ctx->window.current_window];
    const int viewport_h = (win.y + win.h - 4) - ctx->window.origin_y;
    const int content_h_before_footer = ctx->window.content_y - ctx->window.origin_y;
    const int footer_h = 6 + 12 + 4; // spacer + button
                                     // + widget gap
    const int total_h_with_footer = content_h_before_footer + footer_h;
    const int max_scroll_with_footer =
        (viewport_h > 0 && total_h_with_footer > viewport_h)
            ? (total_h_with_footer - viewport_h)
            : 0;
    const bool show_close = (max_scroll_with_footer == 0) ||
                            (win.text_scroll >= (max_scroll_with_footer - 2));

    const int footer_start_y = ctx->window.content_y;
    mdgui_spacing(ctx, footer_h);
    if (show_close) {
      const int saved_demo_content_y = ctx->window.content_y;
      const int saved_demo_indent = ctx->layout.indent;
      const bool saved_demo_has_last = ctx->layout.has_last_item;
      const bool saved_demo_same_line = ctx->layout.same_line;
      ctx->window.content_y = footer_start_y + 6 + win.text_scroll;
      ctx->layout.indent = 10;
      ctx->layout.has_last_item = false;
      ctx->layout.same_line = false;
      if (mdgui_button(ctx, "Close Demo", -12, 12)) {
        ctx->window.windows[ctx->window.current_window].closed = true;
      }
      ctx->window.content_y = saved_demo_content_y;
      ctx->layout.indent = saved_demo_indent;
      ctx->layout.has_last_item = saved_demo_has_last;
      ctx->layout.same_line = saved_demo_same_line;
    }

    if (demo_font)
      mdgui_pop_font(ctx);
    mdgui_end_window(ctx);
  }
}
}


