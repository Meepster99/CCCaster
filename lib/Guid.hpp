#pragma once

#include "Algorithms.hpp"
#include "StringUtils.hpp"

#include <cstring>
#include <cstdint>
#include <string>

// Windows GUID type forward declaration
typedef struct _GUID GUID;


// Guid type
struct Guid
{
    union {
        struct __attribute__((packed)) { // https://learn.microsoft.com/en-us/windows/win32/api/guiddef/ns-guiddef-guid
            union {
                // https://devicehunt.com/search/type/usb/vendor/any/device/any was helpful
                struct __attribute__((packed)) {
                    uint16_t vendorID;
                    uint16_t deviceID;
                };
                uint32_t data1;
            };
            uint16_t data2;
            uint16_t data3;
            uint8_t data4[8];
        };
        uint8_t guid[16];
    };

    Guid() {}

    Guid ( std::initializer_list<uint8_t> guid );

    Guid ( const GUID& guid );

    void getGUID ( GUID& guid ) const;

    std::string getString() const;
    std::string getIdentifiers() const;
    
};

static_assert(sizeof(Guid) == 16, "Guid must be size 16");


// Hash function
namespace std
{

template<> struct hash<Guid>
{
    size_t operator() ( const Guid& a ) const
    {
        size_t seed = 0;

        for ( size_t i = 0; i < sizeof ( a.guid ); ++i )
            hash_combine ( seed, a.guid[i] );

        return seed;
    }
};

} // namespace std


// Comparison operators
inline bool operator< ( const Guid& a, const Guid& b ) { return ( std::memcmp ( &a, &b, sizeof ( a ) ) < 0 ); }
inline bool operator== ( const Guid& a, const Guid& b ) { return ( !std::memcmp ( &a, &b, sizeof ( a ) ) ); }
inline bool operator!= ( const Guid& a, const Guid& b ) { return ! ( a == b ); }


// Stream operators
inline std::ostream& operator<< ( std::ostream& os, const Guid& a )
{
    return ( os << formatAsHex ( a.guid, sizeof ( a.guid ) ) );
}
