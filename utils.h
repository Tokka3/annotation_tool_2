#include <windows.h>
#include <shlobj.h>
#include <string>
#include <imgui.h>
#include <imgui_internal.h>
// definitions for time_now (high_reslution_clock)
#include <chrono>
#include <ctime>

#define time_now std::chrono::high_resolution_clock::now

namespace utils {

    std::string open_directory_dialog(const std::string& title);
}
void UpdateRectangle(float& display_x1, float& display_y1, float& display_x2, float& display_y2);
void draw_crosshair();
