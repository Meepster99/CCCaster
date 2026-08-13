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
#include <dbghelp.h>
#include <cxxabi.h>
#include <cstdlib>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "dbghelp.lib")

BOOL CALLBACK symCallback( HANDLE hProcess, ULONG actionCode, void* callbackData, void* userContext) {
	
	log("```actionCode: %08X", actionCode);

	static char buffer[4096];
	if (actionCode == CBA_DEBUG_INFO) {
	
		const char* msg = reinterpret_cast<const char*>(callbackData);
		snprintf(buffer, sizeof(buffer), "`%s", msg);
		log(buffer);
	}

	return TRUE;
}

namespace ExceptionHandler {

	int resSymInitialize = 0;

	bool wasInitCalled = false;

	std::string numToHex(DWORD v) {
		// i could avoid using this to compress shit further but... who cares
		char buffer[32];
		snprintf(buffer, 32, "0x%08X", v);
		return std::string(buffer);
	}

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

    void logModuleFromAddress(void* address, json& j) {
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

		j["moduleInfo"]["modules"] = json::array();

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

			j["moduleInfo"]["modules"].push_back(path);

            //fprintf(f, "\t%s\n", path);
        }

		j["moduleInfo"]["foundCrash"] = foundCrash;

        if(foundCrash) {
			j["moduleInfo"]["crashInfo"] = {
				{"path", crashPath},
				{"addr", numToHex((DWORD)crashBase)},
				{"offset", numToHex(crashOffset)}
			};
		}
        
    }

	json logSymFromAddr(void* address) {

		json res;

		DWORD64 addr = (DWORD64)address;
		HANDLE process = GetCurrentProcess();

		// why does  https://learn.microsoft.com/en-us/windows/win32/debug/retrieving-symbol-information-by-address
		// alloc this shit this way?
		char addrBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
		PSYMBOL_INFO symbol = (PSYMBOL_INFO)addrBuffer;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->MaxNameLen = MAX_SYM_NAME;
		DWORD64 addrDisplacement = 0;

		if(!SymFromAddr(process, addr, &addrDisplacement, symbol)) {
			res = {
				{"hasInfo", false},
				{"failFunc", "SymFromAddr"},
				{"error", GetLastError()},
			};
			return res;
		}

		const char* baseName = symbol->Name;
		char fixedBaseName[512];
	
		// __cxa_demangle expects a leading _, symfromaddr doesnt give it
		// is this because im crashing in a mingw compiled area, and this will cause issues in non mingw areas?
		// to be safe, im doing it with both options
		snprintf(fixedBaseName, sizeof(fixedBaseName), "_%s", baseName);
		
		int baseStatus = 0;
		char* baseDemangled = abi::__cxa_demangle(baseName, nullptr, nullptr, &baseStatus);

		int fixedStatus = 0;
		char* fixedDemangled = abi::__cxa_demangle(fixedBaseName, nullptr, nullptr, &fixedStatus);
		
		char tempBaseDemangled[512];
		char tempFixedDemangled[512];

		if(baseDemangled == NULL) {
			snprintf(tempBaseDemangled, sizeof(tempBaseDemangled), "(NULL)");
		} else {
			snprintf(tempBaseDemangled, sizeof(tempBaseDemangled), "%s", baseDemangled);
		}

		if(fixedDemangled == NULL) {
			snprintf(tempFixedDemangled, sizeof(tempFixedDemangled), "(NULL)");
		} else {
			snprintf(tempFixedDemangled, sizeof(tempFixedDemangled), "%s", fixedDemangled);
		}

		res = {
			{"hasInfo", true},
			
			{"baseName", baseName},
			{"fixedBaseName", fixedBaseName},

			{"baseDemangleStatus", baseStatus},
			{"fixedDemangleStatus", fixedStatus},

			{"baseDemangle", tempBaseDemangled},
			{"fixedDemangle", tempFixedDemangled},

			{"displacement", numToHex(addrDisplacement)},
		};

		free(baseDemangled);
		free(fixedDemangled);

		return res;
	}

	json logSymGetLineFromAddr(void* address) {

		json res;

		DWORD64 addr = (DWORD64)address;
		HANDLE process = GetCurrentProcess();

		DWORD lineDisplacement = 0;
		IMAGEHLP_LINE lineData{};
		lineData.SizeOfStruct = sizeof(lineData);

		if(!SymGetLineFromAddr(process, addr, &lineDisplacement, &lineData)) {
			res = {
				{"hasInfo", false},
				{"failFunc", "SymGetLineFromAddr"},
				{"error", GetLastError()},
			};
			return res;
		}

		res = {
			{"hasInfo", true},
			{"filename", lineData.FileName},
			{"lineNumber", lineData.LineNumber},
		};
		
		return res;
	}

	void logFunctionInfo(void* address, json& j) {

		// this is hell. trying to get this to run between code running on 3 different compilers is hell.

		
		SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_DEBUG);
		SymSetOptions(SymGetOptions() & ~SYMOPT_DEFERRED_LOADS);

		wchar_t tempPath[MAX_PATH];
		DWORD len = GetTempPathW(MAX_PATH, tempPath);
		
		resSymInitialize = SymInitializeW(GetCurrentProcess(), tempPath, TRUE);
		
		SymSetSearchPathW(GetCurrentProcess(), tempPath);

		DWORD moduleBase = SymGetModuleBase(GetCurrentProcess(), (DWORD)address);

		IMAGEHLP_MODULE64  moduleInfo{};
		moduleInfo.SizeOfStruct = sizeof(moduleInfo);

		char cwd[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, cwd);

		if (SymGetModuleInfo64(GetCurrentProcess(), moduleBase, &moduleInfo)) {
			j["symInfo"] = {
				{"SymType", std::to_string(moduleInfo.SymType)},
				{"LoadedImageName", moduleInfo.LoadedImageName},
				{"ImageName", moduleInfo.ImageName},
				{"ModuleName", moduleInfo.ModuleName},
				{"LoadedPdbName", moduleInfo.LoadedPdbName},
				{"PdbSig", moduleInfo.PdbSig},
				{"PdbAge", moduleInfo.PdbAge},
				{"PdbUnmatched", moduleInfo.PdbUnmatched},
				{"DbgUnmatched", moduleInfo.DbgUnmatched},
				{"GlobalSymbols", moduleInfo.GlobalSymbols},
				{"TypeInfo", moduleInfo.TypeInfo},
				{"Publics", moduleInfo.Publics},
				{"LineNumbers", moduleInfo.LineNumbers},
				{"CWD", cwd},
			};
		} else {
			j["symInfo"]["error"] = GetLastError();
		}

		char searchPath[MAX_PATH];
		bool idrk = SymGetSearchPath(GetCurrentProcess(), searchPath, MAX_PATH);

		j["funcInfo"]["SymGetSearchPath"] = std::string(searchPath);

		j["funcInfo"]["SymInitialize"]["resCode"] = resSymInitialize;
		if(!resSymInitialize) {
			log("SymInitialize failed!");
			j["funcInfo"]["SymInitialize"]["error"] = GetLastError();
		}

		HMODULE dbghelp = GetModuleHandleA("dbghelp.dll");

		char path[MAX_PATH] = {};
		GetModuleFileNameA(dbghelp, path, MAX_PATH);

		j["funcInfo"]["SymFromAddr"] = logSymFromAddr(address);

		j["funcInfo"]["SymGetLineFromAddr"] = logSymGetLineFromAddr(address);

	}

    void logPCInfo(json& j) {

        char pcName[256];
        char userName[256];
        DWORD bufferLen = 256;
        GetComputerNameA(pcName, &bufferLen);
        GetUserNameA(userName, &bufferLen);

		j["pcName"] = pcName;
		j["user"] = userName;

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

		j["CPU"] = brand;

        // grabbing the gpu model is a bit difficult, i could go off the directx device, but i dont want to risk fucking it up
        
        DWORD dwDevice = *(DWORD*)0x0076e7d4;
        IDirect3DDevice9* device = (IDirect3DDevice9*)dwDevice;

		j["GPU"] = {};

        if(device == NULL) { // im taking a lot of risks here
            j["GPU"]["device"] = false; //fprintf(f, "unable to get gpu info :(\n");
			return;
        }
		j["GPU"]["device"] = true; //
	

        IDirect3D9* notDevice = NULL;
        device->GetDirect3D(&notDevice);
        
        if(notDevice == NULL) {
            //fprintf(f, "unable to get IDirect3D9??\n");
			j["GPU"]["notDevice"] = false;
			return;
        }
		j["GPU"]["notDevice"] = true;


        int adapterCount = notDevice->GetAdapterCount();

        D3DADAPTER_IDENTIFIER9 id;
        //fprintf(f, "GPU: %d adapters\n", adapterCount);
		j["GPU"]["adapters"] = json::array();
        for(int i=0; i<adapterCount; i++) { // i dont want to be here anymore 
            notDevice->GetAdapterIdentifier(i, 0, &id);
            //fprintf(f, "\tA%d %s\n", i, id.Description);
			j["GPU"]["adapters"].push_back(id.Description);
        }


    }

	void createCrashDump(PEXCEPTION_POINTERS ep, json& j, const char* timeBuffer) {

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

		j["crashTime"] = timeBuffer;

		logPCInfo(j);

		// if etm is attached, i should grab that version if possible.
		j["version"] = {
			{"code", LocalVersion.code.c_str()},
			{"revision", LocalVersion.revision.c_str()},
			{"buildTime", LocalVersion.buildTime.c_str()},
		};

		j["registers"] = {
			{"EAX", numToHex(ctx->Eax)},
			{"EBX", numToHex(ctx->Ebx)},
			{"ECX", numToHex(ctx->Ecx)},
			{"EDX", numToHex(ctx->Edx)},

			{"ESI", numToHex(ctx->Esi)},
			{"EDI", numToHex(ctx->Edi)},

			{"EBP", numToHex(ctx->Ebp)},
			{"ESP", numToHex(ctx->Esp)},

			{"EIP", numToHex(ctx->Eip)},
		};

		auto exceptionParams = json::array();
		for(int i=0; i<er->NumberParameters; i++) {
			exceptionParams.push_back(numToHex(er->ExceptionInformation[i]));
		}

		j["exception"] = {
			{"code", numToHex(er->ExceptionCode)},
			{"flag", numToHex(er->ExceptionFlags)},
			{"addr", numToHex((DWORD)er->ExceptionAddress)},
			{"params", exceptionParams},
		};

		j["stack"] = json::array();
		
		DWORD* ESP = (DWORD*)ctx->Esp;
		for(int i=0; i<=256; i++) {
			//fprintf(f, "%08X ESP[%04X] = %08X\n", (DWORD)(ESP + i), i*4, ESP[i]);
			j["stack"].push_back(numToHex(ESP[i]));
		}

		j["moduleInfo"]["base"] = moduleName;

		logModuleFromAddress((void*)EIP, j);

		logFunctionInfo((void*)EIP, j);
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
        std::string filename = "CRASH_DUMPS/DUMP_" + std::string(timeBuffer) + ".json";
    
		json j;

        createCrashDump(ep, j, timeBuffer);

		std::ofstream outFile(filename);
		outFile << "IF YOU DONT LIKE ME I HOPE YOU DIE\n";
		outFile << timeBuffer << "\n";
		outFile << std::string(j["pcName"]) << "\n";
		outFile << j.dump(4);
		outFile.close();

        CopyFileA(filename.c_str(), "CRASH_DUMPS/DUMP_RECENT.json", false);

        // make sure everything is flushed (is everything flushed?)

        // todo, submit this to some aws thing. ugh

        sendDump(filename);

    }

    LONG WINAPI unhandledExceptionFilter(PEXCEPTION_POINTERS ep) {

		log("`unhandledExceptionFilter called");
        exceptionFilter(ep);
		log("`unhandledExceptionFilter exited gracefully");

        return EXCEPTION_CONTINUE_SEARCH;
    }

    void init() {

		//if(wasInitCalled) {
		//	return;
		//}
		//wasInitCalled = true;
		// not sure why i dont call the above. paranoia

        SetUnhandledExceptionFilter(unhandledExceptionFilter);
        
       	//int* i = NULL;
        //*i = 0;
    }

}
