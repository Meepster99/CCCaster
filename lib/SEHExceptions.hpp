
// the following is used to catch SEH exceptions
// it NEEDS to be statically compiled, or else it will be very finicky
// god, worst case scenario, ill have this compiled in a seperate step, and link it as a library? so stupid
// thats the reason this is in the lib folder
/*

notes
if you do
DWORD addr = 0;
addr = *(DWORD*)addr;
the compiler will generate a UD2 instruction in an attempt to be nice with it, and fuck everything up
i have no clue if any of this shit works

heres an example for the right way to use this

TRY 
	DWORD addr = 2;
	addr = *(DWORD*)addr;
	log("%d", addr);
EXCEPT
	log("wow, an exception occured, how suprising");
END


*/

#include <map>
#include <vector>
#include <windows.h>
#include <winnt.h>

void __stdcall ___log(const char* msg);
void __stdcall log(const char* format, ...);

extern std::map<DWORD, std::vector<DWORD>> exceptHandlerAddrs;

__attribute__((noinline, cdecl)) EXCEPTION_DISPOSITION __cdecl except_handler(struct _EXCEPTION_RECORD * ExceptionRecord, void * EstablisherFrame, struct _CONTEXT * ContextRecord, void * DispatcherContext);

#define TRY \
	NT_TIB* tib = (NT_TIB*)NtCurrentTeb(); \
	EXCEPTION_REGISTRATION_RECORD excReg; \
	excReg.Next = tib->ExceptionList; \
	excReg.Handler = (PEXCEPTION_ROUTINE)except_handler; \
	tib->ExceptionList = &excReg; \
	if(!exceptHandlerAddrs.contains(GetCurrentThreadId())) {  \
		exceptHandlerAddrs.insert({GetCurrentThreadId(), std::vector<DWORD>()}); \
	} \
	exceptHandlerAddrs[GetCurrentThreadId()].push_back((DWORD)&&EXCEPTLABEL);
    
// this weirdness is because the goto would be optimized out 
#define EXCEPT \
	__asm__ goto ("jmp %l0"::::ENDLABEL); \
	EXCEPTLABEL:
	
#define END \
	ENDLABEL: \
	tib->ExceptionList = excReg.Next; \
	exceptHandlerAddrs[GetCurrentThreadId()].pop_back();

