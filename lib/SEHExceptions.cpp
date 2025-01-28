
#include "SEHExceptions.hpp"

std::map<DWORD, std::vector<CPUState*>> exceptHandlerAddrs;

CPUState* getCpuState() {

    __asmStart R"(
        sub esp, 0x4;
    )" __asmEnd

    PUSH_ALL;
    __asmStart R"(
        
        sub esp, 24;
        push 36;
        call _malloc;
        add esp, 28;

        mov dword ptr [esp + 0x24], eax;
    )" __asmEnd
    POP_ALL;
		
    __asmStart R"(

		// in the future, i should really rewrite this to just,, push and pop everything

        // the following handles all registers but eax, esp, and eip
        push eax;

        mov eax, [esp + 4]; // the above push decded the stack 

        mov dword ptr [eax + 0x00], 1; // eax needs to be done
        mov dword ptr [eax + 0x04], ebx; 
        mov dword ptr [eax + 0x08], ecx; 
        mov dword ptr [eax + 0x0C], edx; 
 
        mov dword ptr [eax + 0x10], esi; 
        mov dword ptr [eax + 0x14], edi; 
 
        mov dword ptr [eax + 0x18], 7; // esp needs to be done
        mov dword ptr [eax + 0x1C], ebp; 
 
        mov dword ptr [eax + 0x20], 0xFFFFFFFF; 
        
        pop eax;

        // the following handles eax;
        push ebx;

        mov ebx, [esp + 4];
        mov [ebx + 0x00], eax;

        pop ebx;

        // the following handles esp

        push eax;
        push ebx;

        mov eax, [esp + 8];

        mov ebx, esp;
        sub ebx, 0x10; // 4 stack uses, 2 pushes, the one push to hold the CPUState struct, and one more for the return addr
        mov [eax + 0x18], ebx;

        pop ebx;
        pop eax;
        
        // the following handles EIP

        push eax;
        push ebx;

        mov eax, [esp + 0x8];
        mov ebx, [esp + 0xC];

        mov dword ptr [eax + 0x20], ebx; 
        
        pop ebx;
        pop eax;

    )" __asmEnd
	

    __asmStart R"(
        pop eax;
        ret;
    )" __asmEnd

}

EXCEPTION_DISPOSITION __cdecl except_handler(struct _EXCEPTION_RECORD * ExceptionRecord, void * EstablisherFrame, struct _CONTEXT * ContextRecord, void * DispatcherContext) {
	//ContextRecord->Eip = exceptHandlerAddrs[GetCurrentThreadId()].back();
	
	CPUState* state = exceptHandlerAddrs[GetCurrentThreadId()].back();

	ContextRecord->Eax = state->EAX;
	ContextRecord->Ebx = state->EBX;
	ContextRecord->Ecx = state->ECX;
	ContextRecord->Edx = state->EDX;
	
	ContextRecord->Edi = state->ESI;
	ContextRecord->Esi = state->EDI;
	
	ContextRecord->Esp = state->ESP;
	ContextRecord->Ebp = state->EBP;
	
    ContextRecord->Eip = state->EIP;

	return ExceptionContinueExecution;
}


#include <windows.h>
#include <stdio.h>
#include <ws2tcpip.h>

#include <winsock2.h>
#include <wininet.h>

void ___log(const char* msg)
{
	const char* ipAddress = "127.0.0.1";
	unsigned short port = 17474;
	int msgLen = strlen(msg);
	const char* message = msg;
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) 
	{
		return;
	}
	SOCKET sendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sendSocket == INVALID_SOCKET) 
	{
		WSACleanup();
		return;
	}
	sockaddr_in destAddr;
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = htons(port);
	if (inet_pton(AF_INET, ipAddress, &destAddr.sin_addr) <= 0) 
	{
		closesocket(sendSocket);
		WSACleanup();
		return;
	}
	int sendResult = sendto(sendSocket, message, strlen(message), 0, (sockaddr*)&destAddr, sizeof(destAddr));
	if (sendResult == SOCKET_ERROR) 
	{
		closesocket(sendSocket);
		WSACleanup();
		return;
	}
	closesocket(sendSocket);
	WSACleanup();
}

void log(const char* format, ...) {
	static char buffer[1024]; // no more random char buffers everywhere.
	va_list args;
	va_start(args, format);
	vsnprintf_s(buffer, 1024, format, args);
	___log(buffer);
	va_end(args);
}

unsigned getEIP() {

    __asmStart R"(
		// not using my normal macros,, bc a branch with them hasnt been merged into main
		mov eax, [esp];
		ret
	)" __asmEnd
    
}
