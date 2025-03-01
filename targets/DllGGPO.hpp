#pragma once

#include "NetLogger.hpp"

#include <cstddef>
#include "ggponet.h"
#include <vector>

#include <windows.h>

#include "DllAsmHacks.hpp"

/*

fuck all of this, everyone, and the existence of computers in general
if i had one wish in the world, it would be to go back in time, and murder everyone who even dared to do anything but be a hunter gatherer
agriculture was not meant to happen
we were not made for this 

also GOD just because i love each and every one of you so SO much that its
ill extend this to 2P as well

ok heres the vibe
command line program

4 params, 3 of which are an IP, one of which is a port

omfg.exe ip0:port ip1:port port ip3:port

no way this works without port forwarding

in the future, ppl connect to one central host(via a seperate tcp protocol wowee how exciting), that will do that bs 
if people want to switch sides while playing, they can explode

knowing me, ill have the test version be what ppl actually use
but also knowing me, ill have so many people be unable to count to 3 properly that i will have to fix it regardless.

issue
rollbacking css? what the fuck?
i dont even want to know what caster does for that
and while part of me doesnt want to reinvent the wheel, it would also be better if there was no wheel at all. 
as in, no rollbacks on css

ugh
ill fix that when i get to it i guess

*/

typedef struct GGPOInput {

    BYTE dir = 0;
    BYTE btn = 0;

    void read(int p);
    void write(int p);

} GGPOInput;


__attribute__((noinline, cdecl)) void ggpoAdvanceFrame();
__attribute__((naked, noinline, cdecl)) void _naked_ggpoAdvanceFrame();

__attribute__((noinline, cdecl)) void ggpoControllerHook();
__attribute__((naked, noinline, cdecl)) void _naked_ggpoControllerHook();

#define GGPOPLAYERNUM (4)
#define GGPOSPECTATENUM (1) 

namespace GGPO {

    extern bool isGGPOOnline;

    extern GGPOSession* ggpo; // this var, in vectorwars, isnt ever accessed, as far as i can tell. what?????
    // The GGPOSession object is your interface to the GGPO framework. my ass
    // create one with ggponet_start_session, when that isnt even called in the example?? // edit2, im high??
    // can i even put this in a namespace??
    extern GGPOSessionCallbacks cb; // unsure if this should be in here, or stack alloced

    extern GGPOPlayerHandle handles[GGPOPLAYERNUM + GGPOSPECTATENUM]; // im not sure if this should be just players or players+spec
    extern GGPOPlayer       players[GGPOPLAYERNUM + GGPOSPECTATENUM];

    extern int ourPlayerNum;

    extern GGPOInput inputs[4];
    void writeAllGGPOInputs();

    void initGGPO();

    bool __cdecl mb_begin_game_callback(const char *);
    bool __cdecl mb_on_event_callback(GGPOEvent *info);
    bool __cdecl mb_advance_frame_callback(int);
    bool __cdecl mb_load_game_state_callback(unsigned char *buffer, int len);
    bool __cdecl mb_save_game_state_callback(unsigned char **buffer, int *len, int *checksum, int);
    bool __cdecl mb_log_game_state(char *filename, unsigned char *buffer, int);
    void __cdecl mb_free_buffer(void *buffer);

    //void VectorWar_Init(HWND hwnd, unsigned short localport, int num_players, GGPOPlayer *players, int num_spectators);
    //void VectorWar_InitSpectator(HWND hwnd, unsigned short localport, int num_players, char *host_ip, unsigned short host_port);
    //void VectorWar_DrawCurrentFrame();
    //void VectorWar_AdvanceFrame(int inputs[], int disconnect_flags);
    //void VectorWar_RunFrame(HWND hwnd);
    //void VectorWar_Idle(int time);
    //void VectorWar_DisconnectPlayer(int player);
    //void VectorWar_Exit();


}

typedef struct PlayerSave {
	/*
	a thought:
	the player and effect structs are very similar. basically sorta kinda the same
	if you squint and have horrid vision.
	what about only backing up the first 0x33C of each player?
	*/
	// BYTE data[0xAFC];

	BYTE data[0x33C];
} PlayerSave;

typedef struct EffectSave {
	BYTE data[0x33C];
} EffectSave;

typedef struct SaveState {

    /*
    lots of stuff is todo here. 
    camera, rng, etc    
    */

	PlayerSave players[4];
	
	std::vector<EffectSave> effects;

	DWORD P1ControlledCharacter;
	DWORD P1NextControlledCharacter;
	DWORD P2ControlledCharacter;
	DWORD P2NextControlledCharacter;

	DWORD slowMo;

	int P1Inactionable;
	int P2Inactionable;
	int FrameTimer;
	int TrueFrameTimer;

	void save();
	void load();

} SaveState;

