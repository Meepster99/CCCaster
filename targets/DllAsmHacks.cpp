#include "DllAsmHacks.hpp"
#include "Messages.hpp"
#include "DllNetplayManager.hpp"
#include "CharacterSelect.hpp"
#include "Logger.hpp"
#include "DllTrialManager.hpp"

#include <windows.h>
#include <d3dx9.h>
#include <fstream>
#include <vector>
#include <iterator>
#include <regex>

using namespace std;


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

void battleResetCallback() {

    for ( const Asm& hack : patch2v2 )
        WRITE_ASM_HACK ( hack );

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

void _naked_charTurnAround() {

    // patched in at  0x0047587b
    // need to ret to 0x00475887

    __asmStart R"(

        // unsure if this modifies flags in the same manner as cmp
        // worst case tho, its inverted, so i can always just flip the z flag ig
        mov al, byte ptr[ecx + 0x000001EC];
        xor al, byte ptr[ebp + 0x000002F0];
        and al, 0x01;
        test al, al;

        // sync up al for the rest of this bs, if needed
        //mov al, byte ptr[ecx + 0x000001EC];
        //cmp byte ptr[ebp + 0x000002F0], al;
    
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

void _naked_throwConnect() {

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

} // namespace AsmHacks
