#pragma once
#include "mycommon.h"

// Vista-style dialog
struct ChooseFolderDialog {
    static fs::path ChooseFolder(const std::wstring& title, void* parentHwnd = nullptr);
};
