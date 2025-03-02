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

GGPOInput GGPO::inputs[GGPOPLAYERNUM];

MemDumpList GGPO::rollbackAddrs;

// -----

// rollback addrs

static const std::vector<MemDump> playerAddrs =
{
    { 0x555130, 0x555140 }, // ??? 0x555130 1 byte: some timer flag
    { 0x555140, 0x555160 },
    { 0x555160, 0x555180 }, // ???
    { 0x555180, 0x555188 },
    { 0x555188, 0x555190 }, // ???
    { 0x555190, 0x555240 },
    ( uint32_t * ) 0x555240, // ???
    { 0x555244, 0x555284 },
    ( uint32_t * ) 0x555284, // ???
    { 0x555288, 0x5552EC },
    ( uint32_t * ) 0x5552EC, // ???
    { 0x5552F0, 0x5552F4 },
    { 0x5552F4, 0x555310 }, // ??? 0x5552F6, 2 bytes: Sion bullets, inverse counter
    { 0x555310, 0x55532C },

    { ( void * ) 0x55532C, 4 },
    // { ( void * ) 0x55532C, 4, {
    //     MemDumpPtr ( 0, 0x24, 1 ), // segfaulted on this once
    //     MemDumpPtr ( 0, 0x30, 2 ),
    // } },

    { 0x555330, 0x55534C }, // ???
    { 0x55534C, 0x55535C },
    { 0x55535C, 0x5553CC }, // ???

    { ( void * ) 0x5553CC, 4 }, // pointer to player struct?

    { 0x5553D0, 0x5553EC }, // ???

    { ( void * ) 0x5553EC, 4 }, // pointer to player struct?
    { ( void * ) 0x5553F0, 4 }, // pointer to player struct?

    { 0x5553F4, 0x5553FC },

    { ( void * ) 0x5553FC, 4 }, // pointer to player struct?
    { ( void * ) 0x555400, 4 }, // pointer to player struct?

    { 0x555404, 0x555410 }, // ???
    { 0x555410, 0x55542C },
    ( uint32_t * ) 0x55542C, // ???
    { 0x555430, 0x55544C },

    { ( void * ) 0x55544C, 4 }, // graphics pointer? this is accessed all the time even when paused

    { ( void * ) 0x555450, 4 }, // graphics pointer? this is accessed all the time even when paused
    // { ( void * ) 0x555450, 4, {
    //     MemDumpPtr ( 0, 0x00, 2 ),
    //     MemDumpPtr ( 0, 0x0C, 2 ),
    //     MemDumpPtr ( 0, 0x0E, 1 ),
    //     MemDumpPtr ( 0, 0x0F, 1 ),
    //     MemDumpPtr ( 0, 0x10, 2 ),
    //     MemDumpPtr ( 0, 0x12, 2 ),
    //     MemDumpPtr ( 0, 0x16, 2 ),
    //     MemDumpPtr ( 0, 0x1B, 1 ),
    //     MemDumpPtr ( 0, 0x1C, 1 ),
    //     MemDumpPtr ( 0, 0x2E, 2 ),
    //     MemDumpPtr ( 0, 0x38, 4, {
    //         MemDumpPtr ( 0, 0x00, 4, {
    //             MemDumpPtr ( 0, 0x00, 1 ),
    //             MemDumpPtr ( 0, 0x02, 2 ),
    //             MemDumpPtr ( 0, 0x04, 2 ),
    //             MemDumpPtr ( 0, 0x06, 1 ),
    //             MemDumpPtr ( 0, 0x08, 1 ),
    //         } ),
    //         MemDumpPtr ( 0, 0x08, 4, {
    //             MemDumpPtr ( 0, 0x00, 1 ),
    //             MemDumpPtr ( 0, 0x02, 1 ),
    //             MemDumpPtr ( 0, 0x06, 2 ),
    //             MemDumpPtr ( 0, 0x0C, 4 ),
    //         } ),
    //         MemDumpPtr ( 0, 0x0C, 1 ),
    //         MemDumpPtr ( 0, 0x11, 1 ),
    //         MemDumpPtr ( 0, 0x14, 1 ),
    //     } ),
    //     MemDumpPtr ( 0, 0x40, 1 ),
    //     MemDumpPtr ( 0, 0x41, 1 ),
    //     MemDumpPtr ( 0, 0x42, 1 ),
    //     MemDumpPtr ( 0, 0x44, 4 ), // more to this?
    //     MemDumpPtr ( 0, 0x4C, 4, {
    //         MemDumpPtr ( 0, 0, 4, {
    //             MemDumpPtr ( 0, 0x00, 2 ),
    //             MemDumpPtr ( 0, 0x02, 2 ),
    //             MemDumpPtr ( 0, 0x04, 2 ),
    //             MemDumpPtr ( 0, 0x06, 2 ),
    //         } ),
    //     } ),
    // } },

    { ( void * ) 0x555454, 4 }, // graphics pointer? this is accessed all the time even when paused

    { ( void * ) 0x555458, 4 }, // pointer to player struct?

    { 0x55545C, 0x555460 },

    // graphics pointer(s)? these are accessed all the time even when paused
    { ( void * ) 0x555460, 4 },
    // { ( void * ) 0x555460, 4, {
    //     MemDumpPtr ( 0, 0x0, 4, {
    //         MemDumpPtr ( 0, 0x4, 4, {
    //             MemDumpPtr ( 0, 0xC, 4 )
    //         } )
    //     } )
    // } },

    { 0x555464, 0x55546C },

    { ( void * ) 0x55546C, 4 }, // graphics pointer? this is accessed all the time even when paused

    { 0x555470, 0x55550C },
    ( uint32_t * ) 0x55550C, // ???
    { 0x555510, 0x555518 },

    { 0x555518, 0x55561A }, // input history (directions)
    { 0x55561A, 0x55571C }, // input history (A button)
    { 0x55571C, 0x55581E }, // input history (B button)
    { 0x55581E, 0x555920 }, // input history (C button)
    { 0x555920, 0x555A22 }, // input history (D button)
    { 0x555A22, 0x555B24 }, // input history (E button)

    { 0x555B24, 0x555B2C },
    { 0x555B2C, 0x555C2C }, // ???
};

static const std::vector<MemDump> miscAddrs =
{
    // The stack range before calling the main dll callback
    // { 0x18FEA0, 0x190000 },

    // Game state
    CC_ROUND_TIMER_ADDR,
    CC_REAL_TIMER_ADDR,
    CC_WORLD_TIMER_ADDR,
    CC_SLOW_TIMER_INIT_ADDR,
    CC_SLOW_TIMER_ADDR,
    CC_INTRO_STATE_ADDR,
    CC_INPUT_STATE_ADDR,
    CC_SKIPPABLE_FLAG_ADDR,

    CC_RNG_STATE0_ADDR,
    CC_RNG_STATE1_ADDR,
    CC_RNG_STATE2_ADDR,
    { CC_RNG_STATE3_ADDR, CC_RNG_STATE3_SIZE },

    // Unknown states
    ( uint32_t * ) 0x563864,
    ( uint32_t * ) 0x56414C,

    // Graphical effects
    { CC_GRAPHICS_ARRAY_ADDR, CC_GRAPHICS_ARRAY_SIZE },
    CC_GRAPHICS_COUNTER,

    CC_SUPER_FLASH_PAUSE_ADDR,
    CC_SUPER_FLASH_TIMER_ADDR,

    { CC_SUPER_STATE_ARRAY_ADDR, CC_SUPER_STATE_ARRAY_SIZE },

    // Player state
    { CC_P1_EXTRA_STRUCT_ADDR, CC_EXTRA_STRUCT_SIZE },
    { CC_P2_EXTRA_STRUCT_ADDR, CC_EXTRA_STRUCT_SIZE },

    CC_P1_WINS_ADDR,
    CC_P2_WINS_ADDR,

    CC_P1_GAME_POINT_FLAG_ADDR,
    CC_P2_GAME_POINT_FLAG_ADDR,

    // HUD misc graphics
    CC_METER_ANIMATION_ADDR,
    CC_P1_SPELL_CIRCLE_ADDR,
    CC_P2_SPELL_CIRCLE_ADDR,

    // HUD status message graphics
    { CC_P1_STATUS_MSG_ARRAY_ADDR, CC_STATUS_MSG_ARRAY_SIZE },
    { CC_P2_STATUS_MSG_ARRAY_ADDR, CC_STATUS_MSG_ARRAY_SIZE },

    // Intro / outro graphics
    ( uint32_t * ) 0x74D9D0,
    ( uint32_t * ) 0x74E4E4,
    ( float * ) 0x74E4E8,
    // ( uint32_t * ) 0x76E79C,

    // Intro graphics/music/voice
    ( uint32_t * ) 0x74D598,
    ( uint32_t * ) 0x74E5B0,
    ( uint32_t * ) 0x74E768,
    { 0x74E770, 0x74E784 },
    { 0x74E78C, 0x74E798 },
    { 0x74E79C, 0x74E7A8 },
    { 0x74E7AC, 0x74E7C0 },
    { 0x74E7C8, 0x74E7D8 },
    { 0x74E7DC, 0x74E7E0 },
    { 0x74E7E4, 0x74E7F4 },
    { 0x74E7F8, 0x74E808 },
    { 0x74E80C, 0x74E810 },
    { 0x74E814, 0x74E828 },
    { 0x74E82C, 0x74E834 },
    { 0x74E838, 0x74E84C },
    { 0x74E850, 0x74E858 },
    { 0x74E85C, 0x74E86C },

    // Intro graphics state part 2
    { 0x76E780, 0x76E78C },

    // Camera position state
    ( uint32_t * ) 0x555124,
    ( uint32_t * ) 0x555128,
    { 0x5585E8, 0x5585F4 },
    { 0x55DEC4, 0x55DED0 },
    { 0x55DEDC, 0x55DEE8 },
    { 0x564B14, 0x564B20 },

    // More camera position state
    ( uint16_t * ) 0x564B10,
    ( uint32_t * ) 0x563750,
    ( uint32_t * ) 0x557DB0,
    ( uint32_t * ) 0x557DB4,

    ( uint8_t * ) 0x557D2B,
    ( uint16_t * ) 0x557DAC,
    ( uint16_t * ) 0x559546,
    ( uint16_t * ) 0x564B00,
    ( uint32_t * ) 0x76E6F8,
    ( uint32_t * ) 0x76E6FC,
    ( uint32_t * ) 0x7B1D2C,

    // Camera scaling state
    ( uint32_t * ) 0x55D204,
    ( uint32_t * ) 0x56357C,
    ( uint32_t * ) 0x55DEE8,
    ( uint32_t * ) 0x564B0C,
    ( uint32_t * ) 0x564AF8,
    ( uint32_t * ) 0x564B24,
    ( uint32_t * ) 0x76E6F4,

    CC_CAMERA_SCALE_1_ADDR,
    CC_CAMERA_SCALE_2_ADDR,
    CC_CAMERA_SCALE_3_ADDR,
};

static const MemDump firstEffect ( CC_EFFECTS_ARRAY_ADDR, CC_EFFECT_ELEMENT_SIZE, {
    MemDumpPtr ( 0x320, 0x38, 4, {
        MemDumpPtr ( 0, 0, 4, {
            MemDumpPtr ( 0, 0, 4 )
        } )
    } )
} );

static const std::vector<MemDump> extra2v2Addrs = 
{
    // specific bs for 2v2
    // no clue if this will work

    { (void*)&AsmHacks::naked_charTurnAroundState[0], sizeof(AsmHacks::naked_charTurnAroundState) },
    { (void*)&AsmHacks::FN1States[0], sizeof(AsmHacks::FN1States) }

    
};


// -----

static bool isInAdvanceFrame = false;

extern "C" {
    DWORD _naked_shouldUpdateGameState = 0;
}

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

    // this isnt needed bc this gets called in the,,, controller callback right?
    // NO THATS AT START OF GAMELOOP 0, NOT THIS
    int disconnectFlags;
    ggpo_synchronize_input(GGPO::ggpo, GGPO::inputs, sizeof(GGPO::inputs), &disconnectFlags); 

    
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

    GGPOErrorCode result;

    result = ggpo_advance_frame(GGPO::ggpo);

    if(!GGPO_SUCCEEDED(result)) {
        logR("advanceFrame's ggpo_advance_frame returned %d", result);
    }

    //ggpo_idle(GGPO::ggpo, 3);
    //int disconnectFlags;
    //ggpo_synchronize_input(GGPO::ggpo, GGPO::inputs, sizeof(GGPO::inputs), &disconnectFlags); 

    //log("exited advanceFrame");

}

void ggpoControllerHook() {

    //log("inside ggpoControllerHook");

    int i = GGPO::ourPlayerNum;

    GGPOErrorCode result = GGPO_OK;

    int disconnectFlags; // no clue

    retryInputs:

    // putting this here, because,,, thats the one thing that this loop doesnt do, but now im calling idle all over the place?
    // and this is called before the next frame even starts?????
    // yea that fixed it. this is CRUCIAL.
    ggpo_idle(GGPO::ggpo, 5);

    GGPO::inputs[i].read(i);

    if(GGPO::handles[i] != GGPO_INVALID_HANDLE) { // why is this here? no clue, but im just emulating vectorwar with christian like suspicion
        result = ggpo_add_local_input(GGPO::ggpo, GGPO::handles[i], &GGPO::inputs[i], sizeof(GGPO::inputs[i]));
    }
    
    //GGPO::inputs[i].log();

    if(result == GGPO_ERRORCODE_NOT_SYNCHRONIZED) {
        //log("not synced. omfg ourPlayerNum == %d", i);
        //Sleep(50);
        //goto retryInputs;
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
            // this is actually a massive issue, because by hanging gamestate in this func, other GGPO callbacks 
            // never occur.
            // the solution to this is,,,, not ideal 
            // i might have to add 2 more patches to 0x0048e0a0 and 0x00432c50 to only exec if a boolean is true.
            // ill patch the calls of those funcs, and not the funcs themselves, bc i am already patching the inside of one of them

            _naked_shouldUpdateGameState = 1;
            return;
        }
    }

    _naked_shouldUpdateGameState = 0;

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

    // this call is JUSTIFIED bc it ONLY OCCURS during non rollback frames
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

void _naked_checkIfShouldRunControls() {

    // patched at 0x0040e410

    __asmStart R"(
        cmp dword ptr __naked_shouldUpdateGameState, 0;
        JE _naked_checkIfShouldRunControls_SKIP;
    )" __asmEnd

    emitCall(0x0048e0a0);   
   
    __asmStart R"(
        _naked_checkIfShouldRunControls_SKIP:
    )" __asmEnd

    emitJump(0x0040e415);

}

void _naked_checkIfShouldUpdateGame() {

    // patched at 0x0040e471

    __asmStart R"(
        cmp dword ptr __naked_shouldUpdateGameState, 1;
        JE _naked_checkIfShouldUpdateGame_CONTINUE;
    )" __asmEnd

    // on return from this function, eax seems to normally be set to 1
    // this emulates that
    __asmStart R"(
        mov eax, 1;
    )" __asmEnd

    emitJump(0x0040e476); // this jumps to after the call would have happened

    __asmStart R"(
        _naked_checkIfShouldUpdateGame_CONTINUE:
    )" __asmEnd

    emitCall(0x00432c50);
    emitJump(0x0040e476);

}

static const AsmHacks::AsmList patchGGPO = {

    PATCHJUMP(0x0040e390, _naked_ggpoControllerHook),
    PATCHJUMP(0x0040e513, _naked_ggpoAdvanceFrame),

    //PATCHJUMP(0x0040e410, _naked_checkIfShouldRunControls), 
    //PATCHJUMP(0x0040e471, _naked_checkIfShouldUpdateGame),

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

    updateRollbackAddresses();

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

    // read the stupid config file to get connection info
    ourPlayerNum = -1;
    std::ifstream inFile("2v2Settings.txt");

    if (!inFile) {
        logR("2v2Settings.txt didnt fucking exist");
        return;
    }

    unsigned short tempLocalPort = -1;

    // how the FUCK do spectators fit in here.
    int fileIndex = 0;
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(inFile, line)) {
        std::string tempLine = UIManager::strip(line);
        if(tempLine.size() > 0) {
            lines.push_back(tempLine);
            
            bool hasColon = false;
            for(int i=0; i<tempLine.size(); i++) {
                if(tempLine[i] == ':') {
                    hasColon = true;
                    break;
                }
            }

            if(!hasColon) {
                tempLocalPort = std::stoi(tempLine);
            }

        }
    }

    // is the input param sizeof(inputs) or sizeof(inputs[0])????
    
    result = ggpo_start_session(&ggpo, &cb, "MELTY4V4", GGPOPLAYERNUM, sizeof(inputs[0]), tempLocalPort); // todo, need to actually grab the port correctly!!

    //result = ggpo_start_synctest(&ggpo, &cb, "MELTY4V4", GGPOPLAYERNUM, sizeof(inputs[0]), 1);

    ggpo_set_disconnect_timeout(ggpo, 3000);
    ggpo_set_disconnect_notify_start(ggpo, 1000);


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

        //players[i].connect_progress = 0;
        //players[i].state = Connecting;

        ggpo_set_frame_delay(ggpo, handles[i], 2);
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
    logR("wowee mb_on_event_callback");
    
    int progress;
    switch (info->code) {
        case GGPO_EVENTCODE_CONNECTED_TO_PEER:
            logY("GGPO_EVENTCODE_CONNECTED_TO_PEER");
            //ngs.SetConnectState(info->u.connected.player, Synchronizing);
            break;
        case GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER:
            logY("GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER");
            //progress = 100 * info->u.synchronizing.count / info->u.synchronizing.total;
            //ngs.UpdateConnectProgress(info->u.synchronizing.player, progress);
            break;
        case GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER:
            logY("GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER");
            //ngs.UpdateConnectProgress(info->u.synchronized.player, 100);
            break;
        case GGPO_EVENTCODE_RUNNING:
            logY("GGPO_EVENTCODE_RUNNING");
            //ngs.SetConnectState(Running);
            //renderer->SetStatusText("");
            break;
        case GGPO_EVENTCODE_CONNECTION_INTERRUPTED:
            logY("GGPO_EVENTCODE_CONNECTION_INTERRUPTED");
            //ngs.SetDisconnectTimeout(info->u.connection_interrupted.player,
            //timeGetTime(),
            //info->u.connection_interrupted.disconnect_timeout);
            break;
        case GGPO_EVENTCODE_CONNECTION_RESUMED:
            logY("GGPO_EVENTCODE_CONNECTION_RESUMED");
            //ngs.SetConnectState(info->u.connection_resumed.player, Running);
            break;
        case GGPO_EVENTCODE_DISCONNECTED_FROM_PEER:
            logY("GGPO_EVENTCODE_DISCONNECTED_FROM_PEER");
            //ngs.SetConnectState(info->u.disconnected.player, Disconnected);
            break;
        case GGPO_EVENTCODE_TIMESYNC:
            logY("GGPO_EVENTCODE_TIMESYNC");
            //Sleep(1000 * info->u.timesync.frames_ahead / 60);
            break;
        default:
            logR("unknown event callback wtf");
            break;
    }
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

    *checksum = newSave->hash();

    logB("leaving mb_save_game_state_callback");
    
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
    SaveState* newSave = (SaveState*)buffer;
    delete newSave;
}

void GGPO::updateRollbackAddresses() {

    rollbackAddrs.append ( miscAddrs );

    rollbackAddrs.append ( playerAddrs );                            // Player 1
    rollbackAddrs.append ( playerAddrs, CC_PLR_STRUCT_SIZE );        // Player 2
    rollbackAddrs.append ( playerAddrs, 2 * CC_PLR_STRUCT_SIZE );    // Puppet 1
    rollbackAddrs.append ( playerAddrs, 3 * CC_PLR_STRUCT_SIZE );    // Puppet 2

    for ( size_t i = 0; i < CC_EFFECTS_ARRAY_COUNT; ++i )
        rollbackAddrs.append ( firstEffect, CC_EFFECT_ELEMENT_SIZE * i );

    /*
    
    issues.
    turning around is a 2v2 variable, and is NOT synced properly in here!

    rng might be fucked still
    not might, definitely
    
    */

    rollbackAddrs.append ( extra2v2Addrs );

    rollbackAddrs.update();

}

// -----

int totalSaveStates = 0;

SaveState::SaveState() {
    data = (char*)malloc(GGPO::rollbackAddrs.totalSize);
    totalSaveStates++;
    if(data == NULL) {
        logR("SaveState malloc FAILED!!!!!!! %08X per, %d total", GGPO::rollbackAddrs.totalSize, totalSaveStates);
    }
}

SaveState::~SaveState() {
    if(data != NULL) {
        free(data);
        totalSaveStates--;
        data = NULL;
    }
}

void SaveState::save() {

    // ok, for this shit, please check out caster's generator.cpp

    if(data == NULL) {
        logR("SaveState save buffer was NULL??");
        return;
    }

    fegetenv(&fp_env);

    char* tempData = data;

    for ( const MemDump& mem : GGPO::rollbackAddrs.addrs )
        mem.saveDump ( tempData );

    // more info is DEF needed here! please check caster's DllRollbackManager.cpp


}

void SaveState::load() {

    if(data == NULL) {
        logR("SaveState load buffer was NULL??");
        return;
    }

    fesetenv(&fp_env);

    const char* tempData = data;

    for ( const MemDump& mem : GGPO::rollbackAddrs.addrs )
        mem.loadDump ( tempData );
    
}

static inline uint32_t murmur_32_scramble(uint32_t k) {
    // credits to wikipedia for these https://en.wikipedia.org/wiki/MurmurHash
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    return k;
}

uint32_t murmur3_32(const uint8_t* key, size_t len, uint32_t seed) { // being able to chaing hashes together like this is super nice
	uint32_t h = seed;
    uint32_t k;
    /* Read in groups of 4. */
    for (size_t i = len >> 2; i; i--) {
        // Here is a source of differing results across endiannesses.
        // A swap here has no effects on hash properties though.
        memcpy(&k, key, sizeof(uint32_t));
        key += sizeof(uint32_t);
        h ^= murmur_32_scramble(k);
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64;
    }
    /* Read the rest. */
    k = 0;
    for (size_t i = len & 3; i; i--) {
        k <<= 8;
        k |= key[i - 1];
    }
    // A swap is *not* necessary here because the preceding loop already
    // places the low bytes in the low places according to whatever endianness
    // we use. Swaps only apply when the memory is copied in a chunk.
    h ^= murmur_32_scramble(k);
    /* Finalize. */
	h ^= len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

int SaveState::hash() {
    
    uint32_t hash = 0x00000001;

    //hash = murmur3_32((uint8_t*)&players[0], sizeof(players), hash);

    //hash = murmur3_32((uint8_t*)effects.data(), sizeof(effects[0]) * effects.size(), hash);

    hash = murmur3_32((uint8_t*)data, GGPO::rollbackAddrs.totalSize, hash);

    return hash;
}

// -----

void GGPOInput::read(int p) {

    /*
    DWORD baseAddr = 0x00771398 + (0x2C * p); // im not sure if this is the ideal addr to use for controls! maybe look at 00770F30?

    dir = *(BYTE*)(baseAddr + 0);

    btn |= *(BYTE*)(baseAddr + 1) ? 0x01 : 0x00;
    btn |= *(BYTE*)(baseAddr + 2) ? 0x02 : 0x00;
    btn |= *(BYTE*)(baseAddr + 3) ? 0x04 : 0x00;
    btn |= *(BYTE*)(baseAddr + 4) ? 0x08 : 0x00;
    */

    char *const baseAddr = * ( char ** ) CC_PTR_TO_WRITE_INPUT_ADDR; // how the hell did he figure this out

    switch ( p )
    {
        case 0:
            dir = ( * ( uint16_t * ) ( baseAddr + CC_P1_OFFSET_DIRECTION ) );
            btn = ( * ( uint16_t * ) ( baseAddr + CC_P1_OFFSET_BUTTONS ) );
            break;

        case 1:
            dir = ( * ( uint16_t * ) ( baseAddr + CC_P2_OFFSET_DIRECTION ) );
            btn = ( * ( uint16_t * ) ( baseAddr + CC_P2_OFFSET_BUTTONS ) );
            break;

        case 2:
            dir = ( * ( uint16_t * ) ( baseAddr + CC_P3_OFFSET_DIRECTION ) );
            btn = ( * ( uint16_t * ) ( baseAddr + CC_P3_OFFSET_BUTTONS ) );
            break;

        case 3:
            dir = ( * ( uint16_t * ) ( baseAddr + CC_P4_OFFSET_DIRECTION ) );
            btn = ( * ( uint16_t * ) ( baseAddr + CC_P4_OFFSET_BUTTONS ) );
            break;

        default:
            ASSERT_IMPOSSIBLE;
            break;
    }

}

void GGPOInput::write(int p) {

    /*
    DWORD baseAddr = 0x00771398 + (0x2C * p); // im not sure if this is the ideal addr to use for controls! maybe look at 00770F30?

    *(BYTE*)(baseAddr + 0) = dir;

    *(BYTE*)(baseAddr + 1) = !!(btn & 0x01);
    *(BYTE*)(baseAddr + 2) = !!(btn & 0x02);
    *(BYTE*)(baseAddr + 3) = !!(btn & 0x04);
    *(BYTE*)(baseAddr + 4) = !!(btn & 0x08);
    */

    char *const baseAddr = * ( char ** ) CC_PTR_TO_WRITE_INPUT_ADDR; // how the hell did he figure this out

    switch ( p )
    {
        case 0:
            ( * ( uint16_t * ) ( baseAddr + CC_P1_OFFSET_DIRECTION ) ) = dir;
            ( * ( uint16_t * ) ( baseAddr + CC_P1_OFFSET_BUTTONS ) ) = btn;
            break;

        case 1:
            ( * ( uint16_t * ) ( baseAddr + CC_P2_OFFSET_DIRECTION ) ) = dir;
            ( * ( uint16_t * ) ( baseAddr + CC_P2_OFFSET_BUTTONS ) ) = btn;
            break;

        case 2:
            ( * ( uint16_t * ) ( baseAddr + CC_P3_OFFSET_DIRECTION ) ) = dir;
            ( * ( uint16_t * ) ( baseAddr + CC_P3_OFFSET_BUTTONS ) ) = btn;
            break;

        case 3:
            ( * ( uint16_t * ) ( baseAddr + CC_P4_OFFSET_DIRECTION ) ) = dir;
            ( * ( uint16_t * ) ( baseAddr + CC_P4_OFFSET_BUTTONS ) ) = btn;
            break;

        default:
            ASSERT_IMPOSSIBLE;
            break;
    }

}

void GGPOInput::log(int p) {
    if(p == -1) {
        logG("%d %04X", dir, btn);
    } else {
        logG("P%d %d %04X", p, dir, btn);
    }
}


