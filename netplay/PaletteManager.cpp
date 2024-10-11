#include "PaletteManager.hpp"
#include "StringUtils.hpp"
#include "Algorithms.hpp"

#include <fstream>
#include <sstream>


/*
using namespace std;


#define PALETTES_FILE_SUFFIX "_palettes.txt"


uint32_t PaletteManager::computeHighlightColor ( uint32_t color )
{
    const uint32_t r = ( color & 0xFF );
    const uint32_t g = ( color & 0xFF00 ) >> 8;
    const uint32_t b = ( color & 0xFF0000 ) >> 16;

    const uint32_t absDivColor2 = 3 * 230 * 230;

    if ( r * r + g * g + b * b > absDivColor2 )
    {
        // If too light, use inverse color
        const uint32_t R = clamped<uint32_t> ( 255 - r, 0, 255 );
        const uint32_t G = clamped<uint32_t> ( 255 - g, 0, 255 );
        const uint32_t B = clamped<uint32_t> ( 255 - b, 0, 255 );

        return ( color & 0xFF000000 ) | COLOR_RGB ( R, G, B );
    }
    else
    {
        return ( color & 0xFF000000 ) | 0xFFFFFF;
    }
}

void PaletteManager::cache ( const uint32_t **allPaletteData )
{
    for ( uint32_t i = 0; i < _originals.size(); ++i )
    {
        for ( uint32_t j = 0; j < _originals[i].size(); ++j )
        {
            _originals[i][j] = SWAP_R_AND_B ( allPaletteData[i][j] );
        }
    }
}

void PaletteManager::apply ( uint32_t **allPaletteData ) const
{
    for ( uint32_t i = 0; i < _originals.size(); ++i )
    {
        for ( uint32_t j = 0; j < _originals[i].size(); ++j )
        {
            allPaletteData[i][j] = ( allPaletteData[i][j] & 0xFF000000 )
                                   | ( 0xFFFFFF & SWAP_R_AND_B ( get ( i, j ) ) );
        }
    }
}

void PaletteManager::cache ( const uint32_t *allPaletteData )
{
    for ( uint32_t i = 0; i < _originals.size(); ++i )
    {
        for ( uint32_t j = 0; j < _originals[i].size(); ++j )
        {
            _originals[i][j] = SWAP_R_AND_B ( allPaletteData [ i * 256 + j ] );
        }
    }
}

void PaletteManager::apply ( uint32_t *allPaletteData ) const
{
    for ( uint32_t i = 0; i < 36; ++i )
    {
        for ( uint32_t j = 0; j < 256; ++j )
        {
            allPaletteData [ i * 256 + j ] = ( allPaletteData [ i * 256 + j ] & 0xFF000000 )
                                             | ( 0xFFFFFF & SWAP_R_AND_B ( get ( i, j ) ) );
        }
    }
}

void PaletteManager::apply ( uint32_t paletteNumber, uint32_t *singlePaletteData ) const
{
    for ( uint32_t j = 0; j < 256; ++j )
    {
        singlePaletteData[j] = ( singlePaletteData [j] & 0xFF000000 )
                               | ( 0xFFFFFF & SWAP_R_AND_B ( get ( paletteNumber, j ) ) );
    }
}

uint32_t PaletteManager::getOriginal ( uint32_t paletteNumber, uint32_t colorNumber ) const
{
    return 0xFFFFFF & _originals[paletteNumber][colorNumber];
}

uint32_t PaletteManager::get ( uint32_t paletteNumber, uint32_t colorNumber ) const
{
    const auto it = _palettes.find ( paletteNumber );

    if ( it != _palettes.end() )
    {
        const auto jt = it->second.find ( colorNumber );

        if ( jt != it->second.end() )
            return 0xFFFFFF & jt->second;
    }

    return getOriginal ( paletteNumber, colorNumber );
}

void PaletteManager::set ( uint32_t paletteNumber, uint32_t colorNumber, uint32_t color )
{
    _palettes[paletteNumber][colorNumber] = 0xFFFFFF & color;

#ifndef DISABLE_SERIALIZATION
    invalidate();
#endif
}

void PaletteManager::clear ( uint32_t paletteNumber, uint32_t colorNumber )
{
    const auto it = _palettes.find ( paletteNumber );

    if ( it != _palettes.end() )
    {
        it->second.erase ( colorNumber );

        if ( it->second.empty() )
            clear ( paletteNumber );
    }

#ifndef DISABLE_SERIALIZATION
    invalidate();
#endif
}

void PaletteManager::clear ( uint32_t paletteNumber )
{
    _palettes.erase ( paletteNumber );

#ifndef DISABLE_SERIALIZATION
    invalidate();
#endif
}

void PaletteManager::clear()
{
    _palettes.clear();

#ifndef DISABLE_SERIALIZATION
    invalidate();
#endif
}

bool PaletteManager::empty() const
{
    return _palettes.empty();
}

void PaletteManager::optimize()
{
    for ( uint32_t i = 0; i < 36; ++i )
    {
        const auto it = _palettes.find ( i );

        if ( it == _palettes.end() )
            continue;

        for ( uint32_t j = 0; j < 256; ++j )
        {
            const auto jt = it->second.find ( j );

            if ( jt == it->second.end() )
                continue;

            if ( ( 0xFFFFFF & jt->second ) != ( 0xFFFFFF & _originals[i][j] ) )
                continue;

            it->second.erase ( j );

            if ( it->second.empty() )
            {
                _palettes.erase ( i );
                break;
            }
        }
    }

#ifndef DISABLE_SERIALIZATION
    invalidate();
#endif
}

bool PaletteManager::save ( const string& folder, const string& charaName )
{
    optimize();

    ofstream fout ( ( folder + charaName + PALETTES_FILE_SUFFIX ).c_str() );
    bool good = fout.good();

    if ( good )
    {
        fout << "######## " << charaName << " palettes ########" << endl;
        fout << endl;
        fout << "#"                                                  << endl;
        fout << "# Color are specified as color_NUMBER_ID=#HEX_CODE" << endl;
        fout << "#"                                                  << endl;
        fout << "#   eg. color_03_123=#FF0000"                       << endl;
        fout << "#"                                                  << endl;
        fout << "# Lines starting with # are ignored"                << endl;
        fout << "#"                                                  << endl;

        for ( auto it = _palettes.cbegin(); it != _palettes.cend(); ++it )
        {
            fout << format ( "\n### Color %02d start ###\n", it->first + 1 ) << endl;

            for ( const auto& kv : it->second )
            {
                const string line = format ( "color_%02d_%03d=#%06X", it->first + 1, kv.first + 1, kv.second );

                fout << line << endl;
            }

            fout << format ( "\n#### Color %02d end ####\n", it->first + 1 ) << endl;
        }

        good = fout.good();
    }

    fout.close();
    return good;
}

bool PaletteManager::load ( const string& folder, const string& charaName )
{
    ifstream fin ( ( folder + charaName + PALETTES_FILE_SUFFIX ).c_str() );
    bool good = fin.good();

    if ( good )
    {
        string line;

        while ( getline ( fin, line ) )
        {
            if ( line.empty() || line[0] == '#' )
                continue;

            vector<string> parts = split ( line, "=" );

            if ( parts.size() != 2 || parts[0].empty() || parts[1].empty() || parts[1][0] != '#' )
                continue;

            vector<string> prefix = split ( parts[0], "_" );

            if ( prefix.size() != 3 || prefix[0] != "color" || prefix[1].empty() || prefix[2].empty() )
                continue;

            const uint32_t paletteNumber = lexical_cast<uint32_t> ( prefix[1] ) - 1;
            const uint32_t colorNumber = lexical_cast<uint32_t> ( prefix[2] ) - 1;

            uint32_t color;

            stringstream ss ( parts[1].substr ( 1 ) );
            ss >> hex >> color;

            _palettes[paletteNumber][colorNumber] = color;
        }

        optimize();

#ifndef DISABLE_SERIALIZATION
        invalidate();
#endif
    }

    fin.close();
    return good;
}

*/

void __attribute__((stdcall)) ___netlog2(const char* msg)
{
	const char* ipAddress = "127.0.0.1";
	unsigned short port = 17474;

	int msgLen = strlen(msg);

	const char* message = msg;

	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) 
	{
		return;
	}

	SOCKET sendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sendSocket == INVALID_SOCKET) 
	{
		WSACleanup();
		return;
	}

	sockaddr_in destAddr;
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = htons(port);
	if (inet_pton(AF_INET, ipAddress, &destAddr.sin_addr) <= 0) 
	{
		closesocket(sendSocket);
		WSACleanup();
		return;
	}

	int sendResult = sendto(sendSocket, message, strlen(message), 0, (sockaddr*)&destAddr, sizeof(destAddr));
	if (sendResult == SOCKET_ERROR) 
	{
		closesocket(sendSocket);
		WSACleanup();
		return;
	}

	closesocket(sendSocket);
	WSACleanup();
}

void __attribute__((stdcall)) netlog2(const char* format, ...) {
	static char buffer[1024]; // no more random char buffers everywhere.
	va_list args;
	va_start(args, format);
	vsnprintf_s(buffer, 1024, format, args);
	___netlog2(buffer);
	va_end(args);
}

bool folderExists(const std::string& folderName) {
    DWORD fileAttributes = GetFileAttributes(folderName.c_str());
    
    if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
        return false;
    }

    return (fileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

bool fileExists(const std::string& fileName) {
    DWORD fileAttributes = GetFileAttributes(fileName.c_str());
    
    if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
        return false;
    }

    return !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

// map of filenames, and their default hashes. only palettes 36-42 are involved in this indexing
const std::map<const char*, const uint32_t> palettePaths = {
    {"./data/shiki.pal", 0x3EA98B86},
    {"./data/p_arc.pal", 0xCEF1142E},
    {"./data/nanaya.pal", 0x0BD2B336},
    {"./data/kishima.pal", 0x9A98F8DC},
    {"./data/aoko.pal", 0x0523FCA2},
    {"./data/ciel.pal", 0xE4FA50A2},
    {"./data/miyako.pal", 0x8EBFC9E6},
    {"./data/sion.pal", 0xA160A344},
    {"./data/ries.pal", 0x69DFC978},
    {"./data/v_sion.pal", 0xC0A59994},
    {"./data/warakia.pal", 0x28E27DE4},
    {"./data/roa.pal", 0x29042346},
    {"./data/m_hisui.pal", 0x9DFF4CA2},
    {"./data/akaakiha.pal", 0x5A50BE16},
    {"./data/warc.pal", 0x16D6BBF4},
    {"./data/p_ciel.pal", 0x2555716E},
    {"./data/arc.pal", 0x5A30A750},
    {"./data/akiha.pal", 0x03338A8A},
    {"./data/hisui.pal", 0x2487E49C},
    {"./data/kohaku.pal", 0x46780A76},
    {"./data/s_akiha.pal", 0xFA75FBFA},
    {"./data/satsuki.pal", 0x2D967A16},
    {"./data/len.pal", 0x11D0AB4E},
    {"./data/ryougi.pal", 0x859FE238},
    {"./data/wlen.pal", 0xC4B536F4},
    {"./data/nero.pal", 0x847AC728},
    {"./data/nechaos.pal", 0x1225BCA4},
    {"./data/m_hisui_m.pal", 0x9DFF4CA2},
    {"./data/neco_p.pal", 0x7B395610},
    {"./data/neco.pal", 0x7BECFFAE},
    {"./data/kohaku_m.pal", 0x0E305EA0},
    {"./data/m_hisui_p.pal", 0x9DFF4CA2}
};

void PaletteManager::loadOurData() {
    // i am currently assuming that everyone will be using palmod for this.
    
    netlog2("loadOurDataInit isnow true");
    loadOurDataInit = true;
    return;

    bool dataFolderExists = folderExists("./data/");

    if(!dataFolderExists) {
        netlog2("data folder didnt exist, no custom palettes");
        return;
    }
   
    // 256 * 64 is 256 colors for 64 paletes, 4 bytes per col. the +4 is for the leading 0x40000000
    // why does palmod have us edit in 5 bit color when its stored in 8?
    constexpr const size_t paletteFileSize = (256 * 64 * 4) + 4; 

    uint32_t buffer[paletteFileSize / 4]; 

    for(const auto& pair : palettePaths) {

        const char* inputPalettePath = pair.first;
        const uint32_t inputHash = pair.second;

        if (!fileExists(std::string(inputPalettePath))) {
            continue;
        }

        std::ifstream inFile(inputPalettePath, std::ios::binary | std::ios::ate);
        if(!inFile) {
            continue;
        }

        size_t fileSize = inFile.tellg();

        if(fileSize != paletteFileSize) {
            inFile.close();
            continue;
        }

        inFile.seekg(0, std::ios::beg);

        if(!inFile.read((char*)buffer, paletteFileSize)) {
            inFile.close();
            continue;
        }

        uint32_t hash = 0;

        for (std::size_t i = 4 + (256 * 36); i < 4 + (256 * (36 + 6)); ++i) {
            hash = (hash * 31) + (buffer[i]); // not a good hash func by any means, but functional.
        }

        netlog2("{\"%24s\", 0x%08X},\n", inputPalettePath, hash);

        if(hash == inputHash) { // no modifications were made to this char's palette, just dip
            continue;
        }
        
    }


}

uint_fast8_t PaletteManager::getCharPaletteIndex(uint_fast8_t base) {
    // translate the games id system into a 0-31 to index the array

    switch (base) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            return base;
        case 17:
        case 18:
        case 19:
        case 20:
            return base - 1;
        case 22:
        case 23:
            return base - 2;
        case 25:
            return base - 3;
        case 28:
        case 29:
        case 30:
        case 31:
            return base - 5;
        case 33:
        case 34:
        case 35:
            return base - 6;
        case 51:
            return 30; // hime
        default:
            return 0xFF;
    }

    return 0xFF;
}