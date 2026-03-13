#pragma once

#include "../core/windowing_internal.hpp"

bool ci_less(const std::string &a, const std::string &b);
std::vector<std::string> enumerate_file_browser_drives();
bool file_browser_path_exists(const std::string &path);
std::string file_browser_default_open_path(const MDGUI_Context *ctx);
void file_browser_refresh_drives(MDGUI_Context *ctx, bool force_scan);
std::string normalize_extension_filter(const char *ext);
std::string path_extension_lower(const std::filesystem::path &p);
bool file_matches_filters(const MDGUI_Context *ctx,
                          const std::filesystem::path &p);
void file_browser_open_path(MDGUI_Context *ctx, const char *path);
