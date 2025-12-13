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
    { (void*)&AsmHacks::FN1States[0], sizeof(AsmHacks::FN1States) },

    CC_GAME_MODE_ADDR, // not sure
    CC_GAME_STATE_ADDR, // even less sure
    ( ( uint32_t * ) 0x00562a6f ), // something relating to scenes??

	// this is stupid. 
	// but like why not just grab. all the ram.
	//{ 0x0554000, 0x07b1d63 }

};

// -----

/*

GGPO is a mysterious black box, where i have NO clue whats happening most of the time
things that could be issues are: 

    vw_on_event_callback not being emulated properly

    config error? not properly,,, idek 

    my calls being inaccurate
        best way to fix calls is to essentially copy vectorwar's state EXACTLY 
        difficult, requires some fucking around with gameloop0, but is preferable to whatever i have going on rn

    ok, great, i redid everything, still shitting itself

*/

extern "C" {
    DWORD _naked_shouldAdvanceFrame = 0;
}

bool GGPO::shouldAdvanceFrame() {
    return _naked_shouldAdvanceFrame == 1;
}

void RNGState::read() {
    rng0 = *CC_RNG_STATE0_ADDR;
    rng1 = *CC_RNG_STATE1_ADDR;
    rng2 = *CC_RNG_STATE2_ADDR;

    memcpy(rng3, CC_RNG_STATE3_ADDR, sizeof(rng3));
}

void RNGState::write() {
    *CC_RNG_STATE0_ADDR = rng0;
    *CC_RNG_STATE1_ADDR = rng1;
    *CC_RNG_STATE2_ADDR = rng2;

    memcpy(CC_RNG_STATE3_ADDR, rng3, sizeof(rng3));
}

void advanceFrame() { // emulates VectorWar_AdvanceFrame
    
    GGPO::writeAllGGPOInputs(); 

    // i WOULD recreate and reverse gameloop0, but i hook those funcs. in the rest of 2v2. i cant have that 
    // god there are SO many issues here
    PUSH_ALL;
    emitCall(0x0048e0a0); // this function IS needed in here, no clue what it do?! but it do be needed
    POP_ALL; 

    PUSH_ALL;
    emitCall(0x00432c50);
    POP_ALL;

    ggpo_advance_frame(GGPO::ggpo);
}

void drawFrame() {

    /*
    static AsmHacks::AsmList patchOutFrameAdvance = {
        //{(void*)(0x0040e410), {0xBB, 0x01, 0x00, 0x00, 0x00}}, // mov ebx, 1; // not sure if this is needed
        //{(void*)(0x0040e471), {0xB8, 0x01, 0x00, 0x00, 0x00}}, // mov eax, 1
    };

    for ( const AsmHacks::Asm& hack : patchOutFrameAdvance )
        WRITE_ASM_HACK ( hack );
    */
    

    PUSH_ALL;
    emitCall(0x0040e390); // calls gameloop0
    POP_ALL;

    
    /*
    for ( const AsmHacks::Asm& hack : patchOutFrameAdvance )
        REVERT_ASM_HACK ( hack );
    */
    
}

void runFrame() {

    // this func is designed to (as closely as i can) emulate VectorWar_RunFrame

    // call this at the TOP of gameloop0

    ggpo_idle(GGPO::ggpo, 5); // vectorwar idle calls idle

    GGPOErrorCode result = GGPO_OK;
    int i = GGPO::ourPlayerNum;
    int disconnectFlags;

    GGPO::inputs[i].read(i);

    result = ggpo_add_local_input(GGPO::ggpo, GGPO::handles[i], &GGPO::inputs[i], sizeof(GGPO::inputs[i]));

    _naked_shouldAdvanceFrame = 0;

    if (GGPO_SUCCEEDED(result)) {
        result = ggpo_synchronize_input(GGPO::ggpo, GGPO::inputs, sizeof(GGPO::inputs), &disconnectFlags);
        if (GGPO_SUCCEEDED(result)) {
            // ok, this shit is ASS
            // i think that,,, doing a boolean to enable/disable this bs would be better 
            //advanceFrame();
            _naked_shouldAdvanceFrame = 1;
        }
    }

    GGPO::writeAllGGPOInputs(); 

    drawFrame();

    if(_naked_shouldAdvanceFrame == 1) {
        ggpo_advance_frame(GGPO::ggpo);
    }

    _naked_shouldAdvanceFrame = 0;

}

void _naked_runFrame() {

    // patched at 0x0040d350

    //PUSH_ALL;
    //advanceFrame();
    //POP_ALL;
    
    PUSH_ALL;
    runFrame();
    //drawFrame();
    POP_ALL;

    __asmStart R"(
        mov eax, 1; // this var controls when the game exits, need to pass it from the actual func!!! except its,,, grabbed from the controller funcs. omfg
    )" __asmEnd

    emitJump(0x0040d355);

}

void _naked_advanceFramePreCheck() {

    __asmStart R"(
        cmp dword ptr __naked_shouldAdvanceFrame, 0;
        JE _naked_advanceFramePreCheck_skipAdvanceFrame;
    )" __asmEnd

    emitCall(0x00432c50);

    __asmStart R"(
    _naked_advanceFramePreCheck_skipAdvanceFrame:
        mov eax, 1;
    )" __asmEnd

    emitJump(0x0040e476); 

}

static const AsmHacks::AsmList patchGGPO = {
    PATCHJUMP(0x0040d350, _naked_runFrame),

    PATCHJUMP(0x0040e471, _naked_advanceFramePreCheck),

    //{(void*)(0x0040e410), {INLINE_NOP_FIVE_TIMES}}, // this func literally makes no difference when commented out
    //{(void*)(0x0040e471), {INLINE_NOP_FIVE_TIMES}},

    //{(void*)(0x0040e410), {0xBB, 0x01, 0x00, 0x00, 0x00}}, // mov ebx, 1; // not sure if this is needed
    //{(void*)(0x0040e471), {0xB8, 0x01, 0x00, 0x00, 0x00}}, // mov eax, 1
};

// -----

void GGPO::writeAllGGPOInputs() {

    #if GGPOPLAYERNUM == 4

    for(int i=0; i<GGPOPLAYERNUM; i++) {
        inputs[i].write(i);
    }

    #else

    inputs[0].write(0);
    inputs[0].write(2);

    inputs[1].write(1);
    inputs[1].write(3);

    #endif

}

void GGPO::initGGPO() {

    if(isGGPOOnline) {
        return;
    }
    isGGPOOnline = true;

    log("temp commented out ggpo lol");
    return;

    updateRollbackAddresses();


    logG("Attempting to init GGPO. PLAYER %d");
    logG("patching ASM");
    
    for ( const AsmHacks::Asm& hack : patchGGPO ) {
        WRITE_ASM_HACK ( hack );
    }

    //return;

    logG("doing actual ggpo bs");

    GGPOErrorCode result;

    cb.begin_game =      mb_begin_game_callback;
    cb.advance_frame =   mb_advance_frame_callback;
    cb.load_game_state = mb_load_game_state_callback;
    cb.save_game_state = mb_save_game_state_callback;
    cb.free_buffer =     mb_free_buffer;
    cb.on_event =        mb_on_event_callback;

	cb.compare_buffers = mb_compare_buffers;


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
    

    #ifdef DOSYNCTEST
    result = ggpo_start_synctest(&ggpo, &cb, "MELTY4V4", GGPOPLAYERNUM, sizeof(inputs[0]), 1);
    #else
    result = ggpo_start_session(&ggpo, &cb, "MELTY4V4", GGPOPLAYERNUM, sizeof(inputs[0]), tempLocalPort); // todo, need to actually grab the port correctly!!
    #endif 

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

        // SETTING NONZERO DELAY WAS CAUSING THE CRASHES??? WHYYY
        if(players[i].type == GGPO_PLAYERTYPE_LOCAL) {
            ggpo_set_frame_delay(ggpo, handles[i], 7);
        }
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

    RNGState rngState = {
        0x30f59eee, 0x1d6cf4a4, 0x32e6d8bb, {0}
    };
    
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
            rngState.write();
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
            Sleep(1000 * info->u.timesync.frames_ahead / 60);
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
bool GGPO::mb_advance_frame_callback(int) { // emulates vw_advance_frame_callback
    //logB("wowee mb_advance_frame_callback");

    /*
    * advance_frame - Called during a rollback.  You should advance your game
    * state by exactly one frame.  Before each frame, call ggpo_synchronize_inpu
    * to retrieve the inputs you should use for that frame.  After each frame,
    * you should call ggpo_advance_fram to notify GGPO.net that you're
    * finished.
    *
    * The flags parameter is reserved.  It can safely be ignored at this time.
    */

    int disconnectFlags;
    ggpo_synchronize_input(GGPO::ggpo, GGPO::inputs, sizeof(GGPO::inputs), &disconnectFlags);
    advanceFrame();

    //logB("leaving mb_advance_frame_callback");

    return true;
}

/*
* vw_load_game_state_callback --
*
* Makes our current state match the state passed in by GGPO.
*/
bool GGPO::mb_load_game_state_callback(unsigned char *buffer, int len) {
    //logB("wowee mb_load_game_state_callback");

    // tbh, when in between states, or in css, just disable this

    /*
    0x14 (in cs)s -> 0x08 (after stagesel) -> 0x01 (ingame) -> 0x05 (both rounds are over) -> 0x08 (another game, i hit once again) -> 0x01 (ingame)
    */

    
    DWORD gameMode = *CC_GAME_MODE_ADDR;
    DWORD gameState = *CC_GAME_STATE_ADDR;

    
  
	/*
	// this disables rolling back when in a ... state i dont want to roll back in but
    if(gameMode == CC_GAME_MODE_LOADING || gameMode == CC_GAME_MODE_RETRY || gameMode == CC_GAME_MODE_CHARA_SELECT) {
        return true;
    }

    if(gameState != 0 && gameState != 0xFF) { // what even are these constants
        return true;
    }*/

	// this neuters everything other than in game. figure out a way to fucking be normal in other ways
	// i wish i could just get screen transition rollbacks to work. but i cant. there has to be a way via ggpo to do things synchronously. and then i hijack that for, everything
	
	/*
	if(gameMode != 1 || gameState != 255) { 
		logG("avoiding loading: mode: %d state: %d", (BYTE)gameMode, (BYTE)gameState);
		return true;
	}
	*/

	((SaveState*)buffer)->load();

	// i should really grab by debug structs from training mode
	/*
	// doing this prevented the crash, but made nothing (even the bg load) and everything register as sion
	DWORD commandDataSave[4];

	for(int i=0; i<4; i++) {
		commandDataSave[i] = 0x555130 + (0xAFC * i) + 0x33C;
	}
    
    //((SaveState*)buffer)->load();

    //logB("leaving mb_load_game_state_callback");

	for(int i=0; i<4; i++) {
		*(DWORD*)(0x555130 + (0xAFC * i) + 0x33C) = commandDataSave[i];
	}*/

    return true;
}

/*
* vw_save_game_state_callback --
*
* Save the current state to a buffer and return it to GGPO via the
* buffer and len parameters.
*/
bool GGPO::mb_save_game_state_callback(unsigned char **buffer, int *len, int *checksum, int) {
    //logB("wowee mb_save_game_state_callback");

    *len = sizeof(SaveState);
    SaveState* newSave = new SaveState();
    newSave->save();
    *buffer = (unsigned char *)newSave;

    *checksum = newSave->hash();

    //logB("leaving mb_save_game_state_callback");
    
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

void GGPO::mb_compare_buffers(void* last, void* cur) {

	// this func doesnt work. and idk why
	// i do not know why
	// the code in synctest.cpp has the issue
	// i dont think im passing in the right buffer

	return;

	log("comparebuffers actually called");

	SaveState* prev = (SaveState*)last;
	SaveState* now = (SaveState*)cur;

	char* prevData = prev->data;
	char* nowData = now->data;

	if(prevData == NULL || nowData == NULL) {
		log("one/both of the buffers were null, just leave");
		return;
	}

	for ( const MemDump& mem : GGPO::rollbackAddrs.addrs ) {
		log("Start loop");
		
		DWORD baseAddr = (DWORD)mem.getAddr();
		DWORD size = mem.size;
		DWORD addr = baseAddr;

		log("addr: %08X size: %d", addr, size);

		bool firstAddrRangePrint = true;

		log("addresses are: %08X %08X", (DWORD)prevData, (DWORD)nowData);
		
		log("trying to read from prev %08X", (DWORD)prevData);
		log("%02X", *prevData);

		log("trying to read from now %08X", (DWORD)nowData);
		log("%02X", *nowData);

		log("done");

		for(int i=0; i<size; i++) {

			
			if(*prevData != *nowData) {
				if(firstAddrRangePrint) {
					firstAddrRangePrint = false;
					log("Attempting print");
					log("memory: %08X -> %08X size: %08X", baseAddr, baseAddr+size, size);
				}

				log("OMFG");
				log("attempting to read from %08X %08X", (DWORD)prevData, (DWORD)nowData);
				log("\tAddr: %08X %02X != %02X", addr, (BYTE)*prevData, (BYTE)*nowData);

			}

			addr++;
			prevData++;
			nowData++;

		}

		log("End loop");
	}


	log("exiting gracefully!");

	Sleep(1000);



	//exit(1);
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

	// how are these pointers getting alloced. how does this work at all?
	// im allocing them. and i forgor. nice job maddy

    for ( const MemDump& mem : GGPO::rollbackAddrs.addrs )
        mem.saveDump ( tempData ); // the func incs this pointer so it is actually working, despite the fact i dont believe it and dont trust it and hate it

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
        mem.loadDump ( tempData ); // the func incs this pointer so it is actually working, despite the fact i dont believe it and dont trust it and hate it
    

	// upon further looking at this code, i honestly have no clue if its actually doing anything or not.
	// who allocates the data ptr???
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


