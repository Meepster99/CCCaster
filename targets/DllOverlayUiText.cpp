#include "DllOverlayUi.hpp"
#include "DllOverlayPrimitives.hpp"
#include "DllHacks.hpp"
#include "DllTrialManager.hpp"
#include "ProcessManager.hpp"
#include "Enum.hpp"

#include <windows.h>
#include <d3dx9.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>

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

D3DXVECTOR2 scalePosTopLeft = { 0.0f, 0.0f };
D3DXVECTOR2 scalePosRenderFactor = { 0.0f, 0.0f };

void updateScaleParams(IDirect3DDevice9 *device) {

    // this should only be called on resize.

    float vWidth = 640;
    float vHeight = 480;
    float wWidth = 640;
    float wHeight = 480;
    bool isWide = false;

    D3DVIEWPORT9 viewport;
	device->GetViewport(&viewport);
	vWidth = viewport.Width;
	vHeight = viewport.Height;

    HWND hwnd = (HWND) * (DWORD*)(0x0074dfac);

	if (hwnd != NULL) {
		RECT rect;
		if (GetClientRect(hwnd, &rect)) {
			wWidth = rect.right - rect.left;
			wHeight = rect.bottom - rect.top;
		}
	}

    const float ratio = 4.0f / 3.0f;

	isWide = wWidth / wHeight > ratio;

    D3DXVECTOR2 factor;
	factor.x = 1.0f;
	factor.y = 1.0f;

	if (isWide) {
		factor.x = (wHeight * ratio) / wWidth;
	} else {
		factor.y = (wWidth / ratio) / wHeight;
	}

	scalePosRenderFactor.x = 1.0f;
	scalePosRenderFactor.y = 1.0f;

	scalePosRenderFactor.x = (vHeight * (vWidth / vHeight)) / 640.0f;
	scalePosRenderFactor.y = (vWidth / (vWidth / vHeight)) / 480.0f;

	scalePosRenderFactor.x *= factor.x;
	scalePosRenderFactor.y *= factor.y;

	scalePosTopLeft.x = 0.0f;
	scalePosTopLeft.y = 0.0f;

	if (isWide) {
		scalePosTopLeft.x = 640.0f / factor.x;
		scalePosTopLeft.x *= (wWidth - (wHeight * ratio)) / (2.0f * wWidth);
    } else {
		scalePosTopLeft.y = 480.0f / factor.y;
		scalePosTopLeft.y *= (wHeight - (wWidth / ratio)) / (2.0f * wHeight);
	}

}

void scalePoint(float& x, float& y) {
    x += scalePosTopLeft.x;
    y += scalePosTopLeft.y;

    x *= scalePosRenderFactor.x;
    y *= scalePosRenderFactor.y;
}

void DrawRectScaled( IDirect3DDevice9 *device, float x1, float y1, float x2, float y2, const DWORD ARGB, bool mirror) {

    if(mirror) {
        x1 = 640 - x1;
        x2 = 640 - x2;
    }
    
    if(x1 > x2) {
        std::swap(x1, x2);
    }

    if(y1 > y2) {
        std::swap(y1, y2);
    }
  
    scalePoint(x1, y1);
    scalePoint(x2, y2);

    const D3DRECT rect = { (long)x1, (long)y1, (long)x2, (long)y2 };
    device->Clear ( 1, &rect, D3DCLEAR_TARGET, ARGB, 0, 0 );
}

void DrawBorderScaled( IDirect3DDevice9 *device, float x1, float y1, float x2, float y2, float w, const DWORD ARGB, bool mirror) {

    if(mirror) {
        x1 = 640 - x1;
        x2 = 640 - x2;
    }

    if(x1 > x2) {
        std::swap(x1, x2);
    }

    if(y1 > y2) {
        std::swap(y1, y2);
    }

    DrawRectScaled(device, x1, y1, x2, y1 + w, ARGB );
    DrawRectScaled(device, x1, y2 - w, x2, y2, ARGB );
    
    DrawRectScaled(device, x1, y1, x1 + w, y2, ARGB );
    DrawRectScaled(device, x2 - w, y1, x2, y2, ARGB );

}

void updateCSSStuff() {

    // ugh

    typedef struct CSSStructCopy {
        unsigned palette;
        unsigned charID;
        unsigned _unknown1;
        unsigned _unknown2; // possibly port number idek
        unsigned moon;
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

    for(int i=2; i<4; i++) {
        players[i]->palette = 27;
        players[i]->charID = 0xC;
        players[i]->moon = i & 1;
    }




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

    
    if(*((uint8_t*)0x0054EEE8) == 0x14) { // check if in css
        updateCSSStuff();
    }

    if(*((uint8_t*)0x0054EEE8) == 0x01) { // check if ingame
    //if(true) {

        updateScaleParams(device);

        

        // draw meter and health info for p2.
        // and any other stuff like that
        
        // these draws are not ideal. in any way.

        DWORD health[4]; // 11400 max
        DWORD redHealth[4];
        // should probs include guard guage here
        DWORD meter[4];
        DWORD heatTime[4];
        DWORD circuitState[4]; // 0 is normal, 1 is heat, 2 is max, 3 is blood heat

        for(int i=0; i<4; i++) {
            health[i] =       *(DWORD*)(0x005551EC + (i * 0xAFC));
            redHealth[i] =    *(DWORD*)(0x005551F0 + (i * 0xAFC));
            
            meter[i] =        *(DWORD*)(0x00555210 + (i * 0xAFC));
            heatTime[i] =     *(DWORD*)(0x00555214 + (i * 0xAFC));
            circuitState[i] = *(DWORD*)(0x00555218 + (i * 0xAFC));

            if(health[i] == 0) { // this char is dead, set its bg flag. (will bg flags need to be unset on round end? or reset on round start?s)
                *(BYTE*)(0x005552A8 + (i * 0xAFC)) = 0x01;
            }
        }

        float x;
        for(int i=2; i<4; i++) {

            // draw meter bars
            x = 30;
            const int meterWidth = 200;
            DrawBorderScaled(device, x, 428, x + meterWidth, 438, 2, 0xFFFFFFFF, i == 3);
            int currentMeterWidth = (meterWidth * meter[i]) / 30000;
            int meterCol = (meter[i] >= 20000) ? 0xFF00FF00 : ((meter[i] >= 10000) ? 0xFFFFFF00 : 0xFFFF0000);
            DrawBorderScaled(device, x, 428, x + currentMeterWidth, 438, 2, meterCol, i == 3);

            // draw health bars
            x = 58;
            const int healthWidth = 218;
            int currentHealthWidth;
            DrawBorderScaled(device, x, 30, x + 218, 40, 2, 0xFFFFFFFF, i == 3); // white bar
            currentHealthWidth = (healthWidth * redHealth[i]) / 11400;
            DrawBorderScaled(device, x + healthWidth - currentHealthWidth, 30, x + healthWidth, 40, 2, 0xFFFF0000, i == 3); // red health
            currentHealthWidth = (healthWidth * health[i]) / 11400;
            DrawBorderScaled(device, x + healthWidth - currentHealthWidth, 30, x + healthWidth, 40, 2, 0xFFFFFF00, i == 3); // yellow health

        }
        
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

    D3DXMATRIX translate, scale;
    D3DXMatrixScaling ( &scale, scaleX, scaleY, 1.0f );
    D3DXMatrixTranslation ( &translate, 0.0f, 1.0f - scaleY, 0.0f );

    device->SetTexture ( 0, 0 );
    device->SetTransform ( D3DTS_VIEW, & ( scale = scale * translate ) );
    device->SetStreamSource ( 0, background, 0, sizeof ( Vertex ) );
    device->SetFVF ( Vertex::Format );
    device->DrawPrimitive ( D3DPT_TRIANGLESTRIP, 0, 2 );

    // Only draw text if fully enabled or showing a message
    if ( state != State::Enabled )
        return;

    if ( ! ( text[0].empty() && text[1].empty() && text[2].empty() && text[3].empty() && text[4].empty() ) )
    {
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
        }
    }
}
