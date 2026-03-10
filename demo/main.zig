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

fn alphaByteFromFloat(v: f32) u8 {
    const clamped = clamp01(v);
    return @as(u8, @intFromFloat(clamped * 255.0 + 0.5));
}

fn alphaFloatFromByte(v: u8) f32 {
    return @as(f32, @floatFromInt(v)) / 255.0;
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
    _ = c.mdgui_checkbox(ctx, "Show realtime graph", &toggle_graph);
    analytics.show_graph = toggle_graph;
    if (!was_show_graph and analytics.show_graph) {
        c.mdgui_set_window_open(ctx, "PERF GRAPH", 1);
        c.mdgui_focus_window(ctx, "PERF GRAPH");
    }

    c.mdgui_label(ctx, "FPS + Frame Time");

    var line1: [64]u8 = undefined;
    const txt_fps = std.fmt.bufPrintZ(&line1, "{d:.1} fps (avg {d:.1})", .{ stats.current_fps, stats.avg_fps }) catch "fps";
    c.mdgui_progress_bar(ctx, clamp01(stats.current_fps / analytics.target_fps), -16, 10, txt_fps.ptr);

    var line2: [64]u8 = undefined;
    const txt_ms = std.fmt.bufPrintZ(&line2, "{d:.2} ms", .{stats.current_ms}) catch "ms";
    c.mdgui_progress_bar(ctx, clamp01(stats.current_ms / analytics.graph_max_ms), -16, 10, txt_ms.ptr);

    var line3: [64]u8 = undefined;
    const txt_minmax = std.fmt.bufPrintZ(&line3, "min/max {d:.1}/{d:.1}", .{ stats.min_fps, stats.max_fps }) catch "min/max";
    c.mdgui_label(ctx, txt_minmax.ptr);

    c.mdgui_separator(ctx, 0);
    c.mdgui_label(ctx, "Frame Time Histogram");

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
        c.mdgui_label(ctx, txt.ptr);
        c.mdgui_progress_bar(ctx, @as(f32, @floatFromInt(bins[i])) / @as(f32, @floatFromInt(max_bin)), -16, 8, null);
    }

    c.mdgui_end_window(ctx);
}

fn drawMainWindow(
    ctx: ?*c.MDGUI_Context,
    renderer: ?*c.SDL_Renderer,
    running: *bool,
    open_file_browser: *bool,
    windows_alpha: *f32,
    show_nested_test_area: bool,
    demo_text: *[128]u8,
    demo_text_multiline: *[1024]u8,
) void {
    if (c.mdgui_begin_window(ctx, "MDGUI", 10, 10, 220, 170) != 0) {
        c.mdgui_begin_menu_bar(ctx);
        if (c.mdgui_begin_menu(ctx, "FILE") != 0) {
            if (c.mdgui_begin_submenu(ctx, "OPEN RECENT") != 0) {
                if (c.mdgui_menu_item(ctx, "Sonic.sms") != 0) {
                    open_file_browser.* = true;
                    c.mdgui_show_toast(ctx, "Opening file browser...", 1800);
                }
                if (c.mdgui_menu_item(ctx, "Tetris.gb") != 0) {
                    open_file_browser.* = true;
                    c.mdgui_show_toast(ctx, "Opening file browser...", 1800);
                }
                c.mdgui_end_submenu(ctx);
            }
            c.mdgui_menu_separator(ctx);
            if (c.mdgui_menu_item(ctx, "OPEN ROM") != 0) {
                open_file_browser.* = true;
                c.mdgui_show_toast(ctx, "Opening file browser...", 1800);
            }
            c.mdgui_menu_separator(ctx);
            if (c.mdgui_menu_item(ctx, "EXIT") != 0) running.* = false;
            c.mdgui_end_menu(ctx);
        }
        if (c.mdgui_begin_menu(ctx, "OPTIONS") != 0) {
            _ = c.mdgui_menu_item(ctx, "VIDEO");
            _ = c.mdgui_menu_item(ctx, "AUDIO");
            c.mdgui_end_menu(ctx);
        }
        c.mdgui_end_menu_bar(ctx);
        c.mdgui_label(ctx, "Demo window is draggable.");
        if (c.mdgui_begin_row(ctx, 2) != 0) {
            if (c.mdgui_button(ctx, "LOAD ROM", 90, 20) != 0) open_file_browser.* = true;
            c.mdgui_next_column(ctx);
            _ = c.mdgui_button(ctx, "OPTIONS", 90, 20);
            c.mdgui_end_row(ctx);
        }
        const text_flags = c.mdgui_input_text(ctx, "Quick note", @ptrCast(&demo_text[0]), demo_text.len, -16);
        if ((text_flags & c.MDGUI_INPUT_TEXT_SUBMITTED) != 0) {
            c.mdgui_set_status_bar_text(ctx, @ptrCast(&demo_text[0]));
            c.mdgui_show_toast(ctx, "Quick note submitted", 2000);
        }
        c.mdgui_spacing(ctx, 4);
        _ = c.mdgui_input_text_multiline(
            ctx,
            "Quick note (multiline)",
            @ptrCast(&demo_text_multiline[0]),
            demo_text_multiline.len,
            -16,
            56,
            c.MDGUI_INPUT_TEXT_MULTILINE_NONE,
        );
        c.mdgui_spacing(ctx, 2);

        c.mdgui_label(ctx, "Window transparency");
        var slider_alpha = windows_alpha.*;
        _ = c.mdgui_slider(ctx, null, &slider_alpha, 0.0, 1.0, -16);
        slider_alpha = clamp01(slider_alpha);
        windows_alpha.* = slider_alpha;
        c.mdgui_set_windows_alpha(ctx, alphaByteFromFloat(slider_alpha));

        var alpha_line: [64]u8 = undefined;
        const alpha_pct = @as(c_int, @intFromFloat(slider_alpha * 100.0 + 0.5));
        const alpha_txt = std.fmt.bufPrintZ(&alpha_line, "Window alpha: {d}%", .{alpha_pct}) catch "Window alpha";
        c.mdgui_label(ctx, alpha_txt.ptr);

        if (show_nested_test_area) {
            c.mdgui_label(ctx, "Nested TEST AREA");
            var view_x: c_int = 0;
            var view_y: c_int = 0;
            var view_w: c_int = 0;
            var view_h: c_int = 0;
            if (c.mdgui_begin_render_window(
                ctx,
                "TEST AREA",
                10,
                8,
                -16,
                64,
                0,
                &view_x,
                &view_y,
                &view_w,
                &view_h,
            ) != 0) {
                _ = c.SDL_SetRenderDrawBlendMode(renderer, c.SDL_BLENDMODE_BLEND);
                _ = c.SDL_SetRenderDrawColor(renderer, 0x10, 0x10, 0x10, 0xff);
                var bg = c.SDL_FRect{
                    .x = @floatFromInt(view_x),
                    .y = @floatFromInt(view_y),
                    .w = @floatFromInt(view_w),
                    .h = @floatFromInt(view_h),
                };
                _ = c.SDL_RenderFillRect(renderer, &bg);

                const ticks = c.SDL_GetTicks();
                const t = @as(c_int, @intCast(ticks % 2000));
                const local_x: c_int = -16 + @divTrunc(t * (view_w + 32), 2000);
                const local_y: c_int = 22;
                var sq = c.SDL_FRect{
                    .x = @floatFromInt(view_x + local_x),
                    .y = @floatFromInt(view_y + local_y),
                    .w = 18,
                    .h = 18,
                };
                _ = c.SDL_SetRenderDrawColor(renderer, 0x30, 0xd0, 0x50, 0xff);
                _ = c.SDL_RenderFillRect(renderer, &sq);
                c.mdgui_end_window(ctx);
            }
        }
        c.mdgui_end_window(ctx);
    }
}

fn drawDemoWindow(ctx: ?*c.MDGUI_Context, show_demo: bool) void {
    if (show_demo) {
        c.mdgui_show_demo_window(ctx);
    }
}

fn drawWindowApiDemo(ctx: ?*c.MDGUI_Context, renderer: ?*c.SDL_Renderer, show_window_api_menu: bool) void {
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
        if (show_window_api_menu) 1 else 0,
        &view_x,
        &view_y,
        &view_w,
        &view_h,
    ) != 0) {
        const window_alpha = c.mdgui_get_window_alpha(ctx, render_api_window_title);
        _ = c.SDL_SetRenderDrawBlendMode(renderer, c.SDL_BLENDMODE_BLEND);
        var bg = c.SDL_FRect{
            .x = @floatFromInt(view_x),
            .y = @floatFromInt(view_y),
            .w = @floatFromInt(view_w),
            .h = @floatFromInt(view_h),
        };
        _ = c.SDL_SetRenderDrawColor(renderer, 0x12, 0x12, 0x12, window_alpha);
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
                    _ = c.SDL_SetRenderDrawColor(renderer, light_r, light_g, light_b, window_alpha);
                } else {
                    _ = c.SDL_SetRenderDrawColor(renderer, dark_r, dark_g, dark_b, window_alpha);
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
    var view_x: c_int = 0;
    var view_y: c_int = 0;
    var view_w: c_int = 0;
    var view_h: c_int = 0;
    if (c.mdgui_begin_render_window(
        ctx,
        "PERF GRAPH",
        250,
        195,
        380,
        155,
        0,
        &view_x,
        &view_y,
        &view_w,
        &view_h,
    ) == 0) return;

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

    const window_flags: c.SDL_WindowFlags = c.SDL_WINDOW_RESIZABLE;
    const window = c.SDL_CreateWindow("MDGUI Demo", 1280, 720, window_flags) orelse return error.SDLWindowFailed;
    _ = c.SDL_SetWindowMinimumSize(window, 640, 360);
    _ = c.SDL_StartTextInput(window);
    defer _ = c.SDL_StopTextInput(window);
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
    c.mdgui_set_window_scrollbar_visible(ctx, render_api_window_title, 0);
    c.mdgui_set_window_scrollbar_visible(ctx, "PERF GRAPH", 0);
    c.mdgui_set_windows_locked(ctx, if (startup_lock_tiled_windows) 1 else 0);
    c.mdgui_set_status_bar_visible(ctx, 1);

    var running = true;
    var input = c.MDGUI_Input{
        .mouse_x = 0,
        .mouse_y = 0,
        .mouse_down = 0,
        .mouse_pressed = 0,
        .mouse_wheel = 0,
        .text_input = null,
        .key_backspace = 0,
        .key_delete = 0,
        .key_enter = 0,
        .key_left = 0,
        .key_right = 0,
        .key_up = 0,
        .key_down = 0,
        .key_home = 0,
        .key_end = 0,
    };
    var show_about = false;
    var show_demo = true;
    var show_nested_test_area = false;
    var show_window_api_menu = false;
    var emu_view_fullscreen = false;
    var selected_rom_buf: [512]u8 = [_]u8{0} ** 512;
    var has_selected_rom = false;
    var analytics = Analytics{};
    var windows_alpha = alphaFloatFromByte(c.mdgui_get_windows_alpha(ctx));
    var demo_text: [128]u8 = [_]u8{0} ** 128;
    var demo_text_multiline: [1024]u8 = [_]u8{0} ** 1024;
    c.mdgui_set_tile_manager_enabled(ctx, if (startup_tile_windows) 1 else 0);
    var retile_needed = startup_tile_windows;
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
        var request_open_main_ui = false;
        var request_open_demo_window = false;
        var request_open_perf_analytics = false;
        var request_open_perf_graph = false;
        var request_open_window_api = false;
        var frame_text_input: [64]u8 = [_]u8{0} ** 64;
        var frame_text_len: usize = 0;
        var key_fallback_input: [64]u8 = [_]u8{0} ** 64;
        var key_fallback_len: usize = 0;
        var saw_text_input_event = false;
        input.mouse_pressed = 0;
        input.mouse_wheel = 0;
        input.text_input = null;
        input.key_backspace = 0;
        input.key_delete = 0;
        input.key_enter = 0;
        input.key_left = 0;
        input.key_right = 0;
        input.key_up = 0;
        input.key_down = 0;
        input.key_home = 0;
        input.key_end = 0;
        var event: c.SDL_Event = undefined;
        while (c.SDL_PollEvent(&event)) {
            if (event.type == c.SDL_EVENT_QUIT) running = false;
            if (event.type == c.SDL_EVENT_WINDOW_RESIZED or
                event.type == c.SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED or
                event.type == c.SDL_EVENT_WINDOW_MAXIMIZED or
                event.type == c.SDL_EVENT_WINDOW_RESTORED)
            {
                retile_needed = true;
            }
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
            if (event.type == c.SDL_EVENT_KEY_DOWN) {
                if (event.key.scancode == c.SDL_SCANCODE_BACKSPACE) input.key_backspace = 1;
                if (event.key.scancode == c.SDL_SCANCODE_DELETE) input.key_delete = 1;
                if (event.key.scancode == c.SDL_SCANCODE_RETURN or event.key.scancode == c.SDL_SCANCODE_KP_ENTER) input.key_enter = 1;
                if (event.key.scancode == c.SDL_SCANCODE_LEFT) input.key_left = 1;
                if (event.key.scancode == c.SDL_SCANCODE_RIGHT) input.key_right = 1;
                if (event.key.scancode == c.SDL_SCANCODE_UP) input.key_up = 1;
                if (event.key.scancode == c.SDL_SCANCODE_DOWN) input.key_down = 1;
                if (event.key.scancode == c.SDL_SCANCODE_HOME) input.key_home = 1;
                if (event.key.scancode == c.SDL_SCANCODE_END) input.key_end = 1;
                const keycode = event.key.key;
                if (keycode >= 32 and keycode <= 126 and key_fallback_len < key_fallback_input.len - 1) {
                    key_fallback_input[key_fallback_len] = @as(u8, @intCast(keycode));
                    key_fallback_len += 1;
                    key_fallback_input[key_fallback_len] = 0;
                }
                if (event.key.repeat == false) {
                    if (event.key.scancode == c.SDL_SCANCODE_F1) {
                        show_window_api_menu = !show_window_api_menu;
                    }
                    if (event.key.scancode == c.SDL_SCANCODE_F10) {
                        if (c.SDL_GetWindowFlags(window) & c.SDL_WINDOW_MAXIMIZED != 0) {
                            _ = c.SDL_RestoreWindow(window);
                        } else {
                            _ = c.SDL_MaximizeWindow(window);
                        }
                        retile_needed = true;
                    }
                    if (event.key.scancode == c.SDL_SCANCODE_F11) {
                        emu_view_fullscreen = !emu_view_fullscreen;
                        c.mdgui_set_window_fullscreen(ctx, render_api_window_title, if (emu_view_fullscreen) 1 else 0);
                    }
                    if (event.key.scancode == c.SDL_SCANCODE_F2) {
                        const tiled_now = c.mdgui_is_tile_manager_enabled(ctx) != 0;
                        c.mdgui_set_tile_manager_enabled(ctx, if (tiled_now) 0 else 1);
                    }
                    if (event.key.scancode == c.SDL_SCANCODE_F3) {
                        const lock_now = c.mdgui_is_windows_locked(ctx) != 0;
                        c.mdgui_set_windows_locked(ctx, if (lock_now) 0 else 1);
                    }
                }
            }
            if (event.type == c.SDL_EVENT_TEXT_INPUT) {
                saw_text_input_event = true;
                var i: usize = 0;
                while (event.text.text[i] != 0 and frame_text_len < frame_text_input.len - 1) : (i += 1) {
                    const ch = event.text.text[i];
                    if (ch >= 32 and ch <= 126) {
                        frame_text_input[frame_text_len] = ch;
                        frame_text_len += 1;
                    }
                }
                frame_text_input[frame_text_len] = 0;
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
        if (!saw_text_input_event and key_fallback_len > 0) {
            var i: usize = 0;
            while (i < key_fallback_len and frame_text_len < frame_text_input.len - 1) : (i += 1) {
                frame_text_input[frame_text_len] = key_fallback_input[i];
                frame_text_len += 1;
            }
            frame_text_input[frame_text_len] = 0;
        }
        var mx: f32 = 0;
        var my: f32 = 0;
        _ = c.SDL_GetMouseState(&mx, &my);
        input.mouse_x = @intFromFloat(mx / 2.0);
        input.mouse_y = @intFromFloat(my / 2.0);
        if (frame_text_len > 0) {
            input.text_input = @ptrCast(&frame_text_input[0]);
        }

        // Set background color based on current theme
        const theme = c.mdgui_get_theme();
        var bg_r: u8 = 0x00;
        var bg_g: u8 = 0x00;
        var bg_b: u8 = 0x4c;
        switch (theme) {
            c.MDGUI_THEME_DARK => {
                bg_r = 0x08;
                bg_g = 0x0a;
                bg_b = 0x0f;
            }, // Dark blue-gray
            c.MDGUI_THEME_AMBER => {
                bg_r = 0x1f;
                bg_g = 0x17;
                bg_b = 0x0f;
            }, // Dark brown
            c.MDGUI_THEME_GRAPHITE => {
                bg_r = 0x0d;
                bg_g = 0x11;
                bg_b = 0x18;
            }, // Dark blue-gray
            c.MDGUI_THEME_MIDNIGHT => {
                bg_r = 0x04;
                bg_g = 0x08;
                bg_b = 0x12;
            }, // Very dark blue
            c.MDGUI_THEME_OLIVE => {
                bg_r = 0x0f;
                bg_g = 0x14;
                bg_b = 0x0c;
            }, // Very dark green
            else => {}, // DEFAULT: already set above
        }

        _ = c.SDL_SetRenderDrawColor(renderer, bg_r, bg_g, bg_b, 0xff);
        _ = c.SDL_RenderClear(renderer);

        const status_stats = analyticsStats(&analytics);
        var status_line: [256]u8 = undefined;
        const status_txt = std.fmt.bufPrintZ(
            &status_line,
            "Mouse {d},{d}  Frame {d:.2} ms  Tile:{s}  Lock:{s}",
            .{
                input.mouse_x,
                input.mouse_y,
                status_stats.current_ms,
                if (c.mdgui_is_tile_manager_enabled(ctx) != 0) "on" else "off",
                if (c.mdgui_is_windows_locked(ctx) != 0) "on" else "off",
            },
        ) catch "Ready";
        c.mdgui_set_status_bar_text(ctx, status_txt);

        c.mdgui_begin_frame(ctx, &input);
        c.mdgui_begin_main_menu_bar(ctx);
        if (c.mdgui_begin_main_menu(ctx, "FILE") != 0) {
            if (c.mdgui_begin_main_submenu(ctx, "TOOLS") != 0) {
                if (c.mdgui_main_menu_item(ctx, "OPEN FILE BROWSER") != 0) {
                    open_file_browser = true;
                    c.mdgui_show_toast(ctx, "Opening file browser...", 1800);
                }
                if (c.mdgui_main_menu_item(ctx, "ABOUT") != 0) {
                    show_about = true;
                    c.mdgui_show_toast(ctx, "About dialog opened", 1600);
                }
                c.mdgui_end_main_submenu(ctx);
            }
            c.mdgui_main_menu_separator(ctx);
            if (c.mdgui_main_menu_item(ctx, "OPEN ROM") != 0) {
                open_file_browser = true;
                c.mdgui_show_toast(ctx, "Opening file browser...", 1800);
            }
            c.mdgui_main_menu_separator(ctx);
            if (c.mdgui_main_menu_item(ctx, "EXIT") != 0) running = false;
            c.mdgui_end_main_menu(ctx);
        }
        if (c.mdgui_begin_main_menu(ctx, "HELP") != 0) {
            if (c.mdgui_main_menu_item(ctx, "ABOUT") != 0) show_about = true;
            if (c.mdgui_main_menu_item(ctx, "DEMO") != 0) {
                const demo_window_open = c.mdgui_is_window_open(ctx, "MDGUI Demo Window") != 0;
                if (!demo_window_open) {
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
            if (c.mdgui_begin_main_submenu(ctx, "OPEN WINDOW") != 0) {
                if (c.mdgui_main_menu_item(ctx, "Main UI") != 0) {
                    const is_open = c.mdgui_is_window_open(ctx, "MDGUI") != 0;
                    if (is_open) {
                        c.mdgui_set_window_open(ctx, "MDGUI", 0);
                    } else {
                        request_open_main_ui = true;
                    }
                }
                if (c.mdgui_main_menu_item(ctx, "Demo Window") != 0) {
                    const is_open = c.mdgui_is_window_open(ctx, "MDGUI Demo Window") != 0;
                    if (is_open) {
                        show_demo = false;
                        c.mdgui_set_window_open(ctx, "MDGUI Demo Window", 0);
                    } else {
                        show_demo = true;
                        request_open_demo_window = true;
                    }
                }
                if (c.mdgui_main_menu_item(ctx, "Perf Analytics") != 0) {
                    const is_open = c.mdgui_is_window_open(ctx, "PERF ANALYTICS") != 0;
                    if (is_open) {
                        analytics.show_window = false;
                        c.mdgui_set_window_open(ctx, "PERF ANALYTICS", 0);
                    } else {
                        analytics.show_window = true;
                        request_open_perf_analytics = true;
                    }
                }
                if (c.mdgui_main_menu_item(ctx, "Perf Graph") != 0) {
                    const is_open = c.mdgui_is_window_open(ctx, "PERF GRAPH") != 0;
                    if (is_open) {
                        analytics.show_graph = false;
                        c.mdgui_set_window_open(ctx, "PERF GRAPH", 0);
                    } else {
                        analytics.show_graph = true;
                        request_open_perf_graph = true;
                    }
                }
                if (c.mdgui_main_menu_item(ctx, "Window API") != 0) {
                    const is_open = c.mdgui_is_window_open(ctx, render_api_window_title) != 0;
                    if (is_open) {
                        c.mdgui_set_window_open(ctx, render_api_window_title, 0);
                    } else {
                        request_open_window_api = true;
                    }
                }
                c.mdgui_end_main_submenu(ctx);
            }
            if (c.mdgui_is_tile_manager_enabled(ctx) != 0) {
                if (c.mdgui_main_menu_item(ctx, "[x] Tile Manager (F2)") != 0) {
                    c.mdgui_set_tile_manager_enabled(ctx, 0);
                }
            } else {
                if (c.mdgui_main_menu_item(ctx, "[ ] Tile Manager (F2)") != 0) {
                    c.mdgui_set_tile_manager_enabled(ctx, 1);
                }
            }
            if (c.mdgui_is_windows_locked(ctx) != 0) {
                if (c.mdgui_main_menu_item(ctx, "[x] Lock Tiled Windows (F3)") != 0) {
                    c.mdgui_set_windows_locked(ctx, 0);
                }
            } else {
                if (c.mdgui_main_menu_item(ctx, "[ ] Lock Tiled Windows (F3)") != 0) {
                    c.mdgui_set_windows_locked(ctx, 1);
                }
            }
            if (show_nested_test_area) {
                if (c.mdgui_main_menu_item(ctx, "[x] Nested Test Area") != 0) {
                    show_nested_test_area = false;
                }
            } else {
                if (c.mdgui_main_menu_item(ctx, "[ ] Nested Test Area") != 0) {
                    show_nested_test_area = true;
                }
            }
            c.mdgui_end_main_menu(ctx);
        }
        c.mdgui_end_main_menu_bar(ctx);

        if (retile_needed and c.mdgui_is_tile_manager_enabled(ctx) != 0) {
            c.mdgui_tile_windows(ctx);

            var min_w: c_int = 0;
            var min_h: c_int = 0;
            c.mdgui_get_tiling_min_size(ctx, &min_w, &min_h);

            // Convert logical min size to physical pixels for SDL
            const phys_min_w: c_int = @intFromFloat(@as(f32, @floatFromInt(min_w)) * 2.0);
            const phys_min_h: c_int = @intFromFloat(@as(f32, @floatFromInt(min_h)) * 2.0);
            _ = c.SDL_SetWindowMinimumSize(window, phys_min_w, phys_min_h);

            retile_needed = false;
        }
        c.mdgui_set_windows_alpha(ctx, alphaByteFromFloat(windows_alpha));

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
                .main_window => drawMainWindow(
                    ctx,
                    renderer,
                    &running,
                    &open_file_browser,
                    &windows_alpha,
                    show_nested_test_area,
                    &demo_text,
                    &demo_text_multiline,
                ),
                .demo => drawDemoWindow(ctx, show_demo),
                .perf_analytics => drawAnalyticsWindow(ctx, &analytics),
                .emu_view => drawWindowApiDemo(ctx, renderer, show_window_api_menu),
                .perf_graph => drawPerfGraphWindow(ctx, &analytics),
            }
        }

        if (request_open_main_ui) {
            c.mdgui_set_window_open(ctx, "MDGUI", 1);
            c.mdgui_focus_window(ctx, "MDGUI");
        }
        if (request_open_demo_window) {
            c.mdgui_set_window_open(ctx, "MDGUI Demo Window", 1);
            c.mdgui_focus_window(ctx, "MDGUI Demo Window");
        }
        if (request_open_perf_analytics) {
            c.mdgui_set_window_open(ctx, "PERF ANALYTICS", 1);
            c.mdgui_focus_window(ctx, "PERF ANALYTICS");
        }
        if (request_open_perf_graph) {
            c.mdgui_set_window_open(ctx, "PERF GRAPH", 1);
            c.mdgui_focus_window(ctx, "PERF GRAPH");
        }
        if (request_open_window_api) {
            c.mdgui_set_window_open(ctx, render_api_window_title, 1);
            c.mdgui_focus_window(ctx, render_api_window_title);
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
