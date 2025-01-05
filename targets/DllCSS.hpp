
#pragma once

#include <windows.h>
#include <d3dx9.h>

__attribute__((noinline)) void cssInit();

extern "C" {
    extern DWORD stageSelEAX;
}

__attribute__((noinline)) void stageSelCallback();

void updateCSSStuff(IDirect3DDevice9 *device);

int getCharMoon(int playerIndex);
