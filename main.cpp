// Dear ImGui: standalone example application for DirectX 9

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <iostream>
#include <tchar.h>
#include "utils.h"
#include <filesystem>
#include <vector>

// Data
static LPDIRECT3D9              g_pD3D = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#include <chrono>
#include "inference/inference.h"

// Macro to get the current time
#define GET_CURRENT_TIME() std::chrono::high_resolution_clock::now()

// Macro to calculate the elapsed time in milliseconds
#define TIME_ELAPSED(start_time) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count()
enum type {
    ct_body = 0,
    ct_head = 1,
    t_body = 2,
    t_head = 3,

};

enum team {
    ct = 0,
    t = 1,
};

struct normalised_annotation {
    float center_x;
    float center_y;
    float w;
    float h;
    type type;
};


struct annotation {
    float x;
    float y;
    float w;
    float h;
    bool is_selected;
    type type;
    bool is_resizing;
    bool is_hovering;
     int resize_corner = -1;
     ImVec2 resize_start_pos;
     ImVec2 resize_start_size;
};


struct image {
    std::string file_name;
    IDirect3DTexture9* texture;
    std::vector<annotation> annotations;
    
};




std::vector<image> images;
#include <cstdio>
static float zoom = 1;
ImVec2 get_scroll_and_zoom_offset(ImVec2 image_pos) {
    ImVec2 offset;

    return offset;
}
bool deleteFile(const std::string& filepath) {
    // The remove function returns 0 on success, non-zero otherwise
    if (std::remove(filepath.c_str()) == 0) {
        return true;
    }
    else {
        return false;
    }
}

normalised_annotation convert_annotation(annotation ann) {
    normalised_annotation normalised = { 0 };
    normalised.w = (ann.w) / 1600;
    normalised.h = (ann.h) / 900;
    normalised.center_x = (ann.x + (ann.w / 2)) / 1600;
    normalised.center_y = (ann.y + (ann.h / 2)) / 900;
    normalised.type = ann.type;
    return normalised;
}
annotation convert_normalised_annotation(normalised_annotation normalised, int width, int height) {
    annotation ann = { 0 };

    ann.w = normalised.w * width;
    ann.h = normalised.h * height;
    ann.x = (normalised.center_x * width) - (ann.w / 2);
    ann.y = (normalised.center_y * height) - (ann.h / 2);
    ann.type = normalised.type;

    return ann;
}
#include <fstream>
void save_annotations(const std::string& annotation_path, const std::vector<image> images) {


    for (auto image : images) {
        image.file_name = image.file_name.substr(0, image.file_name.length() - 4);
        std::ofstream file(image.file_name + ".txt");
        if (!file.is_open()) {
            std::cout << "failed to open file for " << image.file_name + ".txt" << " " << GetLastError() << std::endl;
        }
        for (annotation anno : image.annotations) {
            if (anno.x == -10000) {
                continue;
            }
            normalised_annotation normalised = convert_annotation(anno);
            file << normalised.type << " " << normalised.center_x << " " << normalised.center_y << " " << abs(normalised.w) << " " << abs(normalised.h) << "\n";
        }
        file.close();

    }


}
cv::Mat ConvertTextureToMat(IDirect3DTexture9* texture) {
    IDirect3DSurface9* pSurface = NULL;
    HRESULT hr = texture->GetSurfaceLevel(0, &pSurface);
    if (FAILED(hr)) {
        // Handle error
    }

    D3DLOCKED_RECT lockedRect;
    hr = pSurface->LockRect(&lockedRect, NULL, D3DLOCK_READONLY);
    if (FAILED(hr)) {
        // Handle error
    }

    D3DSURFACE_DESC desc;
    pSurface->GetDesc(&desc);
    cv::Mat img(desc.Height, desc.Width, CV_8UC4, lockedRect.pBits, lockedRect.Pitch);

    pSurface->UnlockRect();
    pSurface->Release();

    cv::Mat imgBGR;
    cv::cvtColor(img, imgBGR, cv::COLOR_BGRA2BGR);
    return imgBGR;
}
void load_annotations(const std::string annotation_path, std::vector<image>& images) {

    for (const auto& entry : std::filesystem::directory_iterator(annotation_path)) {
        if (entry.is_regular_file()) {
            auto path = entry.path();
            // Check for specific image file extensions
            image img;


            if (path.extension() == ".png") {

                std::wstring txt_path_w = path.native();

                std::string txt_path(txt_path_w.begin(), txt_path_w.end());
                txt_path = txt_path.substr(0, txt_path.length() - 4);
                txt_path += ".txt";
                std::ifstream file(txt_path);
                std::string line;
                while (std::getline(file, line)) {
                    std::istringstream iss(line);
                    annotation ann;
                    int type_val;
                    if (iss >> type_val >> ann.x >> ann.y >> ann.w >> ann.h) {
                        ann.type = static_cast<type>(type_val);
                        ann.is_selected = false;
                        normalised_annotation normalised;
                        normalised.type = ann.type;
                        normalised.center_x = ann.x;
                        normalised.center_y = ann.y;
                        normalised.w = ann.w;
                        normalised.h = ann.h;
                        ann = convert_normalised_annotation(normalised, 1600, 900);
                        // Calculate the center coordinates and store them in the struct


                        img.annotations.push_back(ann);
                    }
                }

                std::wstring image_path_w = path.native();

                std::string image_path(image_path_w.begin(), image_path_w.end());
                image_path = image_path.substr(0, image_path.length() - 4);
                image_path += ".png";
                // Prepare the image structure

                img.file_name = path.string();

               
                images.push_back(img);
            }


        }
    }
}
void LoadImagesToVector(const std::string& directory_path, std::vector<image>& images, LPDIRECT3DDEVICE9 pDevice) {
    // Iterate over all files in the directory
    for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
        if (entry.is_regular_file()) {
            auto path = entry.path();
            // Check for specific image file extensions
            if (path.extension() == ".jpg" || path.extension() == ".png" ||
                path.extension() == ".jpeg" || path.extension() == ".bmp") {

                // Prepare the image structure
                image img;
                img.file_name = path.string();

                // Create a texture from the file
                if (D3DXCreateTextureFromFile(pDevice, path.c_str(), &img.texture) != D3D_OK) {
                    std::cerr << "Failed to load texture from file: " << path << std::endl;
                    img.texture = nullptr; // Handle failed texture creation
                }

                // Add the image struct to the vector
                images.push_back(img);
            }
        }
    }
}
int get_scroll_direction() {
    return (int)ImGui::GetIO().MouseWheel;
}
enum scroll_direction : int { none, up = 1, down = -1 };
// Input handling stuff
bool key_state[256];
bool prev_key_state[256];

bool cursor_hovering_box(POINT cp, float x, float y, float w, float h) {
    if (cp.x > x - 5 && cp.y < x + w + 5 && cp.y > y - 5 && cp.y < y + h + 5) {

        if (cp.x > x + 5 && cp.x < x + w - 5 && cp.y > y + 5 && cp.y < y + h - 5) {
            return false;
        }
        return true;
    }
    else {
        return false;
    }

}
bool key_pressed(const int key) {
    if (GetForegroundWindow() != FindWindowA(0, "Annotator")) {
        return false;
    }
    return key_state[key] && !prev_key_state[key];
}

// ========================================================================
bool key_down(const int key) {
    if (GetForegroundWindow() != FindWindowA(0, "Annotator")) {
        return false;
    }
    return key_state[key];
}
std::string image_directory;
bool images_saved;
void save() {
    images_saved = false;
    auto time = GET_CURRENT_TIME();
    while (true) {
        if (TIME_ELAPSED(time) > 30000) {
            save_annotations(image_directory, images);
            time = GET_CURRENT_TIME();
            images_saved = true;
        }
        Sleep(100);
    }
}
cv::Mat screenshot(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0)
        return cv::Mat();

    HDC h_screen = GetDC(NULL);
    if (!h_screen)
        return cv::Mat();

    HDC h_dc = CreateCompatibleDC(h_screen);
    if (!h_dc)
        return cv::Mat();

    HBITMAP h_bitmap = CreateCompatibleBitmap(h_screen, w, h);
    if (!h_bitmap)
        return cv::Mat();

    HGDIOBJ old_obj = SelectObject(h_dc, h_bitmap);
    BOOL b_ret = BitBlt(h_dc, 0, 0, w, h, h_screen, x, y, SRCCOPY);
    if (!b_ret)
        return cv::Mat();

    // initialize bitmap info structure
    BITMAPINFO info = { 0 };
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = w;
    info.bmiHeader.biHeight = -h;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    info.bmiHeader.biSizeImage = 0;
    info.bmiHeader.biXPelsPerMeter = 0;
    info.bmiHeader.biYPelsPerMeter = 0;
    info.bmiHeader.biClrUsed = 0;
    info.bmiHeader.biClrImportant = 0;

    cv::Mat mat;
    mat.create(h, w, CV_8UC4);

    // populate mat with screenshot data
    if (!GetDIBits(h_dc, h_bitmap, 0, h, (LPVOID)mat.data, &info, DIB_RGB_COLORS))
        return cv::Mat();

    // clean up
    DeleteObject(h_bitmap);
    DeleteDC(h_dc);
    ReleaseDC(NULL, h_screen);

    return mat;
}
void run_annotation_inference(struct inference* inf, struct image& img) {
    cv::Mat img_mat = cv::imread(img.file_name, cv::IMREAD_COLOR);
    std::vector<inference_result> inf_results;

    cv::cvtColor(img_mat, img_mat, cv::COLOR_BGRA2BGR);
    inf->run(img_mat, inf_results);
    cv::resize(img_mat, img_mat, cv::Size(640, 640));


    // print image depth
    printf("depth: %d\n", img_mat.depth());

    // print image channels
    printf("channels: %d\n", img_mat.channels());

    // print image size
    printf("size: %d x %d\n", img_mat.cols, img_mat.rows);

    // print image type
    printf("type: %d\n", img_mat.type());
    printf("result amount %d \n", inf_results.size());
    for (inference_result inf : inf_results) {
       
        struct annotation new_anno;
       
        new_anno.w = inf.bounding_box.width * 1600 / 640;
        new_anno.h = inf.bounding_box.height * 900 / 640;
        new_anno.x = inf.bounding_box.x * 1600 / 640;
        new_anno.y = inf.bounding_box.y * 900 / 640;
        new_anno.type = (type)inf.class_id;
        printf("image annotated %lf %lf %lf %lf \n", new_anno.x, new_anno.y, new_anno.w, new_anno.h);
        img.annotations.push_back(new_anno);
    }



}

void run_annotation_inference_on_non_annotated(struct inference* inf,  std::vector<image>& imgs) {

    for (image& img : imgs) {
        if (img.annotations.size() != 0) {
            // skip images that already have annotations
            continue;
        }
        cv::Mat img_mat = cv::imread(img.file_name, cv::IMREAD_COLOR);
        std::vector<inference_result> inf_results;

        cv::cvtColor(img_mat, img_mat, cv::COLOR_BGRA2BGR);
        inf->run(img_mat, inf_results);
        cv::resize(img_mat, img_mat, cv::Size(640, 640));


        // print image depth
        printf("depth: %d\n", img_mat.depth());

        // print image channels
        printf("channels: %d\n", img_mat.channels());

        // print image size
        printf("size: %d x %d\n", img_mat.cols, img_mat.rows);

        // print image type
        printf("type: %d\n", img_mat.type());
        printf("result amount %d \n", inf_results.size());
        for (inference_result inf : inf_results) {

            struct annotation new_anno;

            new_anno.w = inf.bounding_box.width * 1600 / 640;
            new_anno.h = inf.bounding_box.height * 900 / 640;
            new_anno.x = inf.bounding_box.x * 1600 / 640;
            new_anno.y = inf.bounding_box.y * 900 / 640;
            new_anno.type = (type)inf.class_id;
            printf("image annotated %lf %lf %lf %lf \n", new_anno.x, new_anno.y, new_anno.w, new_anno.h);
            img.annotations.push_back(new_anno);
        }
       
    }


}
// Main code
int main(int, char**)
{

    inference* inf = new inference();

    inf->create_session();
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Annotator", WS_OVERLAPPEDWINDOW, 0, 0, 1920, 1080, nullptr, nullptr, wc.hInstance, nullptr);

    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)save, 0, 0, 0);



   // inf->create_session();

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;

    image_directory = utils::open_directory_dialog("Hi There");
    //LoadImagesToVector(image_directory, images, g_pd3dDevice);
    load_annotations(image_directory, images);
    ImGui::GetStyle().ScaleAllSizes(1.0f);

    ImGui::SetNextWindowSize(ImVec2(1900, 1000));
    int image_idx = 0;

    // Add default font in 16.0 pixels
    ImFont* font_default_16 = io.Fonts->AddFontDefault();
    // Add default font in 24.0 pixels
    ImFontConfig config;
    config.SizePixels = 12;
    ImFont* font_default_24 = io.Fonts->AddFontDefault(&config);
    ImFontConfig configo;
    configo.SizePixels = 24;
    ImFont* good_font = io.Fonts->AddFontDefault(&configo);

    config.SizePixels = 25;
    ImFont* font_default_25 = io.Fonts->AddFontDefault(&config);
    char* buffer = (char*)malloc(25);
    char* buffer_timer = (char*)malloc(25);
    auto time_since_images_saved = GET_CURRENT_TIME();
    int temp;

    // Create a texture from the file
    if (D3DXCreateTextureFromFileA(g_pd3dDevice, images[image_idx].file_name.c_str(), &images[image_idx].texture) != D3D_OK) {
        std::cerr << "Failed to load texture from file: " << images[image_idx].file_name.c_str() << std::endl;
        images[image_idx].texture = nullptr; // Handle failed texture creation
    }
    while (!done)
    {


        if (images_saved) {
            time_since_images_saved = GET_CURRENT_TIME();
            images_saved = false;
        }
        POINT cp;
        GetCursorPos(&cp);
        ScreenToClient(hwnd, &cp);
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
        for (int i = 0; i < 256; i++) {
            prev_key_state[i] = key_state[i];
            key_state[i] = GetAsyncKeyState(i);
        }
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;

        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            g_d3dpp.BackBufferWidth = g_ResizeWidth;
            g_d3dpp.BackBufferHeight = g_ResizeHeight;
            g_ResizeWidth = g_ResizeHeight = 0;
            ResetDevice();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.

        {
            static float f = 0.0f;
            static int counter = 0;
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(1900, 1000));
            ImGui::Begin("Hello, world!", reinterpret_cast<bool*>(true), ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

            // Push item flag to disable focus
            ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
         
            if (get_scroll_direction() == scroll_direction::up) {
                if (GetAsyncKeyState(VK_LCONTROL) & 0x8001) {
                    zoom *= 1.1;
                }
            }
            else if (get_scroll_direction() == scroll_direction::down) {
                if (GetAsyncKeyState(VK_LCONTROL) & 0x8001) {
                    zoom /= 1.1;
                }
            }

            ImVec2 image_size = ImVec2(1600 * zoom, 900 * zoom);
       
      
            ImVec2 window_size = ImGui::GetContentRegionAvail();
            ImVec2 image_pos = ImVec2((window_size.x - image_size.x) * 0.5f, (window_size.y - image_size.y) * 0.5f);

            ImGui::SetCursorPos(image_pos);

            ImGui::Image(images[image_idx].texture, image_size);
            static int annotation_type = ct_body;
            static int current_selected_team = team::t;

            ImGui::FocusItem();
            ImGui::Button("Focus");
            static annotation parent;
            //if (key_pressed(VK_UP)) {
            //    current_selected_team = team::t;
            //    annotation_type = t_body;
            //}
            //else if (key_pressed(VK_DOWN)) {
            //    current_selected_team = team::ct;
            //    annotation_type = ct_body;
            //}
            if (key_pressed(VK_RCONTROL)) {
                switch (current_selected_team) {
                //case team::t: {
                //    if (annotation_type == t_body) {
                //        annotation_type = t_head;
                //    }
                //    else {
                //        annotation_type = t_body;
                //    }
                //    break;
                //}
                //case team::ct: {
                //    if (annotation_type == ct_body) {
                //        annotation_type = ct_head;
                //    }
                //    else {
                //        annotation_type = ct_body;
                //    }
                //    break;
                //}

                }
            }
            const char* items[] = { "body", "head"};
            static int class_idx = 0;
            static int real_annotation_type = 0;
            ImGui::PushItemWidth(150);

            ImGui::SetCursorPos(ImVec2(800 + ImGui::GetScrollX(), 0 + ImGui::GetScrollY()));
            ImGui::PushFont(good_font);
         
            if (ImGui::Combo("Type", (&annotation_type), items, IM_ARRAYSIZE(items))) {
                 real_annotation_type = annotation_type;
            }
            
            ImGui::SetCursorPos(ImVec2(1200 + ImGui::GetScrollX(), 0 + ImGui::GetScrollY()));
            if (ImGui::Button("Save")) {
                save_annotations(image_directory, images);
                MessageBox(NULL, L"Images saved", L"Thanks ellis.", 0);
            }
            ImGui::SetCursorPos(ImVec2(200 + ImGui::GetScrollX(), 0 + ImGui::GetScrollY()));
            ImGui::PushItemWidth(300);
             temp = image_idx;
            ImGui::SliderInt("Index", &image_idx, 0, images.size() - 1);
            ImGui::SetCursorPos(ImVec2(1600 + ImGui::GetScrollX(), 0 + ImGui::GetScrollY()));
            if (ImGui::Button("Annotate")) {
                run_annotation_inference(inf, images[image_idx]);
            }
            ImGui::SetCursorPos(ImVec2(1600 + ImGui::GetScrollX(), 0 + ImGui::GetScrollY() + 40));
            if (ImGui::Button("Annotate All")) {
                run_annotation_inference_on_non_annotated(inf, images);
            }
            ImGui::PopFont();
            ImGui::SameLine();

            ImDrawList* draw = ImGui::GetForegroundDrawList();

            sprintf(buffer, "%d/%d", image_idx, images.size() - 1);
            sprintf(buffer_timer, "Saved %d seconds ago", TIME_ELAPSED(time_since_images_saved) / 1000);
            ImGui::PushFont(font_default_25);
            draw->AddText(ImVec2(20, 20), IM_COL32(255, 0, 0, 255), buffer);
            draw->AddText(ImVec2(20, 40), IM_COL32(255, 0, 0, 255), buffer_timer);
            ImGui::PopFont();
            static bool is_drawing = false;
            static annotation new_annotation = { 0, 0, 0, 0, 0 };
            static bool is_annotating = false;
            int is_actually_drawing = false;

            // ... (rest of the code remains the same)
            static bool drawing_begun = false;
            if (key_down(VK_RBUTTON)) {
                is_annotating = true;

                drawing_begun = true;
                if (!is_drawing) {
                    new_annotation.x = (cp.x - image_pos.x + ImGui::GetScrollX()) / zoom;
                    new_annotation.y = (cp.y - image_pos.y + ImGui::GetScrollY()) / zoom;
                    is_drawing = true;
                }
                new_annotation.w = (cp.x - image_pos.x + ImGui::GetScrollX()) / zoom - new_annotation.x;
                new_annotation.h = (cp.y - image_pos.y + ImGui::GetScrollY()) / zoom - new_annotation.y;

                if (new_annotation.w - new_annotation.x > 10 && new_annotation.y - new_annotation.h > 10) {
                    // they've actually drawn a somewhat decent sized square so they probably are drawing
                    is_actually_drawing = true;
                }
            }
            else {
                if (is_drawing) {
                    is_drawing = false;
                }
            }



            if (key_pressed(VK_RETURN)) {
                drawing_begun = false;
                    is_annotating = false;
                    new_annotation.type = (type)annotation_type;

                    images[image_idx].annotations.push_back(new_annotation);

               
            }


          

            if (!is_actually_drawing) {
                int is_hovering_over_body = false;
                for (auto& annotation : images[image_idx].annotations) {
                    if (annotation.x == -1) continue;
                    float display_x1 = annotation.x * zoom + image_pos.x - ImGui::GetScrollX();
                    float display_y1 = annotation.y * zoom + image_pos.y - ImGui::GetScrollY();
                    float display_x2 = (annotation.x + annotation.w) * zoom + image_pos.x - ImGui::GetScrollX();
                    float display_y2 = (annotation.y + annotation.h) * zoom + image_pos.y - ImGui::GetScrollY();
                  

                    if (display_x2 < display_x1) {
                        float temp = display_x1;
                        display_x1 = display_x2;
                        display_x2 = temp;
                    }
                    if (display_y2 < display_y1) {
                        float temp = display_y1;
                        display_y1 = display_y2;
                        display_y2 = temp;
                    }
                    //if (key_pressed(VK_RBUTTON) && annotation.is_selected) {
                    //    annotation.is_selected = false;
                    //}
                    ImVec2 imgui_cp = ImGui::GetMousePos();
                    ImGui::PushFont(font_default_24);
                    if (annotation.type == ct_body) {
                        // draw->AddRectFilled(ImVec2(display_x1 - 1, display_y1 - 13 - 1), ImVec2(display_x1 + ImGui::CalcTextSize("BODY").x + 1, display_y1 - 13 + 1 + ImGui::CalcTextSize("BODY").y ), IM_COL32_BLACK, 2);
                        draw->AddText(ImVec2(display_x1 + 1, display_y1 - 13 + 1), IM_COL32(0, 0, 0, 255), "BODY");
                        draw->AddText(ImVec2(display_x1, display_y1 - 13), IM_COL32(255, 255, 255, 255), "BODY");

                    }
                    else if (annotation.type == ct_head) {
                        // draw->AddRectFilled(ImVec2(display_x1 - 1, display_y1 - 13 - 1), ImVec2(display_x1 + 1 + ImGui::CalcTextSize("HEAD").x, display_y1 - 13 + ImGui::CalcTextSize("HEAD").y + 1), IM_COL32_BLACK, 2);
                        draw->AddText(ImVec2(display_x2 + 1, display_y1 - 13 + 1), IM_COL32(0, 0, 0, 255), "HEAD");
                        draw->AddText(ImVec2(display_x2, display_y1 - 13), IM_COL32(255, 255, 255, 255), "HEAD");

                    }
         
                    ImGui::PopFont();
                    draw->AddRect(ImVec2(display_x1, display_y1), ImVec2(display_x2, display_y2), IM_COL32(255, 255, 255, 100));

         

                    if (!annotation.is_resizing) {
                        annotation.resize_corner = -1;
                        int resize_padding = 7;
                        if (ImGui::IsMouseHoveringRect(ImVec2(display_x1 - resize_padding, display_y1 - resize_padding), ImVec2(display_x1 + resize_padding, display_y1 + resize_padding))
                            || annotation.resize_corner == 0) {
                            annotation.resize_corner = 0;
                            draw->AddCircle(ImVec2(display_x1, display_y1), 3, IM_COL32(255, 0, 0, 255), 10);
                        }
                        else if (ImGui::IsMouseHoveringRect(ImVec2(display_x2 - resize_padding, display_y1 - resize_padding), ImVec2(display_x2 + resize_padding, display_y1 + resize_padding))
                            || annotation.resize_corner == 1) {
                            annotation.resize_corner = 1;
                            draw->AddCircle(ImVec2(display_x2, display_y1), 3, IM_COL32(255, 0, 0, 255), 10);
                        }
                        else if (ImGui::IsMouseHoveringRect(ImVec2(display_x1 - resize_padding, display_y2 - resize_padding), ImVec2(display_x1 + resize_padding, display_y2 + resize_padding))
                            || annotation.resize_corner == 2) {
                            annotation.resize_corner = 2;
                            draw->AddCircle(ImVec2(display_x1, display_y2), 3, IM_COL32(255, 0, 0, 255), 10);
                        }
                        else if (ImGui::IsMouseHoveringRect(ImVec2(display_x2 - resize_padding, display_y2 - resize_padding), ImVec2(display_x2 + resize_padding, display_y2 + resize_padding))
                            || annotation.resize_corner == 3) {
                            annotation.resize_corner = 3;
                            draw->AddCircle(ImVec2(display_x2, display_y2), 3, IM_COL32(255, 0, 0, 255), 10);
                        }
                    }

                    if (annotation.resize_corner != -1 && ImGui::IsMouseClicked(0)) {
                        annotation.is_resizing = true;
                        annotation.resize_start_pos = ImGui::GetMousePos();
                        annotation.resize_start_size = ImVec2(display_x2 - display_x1, display_y2 - display_y1);
                    }
                    if (annotation.is_resizing) {
                        new_annotation.x = -10000;
                        if (key_down(VK_LBUTTON)) {
                            ImVec2 mouse_pos = ImGui::GetMousePos();
                            ImVec2 mouse_delta = mouse_pos - annotation.resize_start_pos;
                            ImVec2 new_size = annotation.resize_start_size;
                            float new_x1 = display_x1;
                            float new_y1 = display_y1;
                            float new_x2 = display_x2;
                            float new_y2 = display_y2;

                            switch (annotation.resize_corner) {
                            case 0:
                                new_x1 += mouse_delta.x;
                                new_y1 += mouse_delta.y;
                               
                                break;
                            case 1:
                                new_x2 += mouse_delta.x;
                                new_y1 += mouse_delta.y;
                                break;
                            case 2:
                                new_x1 += mouse_delta.x;
                                new_y2 += mouse_delta.y;
                                break;
                            case 3:
                                new_x2 += mouse_delta.x;
                                new_y2 += mouse_delta.y;
                                break;
                            }

                            new_size.x = new_x2 - new_x1;
                            new_size.y = new_y2 - new_y1;

                            new_size.x = std::max<float>(new_size.x, 10.0f);
                            new_size.y = std::max<float>(new_size.y, 10.0f);

                            annotation.x = (new_x1 - image_pos.x + ImGui::GetScrollX()) / zoom;
                            annotation.y = (new_y1 - image_pos.y + ImGui::GetScrollY()) / zoom;
                            annotation.w = new_size.x / zoom;
                            annotation.h = new_size.y / zoom;

                            // Update the resize start position and size for the next frame
                            annotation.resize_start_pos = mouse_pos;
                            annotation.resize_start_size = new_size;
                        }
                        else {
                            annotation.is_resizing = false;
                        }
                    }

                    if (ImGui::IsMouseHoveringRect(ImVec2(display_x1, display_y1), ImVec2(display_x2, display_y2))) {
                        if (annotation.type == ct_body) {
                            if (!key_down(VK_RBUTTON)) {
                                annotation_type = ct_head;
                            }
                            is_hovering_over_body = true;

                        }
                        if (!annotation.is_selected) {
                            draw->AddRectFilled(ImVec2(display_x1, display_y1), ImVec2(display_x2, display_y2), IM_COL32(0, 255, 0, 20));
                        }
                        else {
                            draw->AddRectFilled(ImVec2(display_x1, display_y1), ImVec2(display_x2, display_y2), IM_COL32(255, 0, 0, 20));
                        }
                        annotation.is_hovering = true;
                        if (key_pressed(VK_MBUTTON)) {
                            annotation.is_selected = !annotation.is_selected;
                        }
                    }
                    else {
                        annotation.is_hovering = false;
                        if (!annotation.is_selected) {
                            draw->AddRectFilled(ImVec2(display_x1, display_y1), ImVec2(display_x2, display_y2), IM_COL32(0, 255, 0, 30));
                        }
                        else {
                            draw->AddRectFilled(ImVec2(display_x1, display_y1), ImVec2(display_x2, display_y2), IM_COL32(255, 0, 0, 30));
                        }
                    }

                    if (annotation.is_selected && key_pressed(VK_DELETE)) {
                        annotation.x = -10000;
                    }
                }
                if (is_hovering_over_body == false && !key_down(VK_RBUTTON) && !drawing_begun) {
                    annotation_type = ct_body;
                }
            }

            float display_x1 = new_annotation.x * zoom + image_pos.x - ImGui::GetScrollX();
            float display_y1 = new_annotation.y * zoom + image_pos.y - ImGui::GetScrollY();
            float display_x2 = (new_annotation.x + new_annotation.w) * zoom + image_pos.x - ImGui::GetScrollX();
            float display_y2 = (new_annotation.y + new_annotation.h) * zoom + image_pos.y - ImGui::GetScrollY();

            

            draw->AddRect(ImVec2(display_x1, display_y1), ImVec2(display_x2, display_y2), IM_COL32(255, 0, 0, 255));
            //draw->AddCircleFilled(ImVec2(cp.x, cp.y), 3, IM_COL32(255, 0, 0, 255), 5);

            draw->AddLine(ImVec2(cp.x, 0), ImVec2(cp.x, 5000), IM_COL32(0, 255, 0, 100), 1);
            draw->AddLine(ImVec2(0, cp.y), ImVec2(5000, cp.y), IM_COL32(0, 255, 0, 100), 1);

            ImGui::SetMouseCursor(ImGuiMouseCursor_None);

            ImGui::PopItemFlag();

            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f), (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
        g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        static bool deleting_image = false;
        if (key_pressed(VK_BACK)) {
            deleting_image = true;
            MessageBoxA(NULL, "Image deleted", "Image deleted", 0);
            images[image_idx].texture->Release();
            deleteFile(images[image_idx].file_name);
            std::string txt_substr = images[image_idx].file_name.substr(0, images[image_idx].file_name.length() - 4) + ".txt";
            deleteFile(txt_substr);
         
            images.erase(images.begin() + image_idx);
            if (D3DXCreateTextureFromFileA(g_pd3dDevice, images[image_idx].file_name.c_str(), &images[image_idx].texture) != D3D_OK) {
                std::cerr << "Failed to load texture from file: " << images[image_idx].file_name.c_str() << std::endl;
                images[image_idx].texture = nullptr; // Handle failed texture creation
            }

        }
        if (key_pressed(VK_RIGHT)) {
            images[image_idx].texture->Release();

            if (image_idx + 1 < images.size()) {
                image_idx++;
              
            }
            else {
                image_idx = 0;
            }
            temp = image_idx;
           
            if (D3DXCreateTextureFromFileA(g_pd3dDevice, images[image_idx].file_name.c_str(), &images[image_idx].texture) != D3D_OK) {
                std::cerr << "Failed to load texture from file: " << images[image_idx].file_name.c_str() << std::endl;
                images[image_idx].texture = nullptr; // Handle failed texture creation
            }

        }
        if (temp != image_idx) {
            images[temp].texture->Release();
            // Create a texture from the file
            if (D3DXCreateTextureFromFileA(g_pd3dDevice, images[image_idx].file_name.c_str(), &images[image_idx].texture) != D3D_OK) {
                std::cerr << "Failed to load texture from file: " << images[image_idx].file_name.c_str() << std::endl;
                images[image_idx].texture = nullptr; // Handle failed texture creation
            }
        }
        if (key_pressed(VK_LEFT)) {
            images[image_idx].texture->Release();
            if (image_idx > 0) {
                image_idx--;

            }
            else {
                image_idx = images.size() - 1;
            }
            temp = image_idx;
         
            if (D3DXCreateTextureFromFileA(g_pd3dDevice, images[image_idx].file_name.c_str(), &images[image_idx].texture) != D3D_OK) {
                std::cerr << "Failed to load texture from file: " << images[image_idx].file_name.c_str() << std::endl;
                images[image_idx].texture = nullptr; // Handle failed texture creation
            }

        }

        HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);

        // Handle loss of D3D9 device
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
