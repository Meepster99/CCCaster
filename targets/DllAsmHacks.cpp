#include "DllAsmHacks.hpp"
#include "Messages.hpp"
#include "DllNetplayManager.hpp"
#include "CharacterSelect.hpp"
#include "Logger.hpp"
#include "DllTrialManager.hpp"
#include "DllDirectX.hpp"

#include "palettes/palettes.hpp"

#include <windows.h>
#include <d3dx9.h>
#include <fstream>
#include <vector>
#include <iterator>
#include <regex>
#include <stdexcept>
#include <eh.h>

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <tchar.h>

#include <winnt.h>

#include <excpt.h>

// this might need to be stdcall? might not???
//__attribute__((noinline, optimize("O0"), stdcall)) long ehandler(EXCEPTION_POINTERS *pointers);
__attribute__((noinline, naked)) long ehandler(EXCEPTION_POINTERS *pointers);

extern "C" {
    DWORD isAddrValidESP;
    DWORD isAddrValidRET;

    DWORD isAddrValidFS;
    DWORD isAddrDeref;
}

bool tempTestFirstRun = false;

__attribute__((noinline)) void tempTest() {

    Sleep(16);

    log("execHandler ran?!");
    
    return;

    if(tempTestFirstRun) {
        while(true) {}
    }
    tempTestFirstRun = true;

    for(int i=0; i<8; i++) {
        log("[esp + %02X] %08X %08X", i * 4, isAddrValidESP + (4*i), *(DWORD*)(isAddrValidESP + (4*i)));
    }
    

}

extern "C" {
    __attribute__((naked, noinline)) void isAddrValid_ExecHandler(void* unknown);
}

void isAddrValid_ExecHandler(void* unknown) {
    
    // i dont even know what if anything this should return, or how many params it take
    // vibes tell me,, the params should be 1 dword, and its stdcall

    PUSH_ALL;
    tempTest();
    POP_ALL;

    // https://learn.microsoft.com/en-us/cpp/cpp/try-except-statement?view=msvc-170
    // i want to EXCEPTION_EXECUTE_HANDLER,, i think.
    // i could,,, EXCEPTION_CONTINUE_EXECUTION,, if i moved the thing im derefing into a memory address??? maybe??
    // all vibes and everything tell me that,, theres no way that they would continue exec NOT at like,, what
    // ik it says continue_execution, but theres no way it doesnt skip the instruction which caused the error right?
    // idek anymore. EXCEPTION_EXECUTE_HANDLER is what i did in msvc, and is what ill do now 

    __asmStart R"(
        mov eax, 1; // EXCEPTION_EXECUTE_HANDLER
        ret 0x4;
    )" __asmEnd

}

__attribute__((naked, noinline, cdecl)) DWORD isAddrValid(DWORD addr) {

    /*
    
    the vibe:
    overwrite the SEH handler with my own
    do the thing
    restore SEH
    
    still super confused on how to return from the SEH handler though.

    i can use eax, ecx, and edx under cdecl

    */

    // follow 0x00416329

    __asmStart R"(
        mov edx, dword ptr [esp+4];

        push esp;
        push ebp;
        push ebx;
        push esi;
        push edi;

    )" __asmEnd

    // save FS
    //mov eax, fs:0;
    //mov _isAddrValidFS, eax;
    // write our func to the SEH thing
    //mov eax, OFFSET _isAddrValid_ExecHandler; 
    //mov fs:0, eax;
    // restore FS
    //mov eax, _isAddrValidFS;
    //mov fs:0, eax; // restore FS

    __asmStart R"(
    
        // i dont understand how this works, am i supposed to write to fs:0 or not??
        push OFFSET _isAddrValid_ExecHandler;
        push dword ptr fs:0;
        mov dword ptr fs:0, esp;

        // actual read to see if this area is readable
        //mov cl, byte ptr [edx]; 
        
        mov _isAddrDeref, edx;
        mov cl, byte ptr _isAddrDeref;

        mov eax, dword ptr [esp];
        mov fs:0, eax;
        add esp, 8;

        mov eax, edx;

    )" __asmEnd

    __asmStart R"(

        pop edi;
        pop esi;
        pop ebx;
        pop ebp;
        pop esp;

        ret;
    )" __asmEnd
}


__attribute__((noinline, optimize("O0"), cdecl)) bool isAddrValid_OLD(DWORD addr) {
    
    // mingw doesnt support SEH exceptions, which is horrible
    // all of this is a result of me trying to catch access violation exceptions

    // need to get this func to work! check with godbolt.
    // my options for this are limited, and all horrible, but not having this func makes testing extremely difficult    
    // my solution? putting a try except from msvc into a dll, and calling that.
    // gendef SEHLib.dll
    // i686-w64-mingw32-dlltool -d SEHLib.def -D SEHLib.dll -l SEHLib.a
    // strip SEHLib.a
    // i686-w64-mingw32-ranlib SEHLib.a
    // i do not know why stripping and unstripping it magically had it work??
    // i wasnt actually calling the func tho
    // currently for reasons unknown to even god, this causes melty to shit itself. 
    // check event viewer
    // this route is just not it.
    // i genuinely do not understand the resulting code here, even its syntax is wrong.
    // check out https://stackoverflow.com/questions/7244645/porting-vcs-try-except-exception-stack-overflow-to-mingw
    // that solution only works for 64 bit though.
    // im going to have to write this whole thing in asm arent i
    // the SEH bs is at,,, fs:0
    /*
    
    // from the mingw source code:

    #define __try1(pHandler) \
        __asm__ __volatile__ (
            "pushl %0;
            pushl %%fs:0;
            movl %%esp,%%fs:0;
            " : : "g" (pHandler));

    #define	__except1	\
        __asm__ __volatile__ (
        "movl (%%esp),%%eax;
        movl %%eax,%%fs:0;
        addl $8,%%esp;" \
        : : : "%eax");
    
    */

    __asmStart R"(
        mov _isAddrValidESP, esp;
        mov esp, [esp]; // using esp as scratch sapce
        mov _isAddrValidRET, esp;
        mov esp, _isAddrValidESP;
    )" __asmEnd

    tempTest();
    tempTestFirstRun = false;
    
    bool res = true;

    //log("isAddrValid called addr is %08X ret: %08X", (DWORD)isAddrValid, *(DWORD*)isAddrValidESP);
    log("isAddrValid ret: %08X", isAddrValidRET);

    __try1 (ehandler) { 
        volatile BYTE temp = *(BYTE*)addr;
        
        goto isAddrValidContinue; // straight up, does it just FALLTHROUGH HERE??? what fucked up assembly is being created here
    } __except1 {
        // i dont think this area of code is ever even execed
        log("exception occured and we caught it!");
        res = false;
    }

    isAddrValidContinue:

    log("isaddrvalid returning %d", res);


    return res;
}

long ehandler(EXCEPTION_POINTERS *pointers) { 

    // ok so,, for reasons unknown, pointers->ExceptionRecord->ExceptionCod gens an exception itself!!!
    // im just going to ret EXCEPTION_EXECUTE_HANDLER;
    // the callback was fucking looping on itself
    // i dont even understand what the fuck the try1 and except1 things are doing
    // at this point, i know that being in here means that shit is,,, bad
    // i just need to get out
    // random assumptions:
    // isaddrvalid is cdecl, the caller will clean that shit up. issue, what the fuck is this func.
    // the asm from try1 and except1 seem to push/pop 2 things onto the stack?
    // what about the 1 thing which is also in this func as a param??
    // worst case scenario, i just, save the regs and restore them

    __asmStart R"(
        mov _isAddrValidESP, esp;
    )" __asmEnd

    PUSH_ALL;
    tempTest();
    POP_ALL;

    __asmStart R"(
        add esp, 0xC; // complete guess
        mov eax, _isAddrValidRET;
        mov [esp], eax;
        mov eax, 0; // actual return value
        ret;
    )" __asmEnd

}


static int memwrite ( void *dst, const void *src, size_t len )
{
    DWORD old, tmp;

    if ( ! VirtualProtect ( dst, len, PAGE_READWRITE, &old ) )
        return GetLastError();

    memcpy ( dst, src, len );

    if ( ! VirtualProtect ( dst, len, old, &tmp ) )
        return GetLastError();

    return 0;
}


namespace AsmHacks
{

uint32_t currentMenuIndex = 0;

uint32_t menuConfirmState = 0;

uint32_t roundStartCounter = 0;

char *replayName = 0;

uint32_t *autoReplaySaveStatePtr = 0;

uint8_t enableEscapeToExit = true;

uint8_t sfxFilterArray[CC_SFX_ARRAY_LEN] = { 0 };

uint8_t sfxMuteArray[CC_SFX_ARRAY_LEN] = { 0 };

uint32_t numLoadedColors = 0;


// The team order is always (initial) point character first
static unordered_map<uint32_t, pair<uint32_t, uint32_t>> teamOrders =
{
    {  4, {  5,  6 } }, // Maids -> Hisui, Kohaku
    { 34, { 14, 20 } }, // NekoMech -> M.Hisui, Neko
    { 35, {  6, 14 } }, // KohaMech -> Kohaku, M.Hisui
};

extern "C" void charaSelectColorCb()
{
    uint32_t *edi;

    asm ( "movl %%edi,%0" : "=r" ( edi ) );

    Sleep ( 20 ); // This is code that was replaced

    uint32_t *ptrBase = ( uint32_t * ) 0x74D808;

    if ( ! *ptrBase )
        return;

    uint32_t *ptr1     = ( uint32_t * ) ( *ptrBase + 0x1AC ); // P1 color table reference
    uint32_t *partner1 = ( uint32_t * ) ( *ptrBase + 0x1B8 ); // P1 partner color table reference
    uint32_t *ptr2     = ( uint32_t * ) ( *ptrBase + 0x388 ); // P2 color table reference
    uint32_t *partner2 = ( uint32_t * ) ( *ptrBase + 0x394 ); // P2 partner color table reference

    LOG ( "edi=%08X; ptr1=%08X; partner1=%08X; ptr2=%08X; partner2=%08X", edi, ptr1, partner1, ptr2, partner2 );

    const uint32_t chara1 = *CC_P1_CHARACTER_ADDR;
    const uint32_t chara2 = *CC_P2_CHARACTER_ADDR;

    const auto& team1 = teamOrders.find ( chara1 );
    const auto& team2 = teamOrders.find ( chara2 );

    const bool hasTeam1 = ( team1 != teamOrders.end() );
    const bool hasTeam2 = ( team2 != teamOrders.end() );

    if ( edi + 1 == ptr1 && *ptr1 )
    {
        colorLoadCallback ( 1, ( hasTeam1 ? team1->second.first : chara1 ), ( ( uint32_t * ) *ptr1 ) + 1 );
    }
    else if ( edi + 1 == ptr2 && *ptr2 )
    {
        colorLoadCallback ( 2, ( hasTeam2 ? team2->second.first : chara2 ), ( ( uint32_t * ) *ptr2 ) + 1 );
    }
    else if ( edi + 1 == partner1 && *partner1 )
    {
        colorLoadCallback ( 1, ( hasTeam1 ? team1->second.second : chara1 ), ( ( uint32_t * ) *partner1 ) + 1 );
    }
    else if ( edi + 1 == partner2 && *partner2 )
    {
        colorLoadCallback ( 2, ( hasTeam2 ? team2->second.second : chara2 ), ( ( uint32_t * ) *partner2 ) + 1 );
    }
}

static void loadingStateColorCb2 ( uint32_t *singlePaletteData )
{
    const uint32_t chara1 = *CC_P1_CHARACTER_ADDR;
    const uint32_t chara2 = *CC_P2_CHARACTER_ADDR;

    const auto& team1 = teamOrders.find ( chara1 );
    const auto& team2 = teamOrders.find ( chara2 );

    const bool hasTeam1 = ( team1 != teamOrders.end() );
    const bool hasTeam2 = ( team2 != teamOrders.end() );

    if ( hasTeam1 || hasTeam2 )
    {
        uint32_t player = ( numLoadedColors % 2 ) + 1;

        if ( ! hasTeam1 && hasTeam2 )
            player = ( numLoadedColors < 1 ? 1 : 2 );

        uint32_t chara = ( player == 1 ? chara1 : chara2 );

        if ( hasTeam1 && player == 1 )
            chara = ( numLoadedColors < 2 ? team1->second.first : team1->second.second );
        else if ( hasTeam2 && player == 2 )
            chara = ( numLoadedColors < 2 ? team2->second.first : team2->second.second );

        colorLoadCallback (
            player,
            chara,
            * ( player == 1 ? CC_P1_COLOR_SELECTOR_ADDR : CC_P2_COLOR_SELECTOR_ADDR ),
            singlePaletteData );
    }
    else if ( numLoadedColors < 2 )
    {
        colorLoadCallback (
            numLoadedColors + 1,
            ( numLoadedColors == 0 ? chara1 : chara2 ),
            * ( numLoadedColors == 0 ? CC_P1_COLOR_SELECTOR_ADDR : CC_P2_COLOR_SELECTOR_ADDR ),
            singlePaletteData );
    }

    ++numLoadedColors;
}

extern "C" void saveReplayCb()
{
    //netManPtr->exportInputs();
}

extern "C" void loadingStateColorCb()
{
    uint32_t *ebx, *esi;

    asm ( "movl %%ebx,%0" : "=r" ( ebx ) );
    asm ( "movl %%esi,%0" : "=r" ( esi ) );

    uint32_t *ptr = ( uint32_t * ) ( ( uint32_t ( esi ) << 10 ) + uint32_t ( ebx ) + 4 );

    LOG ( "ebx=%08X; esi=%08X; ptr=%08X", ebx, esi, ptr );

    loadingStateColorCb2 ( ptr );
}
extern "C" void (*drawInputHistory) () = (void(*)()) 0x479460;

extern "C" int CallDrawText ( int width, int height, int xAddr, int yAddr, char* text, int textAlpha, int textShade, int textShade2, void* addr, int spacing, int layer, char* out );
/*
      A ------- B
      |         |
      |         |
      C --------D
*/
extern "C" int CallDrawRect ( int screenXAddr, int screenYAddr, int width, int height, int A, int B, int C, int D, int layer );
extern "C" int CallDrawSprite ( int spriteWidth, int dxdevice, int texAddr, int screenXAddr, int screenYAddr, int spriteHeight, int texXAddr, int texYAddr, int texXSize, int texYSize, int flags, int unk, int layer );

extern "C" void renderCallback();
// ARGB
extern "C" void addExtraDrawCallsCb() {
    renderCallback();

    //inputDisplay
    /*
    *(int*) 0x5585f8 = 0x1;
    drawInputHistory();
    *(int*) 0x55df0f = 0x1;
    drawInputHistory();
    *(int*) 0x55df0f = 0x0;
    */
}

extern "C" int loadTextureFromMemory( char* imgbuf1, int img1size, char* imgbuf2, int img2size, int param4 );

extern "C" void addExtraTexturesCb() {
    //MessageBoxA(0, "a", "a", 0);
    string filename = ".//GRP//arrows.png";
    string filename3 = ".//GRP//inputs.png";
    ifstream input( filename.c_str(), ios::binary );
    vector<char> buffer( istreambuf_iterator<char>(input), {} );
    int imgsize = buffer.size();
    char* rawimg = &buffer[0];
    ifstream input3( filename3.c_str(), ios::binary );
    vector<char> buffer3( istreambuf_iterator<char>(input3), {} );
    int imgsize3 = buffer3.size();
    char* rawimg3 = &buffer3[0];
    TrialManager::trialBGTextures = loadTextureFromMemory(rawimg, imgsize, 0, 0, 0);
    TrialManager::trialInputTextures = loadTextureFromMemory(rawimg3, imgsize3, 0, 0, 0);
}

int Asm::write() const
{
    backup.resize ( bytes.size() );
    memcpy ( &backup[0], addr, backup.size() );
    return memwrite ( addr, &bytes[0], bytes.size() );
}

int Asm::revert() const
{
    return memwrite ( addr, &backup[0], backup.size() );
}

extern "C" {
    DWORD naked_charTurnAroundStateRes = 0;
    DWORD naked_charTurnAroundParam1 = 0;
    DWORD naked_charTurnAroundParam2 = 0;
    DWORD naked_charTurnAroundState[4] = { 0, 0, 1, 1 };
    DWORD charTurnAround_ECX;
    DWORD charTurnAround_EBP;
    void charTurnAround();
}

void battleResetCallback() {

    for ( const Asm& hack : patch2v2 )
        WRITE_ASM_HACK ( hack );

    //log("BATTLERESETCALLBACK");

    naked_charTurnAroundState[0] = 0;
    naked_charTurnAroundState[1] = 0;
    naked_charTurnAroundState[2] = 1;
    naked_charTurnAroundState[3] = 1;

}

void _naked_battleResetCallback() {

    PUSH_ALL;
    battleResetCallback();
    POP_ALL;

    __asmStart R"(
        ret 0x4;
    )" __asmEnd

}

extern "C" {
    unsigned naked_fileLoadEBX = 0; // weird that extern c isnt needed in the palettetests branch, did updating the c++ ver do something?
} 

void tempLog(const std::string& s) {
    std::ofstream outfile("log.txt", std::ios_base::app);
    outfile << s << "\n";
}

void fileLoadHook() {

    // while nice, regex might not be the best/fastest way to go about this

    std::string loadString = std::string((char*)naked_fileLoadEBX); // does this copy the string?

    tempLog(loadString);

    // some weird loads to look out for
    if(loadString == R"(.\data\vector.txt)") { 
        return;
    }

    const std::regex fullStringPattern(R"((\.\\.+\\)(.+)\.(.+))");
    std::smatch matches;

    if (!std::regex_search(loadString, matches, fullStringPattern)) {
        tempLog("failed first reg search");
        return;
    }

    if(matches[1] != ".\\data\\") {
        tempLog("failed match 1");
        return;
    }
    
    bool isText = matches[3] == "txt";
    bool isPalette = matches[3] == "pal";

    if(!isText && !isPalette) {
        tempLog("failed match 3");
        return;
    }

    std::string newCharName = "WARC";
    int newCharMoon = 2;
    
    std::string res;

    if(isPalette) {
        res = matches[1].str() + newCharName + "." + matches[3].str();
    } else if(isText) {
        // lots of assumptions here. might not work
        res = matches[1].str() + newCharName + "_" + std::to_string(newCharMoon) + "_c.txt";
    }

    char* tempChar = (char*)naked_fileLoadEBX; // things were being weird with strncpy
    for(char c : res) {
        *tempChar++ = c;
    }
    *tempChar = '\0';

    tempLog("SUCCESS: " + res);

}

void _naked_fileLoad() {

    // patched in at 0041f7c0

    __asmStart R"(
        mov _naked_fileLoadEBX, ebx;
    )" __asmEnd

    PUSH_ALL;
    fileLoadHook();
    POP_ALL;

    // overwritten instructions
    __asmStart R"(
        push ebp;
        mov ebp, esp; // this instr generates with different bytecode than vanilla! hould be ok tho?
        and esp, 0xfffffff8
    )" __asmEnd

    emitJump(0x0041f7c6);

}

void charTurnAround() {

    BYTE al;

    BYTE a = *(BYTE*)(charTurnAround_ECX + 0x000001EC);
    BYTE b = *(BYTE*)(charTurnAround_EBP + 0x000002F0);

    al = a;
    al ^= b;
    al &= 1;

    //naked_charTurnAroundStateRes = 1;
    //return;

    if(al == 0) { // this face check is friendly. dip early.
        naked_charTurnAroundStateRes = 0;
        return;
    }

    log("%d %d", a, b);

    /*
    naked_charTurnAroundStateRes = 0;

    if(a == 1 && b == 3) {
        naked_charTurnAroundStateRes = 1;
    } else if(a == 0 && b == 2) {
        naked_charTurnAroundStateRes = 1;
    }

    */

    if(a > b) {
        std::swap(a, b);
    }

    
    if(a & 1) {
        if(naked_charTurnAroundState[a] && b == 2) {
            naked_charTurnAroundStateRes = 1;
        } else if(!naked_charTurnAroundState[a] && b == 0) {
            naked_charTurnAroundStateRes = 1;
        }
    } else {
        if(naked_charTurnAroundState[a] && b == 3) {
            naked_charTurnAroundStateRes = 1;
        } else if(!naked_charTurnAroundState[a] && b == 1) {
            naked_charTurnAroundStateRes = 1;
        }
    }
    
}

void charTurnAround2() {

    //log("%08X %08X", naked_charTurnAroundParam1, naked_charTurnAroundParam2);

    if(naked_charTurnAroundParam1 == 0) {
        return;
    }

    BYTE owner = *(BYTE*)(naked_charTurnAroundParam1 + 0x2F0);

    int goal = naked_charTurnAroundState[owner];
    goal <<= 1;
    if((owner & 1) == 0) {
        goal += 1;
    }

    int ourXPos = *(int*)(naked_charTurnAroundParam1 + 0x104);
    int otherXPos = *(int*)(0x00555130 + 0x4 + 0x104 + (goal * 0xAFC));
    
    if(otherXPos != ourXPos) { // this,,, might fix some issues in corner? should this comparison also check if the xval is actually in the corner?
        *(BYTE*)(naked_charTurnAroundParam1 + 0x311) = otherXPos < ourXPos;
    }
    
    naked_charTurnAroundStateRes = 0;

}

void _naked_charTurnAround2() {

    __asmStart R"(

        mov eax, [esp + 4];
        mov _naked_charTurnAroundParam1, eax;

        mov eax, [esp + 8];
        mov _naked_charTurnAroundParam2, eax;
    
    )" __asmEnd

    PUSH_ALL;
    charTurnAround2();
    POP_ALL;

    __asmStart R"(
        mov eax, _naked_charTurnAroundStateRes;
    )" __asmEnd

    ASMRET;
}

void _naked_charTurnAround() {

    // patched in at  0x0047587b
    // need to ret to 0x00475887

    __asmStart R"(
        mov _charTurnAround_ECX, ecx;
        mov _charTurnAround_EBP, ebp;
    )" __asmEnd

    PUSH_ALL;
    charTurnAround();
    POP_ALL;

    __asmStart R"(
        mov eax, _naked_charTurnAroundStateRes;
    )" __asmEnd

    emitJump(0x00475887);

}

void _naked_hitBoxConnect1() { 

    // patch at 0046F20D

    __asmStart R"(

        mov al, [ebx + 0x000002F0];
        xor al, [edi + 0x000002F0];
        and al, 0x01;
        test al, al;

        // mov al, [ebx + 0x000002F0];
        // cmp al, [edi + 0x000002F0];

    )" __asmEnd
    

    emitJump(0x0046f213);
}

void _naked_hitBoxConnect2() { 

    // patch at 00468127

    __asmStart R"(

        mov al, [ebx + 0x2F0];
        xor al, [ebp - 0x02C];
        and al, 0x01;
        test al, al;

        // mov al, byte ptr [ebx + 0x2F0];
        // cmp al, byte ptr [ebp - 0x02C];

    )" __asmEnd
    

    emitJump(0x00468130);
}

void _naked_hitBoxConnect3() {

    // patch at 0x0046f67e

    __asmStart R"(

        mov dl, [ebx + 0x2F0];
        xor dl, [ebp + 0x030];
        and dl, 0x01;
        test dl, dl;

        // mov dl, byte ptr [ebx + 0x2F0];
        // cmp dl, byte ptr [ebp + 0x030];

    )" __asmEnd
    

    emitJump(0x0046f687);

}

void _naked_throwConnect1() {

    // patch at 0x004641b2

    __asmStart R"(

        mov al, [esi + 0x2f0];
        xor al, [edi + 0x2f0];
        and al, 0x01;
        test al, al;

        // mov AL,byte ptr [ESI + 0x2f0]
        // cmp byte ptr [EDI + 0x2f0]

    )" __asmEnd
    

    emitJump(0x004641be);

}

void _naked_throwConnect2() {

    // patch at 0x0046fa65

    __asmStart R"(

        mov al, [esi + 0x2f0];
        xor al, [edi + 0x2f0];
        and al, 0x01;
        test al, al;

    )" __asmEnd
    

    emitJump(0x0046fa71);

}

void _naked_proxyGuard() {

    // patch at 00462b87

    __asmStart R"(

        mov al, [edx + 0x2F0];
        xor al, [esi + 0x2F0];
        and al, 0x01;
        test al, al;

    )" __asmEnd

    emitJump(0x00462b93);
    
}

void _naked_dashThrough() {

    // patch at 0046e8c9

    __asmStart R"(

        mov al, [edi + 0x2F0];
        xor al, [ebx + 0x2F0];
        and al, 0x01;
        test al, al;

    )" __asmEnd

    emitJump(0x0046e8d5);

}

void _naked_collisionConnect() {

    // patch at 0046ea27

    __asmStart R"(
        push eax;
        push edi;

        sar eax, 2;
        add eax, 2;
        and eax, 3;
        xor eax, 1;

        sar edi, 2;
        //add edi, 2;
        and edi, 3;
        xor edi, 1;

        cmp eax, edi;

        pop edi;
        pop eax;
    )" __asmEnd

    emitJump(0x0046ea33);

}

void _naked_checkRoundDone() {

    // patched at 0x0047463c 

    emitCall(0x004735e0); // do the original func

    __asmStart R"(
        push ebx;

        // commenting this out disables the timer. which is technically a bugfix?
        //mov ebx, [0x00562A3C]; // timer check
        //cmp ebx, 0;
        //JLE FAIL;

        // something in here should check for double KO

        mov bl, byte ptr [0x005552A8 + (0 * 0xAFC)]; // P0 
        add bl, byte ptr [0x005552A8 + (2 * 0xAFC)]; // P2 
        cmp bl, 2;
        JGE FAIL;

        mov bl, byte ptr [0x005552A8 + (1 * 0xAFC)]; // P1 
        add bl, byte ptr [0x005552A8 + (3 * 0xAFC)]; // P3 
        cmp bl, 2;
        JGE FAIL;

        mov eax, 0; // OK        
        checkrounddoneEND:
        pop ebx;     
        
        push 0x00474641; // jump to resume
        ret;

        FAIL:
        mov eax, 1; // end the round
        JMP checkrounddoneEND;
    )" __asmEnd
}

void _naked_checkRoundDone2() {

    // patch at 0x004735ed

    __asmStart R"(
        
        // edi is already xored, just use that

        mov [esp + 0x10], edi;
        mov [esp + 0x14], edi;
        mov [esp + 0x18], edi;
        mov [esp + 0x1C], edi;

        mov [esp + 0x20], edi;
        mov [esp + 0x24], edi;
        mov [esp + 0x28], edi;
        mov [esp + 0x2C], edi;
    
    )" __asmEnd

    emitJump(0x004735fd); 

}

void _naked_checkWhoWon() {

    __asmStart R"(
    
        push ebx;
        xor eax, eax;

        mov ebx, [0x005551EC + (1 * 0xAFC)]; // P1 HEALTH
        add ebx, [0x005551EC + (3 * 0xAFC)]; // P3 HEALTH
        cmp ebx, 0;
        JLE TEAM0WON;

        mov eax, 1;

        TEAM0WON:
        
        pop ebx;
        ret;
    )" __asmEnd

}   

void cameraMod() {

    float* zoom = (float*)0x0054eb70;

    *zoom = (*zoom * 0.75f); // i reallllly hope this var is regenerated every frame so this doesnt go to 0 :3

}

void _naked_cameraMod() {

    PUSH_ALL;
    cameraMod();
    POP_ALL;

    ASMRET;
}

// -----

void _naked_drawWinCount() {

    __asmStart R"(

        // i *think* i can use eax here.
        mov eax, 0x80;
        cmp [esp + 0], eax;
        JLE LESS;

        mov eax, 0x20;
        add [esp + 0], eax;

        JMP RESUME;
        LESS:

        mov eax, 0x20;
        sub [esp + 0], eax;

        RESUME:
    
        mov eax, 0xFFFFFFFE;
        mov [esp + 4], eax;

    )" __asmEnd

    emitCall(0x0041dbf0); // original func 

    emitJump(0x00426c6c); // resume exec
}

void _naked_drawRoundDots() {

    __asmStart R"(
        mov eax, 10;
        sub [esp + 0xC], eax; // yVal
        mov eax, 0;
        sub [esp + 0x8], eax; // xVal
        
    )" __asmEnd

    emitCall(0x00415580);

    emitJump(0x00424951); 
}

// -----

void _naked_cssTest() {

    NOPS
    NOPS
    NOPS
    NOPS
    NOPS
    NOPS


    emitCall(0x00485F40);
    
    emitJump(0x00485ce3);
}

void _naked_modifyCSSPixelDraw() {

    // patched at 0x0048aad6

    /*/ weird shits occuring in here:
    you have access to the following(i think, so far)

    eax is xPos
    esp+4 has yPos
    ebp is char Index
    need to find the ypos too (maybe, not sure)

    if visual studio messes up block indentation one more time i will freak
    */

    __asmStart R"(

        // not the fastest, but its quick and easy

        cmp ebp, 0;
        JE ZERO;

        cmp ebp, 1;
        JE ONE;

        cmp ebp, 2;
        JE TWO;        

        // THREE:
        mov eax, 300;
        mov [esp + 4], eax;
        mov eax, (640 - 60);
        JMP csspixeldrawEND;

        ONE:
        mov eax, 448;
        mov [esp + 4], eax;
        mov eax, (640 - 60 - 80);
        JMP csspixeldrawEND;

        ZERO:
        mov eax, 448;
        mov [esp + 4], eax;
        mov eax, (60 + 80);
        JMP csspixeldrawEND;

        TWO:
        mov eax, 300;
        mov [esp + 4], eax;
        mov eax, (60);

        csspixeldrawEND:

        //imul eax, ebp, 0x40;
    )" __asmEnd

    emitCall(0x00485b80); // original func

    emitJump(0x0048aadb);

}

extern "C" {
    DWORD drawTextureLoggerESP;
}  

void drawTextureLogger() {

    DWORD ret = *(DWORD*)(drawTextureLoggerESP + 0x0);
    DWORD tex = *(DWORD*)(drawTextureLoggerESP + 0x8);
    
    log("DrawTexture called from %08X tex: %08X", ret, tex);

}

void _naked_drawTextureLogger() {
    
    // patched at 00415580

    __asmStart R"(
        mov _drawTextureLoggerESP, esp;
    )" __asmEnd

    PUSH_ALL;
    drawTextureLogger();
    POP_ALL;

    // overwritten asm
    __asmStart R"(
        push ebp;
        mov ebp, esp;
        and esp,0xfffffff8;
    )" __asmEnd

    emitJump(0x00415586);

}

void _naked_CSSConfirmExtend() {

    // patch at 0x00427563

    NOPS
    NOPS
    NOPS
    NOPS
    NOPS
    NOPS

    emitJump(0x00427588);
}

void _naked_CSSWaitToSelectStage() {

    // 0042794c


    __asmStart R"(

        // weirdly, should be 6, not 5 if we are heading to stage sel
        // i can use eax and edi freely
        // i do not know if [0x12345678] or offset [0x12345678] is needed

        push ebx;

        mov eax, 6;
        mov ebx, 0x0074D8EC;

    naked_CSSWaitToSelectStage_LOOP:
        cmp ebx, (0x0074D8EC + (4 * 0x24));
        JG naked_CSSWaitToSelectStage_END;

        mov edi, [ebx];
        cmp edi, 3
        JGE naked_CSSWaitToSelectStage_CONT;

        cmp edi, eax;
        JGE naked_CSSWaitToSelectStage_CONT;

        mov eax, edi;

    naked_CSSWaitToSelectStage_CONT:

        add ebx, 0x24;
        jmp naked_CSSWaitToSelectStage_LOOP;

    naked_CSSWaitToSelectStage_END:
        pop ebx;

        xor edi, edi;
        mov [esp + 8], eax;

    )" __asmEnd

    emitJump(0x00427972);

}

void _naked_cssInit() {

    // patch at 0048cb4b

    PUSH_ALL;
    cssInit();
    POP_ALL;

    __asmStart R"(
        pop ebx;
        pop ecx;
        ret;
    )" __asmEnd
}

void _naked_stageSelCallback() {

    // patched at 004888e2 and 004888f0

    __asmStart R"(
        mov _stageSelEAX, eax;
    )" __asmEnd

    PUSH_ALL;
    stageSelCallback();
    POP_ALL;

    __asmStart R"(
        
        mov eax, _stageSelEAX;
        
        pop ebx;
        add esp, 0x8;
        ret;
    )" __asmEnd

}

extern "C" {
    DWORD paletteEAX = 0;
    DWORD paletteEBX = 0;
}

int getIndexFromCharName(const std::string& name) {

    std::map<std::string, int> lookup = {
        {"SION",0},
        {"ARC",1},
        {"CIEL",2},
        {"AKIHA",3},
        {"HISUI",5},
        {"KOHAKU",6},
        {"SHIKI",7},
        {"MIYAKO",8},
        {"WARAKIA",9},
        {"NERO",10},
        {"V_SION",11},
        {"WARC",12},
        {"AKAAKIHA",13},
        {"M_HISUI",14},
        {"NANAYA",15},
        {"SATSUKI",17},
        {"LEN",18},
        {"P_CIEL",19},
        {"NECO",20},
        {"AOKO",22},
        {"WLEN",23},
        {"NECHAOS",25},
        {"KISHIMA",28},
        {"S_AKIHA",29},
        {"RIES",30},
        {"ROA",31},
        {"RYOUGI",33},
        {"P_ARC",51},
        {"P_ARC_D",-1}, // i remember having some issues with P_ARC_D,, not doing it
    };

    if(!lookup.contains(name)) {
        log("couldnt find \"%s\"", name.c_str());
        return -1; 
    }

    return lookup[name];
}

void palettePatcher() {

    if(paletteEBX == 0) {
        return;
    }

    char ebxBuffer[256];
    strncpy(ebxBuffer, (char*)paletteEBX, 256);

    std::string ebx(ebxBuffer);
    if(ebx.substr(MAX(0, ebx.size() - 4)) != ".pal") {
        return;
    }

    size_t lastBackslash = ebx.find_last_of('\\');
    if(lastBackslash == std::string::npos) {
        return;
    }
    std::string charName = ebx.substr(lastBackslash+1, ebx.size() - (lastBackslash+1) - 4);

    int charIndex = getIndexFromCharName(charName);
    
    if(charIndex == -1) {
        return;
    }

    DWORD* colors = (DWORD*)(paletteEAX + 4);

    if(palettes.contains(charIndex)) {
        for(auto it = palettes[charIndex].begin(); it != palettes[charIndex].end(); ++it) {
            memcpy(&colors[256 * it->first], (it->second).data(), sizeof(DWORD) * 256); 
        }
    }

}

void _naked_paletteHook() {

    // patched at 0x0041f7c0

    __asmStart R"(
        mov _paletteEBX, ebx;
    )" __asmEnd

    // overridden bytes
    __asmStart R"(
        push ebp;
        mov ebp, esp;
        and esp, 0xfffffff8;
    )" __asmEnd

    emitJump(0x0041f7c6);
}

void _naked_paletteCallback() {

    // patched at 0x0041f87a

    __asmStart R"(
      mov _paletteEAX, eax;
    )" __asmEnd

    PUSH_ALL;
    palettePatcher();
    POP_ALL;

    ASMRET;
}

// -----

extern "C" {
    DWORD getLinkedListElementCallbackEAX;
}

typedef struct MeltyVert {
	float x;
	float y;
	float z;
	float w;
	DWORD diffuse; 
	DWORD specular; 
	float u;
	float v;
} MeltyVert;

typedef struct __attribute__((packed)) LinkedListDataElement {

    LinkedListDataElement* next;
    LinkedListDataElement* prev;

    union {
        MeltyVert verts[4];
        struct {
            MeltyVert v0;
            MeltyVert v1;
            MeltyVert v2;
            MeltyVert v3;
        };
    };

    IDirect3DTexture9* tex;

    DWORD unk1;
    DWORD unk2;

    DWORD miscCounter;
    DWORD misc1;
    DWORD misc2;

} LinkedListDataElement;

static_assert(sizeof(LinkedListDataElement) == 0xA0, "LinkedListDataElement must be size 0xA0");

void getLinkedListElementCallback() {

    DWORD res;

    log("this should be   valid");
    res = isAddrValid(0x00555130);
    log("\t%08X", res);

    log("this should be invalid");
    res = isAddrValid(0);
    log("\t%08X", res);

    return;

    //DWORD* eax = (DWORD*)getLinkedListElementCallbackEAX;
    //eax[0x94 / 4]++;

    LinkedListDataElement* elem = (LinkedListDataElement*)getLinkedListElementCallbackEAX;

    log("linkListCallback");
    if(!isAddrValid((DWORD)elem)) {
        return;
    }

    elem->miscCounter++;

    for(int i=0; i<4; i++) {
        elem->verts[i].diffuse = 0xFFFFFFFF;
        elem->verts[i].specular = 0xFFFFFFFF;
    }

}

void _naked_getLinkedListElementCallback() {

    // patched at 0x00416329

    __asmStart R"(
        mov _getLinkedListElementCallbackEAX, eax;
    )" __asmEnd

    PUSH_ALL;
    getLinkedListElementCallback();
    POP_ALL;

    ASMRET

}

void modifyLinkedList() {

    return;

    DWORD addr = 0x005550A8;
    addr = *(DWORD*)(addr + 8);

    if(*(DWORD*)(addr) != 1) {
        log("%08X was not 1, returning", addr);
        return;
    }

    LinkedListDataElement* elem = (LinkedListDataElement*)*(DWORD*)(addr + 4);

    int index = 0;

    while(isAddrValid((DWORD)elem)) {

        log("%d %08X", index, (DWORD)elem);

        for(int i=0; i<4; i++) {
            elem->verts[i].diffuse = 0x8000FF00;
            elem->verts[i].specular = 0x8000FF00;
        }


        elem = elem->next;
        index++;

        if(index > 1000) {
            break;
        }
    }

}

void _naked_modifyLinkedList() {

    // patched at 00433302

    PUSH_ALL;
    modifyLinkedList();
    POP_ALL;

    __asmStart R"(
        ret 0x4;
    )" __asmEnd

}

} // namespace AsmHacks
