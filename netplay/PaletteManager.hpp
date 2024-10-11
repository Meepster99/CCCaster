#pragma once

#ifndef DISABLE_SERIALIZATION
#include "Protocol.hpp"
#include <cereal/types/map.hpp>
#endif

#include <cstdint>
#include <map>
#include <array>
#include <string>
#include <optional>


/*

#define COLOR_RGB(R, G, B) \
    ( 0xFFFFFF & ( ( ( 0xFF & ( R ) ) << 16 ) | ( ( 0xFF & ( G ) ) << 8 ) | ( 0xFF & ( B ) ) ) )

#define SWAP_R_AND_B(COLOR) \
    ( ( ( COLOR & 0xFF ) << 16 ) | ( COLOR & 0xFF00 ) | ( ( COLOR & 0xFF0000 ) >> 16 ) | ( COLOR & 0xFF000000 ) )


#ifndef DISABLE_SERIALIZATION
class PaletteManager : public SerializableSequence
#else
class PaletteManager
#endif
{
public:

    static uint32_t computeHighlightColor ( uint32_t color );

    void cache ( const uint32_t **allPaletteData );
    void apply ( uint32_t **allPaletteData ) const;

    void cache ( const uint32_t *allPaletteData );
    void apply ( uint32_t *allPaletteData ) const;
    void apply ( uint32_t paletteNumber, uint32_t *singlePaletteData ) const;

    uint32_t getOriginal ( uint32_t paletteNumber, uint32_t colorNumber ) const;
    uint32_t get ( uint32_t paletteNumber, uint32_t colorNumber ) const;
    void set ( uint32_t paletteNumber, uint32_t colorNumber, uint32_t color );

    void clear ( uint32_t paletteNumber, uint32_t colorNumber );
    void clear ( uint32_t paletteNumber );
    void clear();

    bool empty() const;

    bool save ( const std::string& folder, const std::string& charaName );
    bool load ( const std::string& folder, const std::string& charaName );

#ifndef DISABLE_SERIALIZATION
    PROTOCOL_MESSAGE_BOILERPLATE ( PaletteManager, _palettes )
#endif

private:

    std::map<uint32_t, std::map<uint32_t, uint32_t>> _palettes;

    std::array<std::array<uint32_t, 256>, 36> _originals;

    void optimize();
};

*/

// this shouldnt be here. remove it
#include <ws2tcpip.h>
#include <winsock2.h>

void __attribute__((stdcall)) ___netlog2(const char* msg);

void __attribute__((stdcall)) netlog2(const char* format, ...);

class NetplayManager;

extern NetplayManager* netManPtr;

typedef std::map< // key: filename (WARC.pal, example), val: array of 6 palettes for that specific char
    const char*,
    std::array<
        std::array<uint32_t, 256>,
        6
    >
> PaletteData;

#ifndef DISABLE_SERIALIZATION
class PaletteManager : public SerializableSequence
#else
class PaletteManager
#endif
{
public:

    bool loadOurDataInit = false;
    void loadOurData();

   
    MsgPtr clone() const;
    MsgType getMsgType() const;

    /*
    MsgPtr clone() const {
        return MsgPtr();
    }

    MsgType getMsgType() const {
        return MsgType();
    }
    */
    

private:

    uint_fast8_t getCharPaletteIndex(uint_fast8_t base);

    PaletteData ourPaletteData;
    PaletteData otherPaletteData;
        
    

};