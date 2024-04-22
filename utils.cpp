#include "utils.h"
#include <iostream>

void draw_crosshair() {
    ImVec2 cp = ImGui::GetMousePos();

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    draw->AddLine(ImVec2(cp.x, 0), ImVec2(cp.x, 1920), IM_COL32(0, 255, 0, 100), 1);
    draw->AddLine(ImVec2(0, cp.y), ImVec2(1600, cp.y), IM_COL32(0, 255, 0, 100), 1);
}

std::string utils::open_directory_dialog(const std::string& title) {
    // initialize com library
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        return "";
    }

    // create a file open dialog object
    IFileOpenDialog* file_dialog;
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&file_dialog));
    if (FAILED(hr)) {
        CoUninitialize();
        return "";
    }

    // set options for the dialog
    FILEOPENDIALOGOPTIONS options;
    hr = file_dialog->GetOptions(&options);
    if (SUCCEEDED(hr)) {
        options |= FOS_PICKFOLDERS;
        hr = file_dialog->SetOptions(options);
    }

    // set the dialog title
    std::wstring wide_title(title.begin(), title.end());
    hr = file_dialog->SetTitle(wide_title.c_str());
    if (FAILED(hr)) {
        file_dialog->Release();
        CoUninitialize();
        return "";
    }

    // show the dialog
    hr = file_dialog->Show(nullptr);
    if (FAILED(hr)) {
        file_dialog->Release();
        CoUninitialize();
        return "";
    }

    // get the selected item
    IShellItem* item;
    hr = file_dialog->GetResult(&item);
    if (FAILED(hr)) {
        file_dialog->Release();
        CoUninitialize();
        return "";
    }

    // get the path of the selected item
    PWSTR path;
    hr = item->GetDisplayName(SIGDN_FILESYSPATH, &path);
    if (FAILED(hr)) {
        item->Release();
        file_dialog->Release();
        CoUninitialize();
        return "";
    }

    // convert the path to a string
    std::wstring wide_path(path);
    std::string result(wide_path.begin(), wide_path.end());

    // clean up
    CoTaskMemFree(path);
    item->Release();
    file_dialog->Release();
    CoUninitialize();

    return result;
}
