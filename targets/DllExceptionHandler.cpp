#include "DllExceptionHandler.hpp"
#include <windows.h>
#include "NetLogger.hpp"
#include <time.h>
#include <string>
#include <filesystem>
#include <psapi.h>
#include "Version.hpp"
#include <intrin.h>
#include <vector>
#include <array>
#include <d3dx9.h>
#include <fstream>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

namespace ExceptionHandler {

    void sendDump(const std::string& filename) {

        // scrapers will try and take urls here, so forgive the bs 

        std::string xorChars =  "fjhioqweop31i02eudiashjfposkjnfjlfjdakdj013498urfpijdkdl;afsdkfqh011f"; // literal keyboard mash
        uint8_t xorURL[] = {0x0E, 0x1E, 0x1C, 0x19, 0x1C, 0x4B, 0x58, 0x4A, 0x1D, 0x1B, 0x41, 0x5E, 0x05, 0x5B, 0x50, 0x0C, 0x0D, 0x01, 0x0F, 0x15, 0x45, 0x19, 0x0B, 0x12, 0x15, 0x1E, 0x10, 0x1F, 0x1F, 0x1F, 0x11, 0x5D, 0x05, 0x13, 0x5A, 0x1D, 0x08, 0x0C, 0x05, 0x1B, 0x1E, 0x5D, 0x52, 0x59, 0x5B, 0x5C, 0x14, 0x5F, 0x13, 0x02, 0x05, 0x44, 0x11, 0x18, 0x49, 0x09, 0x5A, 0x12, 0x12, 0x5E, 0x56, 0x45, 0x09, 0x1F, 0x46, 0x51, 0x46, 0x42, 0x49};

        std::wstring lambdaURL = L"";
        for(int i=0; i<xorChars.size()-1; i++) {
            if(i < 8) { continue; } 
            lambdaURL += xorURL[i] ^ xorChars[i];
        }        

        std::ifstream inFile(filename, std::ios::binary | std::ios::ate);

        std::streamsize size = inFile.tellg();
        inFile.seekg(0, std::ios::beg);

        char* buffer = (char*)malloc(size);
        inFile.read(buffer, size);
        inFile.close();

        HINTERNET session = WinHttpOpen(L"Ugh/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

        if (!session) {
            log("couldnt open http session??");
            return;
        }
        
        HINTERNET connect = WinHttpConnect(session, lambdaURL.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);

        if (!connect) { 
            log("failed to httpconnect, error:%d", GetLastError());
            log("url was %ls", lambdaURL.c_str());
            WinHttpCloseHandle(session);
            return;
        }

        HINTERNET request = WinHttpOpenRequest(connect, L"POST", L"/", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);

        if (!request) {
            log("failed WinHttpOpenRequest");
            WinHttpCloseHandle(connect);
            WinHttpCloseHandle(session);
            return;
        }

        const wchar_t* headers = L"Content-Type: application/octet-stream\r\n";

        BOOL result = WinHttpSendRequest(request, headers, -1L, (LPVOID)buffer, (DWORD)size, (DWORD)size, 0);

        if(result) {
            log("sent crashdump :)");
            result = WinHttpReceiveResponse(request,nullptr);   
        } else {
            log("post req failed, got a bad result.");
        }

        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);

        free(buffer);
    }

    void logModuleFromAddress(void* address, FILE* f) {
        HMODULE modules[1024];
        DWORD needed;

        if (!EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &needed)) {
            return;
        }

        DWORD count = needed / sizeof(HMODULE);

        
        char path[MAX_PATH] = {'\0'};

        char crashPath[MAX_PATH] = {'\0'};
        LPVOID crashBase = NULL;
        uintptr_t crashOffset = NULL;

        bool foundCrash = false;

        fprintf(f, "Modules:\n");

        for(int i = 0; i < count; i++) {
            MODULEINFO info{};

            if (!GetModuleInformation(GetCurrentProcess(), modules[i], &info, sizeof(info))) {
                continue;
            }

            uintptr_t base = (uintptr_t)info.lpBaseOfDll;
            uintptr_t end  = base + info.SizeOfImage;
            uintptr_t addr = (uintptr_t)address;

            GetModuleFileNameA(modules[i], path, sizeof(path));

            if (addr >= base && addr < end) {
                memcpy(crashPath, path, strlen(path));
                crashBase = info.lpBaseOfDll;
                crashOffset = addr - base;
                foundCrash = true;
            }

            fprintf(f, "\t%s\n", path);
        }

        if(foundCrash) {
            fprintf(f, "Crash info:\n");
            fprintf(f, "\tPath: %s\n", crashPath);
            fprintf(f, "\tBase address: %08X\n", crashBase);
            fprintf(f, "\tOffset: %08X\n", crashOffset);
        } else {
            fprintf(f, "UNABLE TO FIND CRASH MODULE INFO\n");
        }
        
        
    }

    void logPCInfo(FILE* f) {

        char pcName[256];
        char userName[256];
        DWORD bufferLen = 256;
        GetComputerNameA(pcName, &bufferLen);
        GetUserNameA(userName, &bufferLen);

        fprintf(f, "PCNAME: %s\n", pcName);
        fprintf(f, "USER: %s\n", userName);

        // read this stupid article https://learn.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=msvc-170

        std::vector<std::array<int, 4>> extdata;

        char brand[0x40];
        const char* defaultString =  "unknown brand";
        memcpy(brand, defaultString, strlen(defaultString));

        std::array<int, 4> cpuData = {0,0,0,0};
        __cpuid(cpuData.data(), 0x80000000);
        int maxExtID = cpuData[0];

        for (int i = 0x80000000; i <= maxExtID; ++i) {
            __cpuidex(cpuData.data(), i, 0);
            extdata.push_back(cpuData);
        }

        // Interpret CPU brand string if reported
        if (maxExtID >= 0x80000004) {
            memcpy(brand, extdata[2].data(), sizeof(cpuData));
            memcpy(brand + 16, extdata[3].data(), sizeof(cpuData));
            memcpy(brand + 32, extdata[4].data(), sizeof(cpuData));
        }

        fprintf(f, "CPU: %s\n", brand);

        // grabbing the gpu model is a bit difficult, i could go off the directx device, but i dont want to risk fucking it up
        
        DWORD dwDevice = *(DWORD*)0x0076e7d4;
        IDirect3DDevice9* device = (IDirect3DDevice9*)dwDevice;

        if(device == NULL) { // im taking a lot of risks here
            fprintf(f, "unable to get gpu info :(\n");
        }

        IDirect3D9* notDevice = NULL;
        device->GetDirect3D(&notDevice);
        
        if(notDevice == NULL) {
            fprintf(f, "unable to get IDirect3D9??\n");
        }

        int adapterCount = notDevice->GetAdapterCount();

        D3DADAPTER_IDENTIFIER9 id;
        fprintf(f, "GPU: %d adapters\n", adapterCount);
        for(int i=0; i<adapterCount; i++) { // i dont want to be here anymore 
            notDevice->GetAdapterIdentifier(i, 0, &id);
            fprintf(f, "\tA%d %s\n", i, id.Description);
        }

        fprintf(f, "\n-----\n\n");

    }

    void exceptionFilter(PEXCEPTION_POINTERS ep) {
            
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

        logPCInfo(f);
        
        fprintf(f, "%s\n", LocalVersion.code.c_str());
        fprintf(f, "%s\n", LocalVersion.revision.c_str());
        fprintf(f, "%s\n", LocalVersion.buildTime.c_str());
        fprintf(f, "\n-----\n\n");
        
        fprintf(f, "BASEMODULE: %s\n", moduleName);
        logModuleFromAddress((void*)EIP, f);
        fprintf(f, "\n-----\n\n");

        fprintf(f, "EIP:\t%08X\n", ctx->Eip);

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

        for(int i=0; i<er->NumberParameters; i++) {
            fprintf(f, "P%d %08X\n", i, er->ExceptionInformation[i]);
        }
        
        fprintf(f, "\n-----\n\n");

        fprintf(f, "STACK:\n");

        DWORD* ESP = (DWORD*)ctx->Esp;

        for(int i=0; i<=256; i++) {
            fprintf(f, "%08X ESP[%04X] = %08X\n", (DWORD)(ESP + i), i*4, ESP[i]);
        }

        fprintf(f, "\n-----\n\n");

        fclose(f);

        CopyFileA(filename.c_str(), "CRASH_DUMPS/DUMP_RECENT.txt", false);

        // make sure everything is flushed (is everything flushed?)

        // todo, submit this to some aws thing. ugh

        sendDump(filename);

    }

    LONG WINAPI unhandledExceptionFilter(PEXCEPTION_POINTERS ep) {

        exceptionFilter(ep);

        return EXCEPTION_CONTINUE_SEARCH;
    }

    void init() {
        
        SetUnhandledExceptionFilter(unhandledExceptionFilter);
        
        //int* i = NULL;
        //*i = 0;
    }

}
