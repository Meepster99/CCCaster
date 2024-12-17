#include "DllAsmHacks.hpp"
#include "Messages.hpp"
#include "DllNetplayManager.hpp"
#include "CharacterSelect.hpp"
#include "Logger.hpp"
#include "DllTrialManager.hpp"
#include "DllDirectX.hpp"

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

        mov ebx, [0x005551EC + (0 * 0xAFC)]; // P0 HEALTH
        add ebx, [0x005551EC + (2 * 0xAFC)]; // P2 HEALTH
        cmp ebx, 0;
        JLE FAIL;

        mov ebx, [0x005551EC + (1 * 0xAFC)]; // P1 HEALTH
        add ebx, [0x005551EC + (3 * 0xAFC)]; // P3 HEALTH
        cmp ebx, 0;
        JLE FAIL;

        mov eax, 0; // OK        
        END:
        pop ebx;     
        
        push 0x00474641; // jump to resume
        ret;

        FAIL:
        mov eax, 1; // end the round
        JMP END;
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

// -----

extern "C" __attribute__((noinline)) void drawAllPortriats(DWORD index, DWORD texture) {
    
    DWORD xPos = (index & 1) ? 384 : 0;
    DWORD yPos = 100;//0;
    DWORD width = 0x100;
    DWORD height = 0x60;
    DWORD layer = 0x2C0; //0x2C1
    if(index >= 2) {
        xPos += (index & 1) ? 25 : 100;
        width /= 2;
        height /= 2;
        layer |= 1;
    }



    // uVar2 xOffset?
    // uVar3 yOffset?
    // uVar4 width?

    //                  edx    ?        x     y   h    xO yO  wid    hagain?
    //meltyDrawTexture(0x100, 0, texture, xPos, 0, 0x60, 0, 0, 0x100, 0x60, 0xFFFFFFFF, 0, 0x2C1);
    // is one width used for,,, what the fuck
    // oh im soooo fucked ugh

    meltyDrawTexture(texture, 0, 0, 0x100, 0x60, xPos, yPos, width, height, 0xFFFFFFFF, layer);

    //meltyDrawTexture(texture, xPos, yPos, width, height, 0xFFFFFFFF, 0x2C1);

    // are edx and,,, the other one both width?

}

void _naked_drawAllPortriats() {

    // patched in at 00425a98

    PUSH_ALL;
    __asmStart R"(
        push edi; // texture
        push esi; // index
        call _drawAllPortriats;
        add esp, 0x08;
    )" __asmEnd
    POP_ALL;

    emitJump(0x00425af0);
}

extern "C" {
    DWORD naked_fixPortriatLoadSide_loadCount = 0;
}

void _naked_fixPortriatLoadSide() { // this solution is not the most elegant, but it works.

    // patched at 004263b6
    // emit overwritten asm

    // mov ecx, [esp + 0x128];
    // has 0,1,0,0
    // but,,, 12c? has char id,,,
    /*emitByte(0x8B);
    emitByte(0x8C);
    emitByte(0x24);

    emitByte(0x28);
    emitByte(0x01);
    emitByte(0x00);
    emitByte(0x00);*/

    __asmStart R"(
        mov ecx, 1;
        add _naked_fixPortriatLoadSide_loadCount, ecx;

        //mov ecx, [esp + 0x12C];
    )" __asmEnd

    // and the resulting load by one to load the correct file?
    __asmStart R"(
        mov ecx, _naked_fixPortriatLoadSide_loadCount;
        and ecx, 0x01;
        xor ecx, 0x01;
    )" __asmEnd

    emitJump(0x004263bd);

}

void drawHealthBars(int playerIndex) {

}

void drawMeterBars(int playerIndex) {
    
}

void drawGuardBars(int playerIndex) {

}

void drawCharInfo(int playerIndex) {
    
}

extern "C" {
    int newDrawResourcesHud_playerIndex = 0; // is this because,,, i need this to not be in a register where it could be messed up
}
void newDrawResourcesHud() {

    newDrawResourcesHud_playerIndex = 0;

    while(newDrawResourcesHud_playerIndex < 4) {

        drawHealthBars(newDrawResourcesHud_playerIndex);
        drawMeterBars(newDrawResourcesHud_playerIndex);
        drawGuardBars(newDrawResourcesHud_playerIndex);
        drawCharInfo(newDrawResourcesHud_playerIndex);

        // im not exactly happy to be writing this in asm, but its the only way i can without my registers getting fucked up
        // or,,, i could just,, trace all the textures and do all the draws myself?

        /*
        pushVar(_newDrawResourcesHud_playerIndex);
        emitCall(0x00425260); // draw guard guages
        addStack(0x04);

        pushVar(_newDrawResourcesHud_playerIndex);
        emitCall(0x004253c0); // draw meter guages
        addStack(0x04);

        setRegister(ecx, _newDrawResourcesHud_playerIndex);
        emitCall(0x004258e0); // draw moons and palettes

        setRegister(eax, _newDrawResourcesHud_playerIndex);
        emitCall(0x00425a80); // draw portriats
        */

        newDrawResourcesHud_playerIndex++;
    }
}

void _naked_newDrawResourcesHud() {

    // patched at 0042485b

    PUSH_ALL;
    newDrawResourcesHud();
    POP_ALL;

    ASMRET;
}

} // namespace AsmHacks
