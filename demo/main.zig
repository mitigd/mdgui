const std = @import("std");

const c = @cImport({
    @cInclude("SDL3/SDL.h");
    @cInclude("mdgui_c.h");
});

const render_api_window_title = "WINDOW API";
const startup_tile_windows = true;
const startup_lock_tiled_windows = false;

const Analytics = struct {
    const history_len = 240;
    const histogram_bin_count = 6;

    frame_ms_history: [history_len]f32 = [_]f32{16.67} ** history_len,
    fps_history: [history_len]f32 = [_]f32{60.0} ** history_len,
    cursor: usize = 0,
    count: usize = 0,
    show_window: bool = true,
    show_graph: bool = true,
    target_fps: f32 = 60.0,
    graph_max_ms: f32 = 40.0,
};

const AnalyticsStats = struct {
    current_fps: f32,
    current_ms: f32,
    avg_fps: f32,
    min_fps: f32,
    max_fps: f32,
};

fn clamp01(v: f32) f32 {
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

fn analyticsPush(a: *Analytics, frame_ms: f32) void {
    const clamped_ms = if (frame_ms < 0.01) 0.01 else frame_ms;
    const fps = 1000.0 / clamped_ms;
    a.frame_ms_history[a.cursor] = clamped_ms;
    a.fps_history[a.cursor] = fps;
    a.cursor = (a.cursor + 1) % Analytics.history_len;
    if (a.count < Analytics.history_len) a.count += 1;
}

fn analyticsOldestIndex(a: *const Analytics) usize {
    if (a.count < Analytics.history_len) return 0;
    return a.cursor;
}

fn analyticsSampleFrameMs(a: *const Analytics, oldest_relative_index: usize) f32 {
    if (a.count == 0) return 16.67;
    const idx = (analyticsOldestIndex(a) + oldest_relative_index) % Analytics.history_len;
    return a.frame_ms_history[idx];
}

fn analyticsStats(a: *const Analytics) AnalyticsStats {
    if (a.count == 0) {
        return .{
            .current_fps = 0.0,
            .current_ms = 0.0,
            .avg_fps = 0.0,
            .min_fps = 0.0,
            .max_fps = 0.0,
        };
    }

    const newest_idx = if (a.cursor == 0) Analytics.history_len - 1 else a.cursor - 1;
    var sum_fps: f32 = 0.0;
    var min_fps: f32 = a.fps_history[0];
    var max_fps: f32 = a.fps_history[0];
    var i: usize = 0;
    while (i < a.count) : (i += 1) {
        const idx = (analyticsOldestIndex(a) + i) % Analytics.history_len;
        const fps = a.fps_history[idx];
        sum_fps += fps;
        if (fps < min_fps) min_fps = fps;
        if (fps > max_fps) max_fps = fps;
    }

    return .{
        .current_fps = a.fps_history[newest_idx],
        .current_ms = a.frame_ms_history[newest_idx],
        .avg_fps = sum_fps / @as(f32, @floatFromInt(a.count)),
        .min_fps = min_fps,
        .max_fps = max_fps,
    };
}

fn analyticsHistogram(a: *const Analytics) [Analytics.histogram_bin_count]usize {
    var bins: [Analytics.histogram_bin_count]usize = [_]usize{0} ** Analytics.histogram_bin_count;
    var i: usize = 0;
    while (i < a.count) : (i += 1) {
        const ms = analyticsSampleFrameMs(a, i);
        if (ms < 14.0) {
            bins[0] += 1;
        } else if (ms < 16.7) {
            bins[1] += 1;
        } else if (ms < 20.0) {
            bins[2] += 1;
        } else if (ms < 25.0) {
            bins[3] += 1;
        } else if (ms < 33.0) {
            bins[4] += 1;
        } else {
            bins[5] += 1;
        }
    }
    return bins;
}

fn drawAnalyticsWindow(ctx: ?*c.MDGUI_Context, analytics: *Analytics) void {
    if (!analytics.show_window) return;
    if (c.mdgui_begin_window(ctx, "PERF ANALYTICS", 10, 145, 240, 205) == 0) return;

    const stats = analyticsStats(analytics);

    const was_show_graph = analytics.show_graph;
    var toggle_graph = analytics.show_graph;
    _ = c.mdgui_checkbox(ctx, "Show realtime graph", &toggle_graph, 10, 5);
    analytics.show_graph = toggle_graph;
    if (!was_show_graph and analytics.show_graph) {
        c.mdgui_set_window_open(ctx, "PERF GRAPH", 1);
        c.mdgui_focus_window(ctx, "PERF GRAPH");
    }

    c.mdgui_label(ctx, "FPS + Frame Time", 10, 5);

    var line1: [64]u8 = undefined;
    const txt_fps = std.fmt.bufPrintZ(&line1, "{d:.1} fps (avg {d:.1})", .{ stats.current_fps, stats.avg_fps }) catch "fps";
    c.mdgui_progress_bar(ctx, clamp01(stats.current_fps / analytics.target_fps), 10, 3, -16, 10, txt_fps.ptr);

    var line2: [64]u8 = undefined;
    const txt_ms = std.fmt.bufPrintZ(&line2, "{d:.2} ms", .{stats.current_ms}) catch "ms";
    c.mdgui_progress_bar(ctx, clamp01(stats.current_ms / analytics.graph_max_ms), 10, 3, -16, 10, txt_ms.ptr);

    var line3: [64]u8 = undefined;
    const txt_minmax = std.fmt.bufPrintZ(&line3, "min/max {d:.1}/{d:.1}", .{ stats.min_fps, stats.max_fps }) catch "min/max";
    c.mdgui_label(ctx, txt_minmax.ptr, 10, 5);

    c.mdgui_separator(ctx, 10, 5, 0);
    c.mdgui_label(ctx, "Frame Time Histogram", 10, 5);

    const bins = analyticsHistogram(analytics);
    const labels = [_][]const u8{ "<14ms", "14-16.7", "16.7-20", "20-25", "25-33", ">=33" };
    var max_bin: usize = 1;
    for (bins) |v| {
        if (v > max_bin) max_bin = v;
    }

    var i: usize = 0;
    while (i < bins.len) : (i += 1) {
        var row: [64]u8 = undefined;
        const pct = if (analytics.count == 0) 0.0 else (@as(f32, @floatFromInt(bins[i])) * 100.0 / @as(f32, @floatFromInt(analytics.count)));
        const txt = std.fmt.bufPrintZ(&row, "{s}: {d:.0}%", .{ labels[i], pct }) catch "bin";
        c.mdgui_label(ctx, txt.ptr, 10, 3);
        c.mdgui_progress_bar(ctx, @as(f32, @floatFromInt(bins[i])) / @as(f32, @floatFromInt(max_bin)), 10, 1, -16, 8, null);
    }

    c.mdgui_end_window(ctx);
}

fn drawMainWindow(ctx: ?*c.MDGUI_Context, running: *bool, open_file_browser: *bool) void {
    if (c.mdgui_begin_window(ctx, "MDGUI", 10, 10, 190, 130) != 0) {
        c.mdgui_begin_menu_bar(ctx);
        if (c.mdgui_begin_menu(ctx, "FILE") != 0) {
            if (c.mdgui_menu_item(ctx, "OPEN ROM") != 0) open_file_browser.* = true;
            if (c.mdgui_menu_item(ctx, "EXIT") != 0) running.* = false;
            c.mdgui_end_menu(ctx);
        }
        if (c.mdgui_begin_menu(ctx, "OPTIONS") != 0) {
            _ = c.mdgui_menu_item(ctx, "VIDEO");
            _ = c.mdgui_menu_item(ctx, "AUDIO");
            c.mdgui_end_menu(ctx);
        }
        c.mdgui_end_menu_bar(ctx);
        c.mdgui_label(ctx, "Demo window is draggable.", 8, 6);
        if (c.mdgui_button(ctx, "LOAD ROM", 10, 20, 90, 20) != 0) open_file_browser.* = true;
        _ = c.mdgui_button(ctx, "OPTIONS", 10, 45, 90, 20);
        c.mdgui_end_window(ctx);
    }
}

fn drawDemoWindow(ctx: ?*c.MDGUI_Context, show_demo: bool) void {
    if (show_demo) {
        c.mdgui_show_demo_window(ctx);
    }
}

fn drawWindowApiDemo(ctx: ?*c.MDGUI_Context, renderer: ?*c.SDL_Renderer, emu_view_menu: bool) void {
    var view_x: c_int = 0;
    var view_y: c_int = 0;
    var view_w: c_int = 0;
    var view_h: c_int = 0;
    if (c.mdgui_begin_render_window(
        ctx,
        render_api_window_title,
        250,
        10,
        220,
        175,
        if (emu_view_menu) 1 else 0,
        &view_x,
        &view_y,
        &view_w,
        &view_h,
    ) != 0) {
        var bg = c.SDL_FRect{
            .x = @floatFromInt(view_x),
            .y = @floatFromInt(view_y),
            .w = @floatFromInt(view_w),
            .h = @floatFromInt(view_h),
        };
        _ = c.SDL_SetRenderDrawColor(renderer, 0x12, 0x12, 0x12, 0xff);
        _ = c.SDL_RenderFillRect(renderer, &bg);

        // Get accent color from current theme
        var accent_r: u8 = 0x70;
        var accent_g: u8 = 0xf0;
        var accent_b: u8 = 0x70;
        c.mdgui_get_accent_color(&accent_r, &accent_g, &accent_b);

        // Create light and dark variants for checkerboard (cast to avoid overflow)
        const light_r: u8 = @intCast(@min(255, @as(u16, accent_r) + 80));
        const light_g: u8 = @intCast(@min(255, @as(u16, accent_g) + 40));
        const light_b: u8 = @intCast(@min(255, @as(u16, accent_b) + 80));

        const dark_r: u8 = if (accent_r > 30) accent_r - 30 else 0;
        const dark_g: u8 = if (accent_g > 30) accent_g - 30 else 0;
        const dark_b: u8 = if (accent_b > 30) accent_b - 30 else 0;

        const tile_w: c_int = 8;
        const tile_h: c_int = 8;
        var ty: c_int = 0;
        while (ty < view_h) : (ty += tile_h) {
            var tx: c_int = 0;
            while (tx < view_w) : (tx += tile_w) {
                if (@mod(@divTrunc(tx, tile_w) + @divTrunc(ty, tile_h), @as(c_int, 2)) == 0) {
                    _ = c.SDL_SetRenderDrawColor(renderer, light_r, light_g, light_b, 0xff);
                } else {
                    _ = c.SDL_SetRenderDrawColor(renderer, dark_r, dark_g, dark_b, 0xff);
                }
                const cw: c_int = @min(tile_w, view_w - tx);
                const ch: c_int = @min(tile_h, view_h - ty);
                var cell = c.SDL_FRect{
                    .x = @floatFromInt(view_x + tx),
                    .y = @floatFromInt(view_y + ty),
                    .w = @floatFromInt(cw),
                    .h = @floatFromInt(ch),
                };
                _ = c.SDL_RenderFillRect(renderer, &cell);
            }
        }
        c.mdgui_end_window(ctx);
    }
}

fn drawPerfGraphWindow(ctx: ?*c.MDGUI_Context, analytics: *const Analytics) void {
    if (c.mdgui_begin_window(ctx, "PERF GRAPH", 250, 195, 380, 155) == 0) return;

    var ordered: [Analytics.history_len]f32 = [_]f32{16.67} ** Analytics.history_len;
    var i: usize = 0;
    while (i < analytics.count) : (i += 1) {
        ordered[i] = analyticsSampleFrameMs(analytics, i);
    }

    c.mdgui_frame_time_graph(
        ctx,
        &ordered[0],
        @as(c_int, @intCast(analytics.count)),
        analytics.target_fps,
        analytics.graph_max_ms,
        0,
        0,
        0,
        0,
    );

    c.mdgui_end_window(ctx);
}

fn getLogicalRenderSize(renderer: ?*c.SDL_Renderer, out_w: *c_int, out_h: *c_int) void {
    var rw: c_int = 640;
    var rh: c_int = 360;
    _ = c.SDL_GetRenderOutputSize(renderer, &rw, &rh);
    var sx: f32 = 1.0;
    var sy: f32 = 1.0;
    _ = c.SDL_GetRenderScale(renderer, &sx, &sy);
    if (sx > 0.0) rw = @intFromFloat(@as(f32, @floatFromInt(rw)) / sx);
    if (sy > 0.0) rh = @intFromFloat(@as(f32, @floatFromInt(rh)) / sy);
    if (rw < 320) rw = 320;
    if (rh < 240) rh = 240;
    out_w.* = rw;
    out_h.* = rh;
}

fn tileDemoWindows(ctx: ?*c.MDGUI_Context, renderer: ?*c.SDL_Renderer, show_demo: bool, show_graph: bool) void {
    var rw: c_int = 0;
    var rh: c_int = 0;
    getLogicalRenderSize(renderer, &rw, &rh);

    const gap: c_int = 6;
    const top: c_int = 18;
    const bottom: c_int = 6;
    const left: c_int = gap;
    const right: c_int = gap;

    const content_w: c_int = rw - left - right;
    const content_h: c_int = rh - top - bottom;
    if (content_w <= 0 or content_h <= 0) return;

    // Collect active windows in a stable priority order.
    var titles: [5][*:0]const u8 = undefined;
    var count: usize = 0;
    titles[count] = "MDGUI";
    count += 1;
    titles[count] = render_api_window_title;
    count += 1;
    titles[count] = "PERF ANALYTICS";
    count += 1;
    if (show_demo) {
        titles[count] = "MDGUI Demo Window";
        count += 1;
    }
    if (show_graph) {
        titles[count] = "PERF GRAPH";
        count += 1;
    }
    if (count == 0) return;

    const aspect = @as(f32, @floatFromInt(content_w)) / @as(f32, @floatFromInt(@max(content_h, 1)));
    const n_f = @as(f32, @floatFromInt(count));
    var cols: c_int = @intFromFloat(@ceil(std.math.sqrt(n_f * aspect)));
    if (cols < 1) cols = 1;
    if (cols > @as(c_int, @intCast(count))) cols = @as(c_int, @intCast(count));
    var rows: c_int = @divTrunc(@as(c_int, @intCast(count)) + cols - 1, cols);
    if (rows < 1) rows = 1;

    var y = top;
    var idx: usize = 0;
    var row: c_int = 0;
    while (row < rows and idx < count) : (row += 1) {
        const rows_left: c_int = rows - row;
        const remaining: c_int = @as(c_int, @intCast(count - idx));
        const cols_this_row: c_int = @divTrunc(remaining + rows_left - 1, rows_left);
        const h_remaining = (top + content_h) - y - (rows_left - 1) * gap;
        const row_h: c_int = @max(80, @divTrunc(h_remaining, rows_left));

        var x = left;
        var col: c_int = 0;
        while (col < cols_this_row and idx < count) : (col += 1) {
            const cols_left: c_int = cols_this_row - col;
            const w_remaining = (left + content_w) - x - (cols_left - 1) * gap;
            const cell_w: c_int = @max(120, @divTrunc(w_remaining, cols_left));
            c.mdgui_set_window_rect(ctx, titles[idx], x, y, cell_w, row_h);
            idx += 1;
            x += cell_w + gap;
        }
        y += row_h + gap;
    }
}

pub fn main() !void {
    if (c.SDL_Init(c.SDL_INIT_VIDEO) == false) return error.SDLInitFailed;
    defer c.SDL_Quit();

    const window = c.SDL_CreateWindow("MDGUI Demo", 1280, 720, 0) orelse return error.SDLWindowFailed;
    const renderer = c.SDL_CreateRenderer(window, null) orelse return error.SDLRendererFailed;
    _ = c.SDL_SetRenderScale(renderer, 2.0, 2.0);
    if (c.SDL_GetRendererName(renderer)) |renderer_name| {
        std.log.info("SDL renderer backend: {s}", .{std.mem.span(renderer_name)});
    } else {
        std.log.info("SDL renderer backend: <unknown>", .{});
    }
    var out_w: c_int = 0;
    var out_h: c_int = 0;
    _ = c.SDL_GetRenderOutputSize(renderer, &out_w, &out_h);
    var rsx: f32 = 1.0;
    var rsy: f32 = 1.0;
    _ = c.SDL_GetRenderScale(renderer, &rsx, &rsy);
    std.log.info("Render output: {d}x{d}, scale {d:.2}x{d:.2}", .{ out_w, out_h, rsx, rsy });

    const ctx = c.mdgui_create(renderer) orelse return error.MdguiInitFailed;
    defer c.mdgui_destroy(ctx);
    c.mdgui_set_custom_cursor_enabled(ctx, 1); // demo opt-in; library default is off
    c.mdgui_set_windows_locked(ctx, if (startup_lock_tiled_windows) 1 else 0);

    var running = true;
    var input = c.MDGUI_Input{
        .mouse_x = 0,
        .mouse_y = 0,
        .mouse_down = 0,
        .mouse_pressed = 0,
        .mouse_wheel = 0,
    };
    var show_about = false;
    var show_demo = true;
    var emu_view_menu = true;
    var emu_view_fullscreen = false;
    var selected_rom_buf: [512]u8 = [_]u8{0} ** 512;
    var has_selected_rom = false;
    var analytics = Analytics{};
    var pending_tile = startup_tile_windows;
    const perf_freq = c.SDL_GetPerformanceFrequency();
    var last_counter = c.SDL_GetPerformanceCounter();

    while (running) {
        const now_counter = c.SDL_GetPerformanceCounter();
        const elapsed = now_counter - last_counter;
        last_counter = now_counter;
        if (perf_freq > 0) {
            const frame_ms = @as(f32, @floatFromInt(elapsed)) * 1000.0 / @as(f32, @floatFromInt(perf_freq));
            analyticsPush(&analytics, frame_ms);
        }

        var open_file_browser = false;
        var request_tile = false;
        input.mouse_pressed = 0;
        input.mouse_wheel = 0;
        var event: c.SDL_Event = undefined;
        while (c.SDL_PollEvent(&event)) {
            if (event.type == c.SDL_EVENT_QUIT) running = false;
            if (event.type == c.SDL_EVENT_MOUSE_MOTION) {
                input.mouse_x = @intFromFloat(event.motion.x / 2.0);
                input.mouse_y = @intFromFloat(event.motion.y / 2.0);
            }
            if (event.type == c.SDL_EVENT_MOUSE_BUTTON_DOWN and event.button.button == c.SDL_BUTTON_LEFT) {
                input.mouse_x = @intFromFloat(event.button.x / 2.0);
                input.mouse_y = @intFromFloat(event.button.y / 2.0);
                input.mouse_down = 1;
                input.mouse_pressed = 1;
            }
            if (event.type == c.SDL_EVENT_MOUSE_BUTTON_UP and event.button.button == c.SDL_BUTTON_LEFT) {
                input.mouse_x = @intFromFloat(event.button.x / 2.0);
                input.mouse_y = @intFromFloat(event.button.y / 2.0);
                input.mouse_down = 0;
            }
            if (event.type == c.SDL_EVENT_KEY_DOWN and event.key.repeat == false) {
                if (event.key.scancode == c.SDL_SCANCODE_F1) {
                    emu_view_menu = !emu_view_menu;
                }
                if (event.key.scancode == c.SDL_SCANCODE_F11) {
                    emu_view_fullscreen = !emu_view_fullscreen;
                    c.mdgui_set_window_fullscreen(ctx, render_api_window_title, if (emu_view_fullscreen) 1 else 0);
                }
                if (event.key.scancode == c.SDL_SCANCODE_F2) {
                    request_tile = true;
                }
                if (event.key.scancode == c.SDL_SCANCODE_F3) {
                    const lock_now = c.mdgui_is_windows_locked(ctx) != 0;
                    c.mdgui_set_windows_locked(ctx, if (lock_now) 0 else 1);
                }
            }
            if (event.type == c.SDL_EVENT_MOUSE_WHEEL) {
                const wy = @as(f32, event.wheel.y);
                if (wy > 0) {
                    input.mouse_wheel += 1;
                } else if (wy < 0) {
                    input.mouse_wheel -= 1;
                }
            }
        }
        var mx: f32 = 0;
        var my: f32 = 0;
        _ = c.SDL_GetMouseState(&mx, &my);
        input.mouse_x = @intFromFloat(mx / 2.0);
        input.mouse_y = @intFromFloat(my / 2.0);

        _ = c.SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x4c, 0xff);
        _ = c.SDL_RenderClear(renderer);

        c.mdgui_begin_frame(ctx, &input);
        c.mdgui_begin_main_menu_bar(ctx);
        if (c.mdgui_begin_main_menu(ctx, "FILE") != 0) {
            if (c.mdgui_main_menu_item(ctx, "OPEN ROM") != 0) open_file_browser = true;
            if (c.mdgui_main_menu_item(ctx, "EXIT") != 0) running = false;
            c.mdgui_end_main_menu(ctx);
        }
        if (c.mdgui_begin_main_menu(ctx, "HELP") != 0) {
            if (c.mdgui_main_menu_item(ctx, "ABOUT") != 0) show_about = true;
            if (c.mdgui_main_menu_item(ctx, "DEMO") != 0) {
                const demo_window_open = c.mdgui_is_window_open(ctx, "MDGUI Demo Window") != 0;
                if (!show_demo or !demo_window_open) {
                    show_demo = true;
                    c.mdgui_set_window_open(ctx, "MDGUI Demo Window", 1);
                } else {
                    show_demo = false;
                    c.mdgui_set_window_open(ctx, "MDGUI Demo Window", 0);
                }
            }
            c.mdgui_end_main_menu(ctx);
        }
        if (c.mdgui_begin_main_menu(ctx, "WINDOW") != 0) {
            if (c.mdgui_main_menu_item(ctx, "TILE WINDOWS (F2)") != 0) request_tile = true;
            if (c.mdgui_is_windows_locked(ctx) != 0) {
                if (c.mdgui_main_menu_item(ctx, "[x] Lock Tiled Windows (F3)") != 0) {
                    c.mdgui_set_windows_locked(ctx, 0);
                }
            } else {
                if (c.mdgui_main_menu_item(ctx, "[ ] Lock Tiled Windows (F3)") != 0) {
                    c.mdgui_set_windows_locked(ctx, 1);
                }
            }
            c.mdgui_end_main_menu(ctx);
        }
        c.mdgui_end_main_menu_bar(ctx);

        const DrawKind = enum { main_window, demo, perf_analytics, emu_view, perf_graph };
        var draw_kinds: [5]DrawKind = undefined;
        var draw_z: [5]c_int = undefined;
        var draw_count: usize = 0;

        draw_kinds[draw_count] = .main_window;
        draw_z[draw_count] = c.mdgui_get_window_z(ctx, "MDGUI");
        draw_count += 1;

        if (show_demo) {
            draw_kinds[draw_count] = .demo;
            draw_z[draw_count] = c.mdgui_get_window_z(ctx, "MDGUI Demo Window");
            draw_count += 1;
        }

        draw_kinds[draw_count] = .perf_analytics;
        draw_z[draw_count] = c.mdgui_get_window_z(ctx, "PERF ANALYTICS");
        draw_count += 1;

        draw_kinds[draw_count] = .emu_view;
        draw_z[draw_count] = c.mdgui_get_window_z(ctx, render_api_window_title);
        draw_count += 1;

        if (analytics.show_graph) {
            draw_kinds[draw_count] = .perf_graph;
            draw_z[draw_count] = c.mdgui_get_window_z(ctx, "PERF GRAPH");
            draw_count += 1;
        }

        var i: usize = 0;
        while (i < draw_count) : (i += 1) {
            var j: usize = i + 1;
            while (j < draw_count) : (j += 1) {
                if (draw_z[j] < draw_z[i]) {
                    const tz = draw_z[i];
                    draw_z[i] = draw_z[j];
                    draw_z[j] = tz;
                    const tk = draw_kinds[i];
                    draw_kinds[i] = draw_kinds[j];
                    draw_kinds[j] = tk;
                }
            }
        }

        i = 0;
        while (i < draw_count) : (i += 1) {
            switch (draw_kinds[i]) {
                .main_window => drawMainWindow(ctx, &running, &open_file_browser),
                .demo => drawDemoWindow(ctx, show_demo),
                .perf_analytics => drawAnalyticsWindow(ctx, &analytics),
                .emu_view => drawWindowApiDemo(ctx, renderer, emu_view_menu),
                .perf_graph => drawPerfGraphWindow(ctx, &analytics),
            }
        }

        if (pending_tile or request_tile) {
            tileDemoWindows(ctx, renderer, show_demo, analytics.show_graph);
            pending_tile = false;
        }

        if (open_file_browser) {
            c.mdgui_open_file_browser(ctx);
        }
        if (c.mdgui_show_file_browser(ctx)) |picked| {
            const span = std.mem.span(picked);
            const max_copy = selected_rom_buf.len - 1;
            const n = if (span.len < max_copy) span.len else max_copy;
            @memcpy(selected_rom_buf[0..n], span[0..n]);
            selected_rom_buf[n] = 0;
            has_selected_rom = true;
        }
        if (has_selected_rom) {
            if (c.mdgui_message_box(
                ctx,
                "loaded-rom",
                "ROM SELECTED",
                @ptrCast(&selected_rom_buf[0]),
                c.MDGUI_MSGBOX_ONE_BUTTON,
                "OK",
                null,
            ) != 0) {
                has_selected_rom = false;
            }
        }

        if (show_about) {
            if (c.mdgui_message_box_ex(
                ctx,
                "about",
                "ABOUT",
                "Immediate-mode GUI demo\nby: mitigd",
                c.MDGUI_MSGBOX_ONE_BUTTON,
                "OK",
                null,
                c.MDGUI_TEXT_ALIGN_CENTER,
            ) != 0) show_about = false;
        }

        c.mdgui_end_frame(ctx);

        _ = c.SDL_RenderPresent(renderer);
    }
}
