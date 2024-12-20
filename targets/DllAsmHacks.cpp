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

extern "C" {
    DWORD naked_charTurnAroundStateRes = 0;
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

    for(int i=0; i<4; i++) {
        int goal = naked_charTurnAroundState[i];
        goal <<= 1;
        if((i & 1) == 0) {
            goal += 1;
        }

        int ourXPos = *(int*)(0x00555130 + 0x108 + (i * 0xAFC));
        int otherXPos = *(int*)(0x00555130 + 0x108 + (goal * 0xAFC));

        *(BYTE*)(0x00555130 + 0x315 + (i * 0xAFC)) = otherXPos < ourXPos;

    }

    naked_charTurnAroundStateRes = 0;

}

void _naked_charTurnAround2() {

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

extern "C" {
    int newDrawResourcesHud_playerIndex = 0; // is this because,,, i need this to not be in a register where it could be messed up
}

const Point hudAnchors[4] = { // i could/should maybe add a xscale and yscale param here? but god linking everything together,,
    Point(0, 0),
    Point(640, 0),
    Point(0, 48),
    Point(640, 48)
};

void drawAllPortriats(int playerIndex) {

    DWORD texture = *(DWORD*)(0x005642c8 + (playerIndex * 0x20));
    if(texture == 0) {
        return;
    }
    
    /*
    DWORD xPos = (playerIndex & 1) ? 384 : 0;
    DWORD yPos = 100;//0;
    DWORD width = 0x100;
    DWORD height = 0x60;
    DWORD layer = 0x2C0; //0x2C1
    if(playerIndex >= 2) {
        xPos += (playerIndex & 1) ? 25 : 100;
        width /= 2;
        height /= 2;
        layer |= 1;
    }
    */

    /*
    DWORD xPos = (playerIndex & 1) ? 384 : 0;
    DWORD yPos = (playerIndex >= 2) ? 0x60 : 0;
    DWORD width = 0x100;
    DWORD height = 0x60;
    DWORD layer = (playerIndex >= 2) ? 0x2C1 : 0x2C0;
    */

    //DWORD xPos = (playerIndex & 1) ? 384 + 128 : 0;
    //DWORD yPos = (playerIndex >= 2) ? 0x60 / 2 : 0;

    DWORD xPos = hudAnchors[playerIndex].x;
    if(playerIndex & 1) {
        xPos -= 128;
    }

    DWORD yPos = hudAnchors[playerIndex].y;

    DWORD width = 0x100 / 2;
    DWORD height = 0x60 / 2;
    DWORD layer = (playerIndex >= 2) ? 0x2C1 : 0x2C0;

    meltyDrawTexture(texture, 0, 0, 0x100, 0x60, xPos, yPos, width, height, 0xFFFFFFFF, layer);

}

void drawHealthBars(int playerIndex) {


    // edx is w
    // 0 tex x y h texX texY texW texH

    /*

    x: (iVar1 - iVar2) + 0x111
    y: 0x28
    w: is a fuckass float, go grab it
    h: 10

    texX: 0 
    texY: 0x176
    texW: 0xd4
    texH: 10

    */

    //melty only tracks anims for 2/4 players. need to do this on my own

    const DWORD maxHealthBarWidth = 200;
    const DWORD healthBarHeight = 5;

    DWORD texture = *(DWORD*)0x005550f8;

    DWORD health =       *(DWORD*)(0x005551EC + (playerIndex * 0xAFC));
    DWORD redHealth =    *(DWORD*)(0x005551F0 + (playerIndex * 0xAFC));

    const int x = 30; // switch this to anchor when you refactor this please!
    
    //DWORD yPos = playerIndex >= 2 ? 68 : 20;
    DWORD yPos = hudAnchors[playerIndex].y + 20;  
    
    DWORD yWidth = maxHealthBarWidth * (((float)health) / 11400.0f);
    DWORD rWidth = maxHealthBarWidth * (((float)redHealth) / 11400.0f);


    DWORD redHealthColor = 0xFF990b0b;

    if(playerIndex & 1) {
        meltyDrawTexture(texture, 0, 0x196, 0xd4, 10, 640 - maxHealthBarWidth - x, yPos, yWidth, healthBarHeight, 0xFFFFFFFF, 0x2C2);
        meltyDrawTexture(texture, 0, 0x196, 0xd4, 10, 640 - maxHealthBarWidth - x, yPos, rWidth, healthBarHeight, redHealthColor, 0x2C2);
        //meltyDrawTexture(texture, 0, 0x196, 0xd4, 10, 640 - maxHealthBarWidth - x, yPos, maxHealthBarWidth, healthBarHeight, 0xFFc89664, 0x2C2);
    } else {        
        meltyDrawTexture(texture, 0, 0x176, 0xd4, 10, x + maxHealthBarWidth - yWidth, yPos, yWidth, healthBarHeight, 0xFFFFFFFF, 0x2C2);
        meltyDrawTexture(texture, 0, 0x176, 0xd4, 10, x + maxHealthBarWidth - rWidth, yPos, rWidth, healthBarHeight, redHealthColor, 0x2C2); 
        //meltyDrawTexture(texture, 0, 0x176, 0xd4, 10, x + maxHealthBarWidth - maxHealthBarWidth, yPos, maxHealthBarWidth, healthBarHeight, 0xFFc89664, 0x2C2); // background darkened highlight?
    }

}

void drawMeterBars(int playerIndex) {

    // i am not the happiest with this method of tweaking things, but it should work
    // or well actually i have no guarentee it will work now that i think about it, bc im not sure if the animations are even tracked for other chars
    // it doesnt. ill have to remake it
  
    /*
    DWORD xPos = hudAnchors[playerIndex].x;
    DWORD yPos = hudAnchors[playerIndex].y;

    //y += 200;

    __asmStart R"(
        

    )" __asmEnd

    emitCall(0x0041d9e0);

    __asmStart R"(
        add esp, 0x0C;
    )" __asmEnd

    */

    // i am going to explode.

    DWORD moon =         *(DWORD*)(0x0055513C + (playerIndex * 0xAFC));
    DWORD meter =        *(DWORD*)(0x00555210 + (playerIndex * 0xAFC));
    DWORD heatTime =     *(DWORD*)(0x00555214 + (playerIndex * 0xAFC));
    DWORD circuitState = *(WORD*) (0x00555218 + (playerIndex * 0xAFC));
    DWORD circuitBreakTimer;
    
    // im not confident with any of this
    if(*(WORD*)(0x00555234 + (playerIndex * 0xAFC)) == 110) { // means this its a ex penalty meter debuff
        circuitBreakTimer = 0;
    } else {
        circuitBreakTimer = *(WORD*)(0x00555230 + (playerIndex * 0xAFC));
    }

    shouldReverseDraws = (playerIndex & 1);

    // draw meter bars
    const int meterWidth = 200;
    const float meterSize = 15.0f;
    
    float x = 30;
    float y = (playerIndex >= 2) ? 428 + 20 : 428;

    float currentMeterWidth = 0.0f;
    DWORD meterCol = 0xFF000000;

    std::string meterString = "";

    if(circuitBreakTimer == 0) {
        switch(circuitState) {
            case 0:
                if(moon == 2) { // half
                    currentMeterWidth = ((float)meter) / 20000.0f;
                    meterCol = (meter >= 15000) ? 0xFF00FF00 : ((meter >= 10000) ? 0xFFFFFF00 : 0xFFFF0000);
                } else { // full/crescent
                    currentMeterWidth = ((float)meter) / 30000.0f;
                    meterCol = (meter >= 20000) ? 0xFF00FF00 : ((meter >= 10000) ? 0xFFFFFF00 : 0xFFFF0000);
                } 

                meterString = std::to_string(meter / 100) + "." + std::to_string((meter / 10) % 10) + "%";

                break;
            case 1:
                currentMeterWidth = ((float)heatTime) / 600.0f;
                meterCol = 0xFF0000FF;
                meterString = "HEAT";
                break;
            case 2:
                currentMeterWidth = ((float)heatTime) / 600.0f;
                meterCol = 0xFFFFA500;
                meterString = "MAX";
                break;
            case 3:
                currentMeterWidth = ((float)heatTime) / 600.0f;
                meterCol = 0xFFDDDDDD;
                meterString = "BLOOD HEAT";
                break;
            default:
                break;
        }
    } else {
        currentMeterWidth = ((float)circuitBreakTimer) / 600.0f;
        meterCol = 0xFF800080;
        meterString = "BREAK";
    }

    currentMeterWidth = MIN(1.0f, currentMeterWidth);
    RectDraw(x, y, (meterWidth * currentMeterWidth), meterSize, meterCol);//, i == 3);
    BorderDraw(x, y, meterWidth, meterSize, 0xFFFFFFFF);//, i == 3);
    TextDraw(x, y, meterSize, 0xFFFFFFFF, meterString.c_str());//, i == 3); // meter string

    
}

void drawGuardBars(int playerIndex) {

}

void drawMoonsAndPalette(int playerIndex) {

    BYTE moon =    *(BYTE*)(0x0055513C + (playerIndex * 0xAFC));
    BYTE palette = *(BYTE*)(0x0055513B + (playerIndex * 0xAFC));

    moon = CLAMP(moon, 0, 3); // 3 is here,,, since thats eclipse moons

    DWORD texture = *(DWORD*)0x00555110;

    const DWORD width = 0x30 / 2;
    const DWORD xOffset = 12;

    DWORD xPos = hudAnchors[playerIndex].x;
    if(playerIndex & 1) {
        xPos -= width;
    }

    xPos += playerIndex & 1 ? -xOffset : xOffset;

    DWORD yPos = hudAnchors[playerIndex].y + 32;

    meltyDrawTexture(texture, moon * 0x30, 0, 0x30, 0x30, xPos, yPos, width, width, 0xFFFFFFFF, 0x2c3);

}

void newDrawResourcesHud() {

    return;

    newDrawResourcesHud_playerIndex = 0;

    while(newDrawResourcesHud_playerIndex < 4) {

        drawHealthBars(newDrawResourcesHud_playerIndex);
        drawMeterBars(newDrawResourcesHud_playerIndex);
        drawGuardBars(newDrawResourcesHud_playerIndex);
        drawMoonsAndPalette(newDrawResourcesHud_playerIndex);
        drawAllPortriats(newDrawResourcesHud_playerIndex);

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
        mov eax, 84;
        sub [esp + 0xC], eax;    
    )" __asmEnd

    emitCall(0x00415580);

    emitJump(0x00424951); 
}

} // namespace AsmHacks
