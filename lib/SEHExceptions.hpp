
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


typedef struct CPUState {
	
	DWORD EAX;
	DWORD EBX;
	DWORD ECX;
	DWORD EDX;
	
	DWORD ESI;
	DWORD EDI;
	
	DWORD ESP;
	DWORD EBP;
	
	DWORD EIP;
	
	
} CPUState;

static_assert(sizeof(CPUState) == 36, "cpustate must be size 36/0x24");

__attribute__((noinline, naked, cdecl)) CPUState* getCpuState();

extern std::map<DWORD, std::vector<CPUState*>> exceptHandlerAddrs;

__attribute__((noinline, cdecl)) EXCEPTION_DISPOSITION __cdecl except_handler(struct _EXCEPTION_RECORD * ExceptionRecord, void * EstablisherFrame, struct _CONTEXT * ContextRecord, void * DispatcherContext);

__attribute__((noinline, naked, cdecl)) unsigned getEIP();

/*
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

*/


// this solution works, but i cant nest trys without having a func call inside the first one
// also, defines,, i cant eval them, file_line
// i cant have multiple trys in the same func either.
// i could do something like TRY(n), and tbh i might do that
// annoying, but i can at least have plenty of saftey
// i cant #define things in #defines. im just going to SIU

#define TRY(ID) \
	NT_TIB* tib##ID = (NT_TIB*)NtCurrentTeb(); \
	EXCEPTION_REGISTRATION_RECORD excReg##ID; \
	excReg##ID.Next = tib##ID->ExceptionList; \
	excReg##ID.Handler = (PEXCEPTION_ROUTINE)except_handler; \
	tib##ID->ExceptionList = &excReg##ID; \
	if(!exceptHandlerAddrs.contains(GetCurrentThreadId())) {  \
		exceptHandlerAddrs.insert({GetCurrentThreadId(), std::vector<CPUState*>()}); \
	} \
    exceptHandlerAddrs[GetCurrentThreadId()].push_back(getCpuState()); \
	exceptHandlerAddrs[GetCurrentThreadId()][exceptHandlerAddrs[GetCurrentThreadId()].size() - 1]->EIP = (DWORD)(&&EXCEPTLABEL##ID); 

// this weirdness is because the goto would be optimized out 
#define EXCEPT(ID) \
	__asm__ goto ("jmp %l0"::::ENDLABEL##ID); \
	EXCEPTLABEL##ID:
	
#define END(ID) \
	ENDLABEL##ID: \
	tib##ID->ExceptionList = excReg##ID.Next; \
	free(exceptHandlerAddrs[GetCurrentThreadId()].back()); \
	exceptHandlerAddrs[GetCurrentThreadId()].pop_back(); 

// -----

#define __asmStart __asm__ __volatile__ (".intel_syntax noprefix;"); __asm__ __volatile__ (
#define __asmEnd ); __asm__ __volatile__ (".att_syntax;");

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define CLAMP(value, min_val, max_val) MAX(MIN((value), (max_val)), (min_val))
#define PUSH_ALL \
    __asm__ __volatile__( \
        "push %esp;"  \
        "push %ebp;"  \
        "push %edi;"  \
        "push %esi;"  \
        "push %edx;"  \
        "push %ecx;"  \
        "push %ebx;"  \
        "push %eax;"  \
        "push %ebp;"  \
        "mov %esp, %ebp;" \
    )
#define POP_ALL \
    __asm__ __volatile__( \
       "pop %ebp;" \
       "pop %eax;" \
       "pop %ebx;" \
       "pop %ecx;" \
       "pop %edx;" \
       "pop %esi;" \
       "pop %edi;" \
       "pop %ebp;" \
       "pop %esp;" \
    )

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)
#define LINE_STRING TO_STRING(__LINE__)

#define CONCATENATE_DETAIL(x, y) x##y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)
#define LINE_NAME "LINE" LINE_STRING

// needing offset keyword here makes me sad. so much time wasted.
#define emitCall(addr) \
    __asmStart \
    "push offset " LINE_NAME ";" \
	"push " #addr ";" \
    "ret;" \
    LINE_NAME ":" \
    __asmEnd 

#define emitJump(addr) \
    __asmStart \
	"push " #addr ";" \
    "ret;" \
    __asmEnd

#define emitByte(b) asm __volatile__ (".byte " #b);

#define NOPS __asmStart R"( nop; nop; nop; nop; nop; )" __asmEnd
