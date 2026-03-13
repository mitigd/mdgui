#include "windowing_internal.hpp"

extern "C" {
int mdgui_begin_render_window_ex(MDGUI_Context *ctx, const char *title, int x,
                                 int y, int w, int h, int show_menu, int flags,
                                 int *out_x, int *out_y, int *out_w,
                                 int *out_h) {
  if (!ctx)
    return 0;

  // Nested mode: inside an existing
  // parent window, treat render
  // window as a clipped child
  // viewport anchored in parent-local
  // coordinates.
  if (ctx->window.current_window >= 0 &&
      ctx->window.current_window < (int)ctx->window.windows.size()) {
    auto &parent = ctx->window.windows[ctx->window.current_window];
    const auto *subpass = current_subpass(ctx);
    const bool no_chrome = (flags & MDGUI_WINDOW_FLAG_NO_CHROME) != 0;
    const int requested_h = h;
    w = resolve_dynamic_width(ctx, x, w, 8);
    const int ix = ctx->window.origin_x + x + ctx->layout.indent;
    const int logical_y = ctx->window.content_y + y;
    const int iy = subpass ? logical_y : (logical_y - parent.text_scroll);

    if (requested_h == 0 || requested_h < 0) {
      const int viewport_bottom =
          subpass ? subpass->local_h : (parent.y + parent.h - 4);
      int avail_h = viewport_bottom - logical_y;
      if (requested_h < 0)
        avail_h += requested_h;
      h = avail_h;
    }
    if (h < 8)
      h = 8;

    // Parent content clip (active
    // while drawing nested
    // frame/body) so border can never
    // bleed outside tiled/visible
    // parent content.
    const int parent_clip_x = current_viewport_x(ctx);
    const int parent_clip_y = ctx->window.origin_y;
    int parent_clip_w = current_viewport_width(ctx);
    int parent_clip_h = 0;
    if (subpass) {
      parent_clip_h = subpass->local_h - parent_clip_y;
    } else {
      parent_clip_h = (parent.y + parent.h - 4) - parent_clip_y;
    }
    if (parent_clip_w < 1)
      parent_clip_w = 1;
    if (parent_clip_h < 1)
      parent_clip_h = 1;
    mdgui_backend_set_clip_rect(1, parent_clip_x, parent_clip_y, parent_clip_w,
                                parent_clip_h);

    const int cx = no_chrome ? ix : (ix + 1);
    const int cy = no_chrome ? iy : (iy + 1);
    int cw = no_chrome ? w : (w - 2);
    int ch = no_chrome ? h : (h - 2);
    if (cw < 1)
      cw = 1;
    if (ch < 1)
      ch = 1;

    // Intersect child content clip
    // with parent content clip.
    int clip_x1 = cx;
    int clip_y1 = cy;
    int clip_x2 = cx + cw;
    int clip_y2 = cy + ch;
    const int parent_x2 = parent_clip_x + parent_clip_w;
    const int parent_y2 = parent_clip_y + parent_clip_h;
    if (clip_x1 < parent_clip_x)
      clip_x1 = parent_clip_x;
    if (clip_y1 < parent_clip_y)
      clip_y1 = parent_clip_y;
    if (clip_x2 > parent_x2)
      clip_x2 = parent_x2;
    if (clip_y2 > parent_y2)
      clip_y2 = parent_y2;
    int clip_w = clip_x2 - clip_x1;
    int clip_h = clip_y2 - clip_y1;
    if (clip_w <= 0 || clip_h <= 0) {
      note_content_bounds(ctx, ix + w, logical_y + h);
      ctx->window.content_y = logical_y + h + 4;
      if (out_x)
        *out_x = 0;
      if (out_y)
        *out_y = 0;
      if (out_w)
        *out_w = 0;
      if (out_h)
        *out_h = 0;
      (void)title;
      (void)show_menu;
      (void)flags;
      return 0;
    }

    MDGUI_Context::NestedRenderState nested{};
    nested.parent_origin_x = ctx->window.origin_x;
    nested.parent_origin_y = ctx->window.origin_y;
    nested.parent_content_y = ctx->window.content_y;
    nested.parent_content_req_right = ctx->layout.content_req_right;
    nested.parent_content_req_bottom = ctx->layout.content_req_bottom;
    nested.parent_window_has_nonlabel_widget = ctx->layout.window_has_nonlabel_widget;
    nested.parent_alpha_mod = mdgui_backend_get_alpha_mod();
    nested.parent_content_y_after = logical_y + h + 4;
    nested.parent_layout_indent = ctx->layout.indent;
    nested.parent_indent_stack_size = ctx->layout.indent_stack.size();
    ctx->window.nested_render_stack.push_back(nested);
    const bool transparent_bg = (flags & MDGUI_WINDOW_FLAG_TRANSPARENT_BG) != 0;
    const int frame_x1 = ix - 1;
    const int frame_y1 = iy - 1;
    const int frame_x2 = ix + w + 1;
    const int frame_y2 = iy + h + 1;
    if (!no_chrome) {
      mdgui_draw_frame_idx(nullptr, CLR_WINDOW_BORDER, frame_x1, frame_y1,
                           frame_x2, frame_y2);
    }
    if (!transparent_bg) {
      mdgui_fill_rect_idx(nullptr, CLR_BOX_BODY, ix, iy, w, h);
    }
    note_content_bounds(ctx, ix + w, logical_y + h);

    ctx->window.origin_x = cx;
    ctx->window.origin_y = cy;
    ctx->window.content_y = cy;
    ctx->layout.indent = 0;
    mdgui_backend_set_clip_rect(1, clip_x1, clip_y1, clip_w, clip_h);

    if (out_x)
      *out_x = cx;
    if (out_y)
      *out_y = cy;
    if (out_w)
      *out_w = cw;
    if (out_h)
      *out_h = ch;
    (void)title;
    (void)show_menu;
    (void)flags;
    return 1;
  }

  if (!mdgui_begin_window_ex(ctx, title, x, y, w, h, flags))
    return 0;
  if (show_menu) {
    mdgui_begin_menu_bar(ctx);
    mdgui_end_menu_bar(ctx);
  }
  if (ctx->window.current_window < 0)
    return 0;
  const auto &win = ctx->window.windows[ctx->window.current_window];
  const int cx = ctx->window.origin_x;
  int cy = ctx->window.origin_y;
  // Render-window content is raw viewport content; it should not inherit flow
  // layout padding. If menu exists, anchor directly below it.
  if (show_menu && ctx->menus.has_menu_bar)
    cy = ctx->menus.menu_bar_y + ctx->menus.menu_bar_h + 2;
  int cw = win.w - 4;
  int ch = (win.y + win.h - 4) - cy;
  if (cw < 1)
    cw = 1;
  if (ch < 1)
    ch = 1;
  ctx->window.content_y = cy;
  ctx->layout.indent = 0;
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

int mdgui_begin_render_window(MDGUI_Context *ctx, const char *title, int x,
                              int y, int w, int h, int show_menu, int *out_x,
                              int *out_y, int *out_w, int *out_h) {
  return mdgui_begin_render_window_ex(ctx, title, x, y, w, h, show_menu,
                                      MDGUI_WINDOW_FLAG_NONE, out_x, out_y,
                                      out_w, out_h);
}

void mdgui_overlay_init_config(MDGUI_OverlayConfig *config) {
  if (!config)
    return;
  memset(config, 0, sizeof(*config));
  config->subpass_scale = 1.0f;
  config->clamp_to_bounds = 1;
  config->consume_mouse_input_while_dragging = 1;
  config->window_flags =
      MDGUI_WINDOW_FLAG_NO_CHROME | MDGUI_WINDOW_FLAG_EXCLUDE_FROM_TILING;
}

void mdgui_overlay_init_state(MDGUI_OverlayState *state) {
  if (!state)
    return;
  memset(state, 0, sizeof(*state));
  state->use_subpass = 1;
  state->w = 240;
  state->h = 78;
  state->alpha = 255;
}

void mdgui_overlay_handle_drag(MDGUI_Input *input,
                               const MDGUI_OverlayConfig *config,
                               MDGUI_OverlayState *state, int raw_mouse_down,
                               int bounds_w, int bounds_h) {
  if (!input || !state) {
    return;
  }
  if (!state->visible || !state->allow_mouse_drag || state->click_through) {
    state->dragging = 0;
    return;
  }

  sanitize_overlay_state(state);

  if (input->mouse_pressed &&
      point_in_rect(input->mouse_x, input->mouse_y, state->x, state->y,
                    state->w, state->h)) {
    state->dragging = 1;
    state->drag_offset_x = input->mouse_x - state->x;
    state->drag_offset_y = input->mouse_y - state->y;
    if (!config || config->consume_mouse_input_while_dragging) {
      input->mouse_pressed = 0;
    }
  }

  if (state->dragging && raw_mouse_down) {
    int next_x = input->mouse_x - state->drag_offset_x;
    int next_y = input->mouse_y - state->drag_offset_y;
    if (!config || config->clamp_to_bounds) {
      if (bounds_w > 0)
        next_x = clampi(next_x, 0, std::max(0, bounds_w - state->w));
      if (bounds_h > 0)
        next_y = clampi(next_y, 0, std::max(0, bounds_h - state->h));
    }
    state->x = next_x;
    state->y = next_y;
    if (!config || config->consume_mouse_input_while_dragging) {
      input->mouse_pressed = 0;
      input->mouse_down = 0;
    }
  }

  if (!raw_mouse_down)
    state->dragging = 0;
}

int mdgui_begin_overlay(MDGUI_Context *ctx, const MDGUI_OverlayConfig *config,
                        MDGUI_OverlayState *state,
                        MDGUI_OverlayLayout *out_layout) {
  if (out_layout)
    memset(out_layout, 0, sizeof(*out_layout));
  if (!ctx || !config || !config->title || !state)
    return 0;

  sanitize_overlay_state(state);
  const bool click_through = state->click_through != 0;

  if (!state->visible) {
    mdgui_set_window_open(ctx, config->title, 0);
    return 0;
  }

  mdgui_set_window_rect(ctx, config->title, state->x, state->y, state->w,
                        state->h);
  mdgui_set_window_open(ctx, config->title, 1);
  mdgui_set_window_alpha(ctx, config->title, state->alpha);

  int view_x = 0;
  int view_y = 0;
  int view_w = 0;
  int view_h = 0;
  if (!mdgui_begin_render_window_ex(ctx, config->title, state->x, state->y,
                                    state->w, state->h, 0, config->window_flags,
                                    &view_x, &view_y, &view_w, &view_h)) {
    return 0;
  }

  if (ctx->window.current_window >= 0 &&
      ctx->window.current_window < (int)ctx->window.windows.size()) {
    ctx->window.windows[ctx->window.current_window].input_passthrough = click_through;
  }

  int base_x = view_x;
  int base_y = view_y;
  int base_w = view_w;
  int base_h = view_h;
  if (state->use_subpass && config->subpass_scale > 0.0f) {
    int sx = 0;
    int sy = 0;
    int sw = 0;
    int sh = 0;
    if (mdgui_begin_subpass(ctx, config->title, 0, 0, 0, 0, config->subpass_scale,
                            &sx, &sy, &sw, &sh)) {
      base_x = sx;
      base_y = sy;
      base_w = sw;
      base_h = sh;
    }
  }

  if (state->margin_top > 0)
    mdgui_spacing(ctx, state->margin_top);
  if (state->margin_left > 0)
    mdgui_indent(ctx, state->margin_left);

  if (out_layout) {
    out_layout->view_x = base_x;
    out_layout->view_y = base_y;
    out_layout->view_w = base_w;
    out_layout->view_h = base_h;
    out_layout->content_x = base_x + state->margin_left;
    out_layout->content_y = base_y + state->margin_top;
    out_layout->content_w =
        std::max(1, base_w - state->margin_left - state->margin_right);
    out_layout->content_h =
        std::max(1, base_h - state->margin_top - state->margin_bottom);
  }

  return 1;
}

void mdgui_end_overlay(MDGUI_Context *ctx, const MDGUI_OverlayConfig *config,
                       const MDGUI_OverlayState *state) {
  if (!ctx || !config || !state)
    return;
  if (state->margin_left > 0)
    mdgui_unindent(ctx);
  if (state->use_subpass && !ctx->window.subpass_stack.empty())
    mdgui_end_subpass(ctx);
  mdgui_end_window(ctx);
}

void mdgui_run_window_pass(MDGUI_Context *ctx,
                           const MDGUI_WindowPassItem *items, int item_count) {
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
    const int idx =
        find_or_create_window(ctx, it.title, it.x, it.y, it.w, it.h);
    if (idx < 0 || idx >= (int)ctx->window.windows.size())
      continue;
    order.push_back({i, ctx->window.windows[idx].z});
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
        if (ctx->window.current_window >= 0 &&
            ctx->window.current_window < (int)ctx->window.windows.size()) {
          const auto &win = ctx->window.windows[ctx->window.current_window];
          cx = ctx->window.origin_x;
          cy = ctx->window.content_y;
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

}


