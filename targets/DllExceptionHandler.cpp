#include "DllExceptionHandler.hpp"
#include <windows.h>
#include "NetLogger.hpp"
#include <time.h>
#include <string>
#include <filesystem>
#include <psapi.h>

void logModuleFromAddress(void* address, FILE* f)
{
    HMODULE modules[1024];
    DWORD needed;

    if (!EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &needed)) {
        return;
    }

    DWORD count = needed / sizeof(HMODULE);

    for(int i = 0; i < count; i++) {
        MODULEINFO info{};

        if (!GetModuleInformation(GetCurrentProcess(), modules[i], &info, sizeof(info))) {
            continue;
        }

        uintptr_t base = (uintptr_t)info.lpBaseOfDll;
        uintptr_t end  = base + info.SizeOfImage;
        uintptr_t addr = (uintptr_t)address;

        if (addr >= base && addr < end) {
            char path[256];

            GetModuleFileNameA(modules[i], path, sizeof(path));

            fprintf(f, "Crash path: %s\n", path);
            fprintf(f, "Base address: %08X\n", info.lpBaseOfDll);
            fprintf(f, "Offset: %08X\n", addr - base);
            return;
        }
    }

    fprintf(f, "UNABLE TO FIND CRASH MODULE INFO\n");
}

LONG WINAPI unhandledExceptionFilter(PEXCEPTION_POINTERS ep) {

    if (!std::filesystem::is_directory("CRASH_DUMPS")) {
        std::filesystem::create_directories("CRASH_DUMPS");
    }

    time_t timeVal;
    time(&timeVal);
    struct tm* timeInfo = localtime(&timeVal);

    char timeBuffer[32];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H-%M-%S", timeInfo);
    std::string filename = "CRASH_DUMPS/DUMP_" + std::string(timeBuffer) + ".txt";
 
    const EXCEPTION_RECORD* er = ep->ExceptionRecord;
    const CONTEXT* ctx = ep->ContextRecord;

    DWORD EIP = ctx->Eip;
    char* what = (char*)&EIP;

    HMODULE module;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, what, &module);

    char moduleName[256];
    GetModuleFileNameA(module, moduleName, 256);

    MODULEINFO moduleInfo;
    GetModuleInformation(GetCurrentProcess(), module, &moduleInfo, sizeof(moduleInfo));

    FILE* f = fopen(filename.c_str(), "w"); 

    fprintf(f, "IF YOU DONT LIKE ME I HOPE YOU DIE\n\n");
    
    fprintf(f, "%s\n", timeBuffer);
    fprintf(f, "\n-----\n\n");
    
    fprintf(f, "MODULE: %s\n", moduleName);
    logModuleFromAddress((void*)EIP, f);
    fprintf(f, "\n-----\n\n");

    fprintf(f, "EAX:\t%08X\n", ctx->Eax);
    fprintf(f, "EBX:\t%08X\n", ctx->Ebx);
    fprintf(f, "ECX:\t%08X\n", ctx->Ecx);
    fprintf(f, "EDX:\t%08X\n", ctx->Edx);

    fprintf(f, "ESI:\t%08X\n", ctx->Esi);
    fprintf(f, "EDI:\t%08X\n", ctx->Edi);
    
    fprintf(f, "EBP:\t%08X\n", ctx->Ebp);
    fprintf(f, "ESP:\t%08X\n", ctx->Esp);
    
    fprintf(f, "\n-----\n\n");

    fprintf(f, "Exception info:\n");
    fprintf(f, "code: %08X\n", er->ExceptionCode);
    fprintf(f, "flag: %08X\n", er->ExceptionFlags);
    fprintf(f, "addr: %08X\n", er->ExceptionAddress);
    
    fprintf(f, "numparam: %d\n", er->NumberParameters);

    fprintf(f, "\n-----\n\n"); 

    for(int i=0; i<er->NumberParameters; i++) {
        fprintf(f, "P%d %08X\n", i, er->ExceptionInformation[i]);
    }
    
    fprintf(f, "\n-----\n\n");

    fprintf(f, "STACK:\n");

    DWORD* ESP = (DWORD*)ctx->Esp;

    for(int i=0; i<=64; i++) {
        fprintf(f, "%08X ESP[%04X] = %08X\n", (DWORD)(ESP + i), i*4, ESP[i]);
    }

    fprintf(f, "\n-----\n\n");

    fclose(f);

    // make sure everything is flushed (is everything flushed?)

    // todo, submit this to some aws thing. ugh

    return EXCEPTION_CONTINUE_SEARCH;
}

namespace ExceptionHandler {

    void init() {
        
        //SetUnhandledExceptionFilter(unhandledExceptionFilter);
        
        int* i = NULL;
        //*i = 0;
    }

}

