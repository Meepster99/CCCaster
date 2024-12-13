
#include <set>

#include "DllDirectX.hpp"

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

VertexData<PosColVert, 3 * 2048> posColVertData(D3DFVF_XYZ | D3DFVF_DIFFUSE);
VertexData<PosTexVert, 3 * 2048> posTexVertData(D3DFVF_XYZ | D3DFVF_TEX1, &fontTexture);
// need to rework font rendering, 4096 is just horrid
//VertexData<PosColTexVert, 3 * 4096 * 2> posColTexVertData(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, &fontTextureMelty);
VertexData<PosColTexVert, 3 * 4096 * 16> posColTexVertData(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, &fontTextureMelty);

VertexData<MeltyVert, 3 * 4096 * 2> meltyVertData(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, &fontTextureMelty);
VertexData<MeltyVert, 2 * 16384, D3DPT_LINELIST> meltyLineData(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, &fontTextureMelty); // 8192 is overkill

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

	/*
	HMODULE hModule = GetModuleHandle(TEXT("Extended-Training-Mode-DLL.dll"));

	HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(id), L"PNG");
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
	*/

	return true;
}

void _initDefaultFont(IDirect3DTexture9*& resTexture) {

	HRESULT hr;

	BYTE* pngBuffer = NULL;
	unsigned pngSize = 0;
	//bool res = loadResource(IDB_PNG2, pngBuffer, pngSize);
	bool res = false;

	hr = D3DXCreateTextureFromFileInMemory(device, pngBuffer, pngSize, &resTexture);
	if (FAILED(hr)) {
		log("font createtexfromfileinmem failed??");
		resTexture = NULL;
		return;
	}

}

IDirect3DPixelShader9* getFontOutlinePixelShader() {
	return createPixelShader(R"(
			sampler2D textureSampler : register(s0);
			float4 texSize : register(c219);

			float4 main(float2 texCoordIn : TEXCOORD0) : COLOR {
									
					float2 texOffset = 4.0 / texSize;
	
					//texOffset.y /= (4.0 / 3.0);

					float2 texCoord = texCoordIn + (texOffset * 0.5);

					float4 texColor = tex2D(textureSampler, texCoord);
				
					// this outline needs to be a bit different, since im trying to have it be outside the bounds of the drawn color.

					if(texColor.a > 0.0) {
						return float4(texColor.rgb, 1.0);
					}
	
					float2 offsets[8] = { // order is adjusted to check diags last. performance.
						
						texCoord + float2(-texOffset.x, 0.0),
						texCoord + float2(0.0, -texOffset.y),
						texCoord + float2(0.0, texOffset.y),
						texCoord + float2(texOffset.x, 0.0),						

						texCoord + float2(texOffset.x, -texOffset.y),
						texCoord + float2(-texOffset.x, -texOffset.y),
						texCoord + float2(-texOffset.x, texOffset.y),
						texCoord + float2(texOffset.x, texOffset.y)
					};


					float4 tempColor;

					[unroll(8)] for(int i=0; i<8; i++) {

						tempColor = tex2D(textureSampler, offsets[i]);
						if(tempColor.a > 0) {
							return float4(0.0, 0.0, 0.0, 1.0);
						}

					}
			
					return float4(0.0, 0.0, 0.0, 0.0);

			}

	)");
}

IDirect3DVertexShader9* getFontOutlineVertexShader() {
	return createVertexShader(R"(
			struct VS_INPUT {
				float4 position : POSITION;
				float2 texCoord : TEXCOORD0;
			};
    
			struct VS_OUTPUT {
				float4 position : POSITION;
				float2 texCoord : TEXCOORD0;
			};
    
			VS_OUTPUT main(VS_INPUT input) {
				VS_OUTPUT output;
				output.position = input.position;
				output.texCoord = input.texCoord;
				return output;
			}

	)");
}

void _initDefaultFontOutline(IDirect3DTexture9*& fontTex) {

	IDirect3DSurface9* surface = NULL;
	ID3DXBuffer* buffer = NULL;
	IDirect3DTexture9* texture = NULL;

	HRESULT hr;

	hr = device->CreateTexture(fontTexWidth, fontTexHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
	if (FAILED(hr)) {
		log("font createtexture failed");
		return;
	}

	IDirect3DSurface9* oldRenderTarget = nullptr;
	hr = device->GetRenderTarget(0, &oldRenderTarget);
	if (FAILED(hr)) {
		log("font getrendertarget failed");
		return;
	}

	hr = texture->GetSurfaceLevel(0, &surface);
	if (FAILED(hr)) {
		log("font getsurfacelevel failed");
		return;
	}

	hr = device->SetRenderTarget(0, surface);
	if (FAILED(hr)) {
		log("font setrendertarget failed");
		return;
	}

	IDirect3DPixelShader9* textOutlinePixelShader = getFontOutlinePixelShader();
	IDirect3DVertexShader9* textOutlineVertexShader = getFontOutlineVertexShader();


	device->SetPixelShader(textOutlinePixelShader);
	device->SetVertexShader(textOutlineVertexShader);

	D3DXVECTOR4 textureSize((float)fontTexWidth, (float)fontTexHeight, 0.0, 0.0);
	device->SetPixelShaderConstantF(219, (float*)&textureSize, 1);

	device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);



	VertexData<PosTexVert, 3 * 2> tempVertData(D3DFVF_XYZ | D3DFVF_TEX1, &fontTex); // the deconstructor should handle this.	
	tempVertData.alloc();

	PosTexVert v1 = { D3DVECTOR(-1.0f, 1.0f, 0.5f), D3DXVECTOR2(0.0f, 0.0f) };
	PosTexVert v2 = { D3DVECTOR(1.0f, 1.0f, 0.5f), D3DXVECTOR2(1.0f, 0.0f) };
	PosTexVert v3 = { D3DVECTOR(-1.0f, -1.0f, 0.5f), D3DXVECTOR2(0.0f, 1.0f) };
	PosTexVert v4 = { D3DVECTOR(1.0f, -1.0f, 0.5f), D3DXVECTOR2(1.0f, 1.0f) };

	tempVertData.add(v1, v2, v3);
	tempVertData.add(v2, v3, v4);

	device->BeginScene();

	tempVertData.draw();

	device->EndScene();

	//hr = D3DXSaveTextureToFileA("fontTest.png", D3DXIFF_PNG, texture, NULL);
	hr = D3DXSaveTextureToFileInMemory(&buffer, D3DXIFF_PNG, texture, NULL);

	BYTE* bufferPtr = (BYTE*)buffer->GetBufferPointer();
	size_t bufferSize = buffer->GetBufferSize();

	fontBufferWithOutline = (BYTE*)malloc(bufferSize);
	if (fontBufferWithOutline == NULL) {
		log("font malloc failed, what are you doing");
		return;
	}

	memcpy(fontBufferWithOutline, bufferPtr, bufferSize);
	fontBufferSizeWithOutline = bufferSize;

	device->SetRenderTarget(0, oldRenderTarget);

	device->SetPixelShader(NULL);
	device->SetVertexShader(NULL);

	oldRenderTarget->Release();
	surface->Release();
	buffer->Release();
	texture->Release();

	textOutlinePixelShader->Release();
	textOutlineVertexShader->Release();
}

void _initMeltyFont() {

	IDirect3DSurface9* surface = NULL;
	ID3DXBuffer* buffer = NULL;
	IDirect3DTexture9* texture = NULL;

	HRESULT hr;

	BYTE* pngBuffer = NULL;
	unsigned pngSize = 0;
	//bool res = loadResource(IDB_PNG1, pngBuffer, pngSize);
	bool res = false;

	IDirect3DTexture9* meltyTex = NULL;

	hr = D3DXCreateTextureFromFileInMemory(device, pngBuffer, pngSize, &meltyTex);
	if (FAILED(hr)) {
		log("font createtexfromfileinmem failed??");
		return;
	}

	if (!res) {
		log("failed to load melty font resource");
		return;
	}

	hr = device->CreateTexture(fontTexWidth, fontTexHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
	if (FAILED(hr)) {
		log("font createtexture failed");
		return;
	}

	IDirect3DSurface9* oldRenderTarget = nullptr;
	hr = device->GetRenderTarget(0, &oldRenderTarget);
	if (FAILED(hr)) {
		log("font getrendertarget failed");
		return;
	}

	hr = texture->GetSurfaceLevel(0, &surface);
	if (FAILED(hr)) {
		log("font getsurfacelevel failed");
		return;
	}

	hr = device->SetRenderTarget(0, surface);
	if (FAILED(hr)) {
		log("font setrendertarget failed");
		return;
	}

	log("creating font outline pixel shader");
	IDirect3DPixelShader9* textOutlinePixelShader = getFontOutlinePixelShader();
	log("creating font outline vertex shader");
	IDirect3DVertexShader9* textOutlineVertexShader = getFontOutlineVertexShader();

	//device->SetPixelShader(textOutlinePixelShader);
	//device->SetVertexShader(textOutlineVertexShader);

	D3DXVECTOR4 textureSize((float)fontTexWidth, (float)fontTexHeight, 0.0, 0.0);
	device->SetPixelShaderConstantF(219, (float*)&textureSize, 1);

	device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
	
	VertexData<PosTexVert, 3 * 2> tempVertData(D3DFVF_XYZ | D3DFVF_TEX1, &meltyTex); // the deconstructor should handle this.	
	tempVertData.alloc();

	PosTexVert v1 = { D3DVECTOR(-1.0f, 1.0f, 0.5f), D3DXVECTOR2(0.0f, 0.0f) };
	PosTexVert v2 = { D3DVECTOR(1.0f, 1.0f, 0.5f), D3DXVECTOR2(1.0f, 0.0f) };
	PosTexVert v3 = { D3DVECTOR(-1.0f, -1.0f, 0.5f), D3DXVECTOR2(0.0f, 1.0f) };
	PosTexVert v4 = { D3DVECTOR(1.0f, -1.0f, 0.5f), D3DXVECTOR2(1.0f, 1.0f) };

	tempVertData.add(v1, v2, v3);
	tempVertData.add(v2, v3, v4);

	device->BeginScene();

	tempVertData.draw();

	device->EndScene();

	//hr = D3DXSaveTextureToFileA("meltyFontTest.png", D3DXIFF_PNG, texture, NULL);
	hr = D3DXSaveTextureToFileInMemory(&buffer, D3DXIFF_PNG, texture, NULL);
	if (FAILED(hr)) {
		log("_initMeltyFont D3DXSaveTextureToFileInMemory failed");
		return;
	}

	BYTE* bufferPtr = (BYTE*)buffer->GetBufferPointer();
	size_t bufferSize = buffer->GetBufferSize();

	fontBufferMelty = (BYTE*)malloc(bufferSize);
	if (fontBufferMelty == NULL) {
		log("font malloc failed, what are you doing");
		return;
	}

	memcpy(fontBufferMelty, bufferPtr, bufferSize);
	fontBufferMeltySize = bufferSize;

	device->SetRenderTarget(0, oldRenderTarget);

	device->SetPixelShader(NULL);
	device->SetVertexShader(NULL);

	oldRenderTarget->Release();
	surface->Release();
	buffer->Release();
	texture->Release();

	textOutlinePixelShader->Release();
	textOutlineVertexShader->Release();

	meltyTex->Release();

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

	static bool hasRan = false;
	if (hasRan) {
		return;
	}
	hasRan = true;

	DWORD antiAliasBackup;
	device->GetRenderState(D3DRS_MULTISAMPLEANTIALIAS, &antiAliasBackup);
	device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);

	//IDirect3DTexture9* fontTex = NULL;
	_initDefaultFont(fontTexture);
	if (fontTexture == NULL) {
		return;
	}

	//_initDefaultFontOutline(fontTex);
	_initMeltyFont();

	//fontTex->Release();

	log("loaded font BUFFER!!!");
	device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, antiAliasBackup);
}

void initFont() {

	_initFontFirstLoad();

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

	hr = D3DXCreateTextureFromFileInMemory(device, fontBufferMelty, fontBufferMeltySize, &fontTextureMelty);
	if (FAILED(hr)) {
		log("melty font createtexfromfileinmem failed??");
		return;
	}

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

	x /= 480.0f;
	w /= 480.0f;
	y /= 480.0f;
	h /= 480.0f;

	y = 1 - y;

	PosColVert v1 = { ((x + 0) * 1.5f) - 1.0f, ((y + 0) * 2.0f) - 1.0f, 0.5f, ARGB };
	PosColVert v2 = { ((x + w) * 1.5f) - 1.0f, ((y + 0) * 2.0f) - 1.0f, 0.5f, ARGB };
	PosColVert v3 = { ((x + 0) * 1.5f) - 1.0f, ((y - h) * 2.0f) - 1.0f, 0.5f, ARGB };
	PosColVert v4 = { ((x + w) * 1.5f) - 1.0f, ((y - h) * 2.0f) - 1.0f, 0.5f, ARGB };

	scaleVertex(v1.position);
	scaleVertex(v2.position);
	scaleVertex(v3.position);
	scaleVertex(v4.position);

	posColVertData.add(v1, v2, v3);
	posColVertData.add(v2, v3, v4);
}

void RectDraw(const Rect& rect, DWORD ARGB) {
	RectDraw(rect.x1, rect.y1, rect.x2 - rect.x1, rect.y2 - rect.y1, ARGB);
}

void BorderDraw(float x, float y, float w, float h, DWORD ARGB) {

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

// -----

void LineDrawBlend(float x1, float y1, float x2, float y2, DWORD ARGB, bool side) {


	MeltyVert v1 = { x1, y1, ARGB };
	MeltyVert v2 = { x2, y2, ARGB };

	scaleVertex(v1);
	scaleVertex(v2);

	//meltyVertData
	meltyLineData.add(v1, v2);

	return;

	/*
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
	*/

}

void RectDrawBlend(float x, float y, float w, float h, DWORD ARGB) {

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

void BorderDrawBlend(float x, float y, float w, float h, DWORD ARGB) {

	float lineWidth = 480.0f * (1.0f / vHeight);

	h -= lineWidth;
	w -= lineWidth;

	LineDrawBlend(x + lineWidth, y, x + w, y, ARGB, true);
	LineDrawBlend(x, y, x, y + h, ARGB, false);
	LineDrawBlend(x + w, y, x + w, y + h, ARGB, false);
	LineDrawBlend(x, y + h, x + w + lineWidth, y + h, ARGB, true);
}

void BorderRectDrawBlend(float x, float y, float w, float h, DWORD ARGB) {
	RectDrawBlend(x, y, w, h, ARGB);
	BorderDrawBlend(x, y, w, h, ARGB | 0xFF000000);
}

// -----

Rect TextDraw(float x, float y, float size, DWORD ARGB, const char* format) {
	// i do hope that this allocing does not slow things down. i tried saving the va_args for when the actual print func was called, but it would not work

	if (format == NULL) {
		return Rect();
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
		case '{': // blue
			TempARGB = 0xFF8F8FFF;
			str++;
			continue;
		case '~': // red
			TempARGB = 0xFFFF8F8F;
			str++;
			continue;
		case '@': // green 
			TempARGB = 0xFF8FFF8F;
			str++;
			continue;
		case '`': // yellow
			TempARGB = 0xFFFFFF8F;
			str++;
			continue;
		case '^': // purple
			TempARGB = 0xFFFF8FFF;
			str++;
			continue;
		case '*': // black
			TempARGB = 0xFF8F8F8F;
			str++;
			continue;
		case '$': // gray
			TempARGB = 0xFF888888;
			str++;
			continue;
		case '}': // reset 
			TempARGB = ARGB;
			str++;
			continue;
		case '\\': // in case you want to print one of the above chars, you can escape them
			str++;
			if (c == '\0') {
				return res;
			}
			break;
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

void allocVertexBuffers() {

	posColVertData.alloc();
	posTexVertData.alloc();
	posColTexVertData.alloc();
	meltyVertData.alloc();
	meltyLineData.alloc();
}

void _drawGeneralCalls() {

	allocVertexBuffers();

	device->BeginScene();

	posColVertData.draw();

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
	
	// -- ACTUAL RENDERING --
	backupRenderState();

	_drawGeneralCalls();

	restoreRenderState();

	// -- END ACTUAL RENDERING --
	
}

