#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include <windows.h>
#include <d3d9.h>

#include "../minhook/include/MinHook.h"

#define UNREACHABLE assert(false)

typedef LPDIRECT3D9 (WINAPI *D3D9CreateFn)(UINT);
typedef HRESULT ( __stdcall *ResetFn ) ( IDirect3DDevice9 *pDevice, LPVOID );
typedef HRESULT ( __stdcall *PresentFn ) ( IDirect3DDevice9 *pDevice, const RECT *, const RECT *, HWND, LPVOID );
typedef HRESULT ( __stdcall *EndSceneFn ) ( IDirect3DDevice9 *pDevice);

extern void PresentFrameBegin ( IDirect3DDevice9 *device );
extern void PresentFrameEnd ( IDirect3DDevice9 *device );
extern void EndScene ( IDirect3DDevice9 *device );
extern void InvalidateDeviceObjects();

enum d3d9_offsets {
    D3D9_RESET = 16,
    D3D9_PRESENT = 17,
    D3D9_ENDSCENE = 42
};

typedef struct {
    HWND window;
    WNDCLASSEX wclass;
} Window_Context;

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

uintptr_t *VTable = NULL;

ResetFn old_reset;
PresentFn old_present;
EndSceneFn old_endscene;

HRESULT __stdcall hook_reset(IDirect3DDevice9 *device, LPVOID params) {
    InvalidateDeviceObjects();
    HRESULT res = old_reset(device, params);
    return res;
}

HRESULT __stdcall hook_present(IDirect3DDevice9 *device, const RECT *src, const RECT *dest, HWND window, LPVOID unused) {
    PresentFrameBegin(device);
    HRESULT res = old_present(device, src, dest, window, unused);
    PresentFrameEnd(device);
    return res;
}

HRESULT __stdcall hook_endscene(IDirect3DDevice9 *device) {
    EndScene(device);
    HRESULT res = old_endscene(device);
    return res;
}

Window_Context create_window(void) {
    Window_Context context = (Window_Context){
        .wclass = (WNDCLASSEX){
            .cbSize = sizeof(WNDCLASSEX),
            .style = CS_HREDRAW | CS_VREDRAW,
            .lpfnWndProc = DefWindowProc,
            .hInstance = GetModuleHandle(NULL),
            .lpszClassName = "dummy"
        }
    };
    RegisterClassEx(&context.wclass);
    context.window = CreateWindow(
        context.wclass.lpszClassName,
        "dummy window",
        WS_OVERLAPPEDWINDOW,
        0,
        0,
        100,
        100,
        NULL,
        NULL,
        context.wclass.hInstance,
        NULL
    );
    return context;
}

Hook_Status hooks_create(void) {
    Hook_Status ret = HOOK_FAILURE;
    // create dummy window
    Window_Context window_context = create_window();
    if (window_context.window == NULL) {
        ret = HOOK_WINDOW;
        goto CLEANUP_CONTEXT;
    }

    // load d3d9.dll
    HMODULE d3d9_module;
    d3d9_module = GetModuleHandle("d3d9.dll");
    if (d3d9_module == NULL) {
        ret = HOOK_LOAD;
        goto CLEANUP_WINDOW;
    }

    // direct3d object create
    D3D9CreateFn d3d9_create;
    d3d9_create = (D3D9CreateFn)GetProcAddress(d3d9_module, "Direct3DCreate9");
    if (d3d9_create == NULL) {
        ret = HOOK_CREATE;
        goto CLEANUP_WINDOW;
    }

    IDirect3D9 *d3d9;
    d3d9 = d3d9_create(D3D_SDK_VERSION);
    if (d3d9 == NULL) {
        ret = HOOK_CREATE;
        goto CLEANUP_WINDOW;
    }

    // create direct3d device
    D3DPRESENT_PARAMETERS d3d9_params;
    d3d9_params = (D3DPRESENT_PARAMETERS){
        .SwapEffect = D3DSWAPEFFECT_DISCARD,
        .hDeviceWindow = window_context.window,
        .Windowed = 1
    };
    IDirect3DDevice9 *device;
    HRESULT res;
    res = d3d9->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_NULLREF,
        window_context.window,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
        &d3d9_params,
        &device
    );
    if (res != D3D_OK || device == NULL) {
        ret = HOOK_DEVICE;
        goto CLEANUP_D3D;
    }

    // get interface from device
    VTable = (uintptr_t *)calloc(119, sizeof(uintptr_t));
    if (VTable == NULL) {
        ret = HOOK_INTERFACE;
        goto CLEANUP_DEVICE;
    }
    memcpy(VTable, *(uintptr_t **)device, sizeof(uintptr_t) * 119);

    // create the hooks
    if (MH_CreateHook((void *)VTable[D3D9_RESET], (void*)&hook_reset, (void **)&old_reset) != MH_OK) {
        ret = HOOK_MH;
        goto CLEANUP_DEVICE;
    }
    if (MH_CreateHook((void *)VTable[D3D9_PRESENT], (void*)&hook_present, (void **)&old_present) != MH_OK) {
        ret = HOOK_MH;
        goto CLEANUP_DEVICE;
    }
    if (MH_CreateHook((void *)VTable[D3D9_ENDSCENE], (void*)&hook_endscene, (void **)&old_endscene) != MH_OK) {
        ret = HOOK_MH;
        goto CLEANUP_DEVICE;
    }

    ret = HOOK_OK;

    // cleanup in reverse order
CLEANUP_DEVICE:
    device->Release();
CLEANUP_D3D:
    d3d9->Release();
CLEANUP_WINDOW:
    DestroyWindow(window_context.window);
CLEANUP_CONTEXT:
    UnregisterClass(window_context.wclass.lpszClassName, window_context.wclass.hInstance);
    return ret;
}

Hook_Status hooks_enable(void) {
    if (MH_EnableHook((void *)VTable[D3D9_RESET]) != MH_OK) {
        return HOOK_ENABLE;
    }
    if (MH_EnableHook((void *)VTable[D3D9_PRESENT]) != MH_OK) {
        return HOOK_ENABLE;
    }
    if (MH_EnableHook((void *)VTable[D3D9_ENDSCENE]) != MH_OK) {
        return HOOK_ENABLE;
    }
    return HOOK_OK;
}

void hooks_destroy(void) {
    if (MH_DisableHook((void *)VTable[D3D9_RESET]) != MH_OK) {
        UNREACHABLE;
    }
    if (MH_DisableHook((void *)VTable[D3D9_PRESENT]) != MH_OK) {
        UNREACHABLE;
    }
    if (MH_DisableHook((void *)VTable[D3D9_ENDSCENE]) != MH_OK) {
        UNREACHABLE;
    }

    if (MH_RemoveHook((void *)VTable[D3D9_RESET]) != MH_OK) {
        UNREACHABLE;
    }
    if (MH_RemoveHook((void *)VTable[D3D9_PRESENT]) != MH_OK) {
        UNREACHABLE;
    }
    if (MH_RemoveHook((void *)VTable[D3D9_ENDSCENE]) != MH_OK) {
        UNREACHABLE;
    }
}

