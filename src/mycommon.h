#pragma once
#include <cassert>
#include <vector>
#include <queue>
#include <fstream>
#include <thread>
#include <filesystem>

namespace fs = std::filesystem;

using BytesBuffer = std::vector<uint8_t>;

#ifndef MySafeRelease
#define MySafeRelease(ptr)  \
    if (ptr) {              \
        (ptr)->Release();   \
        (ptr) = nullptr;    \
    }
#endif

#ifndef MySafeDelete
#define MySafeDelete(ptr)   \
    if (ptr) {              \
        delete (ptr);       \
        (ptr) = nullptr;    \
    }
#endif 
