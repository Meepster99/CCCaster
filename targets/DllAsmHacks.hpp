#pragma once

#include "Constants.hpp"
#include "Exceptions.hpp"
#include "DllNetplayManager.hpp"
#include "DllCSS.hpp"

#include <stdio.h>
#include <vector>
#include <array>
#include <windows.h>


/*

my naming schemes are horrid, but then again, i dont even know what im naming

LinkedListDataElement is for the data at 005550B0

LinkedListElement is for the data inside the list of lists in 005550A8, [[005550A8]]

*/ 

typedef struct RawMeltyVert {
	float x;
	float y;
	float z;
	float w;
	DWORD diffuse; 
	DWORD specular; 
	float u;
	float v;
} RawMeltyVert;

static_assert(sizeof(RawMeltyVert) == 0x20, "RawMeltyVert must be size 0x20");

enum class ListDrawType : BYTE {
    Unknown = 0,
    Character = 1,
    Shadow = 2,
    Effect = 3,
    HitEffect = 4
};

const char* getListDrawString(ListDrawType t);

enum class ListRetType : BYTE {
    Unknown = 0,
    DrawMenuCursor = 1,
    DrawEffectsTopUIandCharaSelect = 2,
    DrawHitEffect = 3,
    DrawBlur = 4,
    DrawCharacterSelectBackground = 5,
    DrawCharactersAndBackground = 6,
    RenderOnScreen = 7
};

const char* getListRetString(ListRetType t);

#define packedStruct struct __attribute__((packed))

typedef packedStruct RenderList {

    packedStruct LinkedListAssets { 
        // i could, maybe should, have this whole struct be a union of LinkedListAssets and LinkedListAssetsData
        // tbh i will
        // although, is this fucker always accessed as a linked list? or also as an array??
        // theres no fucking way that its,, i,,,

        packedStruct LinkedListAssetsData {
            // straight up? not fuckin sure about the size of this
            // its either going to point to another 
            // alloced at 00414EEA

            LinkedListAssets* nextAsset;

            // not sure about this one at all
            union {
                DWORD flags; //
                struct {
                    BYTE flag0; // most likely a D3DRESOURCETYPE. very weird that its only,,, one byte though,,, so it might be something else. check out 004c0243 for an explanation
                    BYTE flag1;
                    BYTE flag2;
                    BYTE flag3;
                };
            };

            union {
                RawMeltyVert verts[4];
                struct {
                    RawMeltyVert v0;
                    RawMeltyVert v1;
                    RawMeltyVert v2;
                    RawMeltyVert v3;
                };
            };

            IDirect3DTexture9* tex; // at hex 88

            DWORD LinkedListAssetsData_UNK1;
            DWORD LinkedListAssetsData_UNK2

        };

        static_assert(sizeof(LinkedListAssetsData) == 0x94, "LinkedListAssetsData size error");

        union {
            // dawg. i got NO FUCKING CLUE WHATS HAPPENING
            // basically, if this ptr + 4 is nonzero, then hit it with the switch at 004c03cb
            // and its an alloc size 94. if not, then just keep chugging down the list??!
            LinkedListAssets* nextAsset; 
            LinkedListAssetsData* nextData;
        }

        DWORD stupidFlags; // 00414e48 sets this to 0, 004C0566 skips drawing if its,, not 0?
        DWORD unknown2; // swaps between 0 and 1, not sure why. this is what doesThingsIfList+8=0 interacts with
        DWORD possibleCount; // 1600. why? im not sure

    };

    static_assert(sizeof(LinkedListAssets) == 4 * sizeof(DWORD), "LinkedListAssets size error");

    LinkedListAssets* linkedListAssets; // malloced at 0041507D with size 0x6400
    DWORD linkedListAssetsCount; // 1600, the malloc was 0x6400, 0x6400 / 0x10 is 1600.
    
    packedStruct LinkedListRender {

    };  

    LinkedListRender* linkedListRender; 

    DWORD unknownCount1;
    DWORD unknownCount1_sub;

    DWORD unknownCount2;
    DWORD unknownCount2_sub;

} RenderList;

static_assert(sizeof(RenderList) == 7 * sizeof(DWORD), "RenderList size error");

RenderList* renderList = (RenderList*)0x005550A8;



typedef struct __attribute__((packed)) LinkedListSharedData {
    DWORD address;
    
    ListDrawType drawType;
    ListRetType retType;

    BYTE unused1;
    BYTE unused2;
    DWORD id; // todo, have a database of this, and find out where copies occur
    DWORD unused3;

} LinkedListSharedData;

typedef struct __attribute__((packed)) LinkedListAssetElement {

    /*
    
    has a list of lists with:
    ptr
    0
    1 // goes between 0 and one, some kinda of validity check?
    1600 // length
    
    [[[005550A8 + 8] + 4] + 0] + 0 seems to have an address to,,, data stored in [[005550A8 + 0] + 0] + 0
    
    ok i think im high now
    at this point, new solution, log malloc for some reason

    i should have hooked malloc months ago 
    also, i need to remake a sane version of cheat engine that lets me hot load structs

    */

    LinkedListAssetElement* next;
    LinkedListAssetElement* prev;

    union {
        RawMeltyVert verts[4];
        struct {
            RawMeltyVert v0;
            RawMeltyVert v1;
            RawMeltyVert v2;
            RawMeltyVert v3;
        };
    };

    IDirect3DTexture9* tex; // 0x88

    DWORD unk1; // 8C
    DWORD unk2; // 90
    DWORD unk3; // 94
    DWORD unk4; // 98
    DWORD unk5; // 9C
    
    LinkedListSharedData data;

} LinkedListAssetElement;

// im genuinely not sure if i can go over this size. malloc is allocing things onto 0x10 boundaries, and whatever is copying is doing that too
// i should be able to change the copying, but thats a TON of extra effort
// some super weird stuff is occuring with the "melty closed but is still in task manager" thing. tbh i should probs just,,, alloc a pointer? why not? but i should be able to have more space.
// so this relies on malloc going to 0x10 bounds, copies somehow going to 0x10 bounds, heaven and hell loving me
// nope, im an idiot. 004b3b22 mallocs a0.
// im now confused about what the fuck is going on with the hex 94 area??
// that malloc is never even called (maybe called on program load?)
static_assert(sizeof(LinkedListAssetElement) == 0xB0, "LinkedListAssetElement must be size 0xA0");

typedef struct __attribute__((packed)) LinkedListRenderElement {

    /* 
    this is the data in the list at 005550B0

    the data for the list holder is created at 00415139 with that malloc call

    but the elements for this list,,,
    00414ee5? 0x004b3b1d?

    if i was human, i would have both render/asset list share the same struct? but im not even sure of that????

    */


    LinkedListRenderElement* next;
    
    // not sure about this one at all
    union {
        DWORD flags;
        struct {
            BYTE flag0; // most likely a D3DRESOURCETYPE. very weird that its only,,, one byte though,,, so it might be something else. check out 004c0243 for an explanation
            BYTE flag1;
            BYTE flag2;
            BYTE flag3;
        };
    };

    union {
        RawMeltyVert verts[4];
        struct {
            RawMeltyVert v0;
            RawMeltyVert v1;
            RawMeltyVert v2;
            RawMeltyVert v3;
        };
    };

    IDirect3DTexture9* tex; // 0x88

    DWORD unk1; // 8C
    DWORD unk2; // 90
    DWORD unk3; // 94
    DWORD unk4; // 98
    DWORD unk5; // 9C
    
    LinkedListSharedData data;

} LinkedListRenderElement;

static_assert(sizeof(LinkedListRenderElement) == 0xB0, "LinkedListRenderElement must be size idek whats going on ");

#define SHIFTHELD  (GetAsyncKeyState(VK_SHIFT) & 0x8000)
#define UPPRESS    (GetAsyncKeyState(VK_UP)    & 0x0001)
#define DOWNPRESS  (GetAsyncKeyState(VK_DOWN)  & 0x0001)
#define LEFTPRESS  (GetAsyncKeyState(VK_LEFT)  & 0x0001)
#define RIGHTPRESS (GetAsyncKeyState(VK_RIGHT) & 0x0001)

// i wish i ever got the raw strings thing to work
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

/*
#define setRegister(reg, val) \
    __asm__ __volatile__( \
    "movl %%" #reg ", %0" \
    : \
    : "r" (val) \
    : #reg \
    )
*/

#define setRegister(reg, val) __asmStart \
    "mov " #reg ", " #val ";" \
    __asmEnd

#define pushVar(v) __asmStart \
    "push " #v \
    __asmEnd

#define addStack(n) __asmStart \
    "add esp, " #n \
    __asmEnd

#define INT3 __asmStart R"( int3; )" __asmEnd

#define ASMRET __asmStart R"( ret; )" __asmEnd

#define NOPS __asmStart R"( nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; )" __asmEnd

//__attribute__((noinline, optimize("-O0"))) bool isAddrValid(DWORD addr);

#define WRITE_ASM_HACK(ASM_HACK)                                                                                    \
    do {                                                                                                            \
        const int error = ASM_HACK.write();                                                                         \
        if ( error != 0 ) {                                                                                         \
            LOG ( "[%d] %s; %s failed; addr=%08x",                                                                  \
                  error, WinException::getAsString ( error ), #ASM_HACK, ASM_HACK.addr );                           \
            exit ( -1 );                                                                                            \
        }                                                                                                           \
    } while ( 0 )


#define INLINE_DWORD(X)                                                         \
    static_cast<unsigned char> ( unsigned ( X ) & 0xFF ),                       \
    static_cast<unsigned char> ( ( unsigned ( X ) >> 8 ) & 0xFF ),              \
    static_cast<unsigned char> ( ( unsigned ( X ) >> 16 ) & 0xFF ),             \
    static_cast<unsigned char> ( ( unsigned ( X ) >> 24 ) & 0xFF )

#define INLINE_DWORD_FF { 0xFF, 0x00, 0x00, 0x00 }

#define INLINE_FF_12_BYTES { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }

#define INLINE_NOP_TWO_TIMES { 0x90, 0x90 }

#define INLINE_NOP_THREE_TIMES { 0x90, 0x90, 0x90 }

#define INLINE_NOP_FIVE_TIMES { 0x90, 0x90, 0x90, 0x90, 0x90 }

#define INLINE_NOP_SIX_TIMES { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }

#define INLINE_NOP_SEVEN_TIMES { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }

#define PATCHJUMP_HELPER(patchAddr, newAddr) { (void*) patchAddr, { 0xE9, INLINE_DWORD( (newAddr) - ((patchAddr) + 5)) } } 

#define PATCHCALL_HELPER(patchAddr, newAddr) { (void*) patchAddr, { 0xE8, INLINE_DWORD( (newAddr) - ((patchAddr) + 5)) } } 

#define PATCHJUMP(patchAddr, newAddr) PATCHJUMP_HELPER(((unsigned)patchAddr), ((unsigned)newAddr))

#define PATCHCALL(patchAddr, newAddr) PATCHCALL_HELPER(((unsigned)patchAddr), ((unsigned)newAddr))

#define DISABLECALL(patchAddr) { ( void *) (patchAddr), INLINE_NOP_FIVE_TIMES }

namespace AsmHacks
{

// DLL callback function
extern "C" void callback();

// Position of the current menu's cursor, this gets updated by ASM hacks
extern uint32_t currentMenuIndex;

// The value of menuConfirmState is set to 1 if a menu confirm happens.
// A menu confirm will only go through if menuConfirmState is greater than 1.
extern uint32_t menuConfirmState;

// Filename of the saved replay
extern char* replayName;

// Round start counter, this gets incremented whenever players can start moving
extern uint32_t roundStartCounter;

// Auto replay save state, is set to 1 (loading), then 100 (saving), finally 255 (saved)
extern uint32_t *autoReplaySaveStatePtr;

// Flag to enable / disable the Escape key function that exits game (initially true)
extern uint8_t enableEscapeToExit;

// Array of sound effects that were played last frame, set any SFX to 1 to prevent playback.
extern uint8_t sfxFilterArray[CC_SFX_ARRAY_LEN];

// Indicates the next playback of the sound effect should be muted.
// Can mute and play a sound effect at the same time to effectively cancel an existing playback.
extern uint8_t sfxMuteArray[CC_SFX_ARRAY_LEN];

// The pointer to the current color table being loaded
extern uint32_t *currentColorTablePtr;

// The number of colors loaded, used to determine P1/P2/team colors
extern uint32_t numLoadedColors;


// Color loading callback functions
void colorLoadCallback ( uint32_t player, uint32_t chara, uint32_t *paletteData );
void colorLoadCallback ( uint32_t player, uint32_t chara, uint32_t palette, uint32_t *singlePaletteData );


// Struct for storing assembly code
struct Asm
{
    void *const addr;
    const std::vector<uint8_t> bytes;
    mutable std::vector<uint8_t> backup;

    int write() const;
    int revert() const;
};

typedef std::vector<Asm> AsmList;

// Add a call to the callback function just before the beginning of the game's main message loop.
// Note the message loop can get run multiple times per frame step, so be sure to check the world timer.
static const AsmList hookMainLoop =
{
    { MM_HOOK_CALL1_ADDR, {
        0xE8, INLINE_DWORD ( ( ( char * ) &callback ) - MM_HOOK_CALL1_ADDR - 5 ),   // call callback
        0xE9, INLINE_DWORD ( MM_HOOK_CALL2_ADDR - MM_HOOK_CALL1_ADDR - 10 )         // jmp MM_HOOK_CALL2_ADDR
    } },
    { MM_HOOK_CALL2_ADDR, {
        0x6A, 0x01,                                                                 // push 01
        0x6A, 0x00,                                                                 // push 00
        0x6A, 0x00,                                                                 // push 00
        0xE9, INLINE_DWORD ( CC_LOOP_START_ADDR - MM_HOOK_CALL2_ADDR - 5 )          // jmp CC_LOOP_START_ADDR+6 (AFTER)
    } },
    // Write the jump location last, due to dependencies on the callback hook code
    { CC_LOOP_START_ADDR, {
        0xE9, INLINE_DWORD ( MM_HOOK_CALL1_ADDR - CC_LOOP_START_ADDR - 5 ),         // jmp MM_HOOK_CALL1_ADDR
        0x90                                                                        // nop
                                                                                    // AFTER:
    } },
};

// Enable disabled stages and fix Ryougi stage music looping
static const AsmList enableDisabledStages =
{
    // Enable disabled stages
    { ( void * ) 0x54CEBC, INLINE_DWORD_FF },
    { ( void * ) 0x54CEC0, INLINE_DWORD_FF },
    { ( void * ) 0x54CEC4, INLINE_DWORD_FF },
    { ( void * ) 0x54CFA8, INLINE_DWORD_FF },
    { ( void * ) 0x54CFAC, INLINE_DWORD_FF },
    { ( void * ) 0x54CFB0, INLINE_DWORD_FF },
    { ( void * ) 0x54CF68, INLINE_DWORD_FF },
    { ( void * ) 0x54CF6C, INLINE_DWORD_FF },
    { ( void * ) 0x54CF70, INLINE_DWORD_FF },
    { ( void * ) 0x54CF74, INLINE_DWORD_FF },
    { ( void * ) 0x54CF78, INLINE_DWORD_FF },
    { ( void * ) 0x54CF7C, INLINE_DWORD_FF },
    { ( void * ) 0x54CF80, INLINE_DWORD_FF },
    { ( void * ) 0x54CF84, INLINE_DWORD_FF },
    { ( void * ) 0x54CF88, INLINE_DWORD_FF },
    { ( void * ) 0x54CF8C, INLINE_DWORD_FF },
    { ( void * ) 0x54CF90, INLINE_DWORD_FF },
    { ( void * ) 0x54CF94, INLINE_DWORD_FF },
    { ( void * ) 0x54CF98, INLINE_DWORD_FF },
    { ( void * ) 0x54CF9C, INLINE_DWORD_FF },
    { ( void * ) 0x54CFA0, INLINE_DWORD_FF },
    { ( void * ) 0x54CFA4, INLINE_DWORD_FF },

    // Fix Ryougi stage music looping
    { ( void * ) 0x7695F6, { 0x35, 0x00, 0x00, 0x00 } },
    { ( void * ) 0x7695EC, { 0xAA, 0xCC, 0x1E, 0x40 } },
};

// Disable the FPS limit by setting the game's perceived perf freq to 1
static const Asm disableFpsLimit = { CC_PERF_FREQ_ADDR, { INLINE_DWORD ( 1 ), INLINE_DWORD ( 0 ) } };

// Disable the code that updates the FPS counter
static const Asm disableFpsCounter = { ( void * ) 0x41FD43, INLINE_NOP_THREE_TIMES };

// Disable normal joystick and keyboard controls
static const AsmList hijackControls =
{
    // Disable joystick controls
    { ( void * ) 0x41F098, INLINE_NOP_TWO_TIMES   },
    { ( void * ) 0x41F0A0, INLINE_NOP_THREE_TIMES },
    { ( void * ) 0x4A024E, INLINE_NOP_TWO_TIMES   },
    { ( void * ) 0x4A027F, INLINE_NOP_THREE_TIMES },
    { ( void * ) 0x4A0291, INLINE_NOP_THREE_TIMES },
    { ( void * ) 0x4A02A2, INLINE_NOP_THREE_TIMES },
    { ( void * ) 0x4A02B4, INLINE_NOP_THREE_TIMES },
    { ( void * ) 0x4A02E9, INLINE_NOP_TWO_TIMES   },
    { ( void * ) 0x4A02F2, INLINE_NOP_THREE_TIMES },

    // Zero all keyboard keys
    { ( void * ) 0x54D2C0, {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    } },
};

// Copy dynamic menu variables and hijack the menu confirms
static const AsmList hijackMenu =
{
    // Copy the value of the current menu index (edi) to a location we control.
    // This hack uses the strategy of jumping around the extra spaces left in between code.
    { ( void * ) 0x4294D1, {
        0x8B, 0x7E, 0x40,                                           // mov edi,[esi+40]
        0x89, 0x3D, INLINE_DWORD ( &currentMenuIndex ),             // mov [&currentMenuIndex],edi
        0xE9, 0xF1, 0x04, 0x00, 0x00                                // jmp 0x4299D0 (AFTER)
    } },
    { ( void * ) 0x429817, {
        0x85, 0xC9,                                                 // test ecx,ecx
        0xE9, 0xB3, 0xFC, 0xFF, 0xFF                                // jmp 0x4294D1
    } },
    // Write this last due to dependencies
    { ( void * ) 0x4299CB, {
        0xE9, 0x47, 0xFE, 0xFF, 0xFF                                // jmp 0x429817
                                                                    // AFTER:
    } },

    // Code that allows us to selectively override menu confirms.
    // The value of menuConfirmState is set to 1 if a menu confirm happens.
    // A menu confirm will only go through if menuConfirmState is greater than 1.
    { ( void * ) 0x428F52, {
        0x53, 0x51, 0x52,                                           // push ebx,ecx,edx
        0x8D, 0x5C, 0x24, 0x30,                                     // lea ebx,[esp+30]
        0xB9, INLINE_DWORD ( &menuConfirmState ),                   // mov ecx,&menuConfirmState
        0xEB, 0x04                                                  // jmp 0x428F64
    } },
    { ( void * ) 0x428F64, {
        0x83, 0x39, 0x01,                                           // cmp dword ptr [ecx],01
        0x8B, 0x13,                                                 // mov edx,[ebx]
        0x89, 0x11,                                                 // mov [ecx],edx
        0x7F, 0x5B,                                                 // jg 0x428FC8 (LABEL_B)
        0xEB, 0x55                                                  // jmp 0x428FC4 (LABEL_A)
    } },
    { ( void * ) 0x428F7A, {
        0x5A, 0x59, 0x5B,                                           // pop edx,ecx,ebx
        0x90,                                                       // nop
        0xEB, 0x0D                                                  // jmp 0x428F8D (RETURN)
    } },
    { ( void * ) 0x428FC4, {
                                                                    // LABEL_A:
        0x89, 0x03,                                                 // mov [ebx],eax
        0xEB, 0xB2,                                                 // jmp 0x428F7A
                                                                    // LABEL_B:
        0x89, 0x01,                                                 // mov [ecx],eax
        0xEB, 0xAE                                                  // jmp 0x428F7A
    } },
    // Write this last due to dependencies
    { ( void * ) 0x428F82, {
        0x81, 0x3C, 0x24, 0xF5, 0x99, 0x42, 0x00,                   // cmp [esp],0x4299F5 (return addr of menu call)
        0x75, 0x02,                                                 // jne 0x428F8D (RETURN)
        0xEB, 0xC5,                                                 // jmp 0x428F52
                                                                    // RETURN:
        0xC2, 0x04, 0x00                                            // ret 0004
    } },
};

// Increment a counter at the beginning of the round when players can move
static const AsmList detectRoundStart =
{
    { ( void * ) 0x440D16, {
        0xB9, INLINE_DWORD ( &roundStartCounter ),                  // mov ecx,[&roundStartCounter]
        0xE9, 0xE2, 0x02, 0x00, 0x00                                // jmp 0x441002
    } },
    { ( void * ) 0x441002, {
        0x8B, 0x31,                                                 // mov esi,[ecx]
        0x46,                                                       // inc esi
        0x89, 0x31,                                                 // mov [ecx],esi
        0x5E,                                                       // pop esi
        0x59,                                                       // pop ecx
        0xC3                                                        // ret
    } },
    // Write this last due to dependencies
    { ( void * ) 0x440CC5, {
        0xEB, 0x4F                                                  // jmp 0x440D16
    } },
};

extern "C" void saveReplayCb();

// Copies the name of the replay
static const AsmList saveReplay =
{
    { ( void * ) 0x4824D4, {
        0xA3, INLINE_DWORD ( &replayName ),                         // mov [&replayName],eax
        0xE9, 0x45, 0xFC, 0xFF, 0xFF,                               // jmp 0x482123
    } },
    { ( void * ) 0x482123, {
        0x68, 0x34, 0xBF, 0x77, 0x00, 0x90,                         // pushl	$0x77bf34
        0xE9, 0x10, 0x07, 0x00, 0x00,                               // jmp 0x48283e
    } },
    { ( void * ) 0x4830A4, {
        0xE8, INLINE_DWORD ( ( ( char * ) &saveReplayCb ) - 0x4830A4 - 5 ),   // call callback
        0x90, //0x90, 0x90, 0x90, 0x90, 0x90,
        0xE9, 0xF2, 0xFE, 0xFF, 0xFF,                               // jmp 0x482FA1
    } },
    { ( void * ) 0x482FA1, {
        //0x83, 0xC4, 0x04,                                         // add esp, 4
        0x90, 0x90, 0x90,
        //0xE9, 0x9E, 0xF8, 0xFF, 0xFF,                             // jmp 0x48283e
        0x8B, 0x83, 0xCC, 0x00, 0x00, 0x00,                         // mov eax, [ebx+CC]
        0xE9, 0x9E, 0xF8, 0xFF, 0xFF,                               // jmp 0x48284D
    } },
    // Write this last due to dependencies
    { ( void * ) 0x482839, {
        0xE9, 0x96, 0xFC, 0xFF, 0xFF,                               // jmp 0x4824D4
    } },
    // Write this last due to dependencies
    { ( void * ) 0x482847, {
        0xE9, 0x58, 0x08, 0x00, 0x00,                               // jmp 0x4830A4
        0x90,                                                       // nop
    } },
};

// This copies an auto replay save flag to a non-dynamic memory location.
// Used to detect when the auto replay save is done and the menu is up.
static const Asm detectAutoReplaySave =
    { ( void * ) 0x482D9B, {
        0x8D, 0x88, 0xD0, 0x00, 0x00, 0x00,                         // lea ecx,[eax+000000D0]
        0x89, 0x0D, INLINE_DWORD ( &autoReplaySaveStatePtr ),       // mov [&autoReplaySaveStatePtr],ecx
        // Rest of the code is unchanged, just shifted down
        0x59, 0x5E, 0x83, 0xC4, 0x10, 0xC2, 0x04, 0x00
    } };

// Skips check of if the game is already open
static const Asm multiWindow = { ( void * ) MULTIPLE_MELTY, { 0xEB } }; // jmp 0040D262

// Force the game to go to a certain mode
static const Asm forceGotoVersus    = { ( void * ) 0x42B475, { 0xEB, 0x3F } }; // jmp 0042B4B6
static const Asm forceGotoVersusCPU = { ( void * ) 0x42B475, { 0xEB, 0x5C } }; // jmp 0042B4D3
static const Asm forceGotoTraining  = { ( void * ) 0x42B475, { 0xEB, 0x22 } }; // jmp 0042B499
static const Asm forceGotoReplay  = { ( void * ) 0x42B475, { 0xE9, 0xC7, 0x00, 0x00, 0x00 } }; // jmp 0042B541

// Hijack the game's Escape key so we can control when it exits the game
static const Asm hijackEscapeKey =
    { ( void * ) 0x4A0070, {
        0x80, 0x3D, INLINE_DWORD ( &enableEscapeToExit ), 0x00,     // cmp byte ptr [&enableEscapeToExit],00
        0xA0, INLINE_DWORD ( 0x5544F1 ),                            // mov ax,[005544F1]
        0x75, 0x03,                                                 // jne 0x4A0081 (AFTER)
        0x66, 0x31, 0xC0,                                           // xor ax,ax
                                                                    // AFTER:
        0x24, 0x80,                                                 // and al,80
        // Rest of the code is unchanged, just shifted down
        0x33, 0xC9,                                                 // xor ecx,ecx
        0x3C, 0x80,                                                 // cmp al,80
        0x0F, 0x94, 0xC1,                                           // sete cl
        0x8B, 0xC1,                                                 // mov eax,ecx
        0xC3,                                                       // ret
    } };

// This increments a counter for each sound effect played,
// but only actually plays the sound if its muted or at zero plays.
static const AsmList filterRepeatedSfx =
{
    { ( void * ) 0x4DD836, {
        0xB8, INLINE_DWORD ( sfxMuteArray ),                        // mov eax,sfxMuteArray
        0xEB, 0x79,                                                 // jmp 0x4DD8B6
    } },
    { ( void * ) 0x4DD8B6, {
        0x80, 0x3C, 0x30, 0x00,                                     // cmp byte ptr [eax+esi],00
        0xE9, 0xB4, 0x02, 0x00, 0x00                                // jmp 0x4DDB73
    } },
    { ( void * ) 0x4DDB73, {
        0x0F, 0x84, 0x3A, 0x03, 0x00, 0x00,                         // je 0x4DDEB3
        0x58,                                                       // pop eax
        0xE9, 0x25, 0x04, 0x00, 0x00                                // jmp 0x4DDFA4
    } },
    { ( void * ) 0x4DDEB3, {
        0xB8, INLINE_DWORD ( sfxFilterArray ),                      // mov eax,sfxFilterArray
        0x80, 0x04, 0x30, 0x01,                                     // add byte ptr [eax+esi],01
        0xEB, 0x74                                                  // jmp 0x4DDF32
    } },
    { ( void * ) 0x4DDF32, {
        0x80, 0x3C, 0x30, 0x01,                                     // cmp byte ptr [eax+esi],01
        0x58,                                                       // pop eax
        0x0F, 0x87, 0xE6, 0x02, 0x00, 0x00,                         // ja 0x4DE223 (SKIP_SFX)
        0xEB, 0x65                                                  // jmp 0x4DDFA4
    } },
    { ( void * ) 0x4DDFA4, {
        0x8B, 0x3C, 0xB5, INLINE_DWORD ( 0x76C6F8 ),                // mov edi,[esi*4+0076C6F8]
        0xE9, 0x67, 0x02, 0x00, 0x00                                // jmp 0x4DE217 (PLAY_SFX)
    } },
    // Write this last due to dependencies
    { ( void * ) 0x4DE210, {
        0x50,                                                       // push eax
        0xE9, 0x20, 0xF6, 0xFF, 0xFF,                               // jmp 0x4DD836
        0x90                                                        // nop
                                                                    // PLAY_SFX:
                                                                    // test edi,edi
                                                                    // je 0x4DE220
                                                                    // call 0x4F3A0
                                                                    // add ebp,01
                                                                    // add esi,01
                                                                    // SKIP_SFX:
    } },
};

// Mutes the next playback of a specific sound effect
static const AsmList muteSpecificSfx =
{
    { ( void * ) 0x40EEA1, {
        0x8B, 0x14, 0x24,                                           // mov edx,[esp]
        0x81, 0xFA, INLINE_DWORD ( CC_SFX_ARRAY_LEN ),              // cmp edx,CC_SFX_ARRAY_LEN
        0xE9, 0x22, 0x03, 0x00, 0x00                                // jmp 0x40F1D1
    } },
    { ( void * ) 0x40F1D1, {
        0x0F, 0x8D, 0xC1, 0x01, 0x00, 0x00,                         // jnl 0x40F398 (AFTER)
        0xE9, 0xB6, 0x01, 0x00, 0x00                                // jmp 0x40F392
    } },
    { ( void * ) 0x40F392, {
        0x0F, 0x85, 0xCA, 0x00, 0x00, 0x00,                         // jne 0x40F462
                                                                    // AFTER:
        0x8B, 0x50, 0x3C,                                           // mov edx,[eax+3C]
        0x51,                                                       // push ecx
        0x56,                                                       // push esi
        0xEB, 0x3B                                                  // jmp 0x40F3D5
    } },
    { ( void * ) 0x40F462, {
        0x8D, 0x92, INLINE_DWORD ( sfxMuteArray ),                  // lea edx,[edx+sfxMuteArray]
        0xE9, 0x78, 0x06, 0x00, 0x00                                // jmp 0x40FAE5
    } },
    { ( void * ) 0x40FAE5, {
        0x80, 0x3A, 0x00,                                           // cmp byte ptr [edx],00
        0xC6, 0x02, 0x00,                                           // mov byte ptr [edx],00
        0xEB, 0x14                                                  // jmp 0x40FB01
    } },
    { ( void * ) 0x40FB01, {
        0x74, 0x05,                                                 // je 0x40FB03 (DONE_MUTE)
        0xB9, INLINE_DWORD ( DX_MUTED_VOLUME ),                     // mov ecx,DX_MUTED_VOLUME
                                                                    // DONE_MUTE:
        0xE9, 0x8B, 0xF8, 0xFF, 0xFF                                // jmp 0x40F398 (AFTER)
    } },
    // Write this last due to dependencies
    { ( void * ) 0x40F3D5, {
        0xE9, 0xC7, 0xFA, 0xFF, 0xFF                                // jmp 0x40EEA1
    } },
};

// Disables the code that sets the intro state to 0. This is so we can manually set it during rollback
static const Asm hijackIntroState = { ( void * ) 0x45C1F2, INLINE_NOP_SEVEN_TIMES };

// Prevent training mode music from reseting
static const Asm disableTrainingMusicReset = { ( void * ) 0x472C6D, { 0xEB, 0x05 } }; // jmp 00472C74

// Fix the super flash overlays on the two boss stages.
// This is done by changing the string that the games searches for in the stage config .ini.
// Search for 'IsGiantStage' in bg/BgList.ini inside 0000.p.
static const Asm fixBossStageSuperFlashOverlay = { ( void * ) 0x53B3C8, INLINE_FF_12_BYTES };

// The color load callback during character select
extern "C" void charaSelectColorCb();

// Inserts a callback just after the color data is loaded into memory, but before it is read into the sprites.
// The color values can be effectively overridden here. This is only effective during character select.
static const Asm hijackCharaSelectColors =
    { ( void * ) 0x489CD1, {
        0xE8, INLINE_DWORD ( ( ( char * ) &charaSelectColorCb ) - 0x489CD1 - 5 ),       // call charaSelectColorCb
        0x90, 0x90, 0x90,                                                               // nops
    } };

// The color load callback during loading state
extern "C" void loadingStateColorCb();

// Inserts a callback just after the color data is loaded into memory, but before it is read into the sprites.
// The color values can be effectively overridden here. This is only effective during the loading state.
static const AsmList hijackLoadingStateColors =
{
    { ( void * ) 0x448202, {
        0x50,                                                                           // push eax
        0xE8, INLINE_DWORD ( ( ( char * ) &loadingStateColorCb ) - 0x448202 - 1 - 5 ),  // call loadingStateColorCb
        0x58,                                                                           // pop eax
        0x85, 0xC0,                                                                     // test eax,eax
        0xEB, 0x38,                                                                     // jmp 0x448245
    } },
    { ( void * ) 0x448245, {
        0x89, 0x44, 0x24, 0x20,                                                         // mov [esp+20],eax
        0xE9, 0x81, 0x09, 0x00, 0x00,                                                   // jmp 0x448BCF (AFTER)
    } },
    // Write this last due to dependencies
    { ( void * ) 0x448BC9, {
        0xE9, 0x34, 0xF6, 0xFF, 0xFF,                                                   // jmp 0x448202
        0x90,                                                                           // nop
                                                                                        // AFTER:
    } },
};

// Don't render Game UI
static const AsmList disableHealthBars =
{
    { ( void * ) 0x425235, INLINE_NOP_SEVEN_TIMES }, // renderCharaIcon->nop
    { ( void * ) 0x42523c, INLINE_NOP_SEVEN_TIMES }, // renderMoon->nop
    { ( void * ) 0x425253, INLINE_NOP_FIVE_TIMES },  // renderTimer->nop
    { ( void * ) 0x425226, INLINE_NOP_SIX_TIMES }, // renderGuardBar->nop
    //{ ( void * ) 0x42522C, INLINE_NOP_SIX_TIMES },
    { ( void * ) 0x425232, { 0x83, 0xC4, 0x04 } },
    { ( void * ) 0x424E03, { 0xE9, 0x1E, 0x04, 0x00, 0x00, 0x90 } }, // jmp 00425226
    //{ ( void * ) 0x424E03, { 0xE9, 0x4B, 0x04, 0x00, 0x00, 0x90 } }, // jmp 00425253
};

extern "C" void addExtraDrawCallsCb();

static const AsmList addExtraDraws =
{
    { ( void * ) 0x432CD2, {
            0xE8, INLINE_DWORD ( ( ( char * ) &addExtraDrawCallsCb ) - 0x432CD2 - 5 ),       // call charaSelectColorCb
            0x6A, 0xFF,                                          // Push -1
            0xE9, 0x54, 0x00, 0x00, 0x00                         // jmp 432D32
        } },
    { ( void * ) 0x432D30, { 0xEB, 0xA0 } }, // jmp 00424E09
};

extern "C" void addExtraTexturesCb();

static const AsmList addExtraTextures =
{
     /*{ ( void * ) 0x41c0f1, {
         0xE8, INLINE_DWORD ( ( ( char * ) &callbackExtraTextures ) - 0x41c0f1 - 5 ),       // call addExtraTexturesCb
         0x8B, 0x8C, 0x24, 0x1C, 0x01, 0x00, 0x00
         } },*/
    { ( void * ) 0x41BE38, {
         0xE8, INLINE_DWORD ( ( ( char * ) &addExtraTexturesCb ) - 0x41BE38 - 5 ),       // call addExtraTexturesCb
         0xC3
    } },
};

__attribute__((noinline)) void battleResetCallback();

extern "C" {
    extern unsigned naked_fileLoadEBX;
} 
__attribute__((noinline)) void fileLoadHook();

__attribute__((naked, noinline)) void _naked_battleResetCallback();

extern "C" {
    extern DWORD naked_charTurnAroundState[4];
}

__attribute__((naked, noinline)) void _naked_charTurnAround();

__attribute__((naked, noinline)) void _naked_charTurnAround2();

__attribute__((naked, noinline)) void _naked_hitBoxConnect1();

__attribute__((naked, noinline)) void _naked_hitBoxConnect2();

__attribute__((naked, noinline)) void _naked_hitBoxConnect3();

__attribute__((naked, noinline)) void _naked_throwConnect1();

__attribute__((naked, noinline)) void _naked_throwConnect2();

__attribute__((naked, noinline)) void _naked_proxyGuard();

__attribute__((naked, noinline)) void _naked_dashThrough();

__attribute__((naked, noinline)) void _naked_fileLoad();

__attribute__((naked, noinline)) void _naked_collisionConnect();

__attribute__((naked, noinline)) void _naked_checkRoundDone();

__attribute__((naked, noinline)) void _naked_checkRoundDone2();

__attribute__((naked, noinline)) void _naked_checkWhoWon();

__attribute__((noinline)) void cameraMod();

__attribute__((naked, noinline)) void _naked_cameraMod();

// -----

__attribute__((naked, noinline)) void _naked_drawWinCount();

__attribute__((naked, noinline)) void _naked_drawRoundDots();

// -----

__attribute__((naked, noinline)) void _naked_cssTest();

__attribute__((naked, noinline)) void _naked_modifyCSSPixelDraw();

__attribute__((noinline)) void drawTextureLogger();

__attribute__((naked, noinline)) void _naked_drawTextureLogger();

__attribute__((naked, noinline)) void _naked_CSSConfirmExtend();

__attribute__((naked, noinline)) void _naked_CSSWaitToSelectStage();

__attribute__((naked, noinline)) void _naked_cssInit();

__attribute__((naked, noinline)) void _naked_stageSelCallback();

__attribute__((noinline)) void palettePatcher();

__attribute__((naked, noinline)) void _naked_paletteHook();

__attribute__((naked, noinline)) void _naked_paletteCallback();

// -----

__attribute__((noinline)) void getLinkedListElementCallback();

__attribute__((naked, noinline)) void _naked_getLinkedListElementCallback();

__attribute__((noinline)) void modifyLinkedList();

__attribute__((naked, noinline)) void _naked_modifyLinkedList();

// tracking funcs to keep track of whats being drawn

__attribute__((naked, noinline)) void _naked_trackListCharacterDraw();

__attribute__((naked, noinline)) void _naked_trackListShadowDraw();

__attribute__((naked, noinline)) void _naked_trackListEffectDraw();

__attribute__((naked, noinline)) void _naked_linkedListMalloc();

extern "C" {
    __attribute__((noinline, cdecl)) void mallocCallback(DWORD naked_mallocHook_ret, DWORD naked_mallocHook_result, DWORD naked_mallocHook_size);
    __attribute__((naked, noinline)) void _naked_mallocCallback();
    __attribute__((naked, noinline)) void _naked_mallocHook();
}

static const AsmList initPatch2v2 =
{ 

    /*
    
    todo general:
    
        make hud and everything ingame nicer
        make CSS nicer
        explain new controller input screen in a way that doesnt confuse everyone
            "reading the screen explains the screen"
        netplay???

        pointers when chars are offscreen?

        maybe camera changes?

        require FN1 to be bound on all players, flash a color, blah blah yea

    todo ingame:

        double pushback

        proration is stored in the attacker, not defender.
            this is not an easy fix, unless i do some super weird things

        corner priority is suuuuper fucked up, look into it
            might still be fucked

        p2/p3 anims on round end are wrong

        roog needs new knife sprites

        blood heat still causes the meter bar flash

        weird thing where you can lose your jump if your teamate is comboing?

        fix round ends, re add timer back in

        allow for p2/p3 combo count, reduce, counter, etc

        (f)koha round end last arc crash? (maybe didnt wait for anims properly)

    todo css:
    
        pretty
        fix p2/3 not being able to cancel/change bg

        right side mirrordraw also mirrors the moons
        resetting char doersnt reset moon, does reset palette 

            if a moons selected and you press B, and then go back in, keep moon
            if a palette is selected, and you press B, keep palette
            stuffs only reset on moving

    */

    // i actually prefer this patch method(with a lil modification) tbh. its very well done
    
    // battle reset patches:

    { ( void * ) (0x00426810 + 2), { 0x04 }}, // ensure that all 4 characters are loaded properly on reset

    // the reset func can ret early, patch that
    PATCHJUMP(0x004234b9, 0x004234e1),

    // patch the jump to our function
    PATCHJUMP(0x004234e4, _naked_battleResetCallback),

    // patch the port comparison for chars turning around
    //PATCHJUMP(0x0047587b, _naked_charTurnAround),

    PATCHJUMP(0x00475820, _naked_charTurnAround2),

    // i quite literally, do not know what these two patches do!
    // im keeping them here, but pleaes keep that in mind
    PATCHJUMP(0x0046f207, _naked_hitBoxConnect1), // im unsure if this patch is needed. totzally does something though

    PATCHJUMP(0x00468127, _naked_hitBoxConnect2), // im unsure if this patch is needed. this patch was def needed. shielding issues?

    PATCHJUMP(0x0046f67e, _naked_hitBoxConnect3), //, patch is def needed

    PATCHJUMP(0x0046ea27, _naked_collisionConnect), // collision, patch this loop ig

    PATCHJUMP(0x004641b2, _naked_throwConnect1), // this patches grabs, not command grabs and not hitgrabs
    
    PATCHJUMP(0x0046fa65, _naked_throwConnect2), // patches hit/cmd grabs

    PATCHJUMP(0x00462b87, _naked_proxyGuard),

    PATCHJUMP(0x0046e8c9, _naked_dashThrough),

    // zombies are most likely called by me cutting off execution in right at 00474643. something after it must do something to disable it?

    PATCHJUMP(0x0047463c, _naked_checkRoundDone), // prevents game from ending until a team dies

    PATCHJUMP(0x004735ed, _naked_checkRoundDone2), // possibly unneeded stack clear patch

    PATCHCALL(0x00474759, _naked_checkWhoWon),

    { ( void *) (0x00425253), INLINE_NOP_FIVE_TIMES }, // stop showing timer. (i ate it)

    { ( void *) (0x004736fc + 2), { 0x10 }}, // check for each player in this loop

    { ( void *) (0x0048c7d0), { 0xC3 }}, // ret early, allow for for of the same palette 

    //PATCHJUMP(0x0041f7c0, _naked_fileLoad),

    { ( void *) (0x004773ad + 2), { 0xCC }}, // let p2/p3 do damage. dont ask me how i know.

    { ( void *) (0x00448fb6 + 2), { INLINE_DWORD(0x0200) }},
    { ( void *) (0x00449069 + 2), { 0x04 }},

    //PATCHJUMP(0x0044b834, _naked_cameraMod),

    // HUD patches. tbh, most patches here should be removed

    { ( void *) (0x0046040a), INLINE_NOP_FIVE_TIMES }, // this eventually causes the flicker when in not normal meter state. i should look into where it interacts with the linked list, or with the effects array

    // todo, fix this!
    
    { ( void *) (0x00424a60), INLINE_NOP_FIVE_TIMES }, // sion bullets count
    { ( void *) (0x00424abc), INLINE_NOP_FIVE_TIMES }, // sion bullets
    
    { ( void *) (0x004249f7), INLINE_NOP_FIVE_TIMES }, // roa charge count
    { ( void *) (0x004249cd), INLINE_NOP_FIVE_TIMES }, // roa charge

    // maids are unsupported. so i dont care

    //{ ( void *) (0x0042494c), INLINE_NOP_FIVE_TIMES }, // round tracking dots 
    
    PATCHJUMP(0x0042494c, _naked_drawRoundDots), // leaving this one in, tired

    //{ ( void *) (0x00424bde), INLINE_NOP_FIVE_TIMES }, // draw win count
    //{ ( void *) (0x00424bdb), { 0x90 } }, // push for above

    //PATCHJUMP(0x00426c67, _naked_drawWinCount), // needs to be remade!
    { ( void *) (0x00426c67), INLINE_NOP_FIVE_TIMES }, 
    
    { ( void *) (0x0042485b), INLINE_NOP_FIVE_TIMES}, // patch out draws.

    { ( void * ) (0x004253e6 + 1), { 0x04 }},// allow for 4 calls in meter bar draw.

    { ( void * ) (0x00425a84 + 2), { 0x04 }}, // allow for 4 draw calls to occur in the drawPortraitsAndNames loop

    { ( void * ) (0x00425907 + 2), { 0x04 }}, // allow for 4 draw calls during drawMoonsAndPalette

    // css 

    //PATCHJUMP(0x00485cde, _naked_cssTest)

    { ( void * ) (0x00489d78 + 1), { INLINE_DWORD(0x770) } },
    { ( void * ) (0x00489f99 + 2), { INLINE_DWORD(0x770) } },

    { ( void * ) (0x00489d14 + 1), { INLINE_DWORD(0x770) } }, // css thread loader

    { ( void * ) (0x0048a6d0 + 2), { INLINE_DWORD(0x74d994) } },

    { ( void * ) (0x0048aae1 + 2), { 0x04 } },

    { ( void * ) (0x00485CA9), { 0x8B, 0xC5, 0x24, 0x01, 0x90 } }, // hacky bytecode to fix char facing. i didnt want to write another func jump ok?

    PATCHJUMP(0x0048aad6, _naked_modifyCSSPixelDraw),

    //PATCHJUMP(0x00415580, _naked_drawTextureLogger)

    { ( void * ) (0x0048aa28), INLINE_NOP_FIVE_TIMES}, // name draws caused crashes.
    { ( void * ) (0x0048aa97), INLINE_NOP_FIVE_TIMES}, // name draws caused crashes.
    { ( void * ) (0x0048a85e), INLINE_NOP_FIVE_TIMES}, // stop drawing big preview
    { ( void * ) (0x0048a995), INLINE_NOP_FIVE_TIMES}, // stop drawing the shadow behind the big previews
    
    { ( void * ) (0x00427231), INLINE_NOP_SIX_TIMES}, // stop the game from taking in CSS controls

    DISABLECALL(0x0048bea1), // disable rect selector
    DISABLECALL(0x0048bee5), // disable flashing p1/p2

    { ( void * ) (0x0048bd5c + 4), { 0x04 }}, // draw 4 big previews on css 

    DISABLECALL(0x004875d9), // dont draw black gradient on side

    PATCHJUMP(0x0048cb4b, _naked_cssInit),

    PATCHJUMP(0x004888e2, _naked_stageSelCallback),
    PATCHJUMP(0x004888f0, _naked_stageSelCallback),

    PATCHJUMP(0x0041f7c0, _naked_paletteHook),
    PATCHJUMP(0x0041f87a, _naked_paletteCallback),

    PATCHJUMP(0x00486328, 0x0048637b), // prevent code that causes crash with palettes

    //PATCHJUMP(0x00427563, _naked_CSSConfirmExtend),
    
    //PATCHJUMP(0x0042794c, _naked_CSSWaitToSelectStage)

    //{ ( void * ) (0x0048cb39 + 1), { INLINE_DWORD(0x7746a0) } } // patch the css loader to init 4 times. this causes crashes when coming in????

    // renderer modifications

    // malloc logger, for some reason

    PATCHJUMP(0x004e0230, _naked_mallocHook),

    //{ ( void * ) (0x004b3b1d + 1), { INLINE_DWORD(sizeof(LinkedListAssetElement)) }}, // malloc of the linkedListAllAssets
    //{ ( void * ) (0x004b3b27 + 1), { INLINE_DWORD(sizeof(LinkedListAssetElement)) }}, // memset of the linkedListAllAssets
    //{ ( void * ) (0x00414ee0 + 1), { INLINE_DWORD(sizeof(LinkedListAssetElement)) }},
        
    // find where the above things are copied, and patch them. im not even sure if my using the space i thought was mine but wasnt was causing these issues? but i need to be sure



    //PATCHJUMP(0x00416329, _naked_getLinkedListElementCallback),

    //PATCHJUMP(0x004331d4, _naked_modifyLinkedList), // the reason im doing this at 004331d4 and not 0040e48e is bc the game would close, but not actually exit the process. i need the mutex, i am hoping
    //PATCHJUMP(0x0040e48e, _naked_modifyLinkedList),

    // all these patches are so i can track what is actually being drawn, per draw call. (there has to be an easier way of doing this)
    // something that, ugh. assuming i only overwrite instructions which properly align to 5 bytes, i could,, overwrite them, copy them into a func, but like at that point whatthefuck

    //PATCHJUMP(0x0041b3c9, _naked_trackListCharacterDraw),
    //PATCHJUMP(0x0041b47c, _naked_trackListShadowDraw),
    //PATCHJUMP(0x0045410a, _naked_trackListEffectDraw),

};

static const AsmList patch2v2 = 
{

    // patch all character port numbers
    { ( void * ) (0x00555424 + (0 * 0xAFC)), { 0x00 }},
    { ( void * ) (0x00555424 + (1 * 0xAFC)), { 0x01 }},
    { ( void * ) (0x00555424 + (2 * 0xAFC)), { 0x02 }},
    { ( void * ) (0x00555424 + (3 * 0xAFC)), { 0x03 }},

    // patch all character background states
    { ( void * ) (0x005552A8 + (0 * 0xAFC)), { 0x00 }},
    { ( void * ) (0x005552A8 + (1 * 0xAFC)), { 0x00 }},
    { ( void * ) (0x005552A8 + (2 * 0xAFC)), { 0x00 }},
    { ( void * ) (0x005552A8 + (3 * 0xAFC)), { 0x00 }},

    // im not sure what this ref is to, but i think it has to do with duo characters?
    { ( void * ) (0x00555130 + 0x32C + (0 * 0xAFC)), { INLINE_DWORD(0x00) } },
    { ( void * ) (0x00555130 + 0x32C + (1 * 0xAFC)), { INLINE_DWORD(0x00) } },
    { ( void * ) (0x00555130 + 0x32C + (2 * 0xAFC)), { INLINE_DWORD(0x00) } },
    { ( void * ) (0x00555130 + 0x32C + (3 * 0xAFC)), { INLINE_DWORD(0x00) } },

    // fixes hisui and koha missing moves
    { ( void * ) (0x00557DB8 + 0xC + (0 * 0x20C)), { INLINE_DWORD(0x00) } },
    { ( void * ) (0x00557DB8 + 0xC + (1 * 0x20C)), { INLINE_DWORD(0x00) } },
    { ( void * ) (0x00557DB8 + 0xC + (2 * 0x20C)), { INLINE_DWORD(0x00) } },
    { ( void * ) (0x00557DB8 + 0xC + (3 * 0x20C)), { INLINE_DWORD(0x00) } },

};


} // namespace AsmHacks
