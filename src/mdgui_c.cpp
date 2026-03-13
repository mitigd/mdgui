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
#include "../mdgui_impl/core/impl_core.inl"
#include "../mdgui_impl/core/api_context.inl"
#include "../mdgui_impl/core/api_widgets.inl"
#include "../mdgui_impl/core/api_menus_windows.inl"
#include "../mdgui_impl/core/api_overlay_browser_demo.inl"
