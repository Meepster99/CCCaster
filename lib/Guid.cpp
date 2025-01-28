#include "Guid.hpp"
#include "Logger.hpp"

#include <algorithm>

#include <rpc.h>

using namespace std;


Guid::Guid ( initializer_list<uint8_t> guid )
{
    ASSERT ( guid.size() == sizeof ( this->guid ) );

    copy ( guid.begin(), guid.end(), &this->guid[0] );
}

Guid::Guid ( const GUID& guid )
{
    static_assert ( sizeof ( guid ) == sizeof ( this->guid ), "Must be the same size as Windows GUID" );

    memcpy ( &this->guid[0], &guid, sizeof ( this->guid ) );
}

void Guid::getGUID ( GUID& guid ) const
{
    static_assert ( sizeof ( guid ) == sizeof ( this->guid ), "Must be the same size as Windows GUID" );

    memcpy ( &guid, &this->guid[0], sizeof ( this->guid ) );
}

std::string Guid::getString() const {

    char buffer[(16 * 2) + 4 + 1];

    char* curr = buffer;
    
    /*
    for(int i=0; i<16; i++) {
        snprintf(curr, sizeof(buffer) - (curr - buffer), "%02X", guid[i]);
        curr += 2;
        if(i == 3 || i == 5 || i == 7 || i == 9) {
            snprintf(curr, sizeof(buffer) - (curr - buffer), "-");
            curr++;
        }
    }*/

    curr += snprintf(curr, sizeof(buffer) - (curr - buffer), "%08X", data1);
    curr += snprintf(curr, sizeof(buffer) - (curr - buffer), "-");
    curr += snprintf(curr, sizeof(buffer) - (curr - buffer), "%04X", data2);
    curr += snprintf(curr, sizeof(buffer) - (curr - buffer), "-");
    curr += snprintf(curr, sizeof(buffer) - (curr - buffer), "%04X", data3);
    curr += snprintf(curr, sizeof(buffer) - (curr - buffer), "-");

    for(int i=0; i<8; i++) {
        curr += snprintf(curr, sizeof(buffer) - (curr - buffer), "%02X", data4[i]);
    }

    buffer[sizeof(buffer) - 1] = '\0';

    std::string res(buffer);

    memset(buffer, 0, sizeof(buffer));

    snprintf(buffer, sizeof(buffer), " VID:%04X DID:%04X", vendorID, deviceID);

    res += std::string(buffer);

    return res;
}

std::string Guid::getIdentifiers() const {

    char buffer[32];

    snprintf(buffer, sizeof(buffer), " %04X|%04X", vendorID, deviceID);

    std::string res = std::string(buffer);

    return res;
}