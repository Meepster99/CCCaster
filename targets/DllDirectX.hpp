#pragma once
#include <cstdio>
#include <type_traits>
#include <set>
#include <vector>
#include <array>
#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <fstream>
#include <map>

// i am being so so real with you
// if you are getting a bunch of errors with this file for _D3DVECTOR
// go and just add that constructor to the fuckin file
// what kind of fucked up headers is mingw giving me
// for reference, line 1400,  
// _D3DVECTOR(float _x, float _y, float _z) { x=_x; y=_y; z=_z; }
// _D3DVECTOR(const _D3DVECTOR& other) { x=other.x; y=other.y; z=other.z; }
// _D3DVECTOR() { x=0; y=0; z=0; }
// for

static IDirect3DDevice9* device = NULL;

constexpr int charIDList[] = {0,1,2,3,5,6,7,8,9,10,11,12,13,14,15,17,18,19,20,22,23,25,28,29,30,31,33,51,99};
constexpr const char* charIDNames[] = {"Sion","Arc","Ciel","Akiha","Hisui","Kohaku","Tohno","Miyako","Wara","Nero","V. Sion","Warc :3","V. Akiha","Mech","Nanaya","Satuki","Len","P. Ciel","Neco","Aoko","W. Len","NAC","Kouma","Sei","Ries","Roa","Ryougi","Hime","Random"};
constexpr int charGridList[] = { 12, 21, 11, 20, 39, 41, 3, 10, 15, 33, 14, 23, 24, 25, 5, 29, 30, 22, 40, 2, 32, 34, 6, 28, 13, 16, 31, 4, 49};

static_assert(sizeof(charGridList) / sizeof(charGridList[0]) == sizeof(charIDList) / sizeof(charIDList[0]), "chargridlist and charidlist must have the same size");
static_assert(sizeof(charIDList) / sizeof(charIDList[0]) == sizeof(charIDNames) / sizeof(charIDNames[0]), "length of name and id list must be the same");


extern bool updateOccured;

const char* getCharName(int id);

void __stdcall ___log(const char* msg);

void __stdcall log(const char* format, ...);

void __stdcall printDirectXError(HRESULT hr);

void doUpdate();

typedef struct COLOR {
	COLOR() {}
	COLOR(DWORD d) : col(d) {}
	union {
		struct {
			BYTE a = 0xFF;
			BYTE r = 0;
			BYTE g = 0;
			BYTE b = 0;
		};
		DWORD col;
	};
	operator DWORD() const { return col; } // this is super nice syntax. i wonder if i could have defined it with enums? would have saved a lot of casting back with GBAStranger
} COLOR;
 
static_assert(sizeof(COLOR) == 4, "color struct must be 4 bytes");

COLOR avgColors(COLOR col1, COLOR col2, float f);

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define CLAMP(value, min_val, max_val) MAX(MIN((value), (max_val)), (min_val))
#define SAFEMOD(a, b) (((b) + ((a) % (b))) % (b))

extern bool logPowerInfo;
extern bool logVerboseFps;
extern float hitboxOpacity;
extern bool renderingEnable;

extern int enableWaraSearch;
extern int findWara;
extern int crowdSize;
extern int maxCrowdVel;

template <typename T, int size>
class CircularBuffer {
public:

	CircularBuffer() {}

	void pushHead(const T& v) {
		index++;
		if (index >= size) {
			index = 0;
		}
		data[index] = v;
	}

	void pushTail(const T& v) {
		index--;
		if (index < 0) {
			index = size - 1;
		}
		data[index] = v;
	}

	void rollHead() {
		index--;
		if (index >= size) {
			index = 0;
		}
	}

	void rollTail() {
		index++;
		if (index < 0) {
			index = size - 1;
		}
	}

	T& front() {
		return data[index];
	}

	int totalMemory() {
		return sizeof(T) * size;
	}

	void clear() {
		for (size_t i = 0; i < size; i++) {
			data[i] = T();
		}
		index = 0;
	}

	T& operator [](int i) {
		while (i < 0) {
			i += size;
		}
		i += index;
		return data[i % size];
	}
	
	const T& operator [](int i) const {
		while (i < 0) {
			i += size;
		}
		i += index;
		return data[i % size];
	}

	T data[size];
	int index = 0; // index will be the head, index-1 will be the tail
};

template <typename T>
class Vec {
public:

	Vec(int maxSize_ = 16) {
		maxSize = maxSize_;
		if(maxSize != 0) {
			data = (T*)malloc(maxSize * sizeof(T));
		}
	}

	~Vec() {
		if (data != NULL) {
			free(data);
			data = NULL;
		}
	}

	Vec(const Vec& other) = delete;
	Vec& operator=(const Vec& other) = delete;

	int totalMemory() {
		return sizeof(T) * maxSize;
	}

	void resize() {
		maxSize *= 2;
		T* temp = (T*)realloc(data, maxSize * sizeof(T));
		
		if (temp == NULL) {
			log("vec resize failed??!");
			return;
		}

		data = temp;
	}

	void addCapacity(int n) {
		maxSize += n;

		T* temp = (T*)realloc(data, maxSize * sizeof(T));

		if (temp == NULL) {
			log("Vec realloc failed??!");
			return;
		}

		data = temp;
	}

	void push_back(const T& newItem) {

		if (size == maxSize) {
			resize();
		}

		data[size] = newItem;
		size++;
	}

	void emplace_back(const T&& newItem) {

		if (size == maxSize) {
			resize();
		}

		data[size] = std::forward<T>(newItem);
		size++;
	}
	
	T operator [](int i) const { return data[i]; }
	T& operator [](int i) { return data[i]; }

	T* data = NULL;
	int size = 0;
	int maxSize = 0;

};

void _naked_InitDirectXHooks();
void dualInputDisplay();

extern bool lClick;
extern bool mClick;
extern bool rClick;
extern bool lHeld;
extern bool mHeld;
extern bool rHeld;

extern bool iDown;
extern bool jDown;
extern bool kDown;
extern bool lDown;
extern bool mDown;

extern bool shouldReverseDraws;

// my inconsistent use of D3DXVECTOR2 vs point is bad. i should use point

struct Rect;
typedef struct Rect Rect;

typedef struct Point {
	float x = 0.0;
	float y = 0.0;
	constexpr Point() {}
	constexpr Point(float x_, float y_) : x(x_), y(y_) {}
	bool operator==(const Point& rhs) { return x == rhs.x && y == rhs.y; }
	bool operator!=(const Point& rhs) { return x != rhs.x || y != rhs.y; }
	Point operator+(const Point& rhs) { return Point(x + rhs.x, y + rhs.y); }
	Point operator-(const Point& rhs) { return Point(x - rhs.x, y - rhs.y); }
	Point& operator+=(const Point& rhs) { x += rhs.x; y += rhs.y; return *this; }
	Point& operator-=(const Point& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
	Point& operator=(const Point& rhs) { if (this != &rhs) { x = rhs.x; y = rhs.y; } return *this; }

	bool inside(const Rect& rect) const;

	bool outside(const Rect& rect) const;

} Point;

typedef struct Rect {

	// there is specifically not a 4 float constructor due to ambiguity between if its 2 points, or 1 point, and width, height
	constexpr Rect() : x1(0), y1(0), x2(0), y2(0) {}

	constexpr Rect(const Point& a, const Point& b) : x1(a.x), y1(a.y), x2(b.x), y2(b.y) { }

	constexpr Rect(const Point& a, float w, float h) : x1(a.x), y1(a.y), x2(a.x + w), y2(a.y + h) { }

	union {
		struct {
			float x1;
			float y1;
		};
		Point p1;
	};
	
	union {
		struct {
			float x2;
			float y2;
		};
		Point p2;
	};

	bool inside(const Point& p) const {
		return (p.x >= x1 && p.x <= x2 && p.y >= y1 && p.y <= y2);
	}

	bool outside(const Point& p) const {
		return !inside(p);
	}
	
	float w() const {
		return x2 - x1;
	}

	float h() const {
		return y2 - y1;
	}

	Rect& operator=(const Rect& rhs) { if (this != &rhs) { p1 = rhs.p1; p2 = rhs.p2; } return *this; }

	Point topLeft() const {
		return Point(x1, y1);
	}

	Point topRight() const {
		return Point(x2, y1);
	}
	
	Point bottomLeft() const {
		return Point(x1, y2);
	}

	Point bottomRight() const {
		return Point(x2, y2);
	}

	D3DXVECTOR2 topLeftV() const {
		return D3DXVECTOR2(x1, y1);
	}

	D3DXVECTOR2 topRightV() const {
		return D3DXVECTOR2(x2, y1);
	}
	
	D3DXVECTOR2 bottomLeftV() const {
		return D3DXVECTOR2(x1, y2);
	}

	D3DXVECTOR2 bottomRightV() const {
		return D3DXVECTOR2(x2, y2);
	}

} Rect;

namespace UIManager {

	typedef struct UIPair {
		std::string name = "";
		float* f = NULL;
	} UIPair;

	extern std::map<float*, UIPair> UIFloatMap;
	extern std::map<std::string, UIPair> UIStringMap;

	std::string strip(const std::string& s);

	float safeStof(const std::string& s);

	void add(const std::string& name, float* f);
	void add(const std::string& name, Point* p);
	void reload();

};

extern Point mousePos; // no use getting this multiple times a frame

void logMatrix(const D3DMATRIX& matrix);

void writeClipboard(const std::string& text);

// -----

// why have i not abstracted this yet omfg
// also, tbh, i think im reserving these for use only with MeltyTestVert. maybe. might make drawing easier? or i could just pass this to draw func. i dont want to have different vers for scaled and unscaled tho
template <typename T>
class Tri {
public:

	Tri() {}

	Tri(const T& v1_, const T& v2_, const T& v3_) {
		v1 = v1_;
		v2 = v2_;
		v3 = v3_;
	}

	Tri(const T& v1_, const T& v2_, const T& v3_, DWORD col) {
		v1 = v1_;
		v2 = v2_;
		v3 = v3_;

		v1.color = col;
		v2.color = col;
		v3.color = col;
	}

	union {
		T verts[3];
		struct {
			T v1;
			T v2;
			T v3;
		};
	};
};

template <typename T>
class Quad { 
public:
	// really should have made this class more complex to allow for easier texture usage, and also i need to make my point class better. but i also need to actually use that class 
	Quad() {}

	Quad(const T& v1_, const T& v2_, const T& v3_, const T& v4_) {
		v1 = v1_;
		v2 = v2_;
		v3 = v3_;
		v4 = v4_;
	}

	Quad(const T& v1_, const T& v2_, const T& v3_, const T& v4_, DWORD col) {
		v1 = v1_;
		v2 = v2_;
		v3 = v3_;
		v4 = v4_;

		v1.color = col;
		v2.color = col;
		v3.color = col;
		v4.color = col;
	}

	Quad(const Rect& pos, const Rect& texPos = Rect({ 0.0f, 0.0f }, { 0.0f, 0.0f }), DWORD col = 0xFF42E5F4) {
		v1 = T(pos.x1, pos.y1, 0.0f, 1.0f, texPos.x1, texPos.y1, col); 
		v2 = T(pos.x2, pos.y1, 0.0f, 1.0f, texPos.x2, texPos.y1, col);
		v3 = T(pos.x1, pos.y2, 0.0f, 1.0f, texPos.x1, texPos.y2, col);
		v4 = T(pos.x2, pos.y2, 0.0f, 1.0f, texPos.x2, texPos.y2, col);
	}

	union { // this use of unions here is one of my fave things in c++
		T verts[4];
		struct {
			T v1;
			T v2;
			T v3;
			T v4;
		};
	};
};

extern unsigned _vertexBytesTotal;
extern unsigned _vertexBytesTransferedThisFrame;
// this could have been done without a class. i hope the overhead isnt stupid
template <typename T, size_t vertexCount, D3DPRIMITIVETYPE primType = D3DPT_TRIANGLELIST>
class VertexData {
public:

	VertexData(DWORD vertexFormat_, IDirect3DTexture9** texture_ = NULL) {
		vertexFormat = vertexFormat_;
		texture = texture_;
	}

	void alloc() {
		if (vertexBuffer == NULL) {
			if (FAILED(device->CreateVertexBuffer(vertexCount * sizeof(T), 0, vertexFormat, D3DPOOL_MANAGED, &vertexBuffer, NULL))) {
				log("failed to alloc a vertex buffer!");
				vertexBuffer = NULL;
			}
			_vertexBytesTotal += vertexCount * sizeof(T);
		}
	}

	~VertexData() {
		_vertexBytesTotal -= vertexCount * sizeof(T);

		if (vertexBuffer != NULL) {
			vertexBuffer->Release();
		}
	}

	void add(const T& v1, const T& v2) {

		if (vertexIndex >= vertexCount) {
			//log("a vertex buffer overflowed. this is critical. increase the buffer size! current: %d fmt: %08X", vertexCount, vertexFormat);
			// this log call was getting called to often, and would fuck things up
			return;
		}

		vertexData[vertexIndex++] = v1;
		vertexData[vertexIndex++] = v2;

	}

	void add(const T& v1, const T& v2, const T& v3) {

		if (vertexIndex >= vertexCount) {
			//log("a vertex buffer overflowed. this is critical. increase the buffer size! current: %d fmt: %08X", vertexCount, vertexFormat);
			// this log call was getting called to often, and would fuck things up
			return;
		}

		vertexData[vertexIndex++] = v1;
		vertexData[vertexIndex++] = v2;
		vertexData[vertexIndex++] = v3;

	}

	void addScale(const T& v1, const T& v2) {

		if (vertexIndex >= vertexCount) {
			//log("a vertex buffer overflowed. this is critical. increase the buffer size! current: %d fmt: %08X", vertexCount, vertexFormat);
			// this log call was getting called to often, and would fuck things up
			return;
		}

		vertexData[vertexIndex++] = v1;
		vertexData[vertexIndex++] = v2;

		scaleVertex(vertexData[vertexIndex - 2]);
		scaleVertex(vertexData[vertexIndex - 1]);
	}

	void addScale(const T& v1, const T& v2, const T& v3) {

		if (vertexIndex >= vertexCount) {
			//log("a vertex buffer overflowed. this is critical. increase the buffer size! current: %d fmt: %08X", vertexCount, vertexFormat);
			// this log call was getting called to often, and would fuck things up
			return;
		}
		
		vertexData[vertexIndex++] = v1;
		vertexData[vertexIndex++] = v2;
		vertexData[vertexIndex++] = v3;

		scaleVertex(vertexData[vertexIndex - 3]);
		scaleVertex(vertexData[vertexIndex - 2]);
		scaleVertex(vertexData[vertexIndex - 1]);
	}

	void add(const Tri<T>& tri) {
		addScale(tri.v1, tri.v2, tri.v3);
	}

	void add(const Quad<T>& quad) {
		addScale(quad.v1, quad.v2, quad.v3);
		addScale(quad.v2, quad.v3, quad.v4);
	}

	void draw() {

		if (vertexBuffer == NULL) {
			log("a vertex data buffer was null? how.");
			exit(1);
		}

		if (vertexIndex == 0) {
			return;
		}

		if (texture != NULL && *texture != NULL) {
			device->SetTexture(0, *texture);
		}

		// ideally i should be scaling the vertices in here, but is it worth it?

		VOID* pVoid;

		_vertexBytesTransferedThisFrame += vertexIndex * sizeof(T);

		vertexBuffer->Lock(0, vertexIndex * sizeof(T), (void**)&pVoid, 0);
		memcpy(pVoid, &vertexData[0], vertexIndex * sizeof(T));
		vertexBuffer->Unlock();

		DWORD primCount = vertexIndex;

		if constexpr (primType == D3DPT_TRIANGLELIST) {
			primCount /= 3;
		} else if constexpr (primType == D3DPT_LINELIST) {
			primCount /= 2;
		}

		device->SetStreamSource(0, vertexBuffer, 0, sizeof(T));
		device->SetFVF(vertexFormat);
		device->DrawPrimitive(primType, 0, primCount);

		// i could use DrawIndexedPrimitive here? check if faster.
		// would be super nice, esp for text drawing at the minimum
		// wait no, they, they still share tex stuff??

		vertexIndex = 0;

		if (texture != NULL && *texture != NULL) {
			device->SetTexture(0, NULL);
		}
	}

	DWORD vertexFormat = 0;
	IDirect3DVertexBuffer9* vertexBuffer = NULL;
	IDirect3DTexture9** texture = NULL;
	T vertexData[vertexCount]; // i distrust vectors
	unsigned vertexIndex = 0; // number of verts. i,, ugh. i should have written a const size vec class.

};

// -----

typedef struct PosVert {
	D3DVECTOR position;
} PosVert;

typedef struct PosColVert {
	D3DVECTOR position;
	D3DCOLOR color;
	PosColVert() { position = {0, 0, 0}, color = 0; }
	PosColVert(float x, float y, float z, D3DCOLOR c) { position.x = x; position.y = y; position.z = z; color = z; }
	PosColVert(const D3DVECTOR& p, D3DCOLOR c) { position = p; color = c; }
} PosColVert;

typedef struct PosTexVert {
	D3DVECTOR position;
	D3DXVECTOR2 texCoord;
} PosTexVert;

typedef struct PosColTexVert {
	D3DVECTOR position;
	D3DCOLOR color;
	D3DXVECTOR2 texCoord;
} PosColTexVert;

typedef struct MeltyVert { // if having all these initializers causes slowdown, ill cry
	
	union {
		D3DVECTOR position = D3DVECTOR(0.0f, 0.0f, 0.0f);
		struct {
			float x;
			float y;
			float z;
		};
	};
	float rhw = 1.0f;
	D3DCOLOR color = 0xFFFFFFFF;
	union {
		D3DXVECTOR2 uv = D3DXVECTOR2(0.0f, 0.0f); // might not be the smartest idea, but it works
		struct {
			float u;
			float v;
		};
	};
	
	MeltyVert() {}

	MeltyVert(float x, float y, D3DXVECTOR2 uv_, DWORD col = 0xFFFFFFFF) {
		position.x = x;
		position.y = y;
		position.z = 0.0f;
		rhw = 1.0f;
		color = col;
		uv.x = uv_.x;
		uv.y = uv_.y;
	}

	MeltyVert(float x, float y, DWORD col = 0xFFFFFFFF) {
		position.x = x;
		position.y = y;
		position.z = 0.0f;
		rhw = 1.0f;
		color = col;
	}

} MeltyVert;

extern size_t fontBufferSize;
extern BYTE* fontBuffer; // this is purposefully not freed on evict btw
extern IDirect3DTexture9* fontTexture;

extern size_t fontBufferSizeWithOutline;
extern BYTE* fontBufferWithOutline;
extern IDirect3DTexture9* fontTextureWithOutline;

extern BYTE* fontBufferMelty;
extern size_t fontBufferMeltySize;
extern IDirect3DTexture9* fontTextureMelty;

extern IDirect3DTexture9* uiTexture;

extern VertexData<PosColVert, 3 * 2048> posColVertData;//(D3DFVF_XYZ | D3DFVF_DIFFUSE);
extern VertexData<PosTexVert, 3 * 2048> posTexVertData;//(D3DFVF_XYZ | D3DFVF_TEX1, &fontTexture);
// need to rework font rendering, 4096 is just horrid
//extern VertexData<PosColTexVert, 3 * 4096 * 2> posColTexVertData;// (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, &fontTextureMelty);
extern VertexData<PosColTexVert, 3 * 4096 * 4> posColTexVertData;// (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, &fontTextureMelty);
extern VertexData<PosColTexVert, 3 * 4096 * 4> uiVertData;

extern VertexData<MeltyVert, 3 * 4096> meltyVertData;// (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, &fontTextureMelty);
extern VertexData<MeltyVert, 2 * 4096, D3DPT_LINELIST> meltyLineData;// (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, &fontTextureMelty); // 8192 is overkill

// ----

//const int fontTexWidth = 256;
//const int fontTexHeight = 256;
//int fontSize = 32;
extern const int fontTexWidth;
extern const int fontTexHeight;
extern int fontSize;
extern int fontHeight;
extern int fontWidth;
extern float fontRatio;

IDirect3DPixelShader9* createPixelShader(const char* pixelShaderCode);

IDirect3DVertexShader9* createVertexShader(const char* shaderCode);

inline unsigned scaleNextPow2(unsigned v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

bool loadResource(int id, BYTE*& buffer, unsigned& bufferSize);

void initTextureResource(int resourceID, IDirect3DTexture9*& resTexture);

void _initFontFirstLoad();

void initFont();

// -----

extern bool _hasStateToRestore;
extern IDirect3DPixelShader9* _pixelShaderBackup;
extern IDirect3DVertexShader9* _vertexShaderBackup;
extern IDirect3DBaseTexture9* _textureBackup;
extern DWORD _D3DRS_BLENDOP;
extern DWORD _D3DRS_ALPHABLENDENABLE;
extern DWORD _D3DRS_SRCBLEND;
extern DWORD _D3DRS_DESTBLEND;
extern DWORD _D3DRS_SEPARATEALPHABLENDENABLE;
extern DWORD _D3DRS_SRCBLENDALPHA;
extern DWORD _D3DRS_DESTBLENDALPHA;
extern DWORD _D3DRS_MULTISAMPLEANTIALIAS;
extern DWORD _D3DRS_ALPHATESTENABLE;
extern DWORD _D3DRS_ALPHAREF;
extern DWORD _D3DRS_ALPHAFUNC;
extern D3DMATRIX _D3DTS_VIEW;

extern float vWidth;
extern float vHeight;
extern float wWidth;
extern float wHeight;
extern bool isWide;
extern D3DVECTOR factor;
extern D3DVECTOR topLeftPos; // top left of the screen, in pixel coords. maybe should be in directx space but idk
extern D3DVECTOR renderModificationFactor;
extern D3DXVECTOR2 mouseTopLeft;
extern D3DXVECTOR2 mouseBottomRight;
extern D3DXVECTOR2 mouseFactor;

// -----

constexpr BYTE ARROW_0(0x80 + 0x00);
constexpr BYTE ARROW_1(0x80 + 0x01);
constexpr BYTE ARROW_2(0x80 + 0x02);
constexpr BYTE ARROW_3(0x80 + 0x03);
constexpr BYTE ARROW_4(0x80 + 0x04);
constexpr BYTE ARROW_5(0x80 + 0x05);
constexpr BYTE ARROW_6(0x80 + 0x06);
constexpr BYTE ARROW_7(0x80 + 0x07);
constexpr BYTE ARROW_8(0x80 + 0x08);
constexpr BYTE ARROW_9(0x80 + 0x09);

constexpr BYTE BUTTON_A(0x90 + 0x00);
constexpr BYTE BUTTON_B(0x90 + 0x01);
constexpr BYTE BUTTON_C(0x90 + 0x02);
constexpr BYTE BUTTON_D(0x90 + 0x03);
constexpr BYTE BUTTON_E(0x90 + 0x04);
constexpr BYTE BUTTON_DASH(0x90 + 0x05);

constexpr BYTE BUTTON_A_GRAY(0xA0 + 0x00);
constexpr BYTE BUTTON_B_GRAY(0xA0 + 0x01);
constexpr BYTE BUTTON_C_GRAY(0xA0 + 0x02);
constexpr BYTE BUTTON_D_GRAY(0xA0 + 0x03);
constexpr BYTE BUTTON_E_GRAY(0xA0 + 0x04);
constexpr BYTE BUTTON_DASH_GRAY(0xA0 + 0x05);

constexpr BYTE JOYSTICK(0x90 + 0x07); // double size

constexpr BYTE CURSOR(0x80 + 0x0C); // a nice memory
constexpr BYTE CURSOR_LOADING(0x00 + 0x00);

constexpr BYTE ARCICON(0xB0 + 0x00);
constexpr BYTE MECHICON(0xB0 + 0x01);
constexpr BYTE HIMEICON(0xB0 + 0x02);
constexpr BYTE WARAICON(0xB0 + 0x03);
constexpr BYTE WHISK(0xB0 + 0x04); // double size

inline void scaleVertex(D3DVECTOR& v) {
	/*
	if (isWide) {
		v.x /= factor;
	} else {
		v.y /= factor;
	}*/
	// branchless, and with multiplication is faster
	v.x *= factor.x;
	v.y *= factor.y;
	/*
	mov		eax,	[esp + 4];
    movaps	xmm0,	dword ptr [eax];
    movaps	xmm1,	dword ptr [factor];
    mulps	xmm0,	xmm1;
    movaps	[eax],	xmm0;

    ret;

	might be faster, 
	but would require the struct to be alignas(16) 
	at the very least, it is not slower, despite doing 
	2x mults.
	very weird
	
	*/
}

inline void scaleVertex(MeltyVert& v) {
	v.position.x += topLeftPos.x;
	v.position.y += topLeftPos.y;
	
	v.position.x *= renderModificationFactor.x;
	v.position.y *= renderModificationFactor.y;
}

void __stdcall backupRenderState();

void __stdcall restoreRenderState();

// -----

void LineDraw(float x1, float y1, float x2, float y2, DWORD ARGB = 0x8042e5f4, bool side = false);

void RectDraw(float x, float y, float w, float h, DWORD ARGB = 0x8042e5f4);

void RectDraw(const Rect& rect, DWORD ARGB = 0x8042e5f4);

void RectDrawPrio(float x, float y, float w, float h, DWORD ARGB = 0x8042e5f4);

void RectDrawPrio(const Rect& rect, DWORD ARGB = 0x8042e5f4);

void BorderDraw(float x, float y, float w, float h, DWORD ARGB = 0x8042e5f4);

void BorderDraw(const Rect& rect, DWORD ARGB = 0x8042e5f4);

void BorderRectDraw(float x, float y, float w, float h, DWORD ARGB = 0x8042e5f4);

void UIDraw(Rect texRect, Rect screenRect, DWORD ARGB, bool mirror = false);

void UIDraw(const Rect& texRect, const Point& p, DWORD ARGB, bool mirror = false);

void UIDraw(const Rect& texRect, const Point& p, const float scale, DWORD ARGB, bool mirror = false);

// -----

Rect TextDraw(float x, float y, float size, DWORD ARGB, const char* format);

Rect TextDraw(const Point& p, float size, DWORD ARGB, const char* format);

template<typename... Args>
Rect TextDraw(float x, float y, float size, DWORD ARGB, const char* format, Args... args) {

	static char buffer[1024];
	snprintf(buffer, 1024, format, args...);

	return TextDraw(x, y, size, ARGB, buffer);
}

template<typename... Args>
Rect TextDraw(const Point& p, float size, DWORD ARGB, const char* format, Args... args) {
	// if this isnt inlined ill kill someone
	return TextDraw(p.x, p.y, size, ARGB, format, args...);
}

void TextDrawSimple(float x, float y, float size, DWORD ARGB, const char* format, ...);

void __stdcall _doDrawCalls(IDirect3DDevice9 *deviceExt);
