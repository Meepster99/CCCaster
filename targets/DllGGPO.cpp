#include "DllGGPO.hpp"

#include <cstdlib>
#include <regex>
#include <fstream>
#include <string>
#include <vector>

#include "DllDirectX.hpp"

bool GGPO::isGGPOOnline = false;

GGPOSession* GGPO::ggpo = NULL;

GGPOSessionCallbacks GGPO::cb = { 0 };

int GGPO::ourPlayerNum = -1;

GGPOPlayerHandle GGPO::handles[GGPOPLAYERNUM + GGPOSPECTATENUM];

GGPOPlayer GGPO::players[GGPOPLAYERNUM + GGPOSPECTATENUM];

GGPOInput GGPO::inputs[4];

// -----

static bool isInAdvanceFrame = false;

void advanceFrame() {
    /*
        Advances the game state by exactly 1 frame using the inputs specified
        for player 1 and player 2.
    */

    // strong chance this doesnt work. i dont know where or when caster writes its inputs, god that section of code is fucked
    // but if its AFTER this, or threaded, (both are true i think??) then this just doesnt work
    // in an amazing stroke of luck, im actually ok.
    // caster does its shit at 0040d330
    // caster frameStepNormal MIGHT have something for me, if it isnt abstracted to hell
    // yup, abstracted to hell

    /*
    
    as far as i know, 00432c50 updates the game state.
    and 0040e390 is a full loop, including render
    
    
    */

    // i hook the end of the func when ggpo init is called, which is during draw, which means that advance gets called before a framestate even occurs
    // this stupid thing fixes that

    static bool firstRun = true;
    //log("in advanceFrame what firstRun: %d", firstRun); 

    
    /*if(firstRun) {
        firstRun = false;
        return;
    }*/

    //log("writing GGPO inputs");

    GGPO::writeAllGGPOInputs(); 

    
    //log("trying func 1");

    PUSH_ALL;
    // this func updates controls. i think. not sure
    emitCall(0x0048e0a0);
    POP_ALL; 
    

    
    //log("trying func 2");

    PUSH_ALL;
    // this func takes eax as an input.
    // im very very tired your honor
    // maybe not needed?
    emitCall(0x00432c50);
    POP_ALL;
    

    /*
    log("trying func 3");
    isInAdvanceFrame = true;

    PUSH_ALL;
    // this is lazy, i should do something to cut out the renderering part of this. or like,, 
    // ugh
    emitCall(0x0040e390);
    POP_ALL;

    isInAdvanceFrame = false;
    */

    //log("exited advanceframe melty fuckery");

    ggpo_advance_frame(GGPO::ggpo);
    ggpo_idle(GGPO::ggpo, 3);
    int disconnectFlags;
    ggpo_synchronize_input(GGPO::ggpo, GGPO::inputs, sizeof(GGPO::inputs), &disconnectFlags); 

    //log("exited advanceFrame");

}

// -----

void ggpoControllerHook() {

    log("inside ggpoControllerHook");

    int i = GGPO::ourPlayerNum;

    GGPOErrorCode result;

    int disconnectFlags; // no clue

    retryInputs:

    GGPO::inputs[i].read(i);

    result = ggpo_add_local_input(GGPO::ggpo, GGPO::handles[i], &GGPO::inputs[i], sizeof(GGPO::inputs[i]));

    if(result == GGPO_ERRORCODE_NOT_SYNCHRONIZED) {
        log("not synced. omfg ourPlayerNum == %d", i);
        Sleep(1000);
        goto retryInputs;
    }

    //log("local result %d", result);

    if (GGPO_SUCCEEDED(result)) {
        result = ggpo_synchronize_input(GGPO::ggpo, GGPO::inputs, sizeof(GGPO::inputs), &disconnectFlags); 
        //log("sync result %d", result);
        if (GGPO_SUCCEEDED(result)) {
            GGPO::writeAllGGPOInputs();
            ///* pass both inputs to our advance function */
            //AdvanceGameState(&p[0], &p[1], &gamestate);
            // normally here, i would call advance gamestate, but returning from this function literally does that because its hooked
            return;
        }
    }

    goto retryInputs;

}

void ggpoAdvanceFrame() {

    // i hook the end of the func when ggpo init is called, which is during draw, which means that advance gets called before a framestate even occurs
    // this stupid thing fixes that

    if(isInAdvanceFrame) { // should this check go before or after firstrun?????
        return; 
    }

    static bool firstRun = true;
    if(firstRun) {
        firstRun = false;
        return;
    }

    //logY("inside ggpoAdvanceFrame");

    //log("calling ggpo_advance_frame");
    ggpo_advance_frame(GGPO::ggpo);

    //log("calling ggpo_idle");
    ggpo_idle(GGPO::ggpo, 3);

    //int disconnectFlags;
    //ggpo_synchronize_input(GGPO::ggpo, GGPO::inputs, sizeof(GGPO::inputs), &disconnectFlags); 

}

void _naked_ggpoControllerHook() {

    // patched at 0x0040e390.
    // top of frame

    // overwritten asm:
    emitByte(0x51);

    emitByte(0x8B);
    emitByte(0x0D);
    emitByte(0xB0);
    emitByte(0xE6);
    emitByte(0x76);
    emitByte(0x00);

    PUSH_ALL;
    ggpoControllerHook();
    POP_ALL;

    emitJump(0x0040e397);

}

void _naked_ggpoAdvanceFrame() {

    // patched at 0040e5b3

    PUSH_ALL;
    ggpoAdvanceFrame();
    POP_ALL;

    ASMRET;
}


static const AsmHacks::AsmList patchGGPO = {

    PATCHJUMP(0x0040e390, _naked_ggpoControllerHook),
    PATCHJUMP(0x0040e513, _naked_ggpoAdvanceFrame)

};

// -----

void GGPO::writeAllGGPOInputs() {
    for(int i=0; i<4; i++) {
        inputs[i].write(i);
    }
}

void GGPO::initGGPO() {

    if(isGGPOOnline) {
        return;
    }

    isGGPOOnline = true;

    logG("Attempting to init GGPO. PLAYER %d");
    logG("patching ASM");
    
    for ( const AsmHacks::Asm& hack : patchGGPO ) {
        WRITE_ASM_HACK ( hack );
    }

    logG("doing actual ggpo bs");

    GGPOErrorCode result;

    cb.begin_game =      mb_begin_game_callback;
    cb.advance_frame =   mb_advance_frame_callback;
    cb.load_game_state = mb_load_game_state_callback;
    cb.save_game_state = mb_save_game_state_callback;
    cb.free_buffer =     mb_free_buffer;
    cb.on_event =        mb_on_event_callback;

    // is the input param sizeof(inputs) or sizeof(inputs[0])????
    //result = ggpo_start_session(&ggpo, &cb, "MELTY4V4", GGPOPLAYERNUM, sizeof(inputs[0]), 8001); // todo, need to actually grab the port correctly!!

    result = ggpo_start_synctest(&ggpo, &cb, "MELTY4V4", GGPOPLAYERNUM, sizeof(inputs[0]), 1);

    ggpo_set_disconnect_timeout(ggpo, 3000);
    ggpo_set_disconnect_notify_start(ggpo, 1000);


    // read the stupid config file to get connection info
    ourPlayerNum = -1;
    std::ifstream inFile("2v2Settings.txt");

    if (!inFile) {
        logR("2v2Settings.txt didnt fucking exist");
        return;
    }

    // how the FUCK do spectators fit in here.
    int fileIndex = 0;
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(inFile, line)) {
        std::string tempLine = UIManager::strip(line);
        if(tempLine.size() > 0) {
            lines.push_back(tempLine);
        }
    }

    // parse IP addrs for players, and set them up and spectators tooo
    std::regex re(R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})?:?(\d+))");
    std::smatch match;
    for(int i=0; i<lines.size(); i++) {
        logY("%s", lines[i].c_str());
        if (std::regex_match(lines[i], match, re)) {
            
            std::string ipAddr = match[1].matched ? match[1].str() : "";
		    std::string port = match[2].matched ? match[2].str() : "";
            
            log("%s %s", ipAddr.c_str(), port.c_str());

            if(i < GGPOPLAYERNUM) {
                players[i].size = sizeof(players[i]);
                players[i].player_num = i + 1;
                if(ipAddr.size() == 0) { // local player, port only
                    players[i].u.remote.port = std::stoi(port);
                    ourPlayerNum = i;
                    players[i].type = GGPO_PLAYERTYPE_LOCAL;
                } else { // remote player
                    strcpy(players[i].u.remote.ip_address, ipAddr.c_str());
                    players[i].u.remote.port = std::stoi(port);
                    players[i].type = GGPO_PLAYERTYPE_REMOTE;
                }
            } else {  // setup spectators. idk how they work. and its an absolute struggle to care. god
                players[i].size = sizeof(players[i]);
                players[i].player_num = i + 1;
                players[i].type = GGPO_PLAYERTYPE_SPECTATOR;
                strcpy(players[i].u.remote.ip_address, ipAddr.c_str());
                players[i].u.remote.port = std::stoi(port);
            }
        }
    }
    
    for(int i = 0; i < GGPOPLAYERNUM + GGPOSPECTATENUM; i++) {
        log("doing player %d", i);
        result = ggpo_add_player(ggpo, &players[i], &handles[i]);
    }

    ggpo_idle(ggpo, 5);

    logG("initGGPO ran, successfully,,,, somehow");

}

bool GGPO::mb_begin_game_callback(const char *) {
    logB("wowee mb_begin_game_callback");
    return true;
}

/*
* vw_on_event_callback --
*
* Notification from GGPO that something has happened.  Update the status
* text at the bottom of the screen to notify the user.
*/
bool GGPO::mb_on_event_callback(GGPOEvent *info) {
    logB("wowee mb_on_event_callback");
    return true;
}

/*
* vw_advance_frame_callback --
*
* Notification from GGPO we should step foward exactly 1 frame
* during a rollback.
*/
bool GGPO::mb_advance_frame_callback(int) {
    logB("wowee mb_advance_frame_callback");

    /*
    * advance_frame - Called during a rollback.  You should advance your game
    * state by exactly one frame.  Before each frame, call ggpo_synchronize_input
    * to retrieve the inputs you should use for that frame.  After each frame,
    * you should call ggpo_advance_frame to notify GGPO.net that you're
    * finished.
    *
    * The flags parameter is reserved.  It can safely be ignored at this time.
    */

    int disconnectFlags;
    ggpo_synchronize_input(GGPO::ggpo, GGPO::inputs, sizeof(GGPO::inputs), &disconnectFlags);
    advanceFrame();

    logB("leaving mb_advance_frame_callback");

    return true;
}

/*
* vw_load_game_state_callback --
*
* Makes our current state match the state passed in by GGPO.
*/
bool GGPO::mb_load_game_state_callback(unsigned char *buffer, int len) {
    logB("wowee mb_load_game_state_callback");

    ((SaveState*)buffer)->load();

    logB("leaving mb_load_game_state_callback");

    return true;
}

/*
* vw_save_game_state_callback --
*
* Save the current state to a buffer and return it to GGPO via the
* buffer and len parameters.
*/
bool GGPO::mb_save_game_state_callback(unsigned char **buffer, int *len, int *checksum, int) {
    logB("wowee mb_save_game_state_callback");

    *len = sizeof(SaveState);
    SaveState* newSave = new SaveState();
    newSave->save();
    *buffer = (unsigned char *)newSave;

    *checksum = 0;

    logB("wowee mb_save_game_state_callback");
    
    return true;
}

/*
* vw_log_game_state --
*
* Log the gamestate.  Used by the synctest debugging tool.
*/
bool GGPO::mb_log_game_state(char *filename, unsigned char *buffer, int) {
    logB("woweee mb_log_game_state");
    return true;
}

/*
* vw_free_buffer --
*
* Free a save state buffer previously returned in vw_save_game_state_callback.
*/
void GGPO::mb_free_buffer(void *buffer) {
    free(buffer);
}

// -----

void SaveState::save() {

	slowMo = *(WORD*)(0x0055d208);
	//reallyNotSure = *(DWORD*)(0x0055FD04);
	
	//GlobalFreeze              = *(BYTE*) (0x00400000 + adGlobalFreeze				 );
	//SaveDestinationCamX       = *(DWORD*)(0x00400000 + adSaveDestinationCamX		 );
	//SaveDestinationCamY       = *(DWORD*)(0x00400000 + adSaveDestinationCamY		 );
	
	//SaveCurrentCamX           = *(DWORD*)(0x00400000 + adSaveCurrentCamX			 );
	//SaveCurrentCamXCopy       = *(DWORD*)(0x00400000 + adSaveCurrentCamXCopy		 );
	//SaveCurrentCamY           = *(DWORD*)(0x00400000 + adSaveCurrentCamY			 );
	//SaveCurrentCamYCopy       = *(DWORD*)(0x00400000 + adSaveCurrentCamYCopy		 );
	//SaveCurrentCamZoom        = *(DWORD*)(0x00400000 + adSaveCurrentCamZoom			 );
	//SaveDestinationCamZoom    = *(DWORD*)(0x00400000 + adSaveDestinationCamZoom		 );
	//P1ControlledCharacter     = *(DWORD*)(0x00400000 + adP1ControlledCharacter		 );
	//P1NextControlledCharacter = *(DWORD*)(0x00400000 + adP1NextControlledCharacter	 );
	//P2ControlledCharacter     = *(DWORD*)(0x00400000 + adP2ControlledCharacter		 );
	//P2NextControlledCharacter = *(DWORD*)(0x00400000 + adP2NextControlledCharacter	 );
//
	//P1Inactionable            = *(int*)(0x00400000 + adP1Inaction);
	//P2Inactionable            = *(int*)(0x00400000 + adP2Inaction);
	//FrameTimer                = *(int*)(0x00400000 + adFrameCount);
	//TrueFrameTimer            = *(int*)(0x00400000 + adTrueFrameCount);
	
	//memcpy((void*)&playerSaves, (void*)0x00555130, 0xAFC * 4);

	memcpy((void*)&players[0], (void*)(0x00555130 + (0xAFC * 0)), 0x33C);
	memcpy((void*)&players[1], (void*)(0x00555130 + (0xAFC * 1)), 0x33C);
	memcpy((void*)&players[2], (void*)(0x00555130 + (0xAFC * 2)), 0x33C);
	memcpy((void*)&players[3], (void*)(0x00555130 + (0xAFC * 3)), 0x33C);

	// should we also save hit effect data? (would need to fix hit effect pausing for that)

	// effects start at 0x0067BDE8

    // i deserve all your ram
    for (int index = 0; index < 100; index += 1) { // should be 1000, im being generous
        effects.push_back(EffectSave()); 
        memcpy(&effects[effects.size() - 1], (void*)(0x0067BDE8 + (0x33C * index)), 0x33C);
    }
}

void SaveState::load() {

    //log("in SaveState load");

	*(WORD*)(0x0055d208) = slowMo;
	//*(DWORD*)(0x0055FD04) = reallyNotSure;

	//*(BYTE*) (0x00400000 + adGlobalFreeze				 ) = GlobalFreeze              ;
	//*(DWORD*)(0x00400000 + adSaveDestinationCamX		 ) = SaveDestinationCamX       ;
	//*(DWORD*)(0x00400000 + adSaveDestinationCamY		 ) = SaveDestinationCamY       ;
	
	//*(DWORD*)(0x00400000 + adSaveCurrentCamX			 ) = SaveCurrentCamX           ;
	//*(DWORD*)(0x00400000 + adSaveCurrentCamXCopy		 ) = SaveCurrentCamXCopy       ;	 
	//*(DWORD*)(0x00400000 + adSaveCurrentCamY			 ) = SaveCurrentCamY           ;
	//*(DWORD*)(0x00400000 + adSaveCurrentCamYCopy		 ) = SaveCurrentCamYCopy       ;
	//*(DWORD*)(0x00400000 + adSaveCurrentCamZoom			 ) = SaveCurrentCamZoom        ;
	//*(DWORD*)(0x00400000 + adSaveDestinationCamZoom		 ) = SaveDestinationCamZoom    ;
	//*(DWORD*)(0x00400000 + adP1ControlledCharacter		 ) = P1ControlledCharacter     ;
	//*(DWORD*)(0x00400000 + adP1NextControlledCharacter	 ) = P1NextControlledCharacter ;
	//*(DWORD*)(0x00400000 + adP2ControlledCharacter		 ) = P2ControlledCharacter     ;
	//*(DWORD*)(0x00400000 + adP2NextControlledCharacter	 ) = P2NextControlledCharacter ;
//
	//*(int*)(0x00400000 + adP1Inaction                    ) = P1Inactionable;
	//*(int*)(0x00400000 + adP2Inaction                    ) = P2Inactionable;
	//*(int*)(0x00400000 + adFrameCount                    ) = FrameTimer;
	//*(int*)(0x00400000 + adTrueFrameCount                ) = TrueFrameTimer;

	//memcpy((void*)0x00555130, (void*)&playerSaves, 0xAFC * 4);
	
	memcpy((void*)(0x00555130 + (0xAFC * 0)), (void*)&players[0], 0x33C);
	memcpy((void*)(0x00555130 + (0xAFC * 1)), (void*)&players[1], 0x33C);
    memcpy((void*)(0x00555130 + (0xAFC * 2)), (void*)&players[2], 0x33C);
    memcpy((void*)(0x00555130 + (0xAFC * 3)), (void*)&players[3], 0x33C);
	
	// restore effects
    memcpy((void*)0x0067BDE8, effects.data(), effects.size() * sizeof(EffectSave));

    //log("exit SaveState load");

}

void GGPOInput::read(int p) {

    DWORD baseAddr = 0x00771398 + (0x2C * p); // im not sure if this is the ideal addr to use for controls! maybe look at 00770F30?

    dir = *(BYTE*)(baseAddr + 0);

    btn |= *(BYTE*)(baseAddr + 1) ? 0x01 : 0x00;
    btn |= *(BYTE*)(baseAddr + 2) ? 0x02 : 0x00;
    btn |= *(BYTE*)(baseAddr + 3) ? 0x04 : 0x00;
    btn |= *(BYTE*)(baseAddr + 4) ? 0x08 : 0x00;

}

void GGPOInput::write(int p) {

    DWORD baseAddr = 0x00771398 + (0x2C * p); // im not sure if this is the ideal addr to use for controls! maybe look at 00770F30?

    *(BYTE*)(baseAddr + 0) = dir;

    *(BYTE*)(baseAddr + 1) = !!(btn & 0x01);
    *(BYTE*)(baseAddr + 2) = !!(btn & 0x02);
    *(BYTE*)(baseAddr + 3) = !!(btn & 0x04);
    *(BYTE*)(baseAddr + 4) = !!(btn & 0x08);

}
