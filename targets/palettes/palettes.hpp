#pragma once

#include <map>
#include <array>
#include <windows.h>

extern std::map<int, std::map<int, std::array<DWORD, 256>>> palettes;

extern std::map<int, std::array<DWORD, 64>> paletteColorMap;
