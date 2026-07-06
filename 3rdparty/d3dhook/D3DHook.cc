#include "D3DHook.h"

#include <windows.h>
#include <d3d9.h>
#include <cassert>

#include <stdint.h>
#include "Logger.hpp"

#include "../minhook/include/MinHook.h"

#pragma comment(lib, "libMinHook.x86.lib")

typedef enum {
    HOOK_OK,
    HOOK_FAILURE,
    HOOK_WINDOW,
    HOOK_LOAD,
    HOOK_CREATE,
    HOOK_DEVICE,
    HOOK_INTERFACE,
    HOOK_MH,
    HOOK_ENABLE
} Hook_Status;

Hook_Status hooks_create(void);
Hook_Status hooks_enable(void);
void hooks_destroy(void);

void log_status(Hook_Status status) {
    switch (status) {
    case HOOK_OK:
        LOG("DirectX HOOK OK");
        break;
    case HOOK_FAILURE:
        LOG("DirectX HOOK FAILURE");
        break;
    case HOOK_WINDOW:
        LOG("DirectX Failure: HOOK WINDOW");
        break;
    case HOOK_LOAD:
        LOG("DirectX Failure: HOOK LOAD");
        break;
    case HOOK_CREATE:
        LOG("DirectX Failure: HOOK CREATE");
        break;
    case HOOK_DEVICE:
        LOG("DirectX Failure: HOOK DEVICE");
        break;
    case HOOK_INTERFACE:
        LOG("DirectX Failure: HOOK INTERFACE");
        break;
    case HOOK_MH:
        LOG("DirectX Failure: HOOK MinHook");
        break;
    case HOOK_ENABLE:
        break;
    }
}

std::string InitDirectX(void *hwnd) {
    (void)hwnd;
    Hook_Status status;
    if ((status = hooks_create()) != HOOK_OK) {
        log_status(status);
        return "failed to create hooks";
    }
    LOG("DirectX Hook Creation Success");
    return "";
}

std::string HookDirectX() {
    Hook_Status status;
    if ((status = hooks_enable()) != HOOK_OK) {
        log_status(status);
        return "failed to enable hooks";
    }
    LOG("DirectX Hooking Success");
    return "";
}

void UnhookDirectX() {
    hooks_destroy();
}

