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
#include <mutex>
#include <winnt.h>

#include <excpt.h>

const char* getListDrawString(ListDrawType t) {

    const char* res = "INVALID";

    switch(t) {
        case ListDrawType::Unknown:
            res = "Unknown";
            break;
        case ListDrawType::Reset:
            res = "Reset";
            break;
        case ListDrawType::Character:
            res = "Character";
            break;
        case ListDrawType::Shadow:
            res = "Shadow";
            break;
        case ListDrawType::Effect:
            res = "Effect";
            break;
        case ListDrawType::HitEffect:
            res = "HitEffect";
            break;
        default:
            break;
    }
    
    return res;
}

const char* getListRetString(ListRetType t) {
   
    const char* res = "INVALID";

    switch(t) {
        case ListRetType::Unknown:
            res = "Unknown";
            break;
         case ListRetType::Reset:
            res = "Reset";
            break;
        case ListRetType::DrawMenuCursor:
            res = "DrawMenuCursor";
            break;
        case ListRetType::DrawEffectsTopUIandCharaSelect:
            res = "DrawEffectsTopUIandCharaSelect";
            break;
        case ListRetType::DrawHitEffect:
            res = "DrawHitEffect";
            break;
        case ListRetType::DrawBlur:
            res = "DrawBlur";
            break;
        case ListRetType::DrawCharacterSelectBackground:
            res = "DrawCharacterSelectBackground";
            break;
        case ListRetType::DrawCharactersAndBackground:
            res = "DrawCharactersAndBackground";
            break;
        case ListRetType::RenderOnScreen:
            res = "RenderOnScreen";
            break;
        default:
            break;
    }
    
    return res;
}

// this might need to be stdcall? might not???
//__attribute__((noinline, optimize("O0"), stdcall)) long ehandler(EXCEPTION_POINTERS *pointers);
__attribute__((noinline, naked)) long ehandler(EXCEPTION_POINTERS *pointers);

extern "C" {
    DWORD isAddrValidAddr;
    DWORD isAddrValidRes;
    DWORD isAddrValidTemp;
    bool insideCheckAddress = false;
}


EXCEPTION_DISPOSITION __cdecl _except_handler(struct _EXCEPTION_RECORD * ExceptionRecord, void * EstablisherFrame, struct _CONTEXT * ContextRecord, void * DispatcherContext) {
    //log("in exception handler");
    ContextRecord->Eax = (DWORD)&isAddrValidTemp;
    isAddrValidRes = 0;
    return ExceptionContinueExecution;
}

//EXCEPTION_REGISTRATION_RECORD excReg;
/*
__attribute__((noinline)) void initException() {
    static bool init = false;
    if(init) {
        return;
    }

    init = true;

    NT_TIB* tib = (NT_TIB*)NtCurrentTeb();

    excReg.Next = tib->ExceptionList;
    excReg.Handler = (PEXCEPTION_ROUTINE)_except_handler;
    tib->ExceptionList = &excReg;

    //tib->ExceptionList = excReg.Next;
}*/

__attribute__((noinline, cdecl)) void checkAddress() {

    /*
    confused? so am i! read all of this please:
        https://en.wikipedia.org/wiki/Microsoft-specific_exception_handling_mechanisms#SEH
        https://en.wikipedia.org/wiki/Win32_Thread_Information_Block
        https://limbioliong.wordpress.com/2022/01/18/understanding-windows-structured-exception-handling-part-2-digging-deeper/
        https://learn.microsoft.com/en-us/cpp/build/reference/safeseh-image-has-safe-exception-handlers?view=msvc-170
        https://groups.google.com/g/llvm-dev/c/BFmQV5X0buw

        I have done literally everything, but short of forking the compiler, or doing some EXTREMELY JANKY THINGS this is not changing.
        in order for the exception to be recognized, the data for it needs to be in 
        makes me wonder how visual studio does it in the first place
        dlls can have their own xdata, i,, ugh

        this solution at least uses MUCH less inline asm than most of my temps, if it even works
        tbh making this exception always run would be very helpful for me to track down access violations in general 
        thats assuming i can even get it to work

        it works, but it only works on inline asm. i wish i could check texture validity

        if the program sigints while in this func,, could that be whats causing my weird not closing issues?
    */

    // follow 0x00416329

    //initException();

    insideCheckAddress = true;
    isAddrValidRes = 1; // the exception handler sets this to 0 if an exception occurs.
    
    NT_TIB* tib = (NT_TIB*)NtCurrentTeb();

    // i believe this NEEDS TO BE IN THE STACK, or march needs to be at least skylake(or well,, not default?)
    // yep, it needs to be. or its a data section thing,, i
    // when global,  nm build_release_4v4/targets/DllAsmHacks.o | grep excReg --> 00000c10 B _excReg
    // i rebuilt with -fdata-sections and,,, it moved to Data,, 
    // when local,  nm build_release_4v4/targets/DllAsmHacks.o | grep excReg --> 00000c10 B _excReg
    //EXCEPTION_REGISTRATION_RECORD excReg; 
    EXCEPTION_REGISTRATION_RECORD excReg;
    excReg.Next = tib->ExceptionList;
    excReg.Handler = (PEXCEPTION_ROUTINE)_except_handler;
    tib->ExceptionList = &excReg;

    __asmStart R"(
    
        push eax;

        mov eax, _isAddrValidAddr;
        mov eax, [eax];

        pop eax;

    )" __asmEnd

    tib->ExceptionList = excReg.Next;

    insideCheckAddress = false;

}

DWORD isAddrValid(DWORD addr) {

    if(addr == 0) {
        isAddrValidRes = 0;
        return isAddrValidRes;
    }

    isAddrValidAddr = addr;
    //log("trying to call isAddrValid");
    checkAddress();
    //log("isAddrValid resolved");
    return isAddrValidRes;
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
    DWORD FN1States[4] = {0, 0, 0, 0};
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

    // really wish i commented any of this code. what was i on?

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

    //log("%d %d", a, b);

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

void _naked_updateGameStateHook() {

    // patched at 0048e0a0

    __asmStart R"(
    
        // as is tradition, this func doesnt take params, so i can push my own ret value onto the stack
        // eax SHOULD be safe to clobber, not 100% sure tho


		// looking back on this, wtf was i doing here. 
		
        mov eax, OFFSET __naked_updateGameStateCallback;
        push eax; // add our ret
    
    )" __asmEnd

    // overwritten asm

    emitByte(0x83);
    emitByte(0xEC);
    emitByte(0x1C);

    emitByte(0x53);
    emitByte(0x55);

    emitJump(0x0048e0a5);

}

void _naked_updateGameStateCallback() {

    PUSH_ALL;
    updateGameState();
    POP_ALL;

    ASMRET;
}

#include <d3d9.h> // this shouldnt be here.
extern "C" __attribute__((noinline, cdecl)) void fuckingAround(D3DPRESENT_PARAMETERS* presentParam) {

	//return;

	//presentParam->MultiSampleType = D3DMULTISAMPLE_TYPE::D3DMULTISAMPLE_4_SAMPLES;

	// this is,, weird
	// very weird
	// this effects the size of the backbuffer, but doesnt change the window. 
	// i could use this to have a higher resolution window in the same size... window frame yk
	//presentParam->BackBufferWidth *= 1;
	//presentParam->BackBufferHeight *= 2;

    log("sizeof params %d", sizeof(D3DPRESENT_PARAMETERS));

    //presentParam->PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    //presentParam->FullScreen_RefreshRateInHz = 60;

    log("refreshrate %d", presentParam->FullScreen_RefreshRateInHz);
    log("present interval %08X", presentParam->PresentationInterval);
    
	log("omfg %d %d", presentParam->BackBufferWidth, presentParam->BackBufferHeight);

	log("MultiSampleType %d", presentParam->MultiSampleType);
	log("MultiSampleQuality %d", presentParam->MultiSampleQuality);

}

void _naked_fuckingAround() {
 
	// patched at 004bdae4

	emitCall(0x004bd940);

	// esi is a pointer to D3DPRESENT_PARAMETER

	PUSH_ALL;

	__asmStart R"( 
		push esi;
		call _fuckingAround; // i love that basically nowhere tells you about this underscore
		add esp, 0x4;
	)" __asmEnd

	POP_ALL;

	emitJump(0x004bdae9);

}

void _naked_mainDrawCall() {

	// patched at 0043304b

	__asmStart R"(
	
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
	
	
	)" __asmEnd


	emitCall(0x00415580); // drawtexture call

	emitJump(0x00433050); // jump to ret

}

//extern "C" DWORD _naked_fixHitBlockDetection_jumpAddr = 0x0046654e; // shoutout gonp for doing their jumps like this. at least in inline asm, its much better

void _naked_fixHitBlockDetection() {

    // patched at 00466570
    // if this doesnt compile into the same code ill kill myself 

    __asmStart R"( 

        TEST byte ptr[edx + 0x18], 0x4;
        jnz SHOULDJUMP;
        cmp al, 0x13; // if its 3, that means we both hit and were blocked at the same time. allow the cancel since a hit occured 
        jz SHOULDJUMP;
        test al, 0x1;
        jz SHOULDJUMP;

        DONTJUMP:

    )" __asmEnd

    emitJump(0x0046657a);   

    __asmStart R"(
        SHOULDJUMP:
    )" __asmEnd
    emitJump(0x0046654e);


}

// -----

extern "C" {

    DWORD linkedListCallbackAddress = 0; // make sure to reset this to 0 after use!!
    ListDrawType linkedListDrawType = ListDrawType::Unknown;
    ListRetType linkedListRetType = ListRetType::Unknown; // not sure if im even using this

}

#ifdef NBLEEDING
static std::mutex mallocMutex; // maybe i should generalize this to,, the log func? assuming thats even whats causing the crash
static volatile int omfg = 0;
#endif 

void mallocCallback(DWORD naked_mallocHook_ret, DWORD naked_mallocHook_result, DWORD naked_mallocHook_size) {

    #ifdef NBLEEDING
    mallocMutex.lock();
    #endif

    // somehow, someway, logging in here was causing crashes. does my log func call malloc?? and like,,, not its own?? i,,, what the fuck is happening???
    // regarding the above, globals were shared between threads. doing all parameter things on stack fixed it 

    //log("%08X created %08X size %08X", naked_mallocHook_ret, naked_mallocHook_result, naked_mallocHook_size);

    std::ofstream outfile("mallocLog.txt", std::ios_base::app);

    DWORD threadID = GetCurrentThreadId();

    char buffer[256];
    snprintf(buffer, 256, "t: %04X at %08X created %08X size %08X\n", threadID, naked_mallocHook_ret, naked_mallocHook_result, naked_mallocHook_size);

    outfile << buffer;

    #ifdef NBLEEDING
    omfg++;
    mallocMutex.unlock();
    #endif
    
}

void _naked_mallocCallback() {

    __asmStart R"(
        mov ecx, [esp + 0]; // size
        mov edx, [esp + 4]; // ret
    )" __asmEnd

    PUSH_ALL; // does 9 pushes. i could go stack subtracking, or i could take an unnecessary risk and be weird with cdecl, remember that eax, ecx, and edx are caller, NOT callee saved!!!!!
    __asmStart R"(
        
        push ecx; // size
        push eax; // result
        push edx; // ret

        call _mallocCallback;

        add esp, 0xC;
    )" __asmEnd
    POP_ALL;

    __asmStart R"(
        add esp, 0x4; // this is for the duplicated size i pushed on the stack
        ret; // the return address is still on the stack!
    )" __asmEnd

}

void _naked_mallocHook() {

    // patched at 0x004e0230

    __asmStart R"(

        mov eax, [esp + 0x4]; // size
        push eax;

        mov eax, OFFSET __naked_mallocCallback;
        push eax; // add our ret

    )" __asmEnd

    // overwritten asm

    emitByte(0x55);

    emitByte(0x8B);
    emitByte(0x6C);
    emitByte(0x24);
    emitByte(0x08);

    emitJump(0x004e0235); // continue exec

}

void _naked_trackListCharacterDraw() {

    // patched at 0x0041b3c9

    __asmStart R"(
        sub edi, 4;
        mov _linkedListCallbackAddress, edi;
        add edi, 4;
        mov byte ptr _linkedListDrawType, 1; // ListDrawType::Character
    )" __asmEnd

    emitCall(0x0041a390);

    emitJump(0x0041b3ce);

}

void _naked_trackListShadowDraw() {

    // patched at 0x0041b47c

    __asmStart R"(
        sub edi, 4;
        mov _linkedListCallbackAddress, edi;
        add edi, 4;
        mov byte ptr _linkedListDrawType, 2; // ListDrawType::Shadow
    )" __asmEnd

    emitCall(0x0041a390);

    emitJump(0x0041b481);

}

void _naked_trackListEffectDraw() {

    // patched at 0045410a

    __asmStart R"(
        sub eax, 4;
        mov _linkedListCallbackAddress, eax;
        add eax, 4;
        mov byte ptr _linkedListDrawType, 3; // ListDrawType::Effect
    )" __asmEnd

    emitCall(0x00454130);

    emitJump(0x0045410f);

}

void getLinkedListElementCallback(DWORD callbackEAX, DWORD callbackESP) {

    // TODO, GLOBAL VARS FOR THINGS LIKE THIS MIGHT END UP CORRUPTING SHIT IF THREADING BECOMES AN ISSUE

    LinkedListRenderData* ren = (LinkedListRenderData*)callbackEAX;
    //elem->data.miscCounter++;

    ExtraData* data = &ren->data;
    
    data->address = 0;
    data->drawType = ListDrawType::Unknown;
    data->retType = ListRetType::Unknown;

    data->threadID = GetCurrentThreadId();
    // i could do a bunch of stack backtracking here, 
    // or i could find the draw loops and go from there?
    // one option involves a ton of patches, the other is janky as hell

    if(isAddrValid(callbackESP)) {
        DWORD ret = *(DWORD*)callbackESP;

        switch(ret) {
            case 0x00415467:
                data->retType = ListRetType::DrawMenuCursor;
                break;
            case 0x004157ac:
                data->retType = ListRetType::DrawEffectsTopUIandCharaSelect;
                break;
            case 0x00415a35:
                data->retType = ListRetType::DrawHitEffect;
                break;
            case 0x00415bbf:
                data->retType = ListRetType::DrawBlur;
                break;
            case 0x00415d03:
                data->retType = ListRetType::DrawCharacterSelectBackground;
                break;
            case 0x00416128:
                data->retType = ListRetType::DrawCharactersAndBackground;
                data->address  = linkedListCallbackAddress;
                data->drawType = linkedListDrawType;
                linkedListCallbackAddress = 0xFFFFFFFF;
                linkedListDrawType = ListDrawType::Reset;
                break;
            case 0x004164D4:
                data->retType = ListRetType::RenderOnScreen;
                break;
            default:
                log("unknown ret %08X", ret);
                break;
        }
    }

    // these SHOULD be set here, but instead,,, dude idek. im going to wait for somethinbg specific to eat them yk
    //linkedListCallbackAddress = 0;
    //linkedListDrawType = ListDrawType::Unknown;

    data->setHash();

}

void _naked_getLinkedListElementCallback() {

    // patched at 0x00416329

    __asmStart R"(
        // ecx is caller saved
        mov ecx, esp;
    )" __asmEnd

    PUSH_ALL; // a thought occurs, i am not backing up the fp stack. oh no.
    __asmStart R"(
        push ecx;
        push eax;
        call _getLinkedListElementCallback;
        add esp, 0x8;
    )" __asmEnd
    //getLinkedListElementCallback();
    POP_ALL;
    
    ASMRET

}

void _naked_getLinkedListElementHook() {

    // patched at 0x004162b0

    __asmStart R"(
        // overwrite call stack with out own return value
        push OFFSET __naked_getLinkedListElementCallback;
    )" __asmEnd 

    // overwritten asm
    __asmStart R"(
        push esi;
        mov esi, eax;
        test esi, eax;
    )" __asmEnd

    emitJump(0x004162b5); // resume exec

}

void modifyLinkedList() {

    //log("entered modifyLinkedList. pray");

    // d: FFFFFFFF s: 0000BFFF
    
    /*
    static FloatColor diffuse[4];
    static FloatColor specular[4];

    for(int i=0; i<4; i++) {
        UIManager::add("diffuse"  + std::to_string(i), &diffuse[i]);
        UIManager::add("specular" + std::to_string(i), &specular[i]);
    }
    */

    // ----- 

    static FloatColor hWarcDiffuse[4] = {
        FloatColor(1.0, 0.0, 0.0, 0.0),
        FloatColor(1.0, 0.0, 0.0, 0.0),
        FloatColor(1.0, 0.0, 0.0, 0.0),
        FloatColor(1.0, 0.0, 0.0, 0.0)
    };

    static FloatColor hWarcSpecular[4] = {
        FloatColor(0.0, 1.0, 0.0, 0.5),
        FloatColor(0.0, 0.5, 0.0, 0.25),
        FloatColor(0.0, 0.5, 0.0, 0.25),
        FloatColor(0.0, 0.25, 0.0, 0.125)
    };

    // -----

    static FloatColor cWarcSpecular[4] = {
        FloatColor(0.0, 0.0, 1.0,  1.0),
        FloatColor(0.0, 0.0, 0.75, 1.0),
        FloatColor(0.0, 0.0, 0.75, 1.0),
        FloatColor(0.0, 0.0, 0.25, 1.0)
    };

    // -----

    static FloatColor diffuse[4] = {
        FloatColor(0.0, 0.0, 0.0, 0.0),
        FloatColor(0.0, 0.0, 0.0, 0.0),
        FloatColor(0.0, 0.0, 0.0, 0.0),
        FloatColor(0.0, 0.0, 0.0, 0.0)
    };

    static FloatColor specular[4] = {
        FloatColor(0.0, 0.0, 1.0,  1.0),
        FloatColor(0.0, 0.0, 0.75, 1.0),
        FloatColor(0.0, 0.0, 0.75, 1.0),
        FloatColor(0.0, 0.0, 0.25, 1.0)
    };

    FloatColor* activeDiffuse = NULL;
    FloatColor* activeSpecular = NULL;

    for(int i=0; i<4; i++) {
        UIManager::add("diffuse"  + std::to_string(i), &diffuse[i]);
        UIManager::add("specular" + std::to_string(i), &specular[i]);
    }

    /*UIManager::add("diffuse", &diffuse[0]);
    UIManager::add("specular", &specular[0]);
    
    for(int i=1; i<4; i++) {
        diffuse[i] = diffuse[0];
        specular[i] = specular[0];
    }*/

    /*
    static int reloadCount = 0;
    reloadCount++;
    if(reloadCount > 60) {
        reloadCount = 0;
        UIManager::reload();

        //log("d: %08X s: %08X", diffuse[0].getCol(), specular[0].getCol());
    }
    */
    
    // misc scratch variables
    BYTE exists;
    BYTE source;
    DWORD pattern;
    BYTE owner;
    DWORD playerAddr;
    BYTE charID;
    BYTE moon;
    BYTE palette;

    if(*((uint8_t*) 0x0054EEE8) == 0x14) { // straight up disable this when in css
        return;
    }

	//log("start");
	
    DWORD ihatethesyntaxhighlighting;
    DWORD test;
	DWORD actual;

    DWORD addr = *(DWORD*)0x005550A8;
    LinkedListRenderList* linkedListRenderList = renderList->linkedListRenderList;

    actual = addr;
    test = (DWORD)linkedListRenderList;
    //log("actual: %08X test: %08X %d", actual, test, actual == test);

    addr = *(DWORD*)addr;
    LinkedListRenderElement* renderElem = linkedListRenderList->elements[0].nextElement; // the reason things werent working earlier was,, i was doing &(linkedListRenderList->elements[0]) 

    actual = addr;
    test = (DWORD)renderElem;
    //log("actual: %08X test: %08X %d", actual, test, actual == test);

    // idek how i was looping this previously for things to not,, work

    int index = 0;

    while(true) {

        if(!isAddrValid(addr)) {
            //log("\taddr %08X wasnt valid, breaking", addr);
            break;
        }

        if(!isAddrValid((DWORD)renderElem)) {
            //log("\taddr %08X wasnt valid, breaking", (DWORD)renderElem);
            break;
        }

        if((renderElem->stupidFlags & 0xFF) == 0x01) {
            LinkedListRenderData* renderData = (LinkedListRenderData*)renderElem;
            ExtraData* data = &renderData->data;
    
            if(data->verifyHash()) {
                if(data->drawType == ListDrawType::Effect) {

                    bool shouldColor = true;

                    exists = *(BYTE*)(data->address + 0x0);
                    source = *(BYTE*)(data->address + 0x8);
                    pattern = *(DWORD*)(data->address + 0x10);
                    owner = *(BYTE*)(data->address + 0x2F4);
                    playerAddr = 0x00555130 + (0xAFC * owner);
                    charID = *(BYTE*)(playerAddr + 0x5);
                    moon = *(BYTE*)(playerAddr + 0xC);
                    palette = *(BYTE*)(playerAddr + 0xA);

                    bool shouldCheckPattern = false;

                    if(exists != 0 && source != 0xFE) {
                        if(palette == 26 && moon == 0) {
                            shouldCheckPattern = true;
                        } else if(palette == 15 && moon == 2) {
                            shouldCheckPattern = true;
                        }
                    }

                    //if(exists == 0 || source == 0xFE || palette != 26 || moon != 0 || charID != 12) {
                    if(!shouldCheckPattern) {
                        shouldColor = false;
                    } else {
                        // i really should just import the list from training mode
                        // basically, warc gets transformed into an effect for some reason.
                        // i really should whitelist instead of blacklist here
                        // ok tbh, this sucks, but i need to have this be a whitelist, not blacklist, if i ever want to sleep at night
                        // or,,, i could see if i have a colision box? maybe? but thats just another bandaid solution

                        switch(pattern) {
                            case  80 ...  88: // god. visual studio doesnt have this. i really missed it
                            case  90 ...  94:
                            case  96 ...  99:
                            case 102: // on frame 17 of this, warc flashes in for one frame. what? this looks like an unused move tbh, more similar to arc 5a. is eclipse warc in this mode?
                            case 105:
                            case 108:
                            case 110:
                            case 116 ... 123:
                            case 133:
                            case 136 ... 139:
                            case 389 ... 390:
                            case 404:
                            case 443 ... 445:
                            case 465:
                            case 507 ... 508:
                            case 533:
                            case 536:
                            case 538:
                            case 539:
                            case 550:
                            case 555 ... 558:
                                break;
                            default:
                                shouldColor = false;
                                //log("Pattern %5d", pattern);
                                break;
                        }


                        if(moon == 0) { 
                            activeDiffuse = NULL;
                            activeSpecular = cWarcSpecular;
                        } else if(moon == 2) {
                            activeDiffuse = hWarcDiffuse;
                            activeSpecular = hWarcSpecular;
                        }

                    }


                    if(shouldColor) {
                        for(int i=0; i<4; i++) {
                            if(activeDiffuse != NULL) {
                                renderData->verts[i].diffuse = (ARGBColor)activeDiffuse[i];
                            }
                            if(activeSpecular) {
                                renderData->verts[i].specular = (ARGBColor)activeSpecular[i];
                            }
                        }

                        activeDiffuse = NULL;
                        activeSpecular = NULL;

                        //log("%5d %d %08X %5d %-16s %-16s", index, data->verifyHash(), data->address, pattern, getListDrawString(data->drawType), getListRetString(data->retType));
                    }
                    
                }
            }   
            // reset the extradata after this, to prevent the weird flickering? i hope
            // idk why i hadnt done this previously
            // which render implimentation should i put into mainline caster?,, im not sure
            // not reseting the diffuse stuff also may cause some issues, if those things are reused ( i think they are? )
            // regardless, i cant get the flickering to occur in training mode. seems a full game is needed
            // i should just use the ETM version
            memset(data, 0, sizeof(ExtraData));

        }

        addr = *(DWORD*)addr;
        renderElem = renderElem->nextElement;

        index++;
    }

}
  
void _naked_modifyLinkedList1() {
    // patched at 0x004331d4:

    PUSH_ALL;
    modifyLinkedList();
    POP_ALL;

    emitCall(0x004c04e0);

    emitJump(0x004331d9);
    
}

void _naked_modifyLinkedList2() {

    // patched at 0040e48e

    
    // patching at 0040e48e worked, but caused the game to not close properly (either a mutex issue, or my exception handler)
    emitByte(0x89);
    emitByte(0x2D);
    
    emitByte(0xE0);
    emitByte(0x42);
    emitByte(0x55);
    emitByte(0x00);
    
    PUSH_ALL;
    modifyLinkedList();
    POP_ALL;

    emitJump(0x0040e494);
    
}

} // namespace AsmHacks
