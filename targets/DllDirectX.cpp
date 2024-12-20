
#include <set>
#include <windows.h>
#include <ws2tcpip.h>
#include <winsock2.h>
#include "DllDirectX.hpp"
#include "resource.h"
#include "DllAsmHacks.hpp"
#include "aacc_2v2_ui_elements.h"

/*

todo, a lot of this code is either legacy, deprecated, only needed for training mode, or just plain stupid
really need to go clean this up
switch to only one vert format

also, evictresources needs to be hooked too!!

*/

void ___log(const char* msg)
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

void log(const char* format, ...) {
	static char buffer[1024]; // no more random char buffers everywhere.
	va_list args;
	va_start(args, format);
	vsnprintf_s(buffer, 1024, format, args);
	___log(buffer);
	va_end(args);
}

void __stdcall printDirectXError(HRESULT hr) {

}

void debugLinkedList();
void displayDebugInfo();
void debugImportantDraw();
void _naked_InitDirectXHooks();
void dualInputDisplay();

Point mousePos; // no use getting this multiple times a frame

// -----

size_t fontBufferSize = 0;
BYTE* fontBuffer = NULL; // this is purposefully not freed on evict btw
IDirect3DTexture9* fontTexture = NULL;

size_t fontBufferSizeWithOutline = 0;
BYTE* fontBufferWithOutline = NULL;
IDirect3DTexture9* fontTextureWithOutline = NULL;

BYTE* fontBufferMelty = NULL;
size_t fontBufferMeltySize = 0;
IDirect3DTexture9* fontTextureMelty = NULL;

IDirect3DTexture9* uiTexture = NULL;

IDirect3DTexture9* nullTex = NULL;

VertexData<PosColVert, 3 * 2048> posColVertData(D3DFVF_XYZ | D3DFVF_DIFFUSE);
VertexData<PosTexVert, 3 * 2048> posTexVertData(D3DFVF_XYZ | D3DFVF_TEX1, &fontTexture);
// need to rework font rendering, 4096 is just horrid
//VertexData<PosColTexVert, 3 * 4096 * 2> posColTexVertData(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, &fontTextureMelty);
VertexData<PosColTexVert, 3 * 4096 * 4> posColTexVertData(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, &fontTextureMelty);
//VertexData<PosColTexVert, 3 * 4096> uiVertData(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, &uiTexture);
VertexData<PosColTexVert, 3 * 4096 * 4> uiVertData(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, &uiTexture);

VertexData<MeltyVert, 3 * 4096> meltyVertData(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, &fontTextureMelty);
VertexData<MeltyVert, 2 * 4096, D3DPT_LINELIST> meltyLineData(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, &fontTextureMelty); // 8192 is overkill

unsigned _vertexBytesTotal = 0;
unsigned _vertexBytesTransferedThisFrame = 0;

// ----

//const int fontTexWidth = 256;
//const int fontTexHeight = 256;
//int fontSize = 32;
const int fontTexWidth = 512;
const int fontTexHeight = 512;
int fontSize = 64;
int fontHeight = fontSize;
int fontWidth = (((float)fontSize) / 2.0f) - 1; // 1 extra pixel is put between each char and the next. take this into account
float fontRatio = ((float)fontWidth) / ((float)fontSize);

IDirect3DPixelShader9* createPixelShader(const char* pixelShaderCode) {

	IDirect3DPixelShader9* res = NULL;
	ID3DXBuffer* pShaderBuffer = NULL;
	ID3DXBuffer* pErrorBuffer = NULL;

	HRESULT hr;

	hr = D3DXCompileShader(
		pixelShaderCode,
		strlen(pixelShaderCode),
		NULL,
		NULL,
		"main",
		"ps_3_0",
		0,
		&pShaderBuffer,
		&pErrorBuffer,
		NULL
	);

	if (FAILED(hr)) {
		log("pixel shader compile failed???");
		printDirectXError(hr);
		if (pErrorBuffer) {
			log((char*)pErrorBuffer->GetBufferPointer());
			pErrorBuffer->Release();
		}
		return NULL;
	}

	hr = device->CreatePixelShader((DWORD*)pShaderBuffer->GetBufferPointer(), &res);

	if (FAILED(hr)) {
		log("createPixelShader died");
		printDirectXError(hr);
		return NULL;
	}

	pShaderBuffer->Release();

	return res;
}

IDirect3DVertexShader9* createVertexShader(const char* shaderCode) {

	IDirect3DVertexShader9* res = NULL;
	ID3DXBuffer* pShaderBuffer = NULL;
	ID3DXBuffer* pErrorBuffer = NULL;

	HRESULT hr;

	hr = D3DXCompileShader(
		shaderCode,
		strlen(shaderCode),
		NULL,
		NULL,
		"main",
		"vs_3_0",
		0,
		&pShaderBuffer,
		&pErrorBuffer,
		NULL
	);

	if (FAILED(hr)) {
		log("vertex shader compile failed???");
		printDirectXError(hr);
		if (pErrorBuffer) {
			log((char*)pErrorBuffer->GetBufferPointer());
			pErrorBuffer->Release();
		}
		return NULL;
	}

	hr = device->CreateVertexShader((DWORD*)pShaderBuffer->GetBufferPointer(), &res);

	if (FAILED(hr)) {
		log("createVertexShader died");
		printDirectXError(hr);
		return NULL;
	}

	pShaderBuffer->Release();

	return res;
}

bool loadResource(int id, BYTE*& buffer, unsigned& bufferSize) {
	
	HMODULE hModule = GetModuleHandle(TEXT("hook.dll"));

	HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(id), "PNG");
	if (!hResource) {
		log("couldnt find embedded resource?");
		return false;
	}

	bufferSize = SizeofResource(hModule, hResource);
	if (!bufferSize) {
		log("getting resource size failed");
		return false;
	}

	HGLOBAL hMemory = LoadResource(hModule, hResource);
	if (!hMemory) {
		log("Failed to load resource");
		return false;
	}

	buffer = (BYTE*)LockResource(hMemory); // for some reason, this lock never has to be unlocked.
	if (!buffer) {
		log("Failed to lock resource");
		return false;
	}

	log("loaded resource %d successfully", id);

	return true;
}

void initTextureResource(int resourceID, IDirect3DTexture9*& resTexture) {

	HRESULT hr;

	BYTE* pngBuffer = NULL;
	unsigned pngSize = 0;
	bool res = loadResource(resourceID, pngBuffer, pngSize);

	hr = D3DXCreateTextureFromFileInMemory(device, pngBuffer, pngSize, &resTexture);
	if (FAILED(hr)) {
		log("_initDefaultFont font createtexfromfileinmem failed?? resource: %d", resourceID);
		resTexture = NULL;
		return;
	}

	log("inited resource texture %d successfully", resourceID);
}

void _initFontFirstLoad() {
	// i dont know why.
	// i dont know how.
	// but SOMEHOW
	// calling DrawTextA overwrites my hooks, and completely fucks EVERYTHING up.
	// i could rehook after it, but holy shit im already rehooking caster hooks, and rehooking beginstate hooks? i think that rehooking 3 times is to many hooks.
	// im going to need to render the font manually.
	// orrr,,, i could,,, try rehooking again? as a treat?
	// no, no treat, you are going to have to do this painfully.
	// never call this func more than once, or while hooked.

	HRESULT hr;

	static bool hasRan = false;
	if (hasRan) {
		return;
	}
	hasRan = true;

	DWORD antiAliasBackup;
	device->GetRenderState(D3DRS_MULTISAMPLEANTIALIAS, &antiAliasBackup);
	device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);

	initTextureResource(IDB_PNG1, fontTexture);
	if (fontTexture == NULL) {
		log("failed _initDefaultFont");
		return;
	}

	fontTextureMelty = fontTexture;

	initTextureResource(IDB_PNG2, uiTexture);
	if(uiTexture == NULL) {
		log("loading uiTexture failed");
		return;
	}
	
	log("loaded font BUFFER!!!");
	device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, antiAliasBackup);
}

void initFont() {

	/*
	if (fontBuffer == NULL) {
		log("initfont font buffer was null, dipping");
		return;
	}

	if (fontTexture != NULL) {
		fontTexture->Release();
		fontTexture = NULL;
	}
	*/

	if (fontTextureWithOutline != NULL) {
		fontTextureWithOutline->Release();
		fontTextureWithOutline = NULL;
	}

	if (fontTextureMelty != NULL) {
		fontTextureMelty->Release();
		fontTextureMelty = NULL;
	}

	HRESULT hr;

	// this texture is D3DPOOL_MANAGED, and so doesnt need a reset on every reset call! yipee
	// going to be real tho, what guarentee do i have that it is????
	/*hr = D3DXCreateTextureFromFileInMemory(device, fontBuffer, fontBufferSize, &fontTexture);
	if (FAILED(hr)) {
		log("default font createtexfromfileinmem failed??");
		return;
	}*/

	/*
	hr = D3DXCreateTextureFromFileInMemory(device, fontBufferWithOutline, fontBufferSizeWithOutline, &fontTextureWithOutline); // this texture is D3DPOOL_MANAGED, and so doesnt need a reset on every reset call! yipee
	if (FAILED(hr)) {
		log("default outline font createtexfromfileinmem failed??");
		return;
	}
	*/

	_initFontFirstLoad();

	/*
	

	hr = D3DXCreateTextureFromFileInMemory(device, fontBufferMelty, fontBufferMeltySize, &fontTextureMelty);
	if (FAILED(hr)) {
		log("melty font createtexfromfileinmem failed??");
		return;
	}
	*/

	//hr = D3DXSaveTextureToFileA("fontTest.png", D3DXIFF_PNG, fontTexture, NULL);

	log("FONT TEXTURE LOADED");
}

// -----

bool _hasStateToRestore = false;
IDirect3DPixelShader9* _pixelShaderBackup;
IDirect3DVertexShader9* _vertexShaderBackup;
IDirect3DBaseTexture9* _textureBackup;
DWORD _D3DRS_BLENDOP;
DWORD _D3DRS_ALPHABLENDENABLE;
DWORD _D3DRS_SRCBLEND;
DWORD _D3DRS_DESTBLEND;
DWORD _D3DRS_SEPARATEALPHABLENDENABLE;
DWORD _D3DRS_SRCBLENDALPHA;
DWORD _D3DRS_DESTBLENDALPHA;
DWORD _D3DRS_MULTISAMPLEANTIALIAS;
DWORD _D3DRS_ALPHATESTENABLE;
DWORD _D3DRS_ALPHAREF;
DWORD _D3DRS_ALPHAFUNC;
D3DMATRIX _D3DTS_VIEW;

float vWidth = 640;
float vHeight = 480;
float wWidth = 640;
float wHeight = 480;
bool isWide = false;
//float factor = 1.0f;
D3DVECTOR factor = { 1.0f, 1.0f, 1.0f };
D3DVECTOR topLeftPos = { 0.0f, 0.0f, 0.0f }; // top left of the screen, in pixel coords. maybe should be in directx space but idk
D3DVECTOR renderModificationFactor = { 1.0f, 1.0f, 1.0f };
//D3DVECTOR renderModificationOffset = { 1.0f, 1.0f, 1.0f };
D3DXVECTOR2 mouseTopLeft;
D3DXVECTOR2 mouseBottomRight;
D3DXVECTOR2 mouseFactor;
bool lClick = false;
bool mClick = false;
bool rClick = false;
bool lHeld = false;
bool mHeld = false;
bool rHeld = false;

bool iDown = false;
bool jDown = false;
bool kDown = false;
bool lDown = false;
bool mDown = false;

bool shouldReverseDraws = false;

// -----

void __stdcall backupRenderState() {

	if (_hasStateToRestore) {
		return;
	}

	_hasStateToRestore = true;

	//IDirect3DSurface9* prevRenderTarget = NULL;
	//device->GetRenderTarget(0, &prevRenderTarget);
	//log("rendertarget: %08X", prevRenderTarget);
	//prevRenderTarget->Release();


	// store state
	device->GetPixelShader(&_pixelShaderBackup);
	device->GetVertexShader(&_vertexShaderBackup);
	device->GetTexture(0, &_textureBackup);

	device->GetRenderState(D3DRS_BLENDOP, &_D3DRS_BLENDOP);
	device->GetRenderState(D3DRS_ALPHABLENDENABLE, &_D3DRS_ALPHABLENDENABLE);
	device->GetRenderState(D3DRS_SRCBLEND, &_D3DRS_SRCBLEND);
	device->GetRenderState(D3DRS_DESTBLEND, &_D3DRS_DESTBLEND);
	device->GetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, &_D3DRS_SEPARATEALPHABLENDENABLE);
	device->GetRenderState(D3DRS_SRCBLENDALPHA, &_D3DRS_SRCBLENDALPHA);
	device->GetRenderState(D3DRS_DESTBLENDALPHA, &_D3DRS_DESTBLENDALPHA);

	device->GetRenderState(D3DRS_MULTISAMPLEANTIALIAS, &_D3DRS_MULTISAMPLEANTIALIAS);
	device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);

	device->GetRenderState(D3DRS_ALPHATESTENABLE, &_D3DRS_ALPHATESTENABLE);
	device->GetRenderState(D3DRS_ALPHAREF, &_D3DRS_ALPHAREF);
	device->GetRenderState(D3DRS_ALPHAFUNC, &_D3DRS_ALPHAFUNC);

	device->SetPixelShader(NULL);
	device->SetVertexShader(NULL);
	device->SetTexture(0, NULL);

	D3DVIEWPORT9 viewport;
	device->GetViewport(&viewport);
	vWidth = viewport.Width;
	vHeight = viewport.Height;
	//vWidth = (4.0f / 3.0f) * vHeight;

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

	factor.x = 1.0f;
	factor.y = 1.0f;

	if (isWide) {
		factor.x = (wHeight * ratio) / wWidth;
	}
	else {
		factor.y = (wWidth / ratio) / wHeight;
	}

	renderModificationFactor.x = 1.0f;
	renderModificationFactor.y = 1.0f;

	//renderModificationFactor.x = (vHeight * (4.0f / 3.0f)) / 640.0f;
	//renderModificationFactor.y = (vWidth / (4.0f / 3.0f)) / 480.0f;

	renderModificationFactor.x = (vHeight * (vWidth / vHeight)) / 640.0f;
	renderModificationFactor.y = (vWidth / (vWidth / vHeight)) / 480.0f;

	renderModificationFactor.x *= factor.x;
	renderModificationFactor.y *= factor.y;

	topLeftPos.x = 0.0f;
	topLeftPos.y = 0.0f;

	if (isWide) {
		topLeftPos.x = 640.0f / factor.x;
		topLeftPos.x *= (wWidth - (wHeight * ratio)) / (2.0f * wWidth);

	}
	else {
		topLeftPos.y = 480.0f / factor.y;
		topLeftPos.y *= (wHeight - (wWidth / ratio)) / (2.0f * wHeight);
	}

	mouseTopLeft.x = 0.0f;
	mouseTopLeft.y = 0.0f;

	// the border on top of the window is taken into account with this. 
	if (isWide) {
		mouseTopLeft.x = (wWidth - (wHeight * (4.0f / 3.0f))) / 2.0f;
	}
	else {
		mouseTopLeft.y = (wHeight - (wWidth / (4.0f / 3.0f))) / 2.0f;
	}

	mouseBottomRight.x = mouseTopLeft.x + (wHeight * (4.0f / 3.0f));
	mouseBottomRight.y = mouseTopLeft.y + (wWidth / (4.0f / 3.0f));

	if (isWide) {
		mouseFactor.x = 640.0f / (wHeight * (4.0f / 3.0f));
		mouseFactor.y = 480.0f / wHeight;
	}
	else {
		mouseFactor.x = 640.0f / wWidth;
		mouseFactor.y = 480.0f / (wWidth / (4.0f / 3.0f));
	}

	// this deals with caster setting the transform matrices. they only seem to use the view matrix? but it would maybe be good to do the others
	device->GetTransform(D3DTS_VIEW, &_D3DTS_VIEW);

	const D3DMATRIX defaultViewMatrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	device->SetTransform(D3DTS_VIEW, &defaultViewMatrix);


	POINT tempMousePos;
	RECT clientRect;
	if (GetCursorPos(&tempMousePos)) {
		HWND hwnd = (HWND) * (DWORD*)(0x0074dfac);
		ScreenToClient(hwnd, &tempMousePos);

		GetClientRect(hwnd, &clientRect);
		if (tempMousePos.x >= clientRect.left && tempMousePos.x < clientRect.right &&
			tempMousePos.y >= clientRect.top && tempMousePos.y < clientRect.bottom) {

			if (tempMousePos.x < mouseTopLeft.x ||
				tempMousePos.y < mouseTopLeft.y ||
				tempMousePos.x > mouseBottomRight.x ||
				tempMousePos.y > mouseBottomRight.y
				) {
				mousePos = { -100.0f, -100.0f };
			}

			mousePos = Point((float)tempMousePos.x, (float)tempMousePos.y);

			mousePos.x -= mouseTopLeft.x;
			mousePos.y -= mouseTopLeft.y;

			mousePos.x *= mouseFactor.x;
			mousePos.y *= mouseFactor.y;


		}
	}
	else {
		mousePos = { -100.0f, -100.0f };
	}

	/*
	static KeyState lButton(VK_LBUTTON);
	static KeyState mButton(VK_MBUTTON);
	static KeyState rButton(VK_RBUTTON);

	lClick = lButton.keyDown();
	mClick = mButton.keyDown();
	rClick = rButton.keyDown();

	lHeld = lButton.keyHeld();
	mHeld = mButton.keyHeld();
	rHeld = rButton.keyHeld();


	static KeyState iKey('I');
	static KeyState jKey('J');
	static KeyState kKey('K');
	static KeyState lKey('L');

	iDown = iKey.keyDown();
	jDown = jKey.keyDown();
	kDown = kKey.keyDown();
	lDown = lKey.keyDown();

	static KeyState mKey('M');
	mDown = mKey.keyDown();
	*/

}

void __stdcall restoreRenderState() {

	if (!_hasStateToRestore) {
		return;
	}

	_hasStateToRestore = false;

	device->SetTexture(0, _textureBackup);
	device->SetPixelShader(_pixelShaderBackup);
	device->SetVertexShader(_vertexShaderBackup);

	if (_textureBackup != NULL) {
		_textureBackup->Release();
	}

	if (_pixelShaderBackup != NULL) {
		_pixelShaderBackup->Release();
	}
	if (_vertexShaderBackup != NULL) {
		_vertexShaderBackup->Release();
	}

	device->SetRenderState(D3DRS_BLENDOP, _D3DRS_BLENDOP);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, _D3DRS_ALPHABLENDENABLE);
	device->SetRenderState(D3DRS_SRCBLEND, _D3DRS_SRCBLEND);
	device->SetRenderState(D3DRS_DESTBLEND, _D3DRS_DESTBLEND);
	device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, _D3DRS_SEPARATEALPHABLENDENABLE);
	device->SetRenderState(D3DRS_SRCBLENDALPHA, _D3DRS_SRCBLENDALPHA);
	device->SetRenderState(D3DRS_DESTBLENDALPHA, _D3DRS_DESTBLENDALPHA);

	device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, _D3DRS_MULTISAMPLEANTIALIAS);

	device->SetRenderState(D3DRS_ALPHATESTENABLE, _D3DRS_ALPHATESTENABLE);
	device->SetRenderState(D3DRS_ALPHAREF, _D3DRS_ALPHAREF);
	device->SetRenderState(D3DRS_ALPHAFUNC, _D3DRS_ALPHAFUNC);

	device->SetTransform(D3DTS_VIEW, &_D3DTS_VIEW);

}

// -----

void LineDraw(float x1, float y1, float x2, float y2, DWORD ARGB, bool side) {

	x1 /= 480.0f;
	x2 /= 480.0f;
	y1 /= 480.0f;
	y2 /= 480.0f;

	// this is going to need to be changed at different resolutions
	float lineWidth = 1.0f / vHeight;

	// i am,,, i bit confused on how exactly to do this. 
	// current vibes say,,, two very thin triangles.

	// i would like,,, diag lines please too.
	// there are,,, angle issues with that tho

	// feel free to play around with https://www.desmos.com/calculator/ppviwesili
	// side chooses which "side" of the input line is actually drawn
	// you will only really care about this,,, if you really care abt it?

	Point p1 = { x1, y1 };
	Point p2 = { x2, y2 };

	float mx = p2.x - p1.x;
	float my = p2.y - p1.y;
	float m = my / mx;

	float a = atan2(my, mx) + (3.1415926535f / 2.0f);

	float m2 = tan(a);

	Point p3 = { x1, y1 };
	Point p4 = { x2, y2 };

	Point offset = { lineWidth * cos(a), lineWidth * sin(a) };

	if (side) {
		p3.x += offset.x;
		p3.y += offset.y;
		p4.x += offset.x;
		p4.y += offset.y;
	}
	else {
		p3.x -= offset.x;
		p3.y -= offset.y;
		p4.x -= offset.x;
		p4.y -= offset.y;
	}

	p1.y = 1 - p1.y;
	p2.y = 1 - p2.y;
	p3.y = 1 - p3.y;
	p4.y = 1 - p4.y;

	PosColVert v1 = { D3DVECTOR((p1.x * 1.5f) - 1.0f, (p1.y * 2.0f) - 1.0f, 0.5f), ARGB };
	PosColVert v2 = { D3DVECTOR((p2.x * 1.5f) - 1.0f, (p2.y * 2.0f) - 1.0f, 0.5f), ARGB };
	PosColVert v3 = { D3DVECTOR((p3.x * 1.5f) - 1.0f, (p3.y * 2.0f) - 1.0f, 0.5f), ARGB };
	PosColVert v4 = { D3DVECTOR((p4.x * 1.5f) - 1.0f, (p4.y * 2.0f) - 1.0f, 0.5f), ARGB };

	scaleVertex(v1.position);
	scaleVertex(v2.position);
	scaleVertex(v3.position);
	scaleVertex(v4.position);

	posColVertData.add(v1, v2, v3);
	posColVertData.add(v2, v3, v4);

}

void RectDraw(float x, float y, float w, float h, DWORD ARGB) {

	if(shouldReverseDraws) {
		x = 640.0f - x;
		x -= w;
	}

	x /= 480.0f;
	w /= 480.0f;
	y /= 480.0f;
	h /= 480.0f;

	y = 1 - y;

	// this code is not working. the above line draw code which does 
	// this code worked on msvc
	// the above code from the line func, DOES work. { D3DVECTOR((p4.x * 1.5f) - 1.0f, (p4.y * 2.0f) - 1.0f, 0.5f), ARGB };
	// is it bc im using brace syntax and its,,, what the fuck
	// c++ is a horrid language
	// gcc conforms more to the spec, but msvc does what i want(when it assumes correctly)
	// that didnt fix it. i dont know what it is
	// i swapped vert order around. maybe i made an error bc vscode is weird and moves lines sometimes, or maybe this compiler is out of line
	PosColVert v1 = { D3DVECTOR(((x + 0) * 1.5f) - 1.0f, ((y + 0) * 2.0f) - 1.0f, 0.5f), ARGB };
	PosColVert v2 = { D3DVECTOR(((x + w) * 1.5f) - 1.0f, ((y + 0) * 2.0f) - 1.0f, 0.5f), ARGB };
	PosColVert v3 = { D3DVECTOR(((x + 0) * 1.5f) - 1.0f, ((y - h) * 2.0f) - 1.0f, 0.5f), ARGB };
	PosColVert v4 = { D3DVECTOR(((x + w) * 1.5f) - 1.0f, ((y - h) * 2.0f) - 1.0f, 0.5f), ARGB };

	scaleVertex(v1.position);
	scaleVertex(v2.position);
	scaleVertex(v3.position);
	scaleVertex(v4.position);

	//log("what %.4f %.4f %.4f", v1.position.x, v1.position.y, v1.position.z);

	posColVertData.add(v1, v2, v3);
	posColVertData.add(v2, v3, v4);
}

void RectDraw(const Rect& rect, DWORD ARGB) {
	RectDraw(rect.x1, rect.y1, rect.x2 - rect.x1, rect.y2 - rect.y1, ARGB);
}

void BorderDraw(float x, float y, float w, float h, DWORD ARGB) {

	if(shouldReverseDraws) {
		x = 640.0f - x;
		x -= w;
	}

	//x /= 480.0f;
	//w /= 480.0f;
	//y /= 480.0f;
	//h /= 480.0f;

	// this mult occurs here because linedraw has a div. need to be sure this is ok
	float lineWidth = 480.0f * (1.0f / vHeight);

	h -= lineWidth;
	w -= lineWidth;

	LineDraw(x + lineWidth, y, x + w, y, ARGB, true);
	LineDraw(x, y, x, y + h, ARGB, false);
	LineDraw(x + w, y, x + w, y + h, ARGB, false);
	LineDraw(x, y + h, x + w + lineWidth, y + h, ARGB, true);
}

void BorderDraw(const Rect& rect, DWORD ARGB) {
	BorderDraw(rect.x1, rect.y1, rect.x2 - rect.x1, rect.y2 - rect.y1, ARGB);
}

void BorderRectDraw(float x, float y, float w, float h, DWORD ARGB) {

	RectDraw(x, y, w, h, ARGB);
	BorderDraw(x, y, w, h, ARGB | 0xFF000000);

}

DWORD naked_meltyDrawTexture_ret;
void meltyDrawTextureDirect(DWORD EDXVAL, DWORD something1, DWORD texture, DWORD xPos, DWORD yPos, DWORD height, DWORD uVar2, DWORD uVar3, DWORD uVar4, DWORD uVar5, DWORD ARGB, DWORD uVar7, DWORD layer) {


	// this func is hacky, and not worth using.
	return;

	/*
	
	edx: displayWidth
	something: something
	texture
	xpos
	ypos
	he
	
	
	
	*/

	// having inline asm read in the vars here,,, would have been cleaner.
	// but it is less readable, and a massive pain
	// 9 pushes, along with one extra as the ret
	//,, i could extern c this thing, but edx is a param!
	// i could fuck with the stack, save the ret, 
	// that might not be the worst idea

	__asmStart R"(
		pop _naked_meltyDrawTexture_ret; // save ret addr in variable
		pop edx; // instead of doing a mov edx, [esp+4], this can be done, and prevent needing another global
	)" __asmEnd

	// the stack is now in such a way as if we,,, were actually calling the function.
	emitCall(0x00415580);

	__asmStart R"(
		// under normal circumstances, we would add esp, 0x30; // above func is cdecl. clean up after it.
		// but these circumstances are weird. the CALLER of this func will clean up the stack for us!
		// but, we popped off an extra edx. we need to put something back.
		push 0x00000000;
		push _naked_meltyDrawTexture_ret;
		ret;
	)" __asmEnd

}

void meltyDrawTexture(DWORD texture, DWORD texX, DWORD texY, DWORD texW, DWORD texH, DWORD x, DWORD y, DWORD w, DWORD h, DWORD ARGB, DWORD layer) {
	
	/*
	
	texture, texWidth, texHeight, x, y, width, height

	uVar5: texHeight
	uVar4: texWidth

	edx: drawWidth
	uVar1: drawHeight	


	
	*/

	// edx is w
	// 0 tex x y h texX texY texW texH

	//meltyDrawTexture(width, 0, texture, x, y, height, xOff, yOff, width, height, ARGB, 0, layer);

	// xOFF and yOFF might be needed for,,, scaling down a tex properly!
	meltyDrawTextureDirect(w, 0, texture, x, y, h, texX, texY, texW, texH, ARGB, 0, layer);

}

void UIDraw(const Rect& texRect, Rect screenRect, DWORD ARGB) {

	if(shouldReverseDraws) { // is this ok? or are triangles being backfaced here
		screenRect.x1 = 640.0f - screenRect.x1;
		screenRect.x2 = 640.0f - screenRect.x2;
		std::swap(screenRect.x1, screenRect.x2);
	}

	// ill take in the screen rect in normal coords. 	

	float x = screenRect.x1 / 480.0f;
	float y = screenRect.y1 / 480.0f;
	float w = (screenRect.x2 / 480.0f) - (screenRect.x1 / 480.0f);
	float h = (screenRect.y2 / 480.0f) - (screenRect.y1 / 480.0f);

	y = 1.0f - y;

	PosColTexVert v1{ D3DVECTOR(((x + 0) * 1.5f) - 1.0f, ((y + 0) * 2.0f) - 1.0f, 0.5f), ARGB, texRect.topLeftV() };
	PosColTexVert v2{ D3DVECTOR(((x + w) * 1.5f) - 1.0f, ((y + 0) * 2.0f) - 1.0f, 0.5f), ARGB, texRect.topRightV() };
	PosColTexVert v3{ D3DVECTOR(((x + 0) * 1.5f) - 1.0f, ((y - h) * 2.0f) - 1.0f, 0.5f), ARGB, texRect.bottomLeftV() };
	PosColTexVert v4{ D3DVECTOR(((x + w) * 1.5f) - 1.0f, ((y - h) * 2.0f) - 1.0f, 0.5f), ARGB, texRect.bottomRightV() };

	scaleVertex(v1.position);
	scaleVertex(v2.position);
	scaleVertex(v3.position);
	scaleVertex(v4.position);

	uiVertData.add(v1, v2, v3);
	uiVertData.add(v2, v3, v4);

}

void UIDraw(const Rect& texRect, const Point& p, DWORD ARGB) {

	// create a screen rect based on the scale of the texture being draw.

	//Rect tempRect(p, 480.0f * texRect.w(), 480.0f * tempRect.h()); // how is this legal, compiling, syntax.
	Rect tempRect(p, 480.0f * texRect.w(), 480.0f * texRect.h()); 

	UIDraw(texRect, tempRect, ARGB);

}

void UIDraw(const Rect& texRect, const Point& p, const float scale, DWORD ARGB) {

	Rect tempRect(p, scale * 480.0f * texRect.w(), scale * 480.0f * texRect.h()); 

	UIDraw(texRect, tempRect, ARGB);

}

// -----

float getCharWidth(const unsigned char c, const float w) {
	// values taken from the switch in below TextDraw func

	float res = w;

	switch(c) {
		case ARROW_1:
		case ARROW_2:
		case ARROW_3:
		case ARROW_4:
		case ARROW_6:
		case ARROW_7:
		case ARROW_8:
		case ARROW_9:
		case BUTTON_A:
		case BUTTON_B:
		case BUTTON_C:
		case BUTTON_D:
		case BUTTON_A_GRAY:
		case BUTTON_B_GRAY:
		case BUTTON_C_GRAY:
		case BUTTON_D_GRAY:
		case ARCICON:
		case MECHICON:
		case HIMEICON:
		case WARAICON:
		case ARROW_0:
		case ARROW_5:
		case JOYSTICK:
		case WHISK:
			break;
		default: 
			res *= 0.75;
			break;
	}

	return res;
}

Rect TextDraw(float x, float y, float size, DWORD ARGB, const char* format) {
	// i do hope that this allocing does not slow things down. i tried saving the va_args for when the actual print func was called, but it would not work

	if (format == NULL) {
		return Rect();
	}

	if(shouldReverseDraws) {
		// this is NOT accurate. .75 comes from default width
		// i should probs,, either make a second switch statement or have a constexpr array
		// i should consider having periods be thinner

		float tempCharWidth = (fontRatio * size * 2.0f);
		float textWidth = 0.0f;
		const char* c = format;
		while(*c != '\0') {
			textWidth += getCharWidth(*c, tempCharWidth);
			c++;
		}
		
		x = 640.0f - x;
		x -= textWidth;
	}

	Rect res(Point(x, y), Point(x, y + size));

	size *= 2.0f;

	x /= 480.0f;
	y /= 480.0f;
	size /= 480.0f;

	/*
	static char buffer[1024];

	va_list args;
	va_start(args, format);
	vsnprintf(buffer, 1024, format, args);
	va_end(args);
	*/

	DWORD TempARGB = ARGB;

	if (fontTexture == NULL) {
		log("fontTexture was null, im not drawing");
		return Rect();
	}

	DWORD antiAliasBackup;
	//device->GetRenderState(D3DRS_MULTISAMPLEANTIALIAS, &antiAliasBackup);
	//device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);

	float origX = x;
	float origY = y;

	float charWidthOffset = (fontRatio * size) * 1.0f;
	float charHeightOffset = size / 2.0; // melty font is weird

	float w = charWidthOffset;
	float h = charHeightOffset;

	float charWidth = ((float)(fontSize >> 1)) / (float)fontTexWidth;
	float charHeight = (((float)fontSize) / (float)fontTexWidth) / 2.0f;

	const char* str = format;

	while (*str) {

		BYTE c = *str;

		charWidthOffset = w * 0.75;

		D3DXVECTOR2 charTopLeft(charWidth * (c & 0xF), charHeight * ((c - 0x00) / 0x10));

		D3DXVECTOR2 charW(charWidth, 0.0);
		D3DXVECTOR2 charH(0.0, charHeight);

		switch (c) {
		case '\r':
		case '\n':
			x = origX;
			origY += charHeightOffset;
			res.y2 += (480.0f * charHeightOffset);
			str++;
			continue;
		case ' ':
			x += charWidthOffset;
			res.x2 += (480.0f * charWidthOffset);
			str++;
			continue;
		case '\t': // please dont use tabs. please
			str++;
			continue;
		case ARROW_1:
		case ARROW_2:
		case ARROW_3:
		case ARROW_4:
		case ARROW_6:
		case ARROW_7:
		case ARROW_8:
		case ARROW_9:
		case BUTTON_A:
		case BUTTON_B:
		case BUTTON_C:
		case BUTTON_D:
		case BUTTON_A_GRAY:
		case BUTTON_B_GRAY:
		case BUTTON_C_GRAY:
		case BUTTON_D_GRAY:
		case ARCICON:
		case MECHICON:
		case HIMEICON:
		case WARAICON:
			charWidthOffset = w;
			break;
		case ARROW_0:
		case ARROW_5:
			charWidthOffset = w;
			break;
		case JOYSTICK:
		case WHISK:
			charWidthOffset = w;
			charW = D3DXVECTOR2(2.0f / 16.0f, 0.0);
			charH = D3DXVECTOR2(0.0, 2.0f / 16.0f);
			break;
		case CURSOR:
			//charTopLeft.y = (8.0f + ((float)(directxFrameCount & 0b1))) / 16.0f;
			charWidthOffset = w * 4;
			charW = D3DXVECTOR2(4.0f / 16.0f, 0.0);
			charH = D3DXVECTOR2(0.0, 1.0f / 16.0f);
			break;
		case CURSOR_LOADING:
			//charTopLeft.x = ((float)(directxFrameCount & 0b111)) / 8.0f;
			charWidthOffset = w * 2;
			charW = D3DXVECTOR2(2.0f / 16.0f, 0.0);
			charH = D3DXVECTOR2(0.0, 2.0f / 16.0f);
			break;
		default:
			break;
		}

		y = 1 - origY;

		PosColTexVert v1{ D3DVECTOR(((x + 0) * 1.5f) - 1.0f, ((y + 0) * 2.0f) - 1.0f, 0.5f), TempARGB, charTopLeft };
		PosColTexVert v2{ D3DVECTOR(((x + w) * 1.5f) - 1.0f, ((y + 0) * 2.0f) - 1.0f, 0.5f), TempARGB, charTopLeft + charW };
		PosColTexVert v3{ D3DVECTOR(((x + 0) * 1.5f) - 1.0f, ((y - h) * 2.0f) - 1.0f, 0.5f), TempARGB, charTopLeft + charH };
		PosColTexVert v4{ D3DVECTOR(((x + w) * 1.5f) - 1.0f, ((y - h) * 2.0f) - 1.0f, 0.5f), TempARGB, charTopLeft + charW + charH };

		scaleVertex(v1.position);
		scaleVertex(v2.position);
		scaleVertex(v3.position);
		scaleVertex(v4.position);

		posColTexVertData.add(v1, v2, v3);
		posColTexVertData.add(v2, v3, v4);

		x += charWidthOffset;
		res.x2 += (480.0f * charWidthOffset);
		str++;
	}

	//device->SetTexture(0, NULL);

	//device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, antiAliasBackup);

	return res;
}

Rect TextDraw(const Point& p, float size, DWORD ARGB, const char* format) {
	return TextDraw(p.x, p.y, size, ARGB, format);
}

void TextDrawSimple(float x, float y, float size, DWORD ARGB, const char* format, ...) {

	if (format == NULL) {
		return;
	}


	x /= 480.0f;
	y /= 480.0f;
	size /= 480.0f;

	static char buffer[1024];

	va_list args;
	va_start(args, format);
	vsnprintf(buffer, 1024, format, args);
	va_end(args);


	DWORD TempARGB = ARGB;

	if (fontTexture == NULL) {
		log("fontTexture was null, im not drawing");
		return;
	}

	DWORD antiAliasBackup;

	float origX = x;
	float origY = y;

	float charWidthOffset = (fontRatio * size) * 1.01f; // this 1.01 might cause issues when aligning stuff, not sure
	float charHeightOffset = size;

	float w = charWidthOffset;
	float h = charHeightOffset;

	char* str = buffer;

	float maxX = 1.0f + ((wWidth - (wHeight * 4.0f / 3.0f)) / 2.0f) / wWidth;

	const float charWidth = ((float)(fontSize >> 1)) / (float)fontTexWidth;
	const float charHeight = ((float)fontSize) / (float)fontTexWidth;

	const float symbolWidth = charWidth;
	const float symbolHeight = charHeight / 2.0f;

	BYTE c;
	bool needDoubleWidth = false;

	D3DXVECTOR2 charTopLeft;

	D3DXVECTOR2 charW;
	D3DXVECTOR2 charH;

	while (*str) {

		c = *str;

		y = 1 - origY;

		switch (c) {
		case ' ':
			x += charWidthOffset;
			str++;
			continue;
		case ARROW_0:
		case ARROW_5:
			x += charWidthOffset;
			x += charWidthOffset;
			str++;
			continue;
		case ARROW_1:
		case ARROW_2:
		case ARROW_3:
		case ARROW_4:
		case ARROW_6:
		case ARROW_7:
		case ARROW_8:
		case ARROW_9:
		case BUTTON_A:
		case BUTTON_B:
		case BUTTON_C:
		case BUTTON_D:

			charTopLeft.x = charWidth * (*str & 0xF);
			charTopLeft.y = (charHeight * 0x6) + (symbolHeight * ((c - 0x80) / 0x10));

			charW = D3DXVECTOR2(symbolWidth, 0.0);
			charH = D3DXVECTOR2(0.0, symbolHeight);

			needDoubleWidth = true;
			w *= 2.0f;

			break;
		default:
			charTopLeft = D3DXVECTOR2(charWidth * (c & 0xF), charHeight * ((c - 0x20) / 0x10));
			charW = D3DXVECTOR2(charWidth, 0.0);
			charH = D3DXVECTOR2(0.0, charHeight);
			break;
		}

		PosTexVert v1{ D3DVECTOR(((x + 0) * 1.5f) - 1.0f, ((y + 0) * 2.0f) - 1.0f, 0.5f), charTopLeft };
		PosTexVert v2{ D3DVECTOR(((x + w) * 1.5f) - 1.0f, ((y + 0) * 2.0f) - 1.0f, 0.5f), charTopLeft + charW };
		PosTexVert v3{ D3DVECTOR(((x + 0) * 1.5f) - 1.0f, ((y - h) * 2.0f) - 1.0f, 0.5f), charTopLeft + charH };
		PosTexVert v4{ D3DVECTOR(((x + w) * 1.5f) - 1.0f, ((y - h) * 2.0f) - 1.0f, 0.5f), charTopLeft + charW + charH };

		scaleVertex(v1.position);
		scaleVertex(v2.position);
		scaleVertex(v3.position);
		scaleVertex(v4.position);

		if (v1.position.x < maxX) {
			posTexVertData.add(v1, v2, v3);
			posTexVertData.add(v2, v3, v4);
		}

		x += charWidthOffset;
		if (needDoubleWidth) {
			needDoubleWidth = false;
			x += charWidthOffset;
			w /= 2.0f;
		}

		str++;
	}
}

// -----

template <typename T>
struct Smooth {
    
    Smooth(T v) : current(v), goal(v), rate(T(1)) {}

	Smooth(T v, T r) : current(v), goal(v), rate(r) {}
    
    void operator=(const T& other) {
        goal = other;
    }
    
    T operator*() {
        
        // could (maybe should) use if constexpr (std::is_floating_point<T>::value)
        // std::abs aparently avoids unneccessary casting!
        // tbh, i dont even need any sort of type check with std::abs at all
       
	   	T delta = std::abs(current - goal);
        if(delta < rate) {
			current = goal;
            return current;   
        }

		T temp = rate;
	
		temp *= MIN(delta / rate, 10.0f);

        current += (current < goal) ? temp : -temp;
        
        return current;
    }
    
	// todo, add arithmetic overloads

    T current;
    T goal;
    T rate; // how much to alter value on every access. would be a template param, but c++ doesnt allow floats as template values
}; 

const float healthBarSmoothValue = 0.005f;
typedef struct HealthBar {
	Smooth<float> yellowHealth = Smooth<float>(1.0f, healthBarSmoothValue);
	Smooth<float> redHealth = Smooth<float>(1.0f, healthBarSmoothValue);
	Smooth<float> meter = Smooth<float>(1.0f, healthBarSmoothValue);
} HealthBar;

void drawNewUI() {
	
	shouldReverseDraws = false;

    for(int i=0; i<4; i++) {
        if(*(BYTE*)(0x00555130 + 0x1B6 + (i * 0xAFC)) != 0) { // is knocked down
            *(BYTE*)(0x005552A8 + (i * 0xAFC)) = 0x01; // sets isBackground flag
        }
        // doing this write here is dumb. p3p4 moon stuff isnt inited properly, i want to go to sleep
        *(DWORD*)(0x00555130 + 0xC + (i * 0xAFC)) = *(DWORD*)(0x0074d840 + 0xC + (i * 0x2C));
    }

	// this code needs a refactor. but i am tired
	static DWORD FN1States[4] = {0, 0, 0, 0}; 
	DWORD baseControlsAddr = *(DWORD*)0x76E6AC;
	for(int i=0; i<4; i++) {
		BYTE fn1 = *(BYTE*)(baseControlsAddr + 0x25 + (i * 0x14));
		fn1 &= 0b1;

		if(!FN1States[i] && fn1) {
			//log("P%d: %d", i, fn1);
			AsmHacks::naked_charTurnAroundState[i] = !AsmHacks::naked_charTurnAroundState[i];
		}		
		
		FN1States[i] = fn1;
	}

	for(int i=0; i<4; i++) {
		if(i & 1) { // team 2
			if(*(BYTE*)(0x00555130 + 0x1B6 + (0 * 0xAFC)) != 0) {
				AsmHacks::naked_charTurnAroundState[i] = 1;
			} else if(*(BYTE*)(0x00555130 + 0x1B6 + (2 * 0xAFC)) != 0) {
				AsmHacks::naked_charTurnAroundState[i] = 0;
			}
		} else { // team 1
			if(*(BYTE*)(0x00555130 + 0x1B6 + (1 * 0xAFC)) != 0) {
				AsmHacks::naked_charTurnAroundState[i] = 1;
			} else if(*(BYTE*)(0x00555130 + 0x1B6 + (3 * 0xAFC)) != 0) {
				AsmHacks::naked_charTurnAroundState[i] = 0;
			}
		}
	}

	shouldReverseDraws = false;
	bool hasYOffset = false;

	static HealthBar healthBars[4];

	constexpr Rect topBars[2] = { UI::P0TopBar, UI::P1TopBar };
	constexpr Rect meterBars[2] = { UI::P0Meter, UI::P1Meter };

	constexpr int displayInts[4] = {1, 3, 2, 4};

	for(int i=0; i<2; i++) {
		shouldReverseDraws = (i == 1); 

		UIDraw(topBars[i], Point(0, 0), 2.0f, 0xFFFFFFFF);

		UIDraw(meterBars[i], Point(0, 480 - (UI::size * meterBars[i].h())), 2.0f, 0xFFFFFFFF);

	}

	for(int i=0; i<4; i++) {

		shouldReverseDraws = (i & 1);
		hasYOffset = (i >= 2);

		healthBars[i].yellowHealth = ((float)*(DWORD*)(0x005551EC + (i * 0xAFC))) / 11400.0f;
		healthBars[i].redHealth = ((float)*(DWORD*)(0x005551F0 + (i * 0xAFC))) / 11400.0f;
		float meterDivisor =  *(DWORD*)(0x0055513C + (i * 0xAFC)) == 2 ? 20000.0f : 30000.0f;
		healthBars[i].meter = ((float)*(DWORD*)(0x00555210 + (i * 0xAFC))) / meterDivisor;

		RectDraw(0, hasYOffset * 100, 300 * (*healthBars[i].redHealth),    10, 0xFFFF0000);
		RectDraw(0, hasYOffset * 100, 300 * (*healthBars[i].yellowHealth), 10, 0xFFFFFF00);

		RectDraw(0, 438 + hasYOffset * 11, 300 * (*healthBars[i].meter), 10, 0xFF00FF00); // go move the code from uioverlay!



		DWORD facing = AsmHacks::naked_charTurnAroundState[i];
		if((i & 1) == 0) {
			facing += 2;
		}

		TextDraw(0, 200 + (hasYOffset * 32), 16, 0xFFFFFFFF, "P%d: facing %d", displayInts[i], facing + 1);

	}

	
}

// -----

void allocVertexBuffers() {

	posColVertData.alloc();
	posTexVertData.alloc();
	posColTexVertData.alloc();
	meltyVertData.alloc();
	meltyLineData.alloc();
	uiVertData.alloc();
}

void _drawGeneralCalls() {

	allocVertexBuffers();

	device->BeginScene();

	posColVertData.draw();

	uiVertData.draw();

	posColTexVertData.draw();

	

	posTexVertData.draw();

	meltyVertData.draw();
	meltyLineData.draw();

	device->EndScene();
}

// -----

void __stdcall _doDrawCalls(IDirect3DDevice9 *deviceExt) {

	device = deviceExt;

	//directxFrameCount++;

	HRESULT hr = device->TestCooperativeLevel();

	if (hr != S_OK) {
		return;
	}
	
	static bool fontInit = false;
	// this messed with hooks on extended. it might with this as well
	if(!fontInit) {
		initFont();
		fontInit = true;
	}

	//TextDraw(0, 0, 16, 0xFFFFFFFF, "money money money!");

	//UIDraw(Rect(Point(0, 0), 1, 1), Point(0, 0), 0xFFFFFFFF);
	//UIDraw(UI::test, Point(0, 0), 0xFFFFFFFF);
	
	drawNewUI();

	//UIDraw(Rect(Point(0, 0), 1, 1), Rect(Point(100, 100), 100, 100), 0xFFFFFFFF);

	// -- ACTUAL RENDERING --

	backupRenderState();

	_drawGeneralCalls();

	restoreRenderState();

	// -- END ACTUAL RENDERING --
	
}

