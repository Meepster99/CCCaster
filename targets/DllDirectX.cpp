
#include <set>
#include <windows.h>
#include <ws2tcpip.h>
#include <winsock2.h>
#include "DllDirectX.hpp"
#include "resource.h"
#include "DllAsmHacks.hpp"
#include "DllOverlayUi.hpp"
#include "aacc_2v2_ui_elements.h"
#include <regex>
#include <optional>

#include <string>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <wininet.h>
#include <fstream>
#include <time.h>

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

const char* getCharName(int id) {

	int lookup = -1;

	switch (id) {
	case 0:
	case 1:
	case 2:
	case 3:
		lookup = id;
		break;
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
		lookup = id - 1;
		break;
	case 17:
	case 18:
	case 19:
	case 20:
		lookup = id - 2;
		break;
	case 22:
	case 23:
		lookup = id - 3; 
		break;
	case 25:
		lookup = id - 4;
		break;
	case 28:
	case 29:
	case 30:
	case 31:
		lookup = id - 6;
		break;
	case 33:
		lookup = id - 7;
		break;
	case 51:
		lookup = 27; 
	case 4: // maids
	case 34: // NecoMech
	case 35: // KohaMech
	default:
		break;
	}

	if(lookup < 0 || lookup >= (sizeof(charIDNames) / sizeof(charIDNames[0]))) {
		return "???";
	}

	return charIDNames[lookup];
}

std::string getReleaseInfo() {
	// https://api.github.com/repos/fangdreth/MBAACC-Extended-Training-Mode/releases/tags/bleeding-edge

	HINTERNET hInternet = InternetOpenW(L"GitHubAPI", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (hInternet == NULL) {
		log("InternetOpen failed: % d", GetLastError());
		return "";
	}

	HINTERNET hConnect = InternetOpenUrlW(hInternet, L"https://api.github.com/repos/Meepster99/CCCaster/releases/tags/bleeding-edge", NULL, 0, INTERNET_FLAG_RELOAD, 0);
	if (hConnect == NULL) {
		log("InternetOpenUrlW failed: %d", GetLastError());
		InternetCloseHandle(hInternet);
		return "";
	}

	char buffer[8192];
	DWORD bytesRead;
	std::string response;

	while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
		response.append(buffer, bytesRead);
	}

	InternetCloseHandle(hConnect);
	InternetCloseHandle(hInternet);

	std::regex pattern(R"("published_at"\s*:\s*"[^"]*")");

	std::smatch match;
	if (!std::regex_search(response, match, pattern)) {
		log("couldnt find published_at in json: %s", response.c_str());
		return "";
	}

	std::string temp = match[2].str();

	return temp;
}

time_t toTimestamp(const std::string& dateTimeStr) {
	std::tm tm = {};
	std::istringstream ss(dateTimeStr);
	ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%SZ");


	time_t timestamp = mktime(&tm);

	return timestamp;
}

bool needUpdate() {

	std::string omfg = getReleaseInfo();

	log("something worked %s", omfg.c_str());

	if(omfg == "") {
		return false;
	}

	std::string githubTimestamp = "fdlkjahfjdshfljdshflkdsahflkjdsahfaf";
	

	time_t compileTime = std::stoll(COMPILETIMESTAMP);

	log("gh timestamp: %s", githubTimestamp.c_str());
	
	/*
	time_t releaseTime = toTimestamp(githubTimestamp);

	log("release time: %lld", releaseTime);
	log("compile time: %lld", compileTime);
	*/

	return false;

	//return true;
	//return releaseTime > compileTime + (60 * 15); // 15 min offset bc, caster takes a while to compile

}

void updateDLL() {

    // this time, im not doing this bs with a script

	log("trying to update");
    int res = needUpdate();
    log("needupdate res was %d", res);

}

void doUpdate() {

	// i tried doing this upon dll inject, but i think that is a very bad move
	// something about this code causes everything to crash and burn??
	// i am going to need to rewrite all my code for some reason

	static bool hasUpdated = false;
	if(hasUpdated) {
		return;
	}
	hasUpdated = true;

	updateDLL();
	
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

namespace UIManager {

	typedef struct UIPair {
		std::string name = "";
		float* f = NULL;
	} UIPair;

	std::map<float*, UIPair> UIFloatMap;
	std::map<std::string, UIPair> UIStringMap;

	std::string strip(const std::string& s) {
		std::string res = s;
		std::regex trim_re("^\\s+|\\s+$");
		res = std::regex_replace(res, trim_re, "");
		return res;
	}

	float safeStof(const std::string& s) {
		float res = -1;
		try {
			res = std::stof(s);
		} catch (...) {
			return -1;
		}
		return res;
	}

	void add(const std::string& name, float* f) {
		// blah blah blah set.
		
		if(UIFloatMap.contains(f)) {
			return;
		}

		if(UIStringMap.contains(name)) {
			return;
		}

		UIPair temp = { name, f };

		UIFloatMap.insert({f, temp});
		UIStringMap.insert({name, temp});
	}

	void add(const std::string& name, Point* p) {
		add(name + "X", &(p->x));
		add(name + "Y", &(p->y));
	}

	void reload() {

		log("reload called");

		std::ifstream inFile("UIData.txt");
		if (!inFile.is_open()) {
			log("couldnt find UIData.txt");
			return;
		}

		std::string line;
		while (std::getline(inFile, line)) {
			line = UIManager::strip(line);
			std::regex removeDupleWhitespaceRe(R"(^\s+|\s+$|\s+(?=\s))");
			line = std::regex_replace(line, removeDupleWhitespaceRe, "");
				
			std::regex re(R"(^(\S+)(?:\s?)(\S*)?$)");
			std::smatch match;
			if (std::regex_match(line, match, re)) {
				std::string name = match[1].matched ? match[1].str() : "";
				std::string value = match[2].matched ? match[2].str() : "";

				log("%s -> %s", name.c_str(), value.c_str());

				if(UIStringMap.contains(name)) {
					*(UIStringMap[name].f) = safeStof(value);
				}
			}
		}
	}

};

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

void LineDrawPrio(float x1, float y1, float x2, float y2, DWORD ARGB) {

	MeltyVert v1 = { x1, y1, ARGB };
	MeltyVert v2 = { x2, y2, ARGB };

	scaleVertex(v1);
	scaleVertex(v2);

	meltyLineData.add(v1, v2);
}

void LineDrawPrio(const Point& a, const Point& b, DWORD ARGB) {
	LineDrawPrio(a.x, a.y, b.x, b.y, ARGB);
}

void RectDrawPrio(float x, float y, float w, float h, DWORD ARGB) {

	if(shouldReverseDraws) {
		x = 640.0f - x;
		x -= w;
	}

	MeltyVert v1 = { x,     y,     ARGB };
	MeltyVert v2 = { x + w, y,     ARGB };
	MeltyVert v3 = { x,     y + h, ARGB };
	MeltyVert v4 = { x + w, y + h, ARGB };

	scaleVertex(v1);
	scaleVertex(v2);
	scaleVertex(v3);
	scaleVertex(v4);

	meltyVertData.add(v1, v2, v3);
	meltyVertData.add(v2, v3, v4);
}

void RectDrawPrio(const Rect& rect, DWORD ARGB) {
	RectDrawPrio(rect.x1, rect.y1, rect.x2 - rect.x1, rect.y2 - rect.y1, ARGB);
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

void UIDraw(Rect texRect, Rect screenRect, DWORD ARGB, bool mirror) {

	if(shouldReverseDraws) { // is this ok? or are triangles being backfaced here
		screenRect.x1 = 640.0f - screenRect.x1;
		screenRect.x2 = 640.0f - screenRect.x2;
		std::swap(screenRect.x1, screenRect.x2);
	}

	if(mirror) {
		std::swap(texRect.x1, texRect.x2);
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

void UIDraw(const Rect& texRect, const Point& p, DWORD ARGB, bool mirror) {

	// create a screen rect based on the scale of the texture being draw.

	//Rect tempRect(p, 480.0f * texRect.w(), 480.0f * tempRect.h()); // how is this legal, compiling, syntax.
	Rect tempRect(p, 480.0f * texRect.w(), 480.0f * texRect.h()); 

	UIDraw(texRect, tempRect, ARGB, mirror);

}

void UIDraw(const Rect& texRect, const Point& p, const float scale, DWORD ARGB, bool mirror) {

	Rect tempRect(p, scale * 480.0f * texRect.w(), scale * 480.0f * texRect.h()); 

	UIDraw(texRect, tempRect, ARGB, mirror);

}

void drawChevron(Point p, int i) {

	constexpr const Rect* chevronRects[] = {
		&UI::P0Chevron,
		&UI::P1Chevron,
		&UI::P2Chevron,
		&UI::P3Chevron
	};

	if(i < 0 || i > 3) {
		return;
	}

	static float chevronScale = 4.0f;
	//UIManager::add("chevScale", &chevronScale);
	
	p.x -= ((*chevronRects[i]).w() * (UI::size / 2.0f));
	p.y -= ((*chevronRects[i]).h() * UI::size * (chevronScale / 4.0));

	bool shouldReverseDrawsBackup = shouldReverseDraws;
	shouldReverseDraws = false;
	UIDraw(*chevronRects[i], p, chevronScale, 0xFFFFFFFF);
	shouldReverseDraws = shouldReverseDrawsBackup;
}

void drawChevronOnPlayer(int p, int c) {

	// p is the player, c is the chevron 

	int xPos = *(int*)(0x00555130 + 0x108 + (p * 0xAFC));
	int yPos = *(int*)(0x00555130 + 0x10C + (p * 0xAFC));

	float windowWidth = *(uint32_t*)0x0054d048;
	float windowHeight = *(uint32_t*)0x0054d04c;

	int cameraX = *(int*)(0x0055dec4);
	int cameraY = *(int*)(0x0055dec8);
	float cameraZoom = *(float*)(0x0054eb70);

	float xCamTemp = ((((float)(xPos - cameraX) * cameraZoom) / 128.0f) * (windowWidth / 640.0f) + windowWidth / 2.0f);	
	float yCamTemp = ((((float)(yPos - cameraY) * cameraZoom) / 128.0f - 49.0f) * (windowHeight / 480.0f) + windowHeight);
	
	xCamTemp = floor(xCamTemp);
	yCamTemp = floor(yCamTemp);

	float x1Cord, x2Cord, y1Cord, y2Cord;

	x1Cord = ((float)xCamTemp - (windowWidth / 640.0f) * cameraZoom * 5.0f);
	x2Cord = ((windowWidth / 640.0f) * cameraZoom * 5.0f + (float)xCamTemp);
	y1Cord = ((float)yCamTemp - (windowWidth / 640.0f) * cameraZoom * 5.0f);
	y2Cord = yCamTemp;

	x1Cord = ((float)x1Cord * (640.0f / windowWidth));
	x2Cord = ((float)x2Cord * (640.0f / windowWidth));
	y1Cord = ((float)y1Cord * (480.0f / windowHeight));
	y2Cord = ((float)y2Cord * (480.0f / windowHeight));

	Point point( (x1Cord + x2Cord) * 0.5, (y1Cord + y2Cord) * 0.5 );

	// i really,,,, i would/maybe should go through all hurtboxes, find the max height,, ugh. doing this for now
	point.y -= 175;

	drawChevron(point, c);

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
typedef struct Bar {
	Smooth<float> yellowHealth = Smooth<float>(1.0f, healthBarSmoothValue);
	Smooth<float> redHealth = Smooth<float>(1.0f, healthBarSmoothValue);
	Smooth<float> meter = Smooth<float>(1.0f, healthBarSmoothValue);
	Smooth<float> guardQuantity = Smooth<float>(1.0f, healthBarSmoothValue); 
	//Smooth<float> guardQuality = Smooth<float>(1.0f, healthBarSmoothValue); // is this needed?
} Bar;

constexpr int displayInts[4] = {1, 3, 2, 4};

void drawHealthBar(int i, Bar& bar) {
	bool hasYOffset = false;

	shouldReverseDraws = (i & 1);
	hasYOffset = (i >= 2);

	// i,,, ugh. was originally reading health as a dword,,, but it goes negative sometimes?
	bar.yellowHealth = MAX( ((float)*(int*)(0x005551EC + (i * 0xAFC))) / 11400.0f, 0.0f);
	bar.redHealth =    MAX( ((float)*(int*)(0x005551F0 + (i * 0xAFC))) / 11400.0f, 0.0f);

	constexpr Rect yellowBars[2] = { UI::P0HealthShade, UI::P1HealthShade };
	constexpr Rect redBars[2] = { UI::P0RedHealthShade, UI::P1RedHealthShade };

	static Point base(90, 8);
	static Point offset(90, 25);

	static float height = 12.0f;

	static float maxHealthWidth = 186;

	//UIManager::add("base", &base);
	//UIManager::add("off", &offset);
	//UIManager::add("height", &height);
	//UIManager::add("max", &maxHealthWidth);

	float tempMaxHealthWidth = maxHealthWidth;
	Point p = hasYOffset ? base : offset;
	if(shouldReverseDraws) {
		// unsure why this is needed
		p.x += 1; 
		p.y -= 1; 
		//tempMaxHealthWidth += 1;
	}

	Rect drawRect;
	drawRect.p1 = p;
	drawRect.p2 = p;
	drawRect.p2.x += tempMaxHealthWidth;
	drawRect.p2.y += height;

	drawRect.p1.x = drawRect.p2.x - (tempMaxHealthWidth * (*bar.redHealth));
	UIDraw(redBars[shouldReverseDraws], drawRect, 0xFFFFFFFF);

	drawRect.p1.x = drawRect.p2.x - (tempMaxHealthWidth * (*bar.yellowHealth));
	UIDraw(yellowBars[shouldReverseDraws], drawRect, 0xFFFFFFFF);

}

void drawMeterBar(int i, Bar& bar) {

	bool hasYOffset = false;

	shouldReverseDraws = (i & 1);
	hasYOffset = (i >= 2);

	float meterDivisor = *(DWORD*)(0x0055513C + (i * 0xAFC)) == 2 ? 20000.0f : 30000.0f;
	DWORD meterRaw = *(DWORD*)(0x00555210 + (i * 0xAFC));
	float meterVal = ((float)*(DWORD*)(0x00555210 + (i * 0xAFC))) / meterDivisor;
	bar.meter = meterVal;

	float currentMeterWidth = 0.0f;
	DWORD meterCol = 0xFF000000;

	std::string meterString = "";

	DWORD circuitState = *(WORD*)(0x00555218 + (i * 0xAFC));
	DWORD heatTime = *(DWORD*)(0x00555214 + (i * 0xAFC));
	DWORD moon = *(DWORD*)(0x0055513C + (i * 0xAFC));
	DWORD circuitBreakTimer = 0;

	#define DARKEN(b, d) ( (BYTE)(((float)(b)) / (d)) )
	#define DARKENCOLOR(c, d) ( \
		(c & 0xFF000000) | \
		( DARKEN( ((c & 0x00FF0000) >> 16), d ) << 16 ) | \
		( DARKEN( ((c & 0x0000FF00) >> 8), d ) << 8 ) | \
		( DARKEN( ((c & 0x000000FF) >> 0), d ) << 0 )   \
	)

	// there are contrast issues with these vs the white border line spliting 100/200/300/100
	const DWORD meterRedCol =    0xFFFF0000;
	const DWORD meterYellowCol = 0xFFFFFF00; // it might just be my eye issues, but is this yellow like,, is it horrid to see the white meter lines?
	const DWORD meterGreenCol =  0xFF00FF00;
	const DWORD meterHeatCol =   0xFF0000FF;
	const DWORD meterMaxCol =    0xFFFFA500;
	const DWORD meterBloodCol =  0xFFDDDDDD;
	const DWORD meterBreakCol =  0xFF800080;

	if(*(WORD*)(0x00555234 + (i * 0xAFC)) == 110) { // means this its a ex penalty meter debuff
		circuitBreakTimer = 0;
	} else {
		circuitBreakTimer = *(WORD*)(0x00555230 + (i * 0xAFC));
	}

	bool useNormalMeter = false;

	if(circuitBreakTimer == 0) {
		switch(circuitState) {
			case 0:
				useNormalMeter = true;
				if(moon == 2) { // half
					//currentMeterWidth = ((float)meterRaw) / 20000.0f;
					meterCol = (meterRaw >= 15000) ? meterGreenCol : ((meterRaw >= 10000) ? meterYellowCol : meterRedCol);
				} else { // full/crescent
					//currentMeterWidth = ((float)meterRaw) / 30000.0f;
					meterCol = (meterRaw >= 20000) ? meterGreenCol : ((meterRaw>= 10000) ? meterYellowCol : meterRedCol);
				}
				meterString = std::to_string(meterRaw / 100) + "." + std::to_string((meterRaw / 10) % 10) + "%";
				break;
			case 1:
				currentMeterWidth = ((float)heatTime) / 600.0f;
				meterCol = meterHeatCol;
				meterString = "HEAT";
				break;
			case 2:
				currentMeterWidth = ((float)heatTime) / 600.0f;
				meterCol = meterMaxCol;
				meterString = "MAX";
				break;
			case 3:
				currentMeterWidth = ((float)heatTime) / 600.0f;
				meterCol = meterBloodCol;
				meterString = "BLOOD HEAT"; 
				break;
			default:
				break;
		}
	} else {
		currentMeterWidth = ((float)circuitBreakTimer) / 600.0f;
		meterCol = meterBreakCol;
		meterString = "BREAK";
	}
	
	static float meterWidth = 194;
	static float meterSize = 10.0f;
	static Point pBase(32.0f, 436);
	static Point pOffset(32.0f, 449);

	//UIManager::add("meterSize", &meterSize);
	//UIManager::add("meterX", &pBase.x);
	//UIManager::add("meterY", &pBase.y);	
	//UIManager::add("meterXOffset", &pOffset.x);
	//UIManager::add("meterYOffset", &pOffset.y);
	//UIManager::add("meterMax", &meterWidth);

	Point p = hasYOffset ? pBase : pOffset;
	Point textPoint = p;

	if(shouldReverseDraws) {
		p.x += 3.0f;
		textPoint.x += 7.0f;
	}

	float displayWidth = *bar.meter;
	if(!useNormalMeter) {
		displayWidth = currentMeterWidth;
	}
	displayWidth = MIN(1.0f, displayWidth);
	displayWidth *= meterWidth;

	Rect drawRect;
	drawRect.p1 = p;
	drawRect.p2 = p;
	drawRect.p2.y += meterSize;
	
	drawRect.p2.x += displayWidth;	

	UIDraw(UI::CircuitShade, drawRect, meterCol);

	if(moon == 2) {
		float midX = drawRect.x1 + (meterWidth * 0.5f);
		//LineDrawPrio(midX, drawRect.y1, midX, drawRect.y2, 0xFFFFFFFF);
		RectDrawPrio(midX - 1, drawRect.y1, 2, meterSize, 0xFFFFFFFF);
	} else {	
		float onethird = drawRect.x1 + (meterWidth * 0.333f);
		float twothird = drawRect.x1 + (meterWidth * 0.666f);
		//LineDrawPrio(onethird, drawRect.y1, onethird, drawRect.y2, 0xFFFFFFFF);
		//LineDrawPrio(twothird, drawRect.y1, twothird, drawRect.y2, 0xFFFFFFFF);
		RectDrawPrio(onethird - 1, drawRect.y1, 2, meterSize, 0xFFFFFFFF);
		RectDrawPrio(twothird - 1, drawRect.y1, 2, meterSize, 0xFFFFFFFF);
	}

	TextDraw(textPoint, meterSize, 0xFFFFFFFF, meterString.c_str());

}

void drawGuardBar(int i, Bar& bar) {

	// i should have used my debuginfo bs
	DWORD moon = *(DWORD*)(0x0055513C + (i * 0xAFC));
	float quantity = *(float*)(0x00555130 + 0xC4 + (i * 0xAFC)); // dont hmoons have a different max? f: 7000 h: 10500 c: 8000
	float state = *(float*)(0x00555130 + 0xCC + (i * 0xAFC)); // 1 when,,, "normal", 2 when broken. when broken, quantity is still accurate
	float quality = *(float*)(0x00555130 + 0xD8 + (i * 0xAFC)); // float from 0-2, where 0 is blue, red is 2. the gradient is a bit weird

	DWORD maxQuantity = (moon == 2) ? 10500 : ((moon == 1) ? 7000 : 8000);
	bar.guardQuantity = ((float)quantity) / ((float)maxQuantity);

	static Point base(164, 41);
	static Point offset(164, 51);

	static Point reverseOffset(3, -1);
	
	static float barWidth = 118;
	static float barHeight = 13;

	//UIManager::add("baseGuard", &base);
	//UIManager::add("offsetGuard", &offset);
	//UIManager::add("guardWidth", &barWidth);
	//UIManager::add("guardHeight", &barHeight);
	//UIManager::add("guardReverse", &reverseOffset);
	
	float drawQuantity = (*bar.guardQuantity);

	/*
	static float temp = 0.0f;
	temp += 0.001f;
	if(temp > 1.0f) {
		temp = 0.0f;
	}
	drawQuantity = temp;
	*/

	// are these bars named right
	constexpr const Rect* blueGuardBars[4] = {
		&UI::P2GuardShadeBlue,
		&UI::P3GuardShadeBlue,
		&UI::P0GuardShadeBlue,
		&UI::P1GuardShadeBlue
	};

	constexpr const Rect* whiteGuardBars[4] = {
		&UI::P2GuardShadeWhite,
		&UI::P3GuardShadeWhite,
		&UI::P0GuardShadeWhite,
		&UI::P1GuardShadeWhite
	};

	constexpr const Rect* breakGuardBars[4] = {
		&UI::P2BrokenGuard,
		&UI::P3BrokenGuard,
		&UI::P0BrokenGuard,
		&UI::P1BrokenGuard
	};

	Rect blueGuard = *(blueGuardBars[i]);
	Rect whiteGuard = *(whiteGuardBars[i]);
	Rect breakGuard = *(breakGuardBars[i]);

	shouldReverseDraws = (i & 1); // this is offseting textures in a bad way. i would fix it but i have already accounted for it in the other draws
	bool hasYOffset = (i >= 2);

	Point p = hasYOffset ? base : offset;

	if(shouldReverseDraws) {
		p += reverseOffset;
	}

	Rect drawRect;
	drawRect.p1 = p;
	drawRect.p2 = p;
	drawRect.y2 += barHeight;

	if(!hasYOffset) {
		drawRect.x1 += barWidth - (drawQuantity * barWidth);
		drawRect.x2 += barWidth;
		if(shouldReverseDraws) {
			whiteGuard.x2 = whiteGuard.x1 + (whiteGuard.w() * drawQuantity);
			blueGuard.x2 = blueGuard.x1 + (blueGuard.w() * drawQuantity);
		} else {
			whiteGuard.x1 = whiteGuard.x2 - (whiteGuard.w() * drawQuantity);
			blueGuard.x1 = blueGuard.x2 - (blueGuard.w() * drawQuantity);
		}
	} else {
		drawRect.x2 += (drawQuantity * barWidth);
		if(shouldReverseDraws) {
			whiteGuard.x1 = whiteGuard.x2 - (whiteGuard.w() * drawQuantity);
			blueGuard.x1 = blueGuard.x2 - (blueGuard.w() * drawQuantity);
		} else {
			whiteGuard.x2 = whiteGuard.x1 + (whiteGuard.w() * drawQuantity);
			blueGuard.x2 = blueGuard.x1 + (blueGuard.w() * drawQuantity);
		}
	}

	DWORD barCol = 0xFF000000;
	
	if(state != 2) { // not broken

		if(quality > 1.0f) { // red
			// BYTE red = quality < 1.0f ? 0x00 : ((quality - 1.0f) * ((float)0xFF));
		} else { // blue
			// BYTE blue = quality > 1.0f ? 0x00 : ((1.0f - quality) * ((float)0xFF));
		}

		
		
	} else {
		Rect breakDrawRect;
		breakDrawRect.p1 = p;
		breakDrawRect.p2 = p;
		breakDrawRect.x2 += barWidth;
		breakDrawRect.y2 += barHeight;
		barCol = 0xFF808080;
		UIDraw(breakGuard, breakDrawRect, 0x80808080);
	}

	UIDraw(whiteGuard, drawRect, barCol);
	UIDraw(blueGuard, drawRect, (0x00FFFFFF & barCol) | 0x80000000);

}

std::optional<Rect> getCharProfile(int id) {

	/*
	0: Sion
	1: Arc
	2: Ciel
	3: Akiha
		4: Maids
	5: Hisui
	6: Kohaku
	7: Tohno
	8: Miyako
	9: Wara
	10: Nero
	11: V. Sion
	12: Warc
	13: V. Akiha
	14: Mech
	15: Nanaya
	17: Satsuki
	18: Len
	19: P. Ciel
	20: Neco
	22: Aoko
	23: W. Len
	25: NAC
	28: Kouma
	29: Sei
	30: Ries
	31: Roa
	33: Ryougi
		34: NecoMech
		35: KohaMech
	51: Hime
	*/

	int lookup = -1;


	switch (id) {
	case 0:
	case 1:
	case 2:
	case 3:
		lookup = id;
		break;
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
		lookup = id - 1;
		break;
	case 17:
	case 18:
	case 19:
	case 20:
		lookup = id - 2;
		break;
	case 22:
	case 23:
		lookup = id - 3; 
		break;
	case 25:
		lookup = id - 4;
		break;
	case 28:
	case 29:
	case 30:
	case 31:
		lookup = id - 6;
		break;
	case 33:
		lookup = id - 7;
		break;
	case 51:
		lookup = 27; 
	case 4: // maids
	case 34: // NecoMech
	case 35: // KohaMech
	default:
		break;
	}

	// im not sure if this array is correct.
	constexpr const Rect* CSSChars[] = {
		&UI::CSSion, 
		&UI::CSArc,  
		&UI::CSCiel, 
		&UI::CSAki,  
		&UI::CSHisui,
		&UI::CSKoha, 
		&UI::CSTohno,
		&UI::CSMiya, 
		&UI::CSWara, 
		&UI::CSNero, 
		&UI::CSVSion,
		&UI::CSWarc, 
		&UI::CSVaki, 
		&UI::CSMech, 
		&UI::CSNanaya,
		&UI::CSSats, 
		&UI::CSLen,  
		&UI::CSPCiel,
		&UI::CSNeco, 
		&UI::CSAoko, 
		&UI::CSWLen, 
		&UI::CSNac,  
		&UI::CSKouma,
		&UI::CSSei,  
		&UI::CSRies, 
		&UI::CSRoa,  
		&UI::CSRyougi, // i need hime 
		&UI::CSHime
	};

	
	if(lookup < 0 || lookup >= (sizeof(CSSChars) / sizeof(CSSChars[0])))  {
		return std::optional<Rect>();
	}
	
	return std::optional<Rect>(*CSSChars[lookup]);
}

std::optional<Rect> getMoonTex(int id) {

	constexpr const Rect* moonRects[] = {
		&UI::MoonCrescent,
		&UI::MoonFull,
		&UI::MoonHalf,
		&UI::MoonEclipse
	};

	id = CLAMP(id, 0, 3);

	return std::optional<Rect>(*moonRects[id]);
}

void drawFacing(int i, Bar& bar) {
	shouldReverseDraws = (i & 1);
	bool hasYOffset = (i >= 2);

	static int displayCooldown[4] = {0, 0, 0, 0};
	static DWORD prevTurnAroundState[4] = {0, 0, 1, 1};

	if((displayCooldown[i] == 0) && (AsmHacks::naked_charTurnAroundState[i] == prevTurnAroundState[i])) {
		return;
	}

	if(AsmHacks::naked_charTurnAroundState[i] != prevTurnAroundState[i]) {
		displayCooldown[i] = 60;
	}
	prevTurnAroundState[i] = AsmHacks::naked_charTurnAroundState[i];
	
	// this is going to handle,,, portraits, lock on bs,,, moons, palettes once im less tired.
	DWORD facing = AsmHacks::naked_charTurnAroundState[i];
	facing <<= 1;
	if((i & 1) == 0) {
		facing++;
	}

	if(displayCooldown[i] > 15 || (displayCooldown[i] & 1)) {
		int displayVal = displayInts[i] - 1;
		drawChevronOnPlayer(facing, displayVal);
	}
	
	if(displayCooldown[i] > 0) {
		displayCooldown[i]--;
	}
}

void drawPlayerInfo() {

	BYTE charIDs[4];
	BYTE moons[4];
	BYTE palettes[4];

	for(int i=0; i<4; i++) {
		charIDs[i] = *(BYTE*)(0x00555130 + 0x5 + (i * 0xAFC));
		moons[i] = *(BYTE*)(0x00555130 + 0xC + (i * 0xAFC));
		palettes[i] = *(BYTE*)(0x00555130 + 0xA + (i * 0xAFC));
	}

	static Point base(-30, -5);
	static Point offset(5, 30);
	static float scale = 1.5f;
	//UIManager::add("baseX", &base.x);
	//UIManager::add("baseY", &base.y);
	//UIManager::add("offX", &offset.x);
	//UIManager::add("offY", &offset.y);
	//UIManager::add("portraitScale", &scale);
	static Point moonOffset(40, 30);
	static float moonScale = 2.0;
	//UIManager::add("moonOff", &moonOffset);
	//UIManager::add("moonScale", &moonScale);
	static Point paletteOffset(40, 48);
	static float palleteSize = 8.0f;
	//UIManager::add("paletteOff", &paletteOffset);
	//UIManager::add("paletteSize", &palleteSize);
	static Point lockOffset(75, 20);
	static Point lockOffset2(0, 15);
	//UIManager::add("lock", &lockOffset);
	//UIManager::add("lock2", &lockOffset2);

	for(int i=0; i<4; i++) {

		shouldReverseDraws = (i & 1);
		bool hasYOffset = (i >= 2);

		Point p = hasYOffset ? base : offset;
		
		std::optional<Rect> charRect = getCharProfile(charIDs[i]);
		if(charRect.has_value()) {
			UIDraw(charRect.value(), p, scale, 0xFFFFFFFF, shouldReverseDraws);
		}
	}

	for(int i=0; i<4; i++) {

		shouldReverseDraws = (i & 1);
		bool hasYOffset = (i >= 2);

		Point p = hasYOffset ? base : offset;

		std::optional<Rect> moonRect = getMoonTex(moons[i]);
		if(moonRect.has_value()) {
			UIDraw(moonRect.value(), p + moonOffset, moonScale, 0xFFFFFFFF, shouldReverseDraws);
		}

		TextDraw(p + paletteOffset, palleteSize, 0xFFFFFFFF, "%02d", palettes[i] + 1);

	}

	for(int i=0; i<4; i++) {

		shouldReverseDraws = (i & 1);
		bool hasYOffset = (i >= 2);

		Point p = hasYOffset ? base : offset;

		/*
		
		0: 1,3
		1: 0,2
		2: 1,3
		3: 0,2

		
		*/

		DWORD facing = AsmHacks::naked_charTurnAroundState[i];
		facing <<= 1;
		if((i & 1) == 0) {
			facing++;
		}
		
		const char* charString = getCharName(charIDs[facing]);
		int palNum = palettes[facing] + 1;

		Point lockOnPoint = lockOffset;
		if(!hasYOffset) {
			lockOnPoint += lockOffset2;
		}

		TextDraw(p + lockOnPoint - Point(0, palleteSize), palleteSize, 0xFFFFFFFF, "Lock");
		TextDraw(p + lockOnPoint, palleteSize, 0xFFFFFFFF, "%s", charString);
		TextDraw(p + lockOnPoint + Point(0, palleteSize), palleteSize, 0xFFFFFFFF, "%02d", palNum);

	}
}

void drawNewUI() {
	
	shouldReverseDraws = false;

    for(int i=0; i<4; i++) {
        if(*(BYTE*)(0x00555130 + 0x1B6 + (i * 0xAFC)) != 0) { // is knocked down
            *(BYTE*)(0x005552A8 + (i * 0xAFC)) = 0x01; // sets isBackground flag
        }
        // doing this write here is dumb. p3p4 moon stuff isnt inited properly, i want to go to sleep
        *(DWORD*)(0x00555130 + 0xC + (i * 0xAFC)) = *(DWORD*)(0x0074d840 + 0xC + (i * 0x2C));
    }

	/*
	if(*((uint8_t*)0x0054EEE8) == 0x01 && DllOverlayUi::isDisabled()) { // check if ingame
	//if(true) {

	} else {
		return;
	}

	*/

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

	bool deadState[4];
	for(int i=0; i<4; i++) {
		deadState[i] = (*(BYTE*)(0x00555130 + 0x1B6 + (i * 0xAFC)) != 0);
	}

	for(int i=0; i<4; i++) {
		if(i & 1) { // team 2
			// todo, have it not switch looking at when both die. this needs to be tested!!!!!!!!
			if(deadState[0] && !deadState[2]) {
				AsmHacks::naked_charTurnAroundState[i] = 1;
			} else if(deadState[2] && !deadState[0]) {
				AsmHacks::naked_charTurnAroundState[i] = 0;
			}
		} else { // team 1
			if(deadState[1] && !deadState[3]) { 
				AsmHacks::naked_charTurnAroundState[i] = 1;
			} else if(deadState[3] && !deadState[1]) {
				AsmHacks::naked_charTurnAroundState[i] = 0;
			}
		}
	}

	for(int i=0; i<4; i++) {

		DWORD target = AsmHacks::naked_charTurnAroundState[i];
		target <<= 1;
		if((i & 1) == 0) {
			target++;
		}

		// i wonder if this could have solved all port problems earlier. im not even going to test to see though
		// todo, what is this number by default and does it matter
		// todo, does this have horrendous consequences
		
		*(DWORD*)(0x00555130 + 0x2C0 + (i * 0xAFC)) = 0x00555130 + 0x4 + (target * 0xAFC);
	}

	shouldReverseDraws = false;

	static Bar bars[4];

	constexpr Rect topBars[2] = { UI::P0TopBar, UI::P1TopBar };
	constexpr Rect meterBars[2] = { UI::P0BottomBar, UI::P1BottomBar };

	static Point topBarsPoint(0.0, 0.0);
	static float topBarsScale = 4.25f;
	//UIManager::add("topBarsX", &topBarsPoint.x);
	//UIManager::add("topBarsY", &topBarsPoint.y);
	//UIManager::add("topBarsScale", &topBarsScale);

	static float meterScale = 4.25f;
	//UIManager::add("meterScale", &meterScale);

	static Point winsPoint(190, 78);
	//UIManager::add("wins", &winsPoint);

	if(*(BYTE*)(0x0054EEE8) == 0x01 && DllOverlayUi::isDisabled()) {
	//if(true) {

		for(int i=0; i<2; i++) {
			shouldReverseDraws = (i == 1); 

			UIDraw(topBars[i], topBarsPoint, topBarsScale, 0xFFFFFFFF);

			UIDraw(meterBars[i], Point(0, 480 - (UI::size * meterBars[i].h())), meterScale, 0xFFFFFFFF);

			int winCount = *(int*)(0x0077497c + (i * 4));
			TextDraw(winsPoint, 8, 0xFFFFFFFF, "%d %s", winCount, (winCount != 1) ? "WINS" : "WIN");
		}

		// above are texture draws. below is the bar draws

		#define LOOP4(func) for(int i=0; i<4; i++) { func(i, bars[i]); }

		drawPlayerInfo();
		LOOP4(drawMeterBar);
		LOOP4(drawHealthBar);
		LOOP4(drawGuardBar);
		LOOP4(drawFacing);
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
	
	if(SHIFTHELD && UPPRESS) {
		UIManager::reload();
	}

	doUpdate();
	drawNewUI();

	//UIDraw(Rect(Point(0, 0), 1, 1), Rect(Point(100, 100), 100, 100), 0xFFFFFFFF);

	// -- ACTUAL RENDERING --

	backupRenderState();

	_drawGeneralCalls();

	restoreRenderState();

	// -- END ACTUAL RENDERING --
	
}

