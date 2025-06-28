#pragma once
#ifdef _WIN32
# define GRABCUT_API extern "C" __declspec(dllexport)
#else
# define GRABCUT_API extern "C"
#endif

GRABCUT_API int grabcut_exec(
    const char* imagePath,
    const char* outPath,
    const int x, const int y,
    const int width, const int height,
    const int margin
);