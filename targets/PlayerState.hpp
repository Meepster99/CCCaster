#pragma once

#include <windows.h>

struct PlayerState {
    static constexpr DWORD BASE_ADDR = 0x00555130;
    static constexpr DWORD DATA_SIZE = 0xAFC;
    static constexpr DWORD CSS_BASE_ADDR = 0x0074d840;
    static constexpr DWORD CSS_DATA_SIZE = 0x2C;
    
    // Memory offsets
    static constexpr DWORD OFFSET_MOON = 0x0C;
    static constexpr DWORD OFFSET_PALETTE = 0x0A;
    static constexpr DWORD OFFSET_HEALTH = 0xBC;
    static constexpr DWORD OFFSET_RED_HEALTH = 0xC0;
    static constexpr DWORD OFFSET_METER = 0xE0;
    static constexpr DWORD OFFSET_HEAT_TIME = 0xE4;
    static constexpr DWORD OFFSET_CIRCUIT_STATE = 0xE8;
    static constexpr DWORD OFFSET_CIRCUIT_BREAK = 0x100;
    static constexpr DWORD OFFSET_EX_PENALTY = 0x104;
    static constexpr DWORD OFFSET_BACKGROUND_FLAG = 0x174;
    static constexpr DWORD OFFSET_HITSTUN = 0x1AC;
    static constexpr DWORD OFFSET_KNOCKDOWN_FLAG = 0x1B0;

    // State data
    DWORD health;      // 11400 max
    DWORD redHealth;
    DWORD moon;
    DWORD meter;
    DWORD heatTime;
    WORD circuitState; // 0 is normal, 1 is heat, 2 is max, 3 is blood heat
    BYTE palette;
    DWORD hitstun;
    WORD circuitBreakTimer;
    BYTE knockedDown;

    void update(int playerIndex);
};

// Declare these as extern
extern PlayerState players[4];
extern void updateAllPlayers();
