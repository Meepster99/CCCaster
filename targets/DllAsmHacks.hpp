#pragma once

#include "Constants.hpp"
#include "Exceptions.hpp"
#include "DllNetplayManager.hpp"

#include <vector>
#include <array>

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

#define INT3 __asmStart R"( int3; )" __asmEnd

#define NOPS __asmStart R"( nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; )" __asmEnd

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

__attribute__((naked, noinline)) void _naked_charTurnAround();

__attribute__((naked, noinline)) void _naked_hitBoxConnect1();

__attribute__((naked, noinline)) void _naked_hitBoxConnect2();

__attribute__((naked, noinline)) void _naked_hitBoxConnect3();

__attribute__((naked, noinline)) void _naked_throwConnect();

__attribute__((naked, noinline)) void _naked_fileLoad();

__attribute__((naked, noinline)) void _naked_collisionConnect();

__attribute__((naked, noinline)) void _naked_checkRoundDone();

__attribute__((naked, noinline)) void _naked_checkRoundDone2();

__attribute__((naked, noinline)) void _naked_checkWhoWon();

static const AsmList initPatch2v2 =
{
    
    // todo, dont reface when comboing
    // todo, camera, max zoom maybe?
    // fn1/fn2 change what char you are focusing?
    // or i can have it lock on to a combo, prio if you are hitting 
    
    // if blocking and a teamate hits you,, proxy block occurs
    // draw sion bullets/roa electric thingy for p2/p3

    // not getting jump cancel back under certain circumstances if teamate is comboing(could this relate to the weird hud code that deals with that? remember when disabling hud fucked up that?)
    // if p2 dies, first? and,,, then the round ends? it only reads p2 and p1 for round determine
    // chisui is missing some,,, item toss move?

    // it needs to occur when initially loading into a game
    // i actually prefer this patch method(with a lil modification) tbh. its very well done
    
    // battle reset patches:

    { ( void * ) (0x00426810 + 2), { 0x04 }}, // ensure that all 4 characters are loaded properly on reset

    // the reset func can ret early, patch that
    PATCHJUMP(0x004234b9, 0x004234e1),

    // patch the jump to our function
    PATCHJUMP(0x004234e4, _naked_battleResetCallback),

    // patch the port comparison for chars turning around
    PATCHJUMP(0x0047587b, _naked_charTurnAround),

    PATCHJUMP(0x0046f207, _naked_hitBoxConnect1), // im unsure if this patch is needed

    PATCHJUMP(0x00468127, _naked_hitBoxConnect2), // im unsure if this patch is needed

    PATCHJUMP(0x0046f67e, _naked_hitBoxConnect3),//, patch is def needed

    PATCHJUMP(0x0046ea27, _naked_collisionConnect), // collision, patch this loop ig
    
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
    { ( void *) (0x00449069 + 2), { 0x04 }}

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
    { ( void * ) (0x0055545C + (0 * 0xAFC)), { INLINE_DWORD(0x00) } },
    { ( void * ) (0x0055545C + (1 * 0xAFC)), { INLINE_DWORD(0x00) } },
    { ( void * ) (0x0055545C + (2 * 0xAFC)), { INLINE_DWORD(0x00) } },
    { ( void * ) (0x0055545C + (3 * 0xAFC)), { INLINE_DWORD(0x00) } }

};


} // namespace AsmHacks
