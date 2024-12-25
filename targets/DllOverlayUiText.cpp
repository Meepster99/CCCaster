#include "DllOverlayUi.hpp"
#include "DllOverlayPrimitives.hpp"
#include "DllHacks.hpp"
#include "DllTrialManager.hpp"
#include "ProcessManager.hpp"
#include "Enum.hpp"
#include "Version.hpp"
#include "DllDirectX.hpp"

#include <windows.h>
#include <d3dx9.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <regex>
#include <vector>

using namespace std;
using namespace DllOverlayUi;


#define OVERLAY_FONT                    "Tahoma"

#define OVERLAY_FONT_HEIGHT             ( 14 )

#define OVERLAY_FONT_WIDTH              ( 5 )

#define OVERLAY_FONT_WEIGHT             ( 600 )

#define OVERLAY_TEXT_COLOR              D3DCOLOR_XRGB ( 255, 255, 255 )

#define OVERLAY_DEBUG_COLOR             D3DCOLOR_XRGB ( 255, 0, 0 )

#define OVERLAY_BUTTON_COLOR            D3DCOLOR_XRGB ( 0, 255, 0 )

#define OVERLAY_BUTTON_DONE_COLOR       D3DCOLOR_XRGB ( 0, 0, 255 )

#define OVERLAY_COMBO_BG_COLOR          D3DCOLOR_XRGB ( 68, 68, 68 )

#define OVERLAY_TEXT_BORDER             ( 10 )

#define OVERLAY_SELECTOR_L_COLOR        D3DCOLOR_XRGB ( 210, 0, 0 )

#define OVERLAY_SELECTOR_R_COLOR        D3DCOLOR_XRGB ( 30, 30, 255 )

#define OVERLAY_SELECTOR_X_BORDER       ( 5 )

#define OVERLAY_SELECTOR_Y_BORDER       ( 1 )

#define OVERLAY_BG_COLOR                D3DCOLOR_ARGB ( 220, 0, 0, 0 )

#define OVERLAY_CHANGE_DELTA            ( 4 + abs ( height - newHeight ) / 4 )

#define INLINE_RECT(rect)               rect.left, rect.top, rect.right, rect.bottom


ENUM ( State, Disabled, Disabling, Enabled, Enabling );

static State state = State::Disabled;

ENUM ( Mode, None, Trial, Mapping );

static Mode mode = Mode::None;

static int height = 0, oldHeight = 0, newHeight = 0;

static int initialTimeout = 0, messageTimeout = 0;

static array<string, 5> text;

static array<RECT, 4> selector;

static array<bool, 4> shouldDrawSelector { false, false, false, false };

static ID3DXFont *font = 0;

static IDirect3DVertexBuffer9 *background = 0;

static array<string, 4> selectorLine;

static array<int, 4> selectorIndex = {0, 0, 0, 0};

namespace DllOverlayUi
{

array<string, 5> getText()
{
    return text;
}

array<string, 4> getSelectorLine()
{
    return selectorLine;
}

int getHeight()
{
    return height;
}

int getNewHeight()
{
    return newHeight;
}

array<RECT, 4> getSelector()
{
    return selector;
}

array<bool, 4> getShouldDrawSelector()
{
    return shouldDrawSelector;
}

void enable()
{
    TrialManager::showCombo = false;
    if ( state != State::Enabled )
        state = State::Enabling;
}

void disable()
{
    TrialManager::showCombo = true;
    if ( state != State::Disabled )
        state = State::Disabling;
}

void toggle()
{
    if ( isEnabled() )
        disable();
    else
        enable();
}

static inline int getTextHeight ( const array<string, 5>& newText )
{
    int height = 0;

    for ( const string& text : newText )
        height = max ( height, OVERLAY_FONT_HEIGHT * ( 1 + count ( text.begin(), text.end(), '\n' ) ) );

    return height;
}

void updateText()
{
    updateText ( text );
}

void updateText ( const array<string, 5>& newText )
{
    switch ( state.value )
    {
        default:
        case State::Disabled:
            height = oldHeight = newHeight = 0;
            text = { "", "", "" };
            return;

        case State::Disabling:
            newHeight = 0;

            if ( height != newHeight )
                break;

            state = State::Disabled;
            oldHeight = 0;
            text = { "", "", "" };
            break;

        case State::Enabled:
            newHeight = getTextHeight ( newText );

            if ( newHeight > height )
                break;

            if ( newHeight == height )
                oldHeight = height;

            text = newText;
            break;

        case State::Enabling:
            newHeight = getTextHeight ( newText );

            if ( height != newHeight )
                break;

            state = State::Enabled;
            oldHeight = height;
            text = newText;
            break;
    }

    if ( height == newHeight )
        return;

    if ( newHeight > height )
        height = clamped ( height + OVERLAY_CHANGE_DELTA, height, newHeight );
    else
        height = clamped ( height - OVERLAY_CHANGE_DELTA, newHeight, height );
}

bool isEnabled()
{
    return ( state != State::Disabled ) && ( messageTimeout <= 0 );
}

bool isDisabled()
{
    return ( state == State::Disabled );
}

bool isToggling()
{
    return ( state == State::Enabling || state == State::Disabling );
}

bool isTrial()
{
    return mode == Mode::Trial;
}

bool isMapping()
{
    return mode == Mode::Mapping;
}

void setTrial()
{
    mode = Mode::Trial;
}

void setMapping()
{
    mode = Mode::Mapping;
}

void showMessage ( const string& newText, int timeout )
{
    // Get timeout in frames
    initialTimeout = messageTimeout = ( timeout / 17 );

    // Show the message in the middle
    text = { "", newText, "" };
    shouldDrawSelector = { false, false, false, false };

    enable();
}

void updateMessage()
{
    updateText ( text );

    if ( messageTimeout == 1 )
    {
        if ( state == State::Disabled )
            messageTimeout = 0;
        return;
    }

    if ( messageTimeout <= 2 )
    {
        disable();
        messageTimeout = 1;
        return;
    }

    // Reset message timeout when backgrounded
    if ( DllHacks::windowHandle != GetForegroundWindow() )
        messageTimeout = initialTimeout;
    else
        --messageTimeout;
}

void updateSelector ( uint8_t index, int position, const string& line )
{
    if ( index >= shouldDrawSelector.size() )
        return;

    selectorIndex[index] = position;// _overlayPositions[index];

    selectorLine[index] = line;
    if ( position == 0 || line.empty() )
    {
        shouldDrawSelector[index] = false;
        return;
    }

    RECT rect;
    rect.top = rect.left = 0;
    rect.right = 1;
    rect.bottom = OVERLAY_FONT_HEIGHT;
    DrawText ( font, line, rect, DT_CALCRECT, D3DCOLOR_XRGB ( 0, 0, 0 ) );

    rect.top    += OVERLAY_TEXT_BORDER + position * OVERLAY_FONT_HEIGHT - OVERLAY_SELECTOR_Y_BORDER + 1;
    rect.bottom += OVERLAY_TEXT_BORDER + position * OVERLAY_FONT_HEIGHT + OVERLAY_SELECTOR_Y_BORDER;

    if ( index == 0 )
    {
        rect.left  += OVERLAY_TEXT_BORDER - OVERLAY_SELECTOR_X_BORDER;
        rect.right += OVERLAY_TEXT_BORDER + OVERLAY_SELECTOR_X_BORDER;
    }
    else
    {
        rect.left  = ( * CC_SCREEN_WIDTH_ADDR ) - rect.right - OVERLAY_TEXT_BORDER - OVERLAY_SELECTOR_X_BORDER;
        rect.right = ( * CC_SCREEN_WIDTH_ADDR ) - OVERLAY_TEXT_BORDER + OVERLAY_SELECTOR_X_BORDER;
    }

    selector[index] = rect;
    shouldDrawSelector[index] = true;
}

bool isShowingMessage()
{
    return ( messageTimeout > 0 );
}

#ifndef RELEASE

string debugText;

int debugTextAlign = 0;

#endif // NOT RELEASE

} // namespace DllOverlayUi


struct Vertex
{
    FLOAT x, y, z;
    DWORD color;

    static const DWORD Format = ( D3DFVF_XYZ | D3DFVF_DIFFUSE );
};



void initOverlayText ( IDirect3DDevice9 *device )
{
    D3DXCreateFont ( device,                                    // device pointer
                     OVERLAY_FONT_HEIGHT,                       // height
                     OVERLAY_FONT_WIDTH,                        // width
                     OVERLAY_FONT_WEIGHT,                       // weight
                     1,                                         // # of mipmap levels
                     FALSE,                                     // italic
                     ANSI_CHARSET,                              // charset
                     OUT_DEFAULT_PRECIS,                        // output precision
                     ANTIALIASED_QUALITY,                       // quality
                     DEFAULT_PITCH | FF_DONTCARE,               // pitch and family
                     OVERLAY_FONT,                              // typeface name
                     &font );                                   // pointer to ID3DXFont

    static const Vertex verts[4] =
    {
        { -1, -1, 0, OVERLAY_BG_COLOR },
        {  1, -1, 0, OVERLAY_BG_COLOR },
        { -1,  1, 0, OVERLAY_BG_COLOR },
        {  1,  1, 0, OVERLAY_BG_COLOR },
    };

    device->CreateVertexBuffer ( 4 * sizeof ( Vertex ),         // buffer size in bytes
                                 0,                             // memory usage flags
                                 Vertex::Format,                // vertex format
                                 D3DPOOL_MANAGED,               // memory storage flags
                                 &background,                    // pointer to IDirect3DVertexBuffer9
                                 0 );                           // unused

    void *ptr;

    background->Lock ( 0, 0, ( void ** ) &ptr, 0 );
    memcpy ( ptr, verts, 4 * sizeof ( verts[0] ) );
    background->Unlock();
}

void invalidateOverlayText()
{
    if ( font )
    {
        font->OnLostDevice();
        font = 0;
    }

    if ( background )
    {
        background->Release();
        background = 0;
    }
}

#define SAFEMOD(a, b) (((b) + ((a) % (b))) % (b))

typedef struct RawInput {
    BYTE dir = 0;
    BYTE btn = 0;
    void set(int playerIndex) {
        DWORD baseControlsAddr = *(DWORD*)0x76E6AC;
        if(baseControlsAddr == 0) {
            return;
        }

        dir = *(BYTE*)(baseControlsAddr + 0x18 + (playerIndex * 0x14));
        btn = *(BYTE*)(baseControlsAddr + 0x24 + (playerIndex * 0x14));
    }
} RawInput;

typedef struct OurCSSData { // variables i want to keep track of
    int idIndex = 0; // what char is hovered, indexed in the below list
    int selectIndex = 0; // what vertical position, char/moon/palette/ready is selected
    RawInput prevInput;
    RawInput input;
    // i should probs just read from where melty gets these,,, but im tired ok? 
    BYTE pressDir() {
        if(prevInput.dir != input.dir) {
            return input.dir;
        }
        return 0;
    }
} OurCSSData;

OurCSSData ourCSSData[4];

void updateCSSStuff(IDirect3DDevice9 *device) {

    shouldReverseDraws = false;

    // ugh. this might not be the best place for this

    typedef struct CSSStructCopy {
        int palette;
        int charID;
        unsigned _unknown1;
        unsigned _unknown2; // possibly port number idek
        int moon;
        unsigned _unknown3;
        unsigned _unknown4;
        unsigned _unknown5;
        unsigned _unknown6;
        unsigned _unknown7;
        unsigned _unknown8;
    } CSSStructCopy;

    static_assert(sizeof(CSSStructCopy) == 0x2C, "CSSStructCopy must be size 0x2C");

    CSSStructCopy* player0 = (CSSStructCopy*)(0x0074d83C + (0 * 0x2C));
    CSSStructCopy* player1 = (CSSStructCopy*)(0x0074d83C + (1 * 0x2C));
    CSSStructCopy* player2 = (CSSStructCopy*)(0x0074d83C + (2 * 0x2C));
    CSSStructCopy* player3 = (CSSStructCopy*)(0x0074d83C + (3 * 0x2C));

    CSSStructCopy* players[4] = {
        (CSSStructCopy*)(0x0074d83C + (0 * 0x2C)),
        (CSSStructCopy*)(0x0074d83C + (1 * 0x2C)),
        (CSSStructCopy*)(0x0074d83C + (2 * 0x2C)),
        (CSSStructCopy*)(0x0074d83C + (3 * 0x2C))
    };

    static_assert(sizeof(charIDList) / sizeof(charIDList[0]) == sizeof(charIDNames) / sizeof(charIDNames[0]), "length of name and id list must be the same");
    constexpr int charIDCount = sizeof(charIDList) / sizeof(charIDList[0]); 

    std::function<void(int playerIndex, int inc)> ControlFuncs[] = {

        [&](int playerIndex, int inc) mutable -> void {
            ourCSSData[playerIndex].idIndex = SAFEMOD(ourCSSData[playerIndex].idIndex + inc, charIDCount); 
        },

        [&](int playerIndex, int inc) mutable -> void {
            players[playerIndex]->moon = SAFEMOD(players[playerIndex]->moon + inc, 3);
        },

        [&](int playerIndex, int inc) mutable -> void {
            players[playerIndex]->palette = SAFEMOD(players[playerIndex]->palette + inc, 36);
        },

        [&](int playerIndex, int inc) mutable -> void {

        }

    };

    const int menuOptionCount = sizeof(ControlFuncs) / sizeof(ControlFuncs[0]);

    // handle controls
    // idk where i should grab controls from either, what reads the stuff melty writes to? or i could be safe with a direct melty write, but have to keep track 
    // of presses myself
    // maybe one less patch tho
    for(int i=2; i<4; i++) {

        ourCSSData[i].input.set(i);

        BYTE pressDir = ourCSSData[i].pressDir();

        switch(pressDir) {
            case 2:
                ourCSSData[i].selectIndex = SAFEMOD(ourCSSData[i].selectIndex + 1, menuOptionCount);
                break;
            case 8:
                ourCSSData[i].selectIndex = SAFEMOD(ourCSSData[i].selectIndex - 1, menuOptionCount);
                break;
            case 4:
                ControlFuncs[ourCSSData[i].selectIndex](i, -1);
                break;
            case 6:
                ControlFuncs[ourCSSData[i].selectIndex](i,  1);
                break;
            default:
                break;
        }
        
        ourCSSData[i].prevInput = ourCSSData[i].input;
    }

    constexpr float cssMenuFontSize = 12.0f;
    constexpr float cssMenuSelectorWidth = 128.0f;

    std::function<void(int selfIndex, int playerIndex, float& x, float& y)> CSSFuncs[] = { // i could pass in more params, but tbh ill just let each function do its own vibe

        [&](int selfIndex, int playerIndex, float& x, float& y) mutable -> void {

            bool mirror = playerIndex & 1;
            DWORD bgCol = selfIndex == ourCSSData[playerIndex].selectIndex ? 0xFFFF0000 : 0xFF000000;

            players[playerIndex]->charID = charIDList[ourCSSData[playerIndex].idIndex];

            int displayIndex = playerIndex;
            if(displayIndex == 1) {
                displayIndex = 2;
            } else if(displayIndex == 2) {
                displayIndex = 1;
            }

            std::string tempCharString = "P" + std::to_string(displayIndex + 1) + ": " + charIDNames[ourCSSData[playerIndex].idIndex];
            RectDraw(x, y, cssMenuSelectorWidth, cssMenuFontSize, bgCol);
            TextDraw(x, y, cssMenuFontSize, 0xFFFFFFFF, tempCharString.c_str());
            y += cssMenuFontSize;      
        },

        [&](int selfIndex, int playerIndex, float& x, float& y) mutable -> void {

            bool mirror = playerIndex & 1;
            DWORD bgCol = selfIndex == ourCSSData[playerIndex].selectIndex ? 0xFFFF0000 : 0xFF000000;

            const char* tempMoonString = players[playerIndex]->moon == 0 ? "Crescent" : (players[playerIndex]->moon == 1 ? "Full" : "Half"); 

            RectDraw(x, y, cssMenuSelectorWidth, cssMenuFontSize, bgCol);
            TextDraw(x, y, cssMenuFontSize, 0xFFFFFFFF, tempMoonString);
            y += cssMenuFontSize;      
        },

        [&](int selfIndex, int playerIndex, float& x, float& y) mutable -> void {

            bool mirror = playerIndex & 1;
            DWORD bgCol = selfIndex == ourCSSData[playerIndex].selectIndex ? 0xFFFF0000 : 0xFF000000;

            std::string tempPaletteString = "Palette: " + std::to_string(players[playerIndex]->palette + 1);
            RectDraw(x, y, cssMenuSelectorWidth, cssMenuFontSize, bgCol);
            TextDraw(x, y, cssMenuFontSize, 0xFFFFFFFF, tempPaletteString.c_str());
            y += cssMenuFontSize;      
        },

        [&](int selfIndex, int playerIndex, float& x, float& y) mutable -> void {

            bool mirror = playerIndex & 1;
            DWORD bgCol = selfIndex == ourCSSData[playerIndex].selectIndex ? 0xFFFF0000 : 0xFF000000;

            RectDraw(x, y, cssMenuSelectorWidth, cssMenuFontSize, bgCol);
            TextDraw(x, y, cssMenuFontSize, 0xFFFFFFFF, "Ready(notworking)");
            y += cssMenuFontSize;      
        }

    };

    static_assert(sizeof(CSSFuncs) / sizeof(CSSFuncs[0]) == sizeof(ControlFuncs) / sizeof(ControlFuncs[0]), "each cssfunc must have a controlfunc!");

    // draw the shit 
    for(int i=2; i<4; i++) {

        shouldReverseDraws = (i == 3);

        float x = 20.0f;
        float y = 100.0f;

        for(size_t selfIndex = 0; selfIndex < sizeof(CSSFuncs) / sizeof(CSSFuncs[0]); selfIndex++) {
            CSSFuncs[selfIndex](selfIndex, i, x, y);
        }
    }

    shouldReverseDraws = true;
    TextDraw(10, 10 + (0 * 8), 8, 0xFFFFFFFF, "please follow me on twitter so i have motivation for this");
    TextDraw(10, 10 + (1 * 8), 8, 0xFFFFFFFF, "@Meepster99");
    TextDraw(10, 10 + (2 * 8), 8, 0xFFFFFFFF, ":3");
    
    shouldReverseDraws = false;
    if(updateOccured) {
        TextDraw(10, 450 + (0 * 8), 8, 0xFF42e5f4, "A new update was downloaded. Restart melty please.");
    }
    TextDraw(10, 450 + (1 * 8), 8, 0xFFFFFFFF, "send bug reports in https://discord.gg/vzvyjz775r");
    TextDraw(10, 450 + (2 * 8), 8, 0xFFFFFFFF, "Version: %s", LocalVersion.revision.c_str());
    

}

std::string strip(const std::string& s) {
	std::string res = s;
	std::regex trim_re("^\\s+|\\s+$");
	res = std::regex_replace(res, trim_re, "");
	return res;
}

std::vector<std::string> stripMenuString(std::string s) {

    std::vector<std::string> res;

    int i=0; 
    while(true) {
                
        int temp = s.find('\n', i);

        std::string tempString = strip(s.substr(i, temp - i));
        if(tempString.size() != 0) {
            res.push_back(tempString);
        }
        
        if(temp == std::string::npos) {
            break;
        }
        i = temp + 1;
        
    }

    return res;
}

void renderOverlayText ( IDirect3DDevice9 *device, const D3DVIEWPORT9& viewport )
{
#ifndef RELEASE

    if ( ! debugText.empty() )
    {
        RECT rect;
        rect.top = rect.left = 0;
        rect.right = viewport.Width;
        rect.bottom = viewport.Height;

        DrawText ( font, debugText, rect, DT_WORDBREAK |
                   ( debugTextAlign == 0 ? DT_CENTER : ( debugTextAlign < 0 ? DT_LEFT : DT_RIGHT ) ),
                   OVERLAY_DEBUG_COLOR );
    }

#endif // RELEASE

    if(*((uint8_t*) 0x0054EEE8) == 0x14 && DllOverlayUi::isDisabled()) { // check if in css
        updateCSSStuff(device);
    }

    if ( ! TrialManager::dtext.empty() && !TrialManager::hideText ) {
        int debugTextAlign = 1;
        RECT rect2;
        rect2.top = rect2.left = 0;
        rect2.right = viewport.Width;
        rect2.bottom = viewport.Height;
        DrawText ( font, TrialManager::dtext, rect2, DT_WORDBREAK |
                   ( debugTextAlign == 0 ? DT_CENTER : ( debugTextAlign < 0 ? DT_LEFT : DT_RIGHT ) ),
                   OVERLAY_DEBUG_COLOR );
    }
    if ( ! TrialManager::comboTrialText.empty() && !TrialManager::hideText )
    {
        /*
        if ( TrialManager::trialTextures == NULL ) {
            char* filename = "arrow.png";
            char* filename2 = "tutorial00.bmp";
            ifstream input( filename, ios::binary );
            vector<char> buffer( istreambuf_iterator<char>(input), {} );
            int imgsize = buffer.size();
            char* rawimg = &buffer[0];
            int (*loadTextureFromMemory) (int, char*, int, int) = (int(*)(int, char*, int, int)) 0x4bd2d0;
            D3DXCreateTextureFromFile( device, filename, &TrialManager::trialTextures );
            TrialManager::trialTextures2 = loadTextureFromMemory(0, rawimg, imgsize, 0);
        }
        */
        RECT rect;
        rect.top = 70;
        rect.left = 20;
        rect.right = viewport.Width - 20;
        rect.bottom = viewport.Height - 20;
        wstring w = TrialManager::fullStrings[TrialManager::currentTrialIndex];
        TextCalcRectW( font, w, rect, DT_CENTER | DT_WORDBREAK, 0);
        rect.left = 20;
        rect.right = viewport.Width - 20;
        DrawRectangle ( device, INLINE_RECT ( rect ), OVERLAY_COMBO_BG_COLOR );
        uint32_t i = 0;
        rect.left = 30;
        for ( wstring text : TrialManager::comboTrialText ) {
            TextCalcRectW( font, text, rect, DT_LEFT, 0);
            D3DCOLOR color = ( TrialManager::comboTrialPosition > i ) ? OVERLAY_BUTTON_DONE_COLOR :
              ( TrialManager::comboTrialPosition == i ) ? OVERLAY_DEBUG_COLOR : OVERLAY_BUTTON_COLOR;
            DrawTextW ( font, text, rect, DT_WORDBREAK |
                   ( TrialManager::comboTrialTextAlign == 0 ? DT_CENTER : ( TrialManager::comboTrialTextAlign < 0 ? DT_LEFT : DT_RIGHT ) ),
                       color );
            rect.left = rect.right;
            long int newlineBreakpoint = viewport.Width - 70;
            if ( rect.left > newlineBreakpoint ) {
                rect.left = 29;
                rect.top = rect.bottom;
            }
        ++i;
        }

        RECT rect3;
        rect3.top = 50;
        rect3.left = 30;
        rect3.right = viewport.Width;
        rect3.bottom= viewport.Height;

        TextCalcRectW( font, TrialManager::comboName, rect3, DT_LEFT | DT_WORDBREAK, 0);
        rect3.left = 20;
        rect3.right = viewport.Width - 20;
        DrawRectangle ( device, INLINE_RECT ( rect3 ), OVERLAY_COMBO_BG_COLOR );
        DrawTextW ( font, TrialManager::comboName, rect3, DT_WORDBREAK | DT_LEFT,
                    OVERLAY_DEBUG_COLOR );
    }
    if ( state == State::Disabled )
        return;

    // Calculate message width if showing one
    float messageWidth = 0.0f;
    if ( isShowingMessage() )
    {
        RECT rect;
        rect.top = rect.left = 0;
        rect.right = 1;
        rect.bottom = OVERLAY_FONT_HEIGHT;

        DrawText ( font, text[1], rect, DT_CALCRECT, D3DCOLOR_XRGB ( 0, 0, 0 ) );

        messageWidth = rect.right + 2 * OVERLAY_TEXT_BORDER;
    }

    // Scaling factor for the overlay background
    const float scaleX = ( isShowingMessage() ? messageWidth / viewport.Width : 1.0f );
    const float scaleY = float ( height + 2 * OVERLAY_TEXT_BORDER ) / viewport.Height;

    // i tired to comment out this code, only to have my draw calls not draw?
    D3DXMATRIX translate, scale;
    D3DXMatrixScaling ( &scale, scaleX, scaleY, 1.0f );
    D3DXMatrixTranslation ( &translate, 0.0f, 1.0f - scaleY, 0.0f );

    device->SetTexture ( 0, 0 );
    device->SetTransform ( D3DTS_VIEW, & ( scale = scale * translate ) );
    device->SetStreamSource ( 0, background, 0, sizeof ( Vertex ) );
    device->SetFVF ( Vertex::Format );
    //device->DrawPrimitive ( D3DPT_TRIANGLESTRIP, 0, 2 );

    // Only draw text if fully enabled or showing a message
    /*if ( state != State::Enabled )
        return;*/

    if ( ! ( text[0].empty() && text[1].empty() && text[2].empty() && text[3].empty() && text[4].empty() ) )
    {

        static unsigned frameCounter = 0;

        /*
        const int centerX = viewport.Width / 2;

        RECT rect;
        rect.left   = centerX - int ( ( viewport.Width / 2 ) * 1.0 ) + OVERLAY_TEXT_BORDER;
        rect.right  = centerX + int ( ( viewport.Width / 2 ) * 1.0 ) - OVERLAY_TEXT_BORDER;
        rect.top    = OVERLAY_TEXT_BORDER;
        rect.bottom = rect.top + height + OVERLAY_TEXT_BORDER;

        for(size_t _i = 0; _i < text.size(); _i++) {
            // -1s are here since text has size 5, selectors have size 4
            if ( _i > 0 && newHeight == height && shouldDrawSelector[_i - 1] ) {

                RECT tempRect = selector[_i - 1];
                tempRect.left = rect.left;
                tempRect.right = rect.left + (viewport.Width / 5);
                long tempRectHeight = tempRect.bottom - tempRect.top;
                tempRect.top += (2 * tempRectHeight);
                tempRect.bottom = tempRect.top + tempRectHeight;

                DrawRectangle ( device, INLINE_RECT ( tempRect ), OVERLAY_SELECTOR_L_COLOR );
            }

            if ( ! text[_i].empty() ) {

                std::string tempText = "";
                //tempText += std::to_string(_i) + " ";
                if(_i > 0) {
                    tempText += std::to_string(shouldDrawSelector[_i - 1]);
                }
                tempText += text[_i];

                DrawText ( font, tempText, rect, DT_WORDBREAK | DT_LEFT, OVERLAY_TEXT_COLOR );
                // this shouldnt be needed? how did this system work previously?
                rect.left += (viewport.Width / 5);
                rect.right += (viewport.Width / 5);

            }
        } */

       
        // look. i know this sucks. im super tired. ill fix it tomorow

        const Point textPosData[5] = {
            Point(640/3, 480/4),
            Point(0,     0), // P0
            Point(0,     0), // P2
            Point(0, 480/2), // P1
            Point(0, 480/2)  // P3
        };
        const float fontSize = 12.0f;

        RectDraw(0, 0, 640, 480, 0x80000000); 

        for(int i=0; i<text.size(); i++) { 

            shouldReverseDraws = false;

            if(i == 2 || i == 4) {
                shouldReverseDraws = true;
            }

            Point textPoint = textPosData[i];

            std::vector<std::string> strings = stripMenuString(text[i]);

            Rect maxRect;

            for(int j=0; j<strings.size(); j++) {

                DWORD textCol = 0xFFFFFFFF;
                if(j == 0) {
                    textCol = 0xFF42e5f4;
                }

                if(j == 1 && i == 0) {
                   
                    frameCounter++;
                    float f = std::abs((int)(frameCounter & 0b111111) - 32);
                    f = ((-1.0f / 32.0f) * f) + 1.0f;

                    textCol = avgColors(0xFFbd1a0b, 0xFF42e5f4, f);
                }

                if(i > 0 && shouldDrawSelector[i - 1] && j == selectorIndex[i - 1]) {
                    RectDraw(textPoint.x, textPoint.y + (2 * fontSize), 164, fontSize, 0xE0FF0000);
                }

                Rect tempRect = TextDraw(textPoint, fontSize, textCol, strings[j].c_str());
                textPoint.y += fontSize;

                if(j == 0) {
                    maxRect = tempRect;
                } else {
                    maxRect.x1 = MIN(maxRect.x1, tempRect.x1);
                    maxRect.y1 = MIN(maxRect.y1, tempRect.y1);
                    maxRect.x2 = MAX(maxRect.x2, tempRect.x2);
                    maxRect.y2 = MAX(maxRect.y2, tempRect.y2);
                }
            }

            //RectDraw(maxRect, 0xC0000000);
        }
    }
}
