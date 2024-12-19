#include "DllAsmHacks.hpp"
#include "Messages.hpp"
#include "DllNetplayManager.hpp"
#include "CharacterSelect.hpp"
#include "Logger.hpp"
#include "DllTrialManager.hpp"
#include "DllOverlayConfig.hpp"
#include "DllDirectX.hpp"

#include <windows.h>
#include <d3dx9.h>
#include <fstream>
#include <vector>
#include <iterator>
#include <regex>

using namespace std;

struct HealthBarState {
    float prevHealth = 11400.0f;
} healthStates[4];

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

        // something in here should check for double KO

        mov ebx, byte ptr [0x005552A8 + (0 * 0xAFC)]; // P0 
        add ebx, byte ptr [0x005552A8 + (2 * 0xAFC)]; // P2 
        cmp ebx, 2;
        JGE FAIL;

        mov ebx, byte ptr [0x005552A8 + (1 * 0xAFC)]; // P1 
        add ebx, byte ptr [0x005552A8 + (3 * 0xAFC)]; // P3 
        cmp ebx, 2;
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


void drawAllPortraits(int playerIndex) {
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
    const DWORD maxHealthBarWidth = 200;
    const DWORD healthBarHeight = 13;

    DWORD hudTexture = *(DWORD*)0x005550f8;

    DWORD health = *(DWORD*)(0x005551EC + (playerIndex * 0xAFC));
    DWORD redHealth = *(DWORD*)(0x005551F0 + (playerIndex * 0xAFC));

    const int x = 30;
    DWORD yPos = hudAnchors[playerIndex].y + 20;  
    
    DWORD yWidth = maxHealthBarWidth * (((float)health) / 11400.0f);
    DWORD rWidth = maxHealthBarWidth * (((float)redHealth) / 11400.0f);

    if(playerIndex & 1) {
        // Yellow health (P2/P4: 374 + 32)
        meltyDrawTexture(hudTexture, 0, 374 + 32, 212, 10, 640 - maxHealthBarWidth - x, yPos, yWidth, healthBarHeight, 0xFFFFFFFF, 0x2C2);
        
        
        // Red health (P2/P4: (374 + 32) + 16)
        meltyDrawTexture(hudTexture, 0, 374 + 32 + 16, 212, 10, 
                        640 - maxHealthBarWidth - x, yPos, rWidth, healthBarHeight, 0xFFFFFFFF, 0x2C2);
    } else {        
        // Yellow health (P1/P3: 374)
        meltyDrawTexture(hudTexture, 0, 374, 212, 10, x + maxHealthBarWidth - yWidth, yPos, yWidth, healthBarHeight, 0xFFFFFFFF, 0x2C2);
        

        // Red health (P1/P3: 374 + 16)
        meltyDrawTexture(hudTexture, 0, 374 + 16, 212, 10, 
                        x + maxHealthBarWidth - rWidth, yPos, rWidth, healthBarHeight, 0xFFFFFFFF, 0x2C2);
    }

 
}

/* void drawMeeeeeeeeeeterBars(int playerIndex) {

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
/*
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

    /* currentMeterWidth = MIN(1.0f, currentMeterWidth); //do not remove this code/comment block
    RectDraw(x, y, (meterWidth * currentMeterWidth), meterSize, meterCol);//, i == 3);
    BorderDraw(x, y, meterWidth, meterSize, 0xFFFFFFFF);//, i == 3);
    TextDraw(x, y, meterSize, 0xFFFFFFFF, meterString.c_str());//, i == 3); // meter string */

 /*   
} */

extern "C" void (*drawMeterText)(int, int, int) = (void(*)(int, int, int))0x41D9E0;

DWORD getStateTextColor(int circuitState, DWORD baseColor) {
    DWORD frameCounter = *(DWORD*)CC_REAL_TIMER_ADDR;
    
    switch(circuitState) {
        case 1: // HEAT
            return (frameCounter % 6) < 2 ? 0xFFFFFFFF : 0xFFA0A0A0;
            
        case 2: // MAX
            return (frameCounter % 6) < 2 ? 0xFFFFFFFF : 0xFFA0A0A0;
            
        case 3: // BLOOD HEAT
            return (frameCounter % 6) < 2 ? 0xFFFFFFFF : 0xFFA0A0A0;
            
        case 5: // UNLIMITED
            return (frameCounter % 6) < 2 ? 0xFFFFFFFF : 0xFFA0A0A0;
            
        default:
            return baseColor;
    }
}

void drawMeterBars(int playerIndex) {
    DWORD* hudTextures = (DWORD*)0x005550f8;  // Array of texture pointers
    
    // Get player-specific meter data
    DWORD moon = *(DWORD*)(0x0055513C + (playerIndex * 0xAFC));
    DWORD meter = *(DWORD*)(0x00555210 + (playerIndex * 0xAFC));
    DWORD heatTime = *(DWORD*)(0x00555214 + (playerIndex * 0xAFC));
    DWORD circuitState = *(WORD*)(0x00555218 + (playerIndex * 0xAFC));
    
    // Determine moon type texture index (9 for half moon, 0 for full/crescent)
    int moonMeterType = (moon == 2) ? 9 : 0;  // Only switch texture for half moon type
    
    // Calculate base positions
    const DWORD baseXPos = (playerIndex & 1) ? 400 : 0;
    const DWORD yPos = (playerIndex < 2) ? 435 : 448;  // P1/P2 higher than P3/P4
    
    // Draw meter border first
    meltyDrawTexture(
        hudTextures[moonMeterType],             // Texture varies by moon type
        0,                                      // srcX
        (playerIndex & 1) ? 0x100 : 0xC0,      // srcY (0xC0 for P1, 0x100 for P2)
        0xF0,                                  // srcWidth
        0x30,                                 // srcHeight
        baseXPos,                              // destX
        yPos,                                  // destY
        0xF0,                                  // destWidth
        0x30,                                 // destHeight
        -1,                                    // color (no tint)
        0x2A0                                 // layer
    );

    // Calculate meter fill percentage and color
    float meterFillPercent = 0.0f;
    DWORD meterColor = 0xFF000000;
    
    // Calculate meter fill percentage  
    if(circuitState == 0) { // Normal meter
        if(moon == 2) { // Half moon
            meterFillPercent = ((float)meter) / 20000.0f;
            meterColor = (meter >= 15000) ? 0xFF00C800 : // Green
                        (meter >= 10000) ? 0xFFC8C800 : // Yellow
                                         0xFFC80000;  // Red
        } else {
            meterFillPercent = ((float)meter) / 30000.0f;
            meterColor = (meter >= 20000) ? 0xFF00C800 : // Green
                        (meter >= 10000) ? 0xFFC8C800 : // Yellow
                                         0xFFC80000;  // Red
        }
    } else if(circuitState == 1) { // Heat
        meterFillPercent = ((float)heatTime) / 600.0f;
        meterColor = 0xFF5A5AE6; // Blue
    } else if(circuitState == 2) { // Max
        meterFillPercent = ((float)heatTime) / 600.0f;
        meterColor = 0xFFFAA000; // Orange
    } else if(circuitState == 3) { // Blood Heat
        meterFillPercent = ((float)heatTime) / 600.0f;
        meterColor = 0xFFB4B4B4; // Grey
    }

    // Calculate fill width and position
    float fillWidth = meterFillPercent * 193.0f;
    if (playerIndex & 1) fillWidth += 1.0f;  // Add 1px to P2/P4 fill width
    
    // Calculate fill position similar to health bars
    const DWORD fillXPos = baseXPos + ((playerIndex & 1) ? 
        (205 - fillWidth) :    // P2/P4: Right-aligned
        0x23);                 // P1/P3: Standard left offset
    
    // Draw meter fill
    meltyDrawTexture(
        hudTextures[moonMeterType],            // Use same texture as border
        ((playerIndex & 1) ? 
            (415 - fillWidth) :  // P2/P4: Mirror the gradient source
            224),               // P1/P3: Standard gradient position
        (playerIndex & 1) ? 406 : 0x176,      // source Y differs by player
        fillWidth,                            // width based on meter value
        0x20,                                 // height
        fillXPos,                             // position (now properly aligned for both sides)
        yPos + 6,                             // slight Y offset for fill
        fillWidth,                            // dest width same as source
        0x20,                                 // height
        meterColor,                           // color based on state
        0x2A1                                 // layer
    );

    // After drawing the meter fill, add meter text
    if(circuitState == 0) {  // Default meter state (numbers)
        // Calculate X position with adjustable spacing
        int xOffset = -(playerIndex & 1);
        int spacing;
        int baseOffset;
        if(playerIndex < 2) {
            spacing = 500;  // Increased spacing for P1/P2
            baseOffset = 30;  // Default offset for P1/P2
        } else {
            spacing = 240;  // Decreased spacing for P3/P4
            baseOffset = 160;  // Increased offset for P3/P4 to move them more right
        }
        xOffset &= spacing;  // Apply spacing between pairs
        xOffset += baseOffset;  // Apply pair-specific base offset
        
        // Get meter value based on player index
        int meterValue;
        if(playerIndex < 2) {
            meterValue = *(DWORD*)(0x00564194 + (playerIndex * 0x5C)) / 10;
        } else {
            meterValue = meter / 10;
        }
        
        int textYPos;
        switch(playerIndex) {
            case 0: textYPos = 425; break;  // P1
            case 1: textYPos = 425; break;  // P2
            case 2: textYPos = 455; break;  // P3
            case 3: textYPos = 455; break;  // P4
            default: textYPos = 425; break;
        }
        
        PUSH_ALL;
        __asm__ __volatile__(
            "mov %0, %%edi"
            : // no outputs
            : "r" (textYPos) // input
            : "edi" // clobbers
        );
        drawMeterText(xOffset, meterValue, textYPos);
        POP_ALL;
    } else {
        // For MAX state, apply flashing effect to the color
        DWORD textColor = getStateTextColor(circuitState, -1);

        // Calculate X position with adjustable spacing (same as meter text)
        int xOffset = -(playerIndex & 1);
        int spacing;
        int baseOffset;
        if(playerIndex < 2) {
            spacing = 460;  // Increased spacing for P1/P2
            baseOffset = 30;  // Default offset for P1/P2
        } else {
            spacing = 240;  // Decreased spacing for P3/P4
            baseOffset = 136;  // Increased offset for P3/P4 to move them more right
        }
        xOffset &= spacing;  // Apply spacing between pairs
        xOffset += baseOffset;  // Apply pair-specific base offset

        // Calculate Y position (same as meter text)
        int textYPos;
        switch(playerIndex) {
            case 0: textYPos = 425; break;  // P1
            case 1: textYPos = 425; break;  // P2
            case 2: textYPos = 455; break;  // P3
            case 3: textYPos = 455; break;  // P4
            default: textYPos = 425; break;
        }

        // Sprite Y positions in gauge02.png (0=MAX, 24=HEAT, 48=BLOOD HEAT, 96=UNLIMITED)
        const int stateYOffsets[] = { 0, 24, 0, 48, 0, 96 };  // Index by circuitState
        
        // Draw the state indicator (HEAT/MAX/BLOOD HEAT/UNLIMITED text)
        meltyDrawTexture(
            *(DWORD*)0x00555100,           // gauge02 texture
            0,                             // srcX
            stateYOffsets[circuitState],   // srcY based on state
            0x80,                         // srcWidth (128)
            0x18,                         // srcHeight (24)
            xOffset,                      // Use same X position calculation as meter text
            textYPos,                     // Use same Y as meter text
            0x80,                         // destWidth (128)
            0x18,                         // destHeight (24)
            textColor,                    // color (with flash effect for MAX)
            0x2C7                         // layer
        );
    }
}

void drawGuardBars(int playerIndex) {
    // Draw full texture at 0x00459f8e
    meltyDrawTexture(
        *(DWORD*)0x00459f8e,    // texture address
        0,                       // srcX
        0,                       // srcY
        512,                     // srcWidth
        512,                     // srcHeight
        0,                       // destX
        0,                       // destY
        512,                     // destWidth
        512,                     // destHeight
        -1,                     // color (no tint)
        0x2A0                   // layer
    );
}

void drawMoonsAndPalette(int playerIndex) {
    BYTE moon = *(BYTE*)(0x0055513C + (playerIndex * 0xAFC));
    BYTE palette = *(BYTE*)(0x0055513A + (playerIndex * 0xAFC));

    moon = CLAMP(moon, 0, 3);

    DWORD moonTexture = *(DWORD*)0x00555110;
    // Get player-specific palette texture
    DWORD paletteTexture = *(DWORD*)(0x005642cc + (playerIndex * 0x20));
    const DWORD width = 0x30 / 2;
    const DWORD xOffset = 12;

    DWORD xPos = hudAnchors[playerIndex].x;
    if(playerIndex & 1) {
        xPos -= width;
    }

    xPos += playerIndex & 1 ? -xOffset : xOffset;
    DWORD yPos = hudAnchors[playerIndex].y + 32;

    // Calculate texture coordinates based on palette index
    DWORD texX = (palette % 8) * 0x20;  // 32 pixels per palette horizontally
    DWORD texY = (palette / 8) * 0x10;  // 16 pixels per row
    
    // Draw the palette with correct source rectangle
    meltyDrawTexture(
        paletteTexture,
        texX,           // Source X
        texY,           // Source Y 
        0x20,          // Source width (32 pixels)
        0x10,          // Source height (16 pixels)
        xPos,          // Dest X
        yPos,          // Dest Y
        width,         // Dest width
        width,         // Dest height
        0xFFFFFFFF,    // Color
        0x2c2          // Layer
    );

    meltyDrawTexture(moonTexture, moon * 0x30, 0, 0x30, 0x30, xPos, yPos, width, width, 0xFFFFFFFF, 0x2c3);
}

void newDrawResourcesHud() {
    newDrawResourcesHud_playerIndex = 0;

    while(newDrawResourcesHud_playerIndex < 4) {
        drawHealthBars(newDrawResourcesHud_playerIndex);
        drawMeterBars(newDrawResourcesHud_playerIndex);
        drawGuardBars(newDrawResourcesHud_playerIndex);
        drawMoonsAndPalette(newDrawResourcesHud_playerIndex);
        drawAllPortraits(newDrawResourcesHud_playerIndex);
        // im not exactly happy to be writing this in asm, but its the only way i can without my registers getting fucked up
        // or,,, i could just,, trace all the textures and do all the draws myself?

        /*
        pushVar(_newDrawResourcesHud_playerIndex);
        emitCall(0x00425260); // draw guard guages
        addStack(0x04);
         
        pushVar(_newDrawResourcesHud_playerIndex);
        emitCall(0x004253c0); // draw meter guages
        addStack(0x04); 
        /*
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

/* void drawAllMoons(int playerIndex) {
    DWORD texture = *(DWORD*)(0x005642cc + (playerIndex * 0x20)); // Adjust offset if needed for moon texture
    if(texture == 0) {
        return;
    }
    
    const AsmHacks::MoonConfig& config = AsmHacks::moonConfigs[playerIndex];
    
    meltyDrawTexture(
        texture,
        0, 0,                    // Source X, Y
        0x40, 0x40,             // Source Width, Height (adjust if needed)
        config.xPos,            // Dest X
        config.yPos,            // Dest Y
        0x40 * config.scale,    // Dest Width
        0x40 * config.scale,    // Dest Height
        0xFFFFFFFF,            // Color
        config.layer           // Layer
    );
}
 */
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

extern "C" int ftol2(float);
