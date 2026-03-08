# mgui

Immediate-mode GUI library (C/C++) with an SDL3 Zig demo application.

![mgui header](assets/readme-header.png)

## API at a Glance

```c
MGUI_Input in = { .mouse_x = mx, .mouse_y = my, .mouse_down = down, .mouse_pressed = pressed, .mouse_wheel = wheel };
mgui_begin_frame(ctx, &in);

if (mgui_begin_window(ctx, "Debug Panel", 20, 20, 220, 140)) {
  static float volume = 0.6f;
  mgui_label(ctx, "Audio", 8, 6);
  mgui_slider(ctx, "Volume", &volume, 0.0f, 1.0f, 8, 6, -16);
  if (mgui_button(ctx, "Apply", 8, 10, 60, 12)) {
    // handle click
  }
  mgui_end_window(ctx);
}

mgui_end_frame(ctx);
```

## Repository Layout

- `include/`: Public headers (`mgui_c.h`, primitives/font headers)
- `src/`: C/C++ implementation (`mgui_c.cpp`, `mgui_glue.cpp`)
- `demo/`: Zig demo app entrypoint (`main.zig`)
- `scripts/release.sh`: Optional helper scripts for POSIX release packaging
- `scripts/release.ps1`: Optional PowerShell helper for Windows release packaging

## Build

Prerequisites:
- Zig `0.15.2` or newer
- C/C++ toolchain supported by your Zig target

Commands:

```sh
zig build
zig build run
```

## Release Packaging (Cross-Platform)

POSIX (Linux/macOS):

```sh
./scripts/release.sh 0.1.0
```

Windows PowerShell:

```powershell
./scripts/release.ps1 release 0.1.0
```

Artifacts:
- Linux/macOS: `mgui-<version>-<os>-<arch>.tar.gz`
- Windows: `mgui-<version>-win64.zip`

## Notes

- Window identity is keyed by window title (`mgui_begin_window` title string).
- For best behavior, keep titles stable across frames.

## Render Backend Compatibility

`mgui` now uses a pluggable render backend API instead of hard-wiring widget draw calls to SDL renderer APIs.

- Backend-agnostic core primitives/font code: `src/mgui_glue.cpp`
- SDL compatibility backend implementation: `src/mgui_backend_sdl.cpp`
- OpenGL compatibility backend adapter: `src/mgui_backend_opengl.cpp`
- Vulkan compatibility backend adapter: `src/mgui_backend_vulkan.cpp`
- UI/window logic: `src/mgui_c.cpp`
- Backend helper declarations: `include/mgui_backends.h`

```c
MGUI_Context *ctx = mgui_create(sdl_renderer);
```

For non-SDL renderers (Vulkan/OpenGL/etc), provide a `MGUI_RenderBackend` with callback functions and create with:

```c
#include "mgui_backends.h"

MGUI_BackendCallbacks callbacks = { ... };
MGUI_RenderBackend backend = { ... };

// Or use per-backend adapter helpers:
mgui_make_vulkan_backend(&backend, &callbacks);
// mgui_make_opengl_backend(&backend, &callbacks);

MGUI_Context *ctx = mgui_create_with_backend(&backend);
```

## Credits

**Heavily** inspired by the old Nes Emulator "NESticle" by: Icer Addis
