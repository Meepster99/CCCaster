
#include "DllDirectX.hpp"
#include "Version.hpp"
#include <stdlib.h>
#include <time.h>  
#include "aacc_2v2_ui_elements.h"
#include <array>
#include "palettes/palettes.hpp"
#include "DllOverlayUi.hpp"

#ifndef F12PRESS 
#define F12PRESS   (GetAsyncKeyState(VK_F12) & 0x0001)
#endif

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
	int paletteIndex = 0; 
	int moonIndex = 0;
	bool randomPalette = false;
	bool offsetPalette = false;
	RawInput prevInput;
	RawInput input;
	// i should probs just read from where melty gets these,,, but im tired ok? 
	BYTE pressDir() {
		if(prevInput.dir != input.dir) {
			return input.dir;
		}
		return 0;
	}

	BYTE pressBtn() {
		if(prevInput.btn != input.btn) {
			return (prevInput.btn ^ input.btn) & input.btn;
		}
		return 0;
	}

	int holdCounter = 0;
	BYTE holdDir() {

		if(prevInput.dir == input.dir) {
			holdCounter++;
			if(holdCounter > 20) { // strobe the result
				holdCounter = 10;
				return input.dir;
			}
			return 0;
		}
		
		holdCounter = 0;
		return 0; 
	}

} OurCSSData;

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

typedef struct CSSStructDisplay {
	unsigned unknown1;
	unsigned cssPhase;
	unsigned portNumber;
	unsigned existsIfMinusOne;
	unsigned gridPos;
	unsigned charID;
	unsigned moon;
	unsigned palette;
	unsigned unknown2;
} CSSStructDisplay;

static_assert(sizeof(CSSStructDisplay) == 0x24, "CSSStructDisplay must be size 0x24");

OurCSSData ourCSSData[4];

int getCharMoon(int playerIndex) {
	return ourCSSData[playerIndex].moonIndex;
}

CSSStructCopy* players[4] = {
	(CSSStructCopy*)(0x0074d83C + (0 * 0x2C)),
	(CSSStructCopy*)(0x0074d83C + (1 * 0x2C)),
	(CSSStructCopy*)(0x0074d83C + (2 * 0x2C)),
	(CSSStructCopy*)(0x0074d83C + (3 * 0x2C))
};

CSSStructDisplay* dispPlayers[4] = {
	(CSSStructDisplay*)(0x0074D8E8 + (0 * 0x24)),
	(CSSStructDisplay*)(0x0074D8E8 + (1 * 0x24)),
	(CSSStructDisplay*)(0x0074D8E8 + (2 * 0x24)),
	(CSSStructDisplay*)(0x0074D8E8 + (3 * 0x24))
};

constexpr float cssMenuFontSize = 12.0f;
constexpr float cssMenuSelectorWidth = 128.0f;
constexpr int charIDCount = sizeof(charIDList) / sizeof(charIDList[0]); 
const int gridWidth = 9;
const int gridHeight = 6;

int getGoalGridPos(int startGrid, int dir) {

	int x = startGrid % gridWidth;
	int y = startGrid / gridWidth;

	if(dir == 2) {
		y++;
	} else if(dir == 8) {
		y--;
	} else if(dir == 4) {
		x--;
	} else if(dir == 6) {
		x++;
	} else { // direction isnt one of the normal 8, so just ret
		return startGrid;
	}

	if(x < 0) {
		x = gridWidth - 1;
	} else if(x >= gridWidth) {
		x = 0;
	}

	if(y < 0) {
		y = gridHeight - 1;
	} else if(y >= gridHeight) {
		y = 0;
	}

	int newGrid = x + (y * gridWidth);

	// linear search doesnt really matter here
	int newIndex = -1;
	for(int i=0; i<charIDCount; i++) {
		if(charGridList[i] == newGrid) {
			newIndex = i;
			break;
		}	
	}

	if(newIndex == -1) {
		// this means our resulting position was not valid, in which case, we will now recurse. i should maybe add a depth fallback case?
		return getGoalGridPos(newGrid, dir);
	}

	return newGrid;
}

std::function<void(int playerIndex, int dir)> ControlFuncs[] = {

	[](int playerIndex, int dir) mutable -> void {
		//ourCSSData[playerIndex].idIndex = SAFEMOD(ourCSSData[playerIndex].idIndex + dir, charIDCount); 

		int startGrid = charGridList[ourCSSData[playerIndex].idIndex];

		int newGrid = getGoalGridPos(startGrid, dir); 

		// linear search doesnt really matter here
		int newIndex = -1;
		for(int i=0; i<charIDCount; i++) {
			if(charGridList[i] == newGrid) {
				newIndex = i;
				break;
			}	
		}

		if(newIndex == -1) {
			return;
		}
		
		if(newIndex != ourCSSData[playerIndex].idIndex) { // reset the palette and moon of the char, now that a new one is here
			ourCSSData[playerIndex].moonIndex = 0;
			players[playerIndex]->moon = 0;
			ourCSSData[playerIndex].paletteIndex = 0;
			ourCSSData[playerIndex].randomPalette = false;
		}
		
		ourCSSData[playerIndex].idIndex = newIndex;
	},

	[](int playerIndex, int dir) mutable -> void {

		int inc = 0;
		if(dir == 6 || dir == 2) {
			inc = 1;
		} else if(dir == 4 || dir == 8) {
			inc = -1;
		} else {
			return;
		}

		players[playerIndex]->moon = SAFEMOD(players[playerIndex]->moon + inc, 3);
		ourCSSData[playerIndex].moonIndex = players[playerIndex]->moon;
	},

	[](int playerIndex, int dir) mutable -> void {
		//players[playerIndex]->palette = SAFEMOD(players[playerIndex]->palette + dir, 36);

		if(playerIndex & 1) { // on right side, switch 4/6
			if(dir == 4) {
				dir = 6;
			} else if(dir == 6) {
				dir = 4;
			}
		}

		int currentPalette = ourCSSData[playerIndex].paletteIndex;

		if(ourCSSData[playerIndex].randomPalette) {
			if(dir == 2) {
				ourCSSData[playerIndex].paletteIndex = (ourCSSData[playerIndex].paletteIndex / 6) * 6;
				ourCSSData[playerIndex].randomPalette = false;
			} else if(dir == 8) {
				ourCSSData[playerIndex].paletteIndex = ((ourCSSData[playerIndex].paletteIndex / 6) * 6) + 5;
				ourCSSData[playerIndex].randomPalette = false;
			}
			return;
		}

		if( ((currentPalette % 6) == 0 && dir == 8) || ((currentPalette % 6) == 5 && dir == 2) ) {
			ourCSSData[playerIndex].randomPalette = true;
			return;
		}

		int paletteInc = 0;

		switch(dir) {
			case 2:
				paletteInc = 1;
				break;
			case 8:
				paletteInc = -1;
				break;
			case 4:
				paletteInc = -6;
				break;
			case 6:
				paletteInc = 6;
				break;
			default:
				break;
		}

		currentPalette = SAFEMOD(currentPalette + paletteInc, 36);

		ourCSSData[playerIndex].paletteIndex = currentPalette;

	},

	[](int playerIndex, int dir) mutable -> void {

	}

};

// -----

static Point moonRate(0.15, 0.15);

std::function<Point(const Point& current, const Point& goal, const Point& rate)> moonRateFunc = [](const Point& current, const Point& goal, const Point& rate) -> Point {

	Point delta = goal - current;
	
	return delta * rate;
};

Smooth<Point> startingMoonSmoothPoint = Smooth<Point>(Point(0, 0), moonRate, moonRateFunc); // rate is what percentage of this bs to do per frame.
static Smooth<Point> moonPositions[4][3] = {
	{ startingMoonSmoothPoint, startingMoonSmoothPoint, startingMoonSmoothPoint },
	{ startingMoonSmoothPoint, startingMoonSmoothPoint, startingMoonSmoothPoint },
	{ startingMoonSmoothPoint, startingMoonSmoothPoint, startingMoonSmoothPoint },
	{ startingMoonSmoothPoint, startingMoonSmoothPoint, startingMoonSmoothPoint }
};

static Point paletteRate(0.15, 0.15);
std::array<std::array<Smooth<Point>, 36>, 4> palettePositions = []() { // i actually despise how this is the best option i have for doing this. and it isnt even proper bc i still need a () constructor
    
	Smooth<Point> startingPaletteSmoothPoint = Smooth<Point>(Point(0, 0), paletteRate); 

	std::array<std::array<Smooth<Point>, 36>, 4> res;

	for(int i=0; i<4; i++) {
		for(int j=0; j<36; j++) {
			res[i][j].current = Point(0, 0);
			res[i][j].goal = Point(0, 0);
			res[i][j].rate = paletteRate;
		}
	}
    
    return res;
}();


std::function<void(int playerIndex, Point p)> drawCharSelect = [](int playerIndex, Point p) mutable -> void {
	// since the css grid thingys are always drawn, this func doesnt do much, but im leaving it here for consistency
};

std::function<void(int playerIndex, Point p)> drawMoonSelect = [](int playerIndex, Point p) mutable -> void {

	// upon entering moon select, if people have a random char selected, i need to give them a random char
	if(ourCSSData[playerIndex].idIndex == charIDCount - 1) {
		ourCSSData[playerIndex].idIndex = rand() % (charIDCount - 1);
	}

	if(ourCSSData[playerIndex].randomPalette) { // if we are in the moon state and randomPalette is still true, reset it
		ourCSSData[playerIndex].randomPalette = false;
	}

	const Rect* moonRects[] = {
		&UI::MoonCrescent,
		&UI::MoonFull,
		&UI::MoonHalf,
		&UI::MoonEclipse
	};
	
	static Point moonOffset(40, -180);
	static float moonScale = 4.0f;
	//UIManager::add("moonOffset", &moonOffset);
	//UIManager::add("moonScale", &moonScale);

	int moonIndex = CLAMP(players[playerIndex]->moon, 0, 3);
	int prevMoonIndex = SAFEMOD(moonIndex - 1, 3);
	int nextMoonIndex = SAFEMOD(moonIndex + 1, 3);
	
	static Point otherMoonOffset(20, 0);
	static float moonTransparency = 0.33;
	static float otherMoonScale = 4.0f;
	//UIManager::add("otherMoonOffset", &otherMoonOffset);
	//UIManager::add("moonTransparency", &moonTransparency);
	//UIManager::add("otherMoonScale", &otherMoonScale);	
	DWORD moonCol = (((BYTE)(0xFF * moonTransparency)) << 24) | 0x00FFFFFF;

	moonPositions[playerIndex][moonIndex] = Point(0, 0);

	if(shouldReverseDraws) {
		moonPositions[playerIndex][prevMoonIndex] = otherMoonOffset;
		moonPositions[playerIndex][nextMoonIndex] = -otherMoonOffset;
	} else {
		moonPositions[playerIndex][prevMoonIndex] = -otherMoonOffset;
		moonPositions[playerIndex][nextMoonIndex] = otherMoonOffset;
	}

	Point centerPos = p + moonOffset + moonPositions[playerIndex][moonIndex];
	Point leftPos =   p + moonOffset + moonPositions[playerIndex][prevMoonIndex];
	Point rightPos =  p + moonOffset + moonPositions[playerIndex][nextMoonIndex];

	UIDraw(*moonRects[prevMoonIndex], leftPos, otherMoonScale, moonCol);
	UIDraw(*moonRects[nextMoonIndex], rightPos, otherMoonScale, moonCol);

	UIDraw(*moonRects[moonIndex], centerPos, moonScale, 0xFFFFFFFF); // so many issues would have been solved if i scaled this function to draw at texture center
	

};

std::function<void(int playerIndex, Point p)> drawPaletteSelect = [](int playerIndex, Point p) mutable -> void {

	const int width = 6;
	const int height = 6; // plus 1 for the random palette, but yea

	int charID = charIDList[ourCSSData[playerIndex].idIndex];
	int palette = ourCSSData[playerIndex].paletteIndex;
	bool isRandomPalette = ourCSSData[playerIndex].randomPalette;
	int offsetPaletteVal = ourCSSData[playerIndex].offsetPalette ? 36 : 0;

	static Point paletteOffset(40, -180);
	static float paletteScale = 12.0f;
	//UIManager::add("paletteOffset", &paletteOffset);
	//UIManager::add("paletteScale", &paletteScale);

	int palX = palette / width;
	int palY = palette % width;

	Point basePoint = p + paletteOffset;

	static Point paletteDrawSize(30, 10);
	static Point paletteSize(6, 11);
	static Point paletteStartOffset(5, 4);
	static float fontSize = 10.0f;
	static Point paletteFontOffset(14, 0);
	static Point paletteColorSize(20, 10);
	static Point paletteColorOffset(10, 0);

	//UIManager::add("paletteDrawSize", &paletteDrawSize);
	//UIManager::add("paletteSize", &paletteSize);
	//UIManager::add("paletteStartOffset", &paletteStartOffset);
	//UIManager::add("paletteFontSize", &fontSize);
	//UIManager::add("paletteFontOffset", &paletteFontOffset);
	//UIManager::add("paletteColorSize", &paletteColorSize);
	//UIManager::add("paletteColorOffset", &paletteColorOffset);

	for(int x=5; x>=0; x--) {
		
		Point startOffset = (paletteStartOffset * x) + (Point(0, paletteSize.y) * 6);
		
		for(int y=6; y>=0; y--) {
			
			bool isRandomDraw = false;
			if(x != 0 && y == 6) { // prevents random from being drawn anywhere but x=0
				startOffset.y -= paletteSize.y;
				continue;
			} else if(y == 6) {
				isRandomDraw = true;
			}

			Point baseDrawPoint;

			if(isRandomDraw) {
				baseDrawPoint = basePoint + startOffset;
			} else {
				int paletteIndex = (((palX + x) % 6) * 6) + y;
				
				if(paletteIndex == palette && !isRandomPalette) {
					palettePositions[playerIndex][paletteIndex] = startOffset + Point(paletteStartOffset.x * 2, 0);
				} else {
					palettePositions[playerIndex][paletteIndex] = startOffset;
				}
				
				baseDrawPoint = basePoint + palettePositions[playerIndex][paletteIndex];
			}

			
			Rect drawRect;
			drawRect.p1 = baseDrawPoint;
			drawRect.p2 = baseDrawPoint + paletteDrawSize;

			DWORD color = 0xFF000000;
			DWORD borderColor = 0xFF646464;
			if(x == 0) {
				color = 0xFF000000;
				borderColor = 0xFFC8C8C8;
			}

			Point textPoint = drawRect.p1;

			if( ((x == 0) && ((palX * 6) + y == palette) && !isRandomPalette) || (isRandomPalette && (y == 6)) ) {
				drawRect.p1 -= Point(1, 1);
				drawRect.p2 += Point(1, 1);
				color = 0xFFFFFFFF;
				borderColor = 0xFFFF0000;
				
				DWORD animColor = 0xFF000000;
				switch(playerIndex) {
					case 0:
						animColor = 0xFFff0000;
						break;
					case 1:
						animColor = 0xFF00ca50;
						break;
					case 2:
						animColor = 0xFF0000ff;
						break;
					case 3:
						animColor = 0xFFff9d00;
						break;
					default:
						break;
				}
				borderColor = animColor;

				addAnimatedRect(drawRect, animColor);
			}

			RectDrawTex(drawRect, color);

			Rect drawRectOrig = drawRect;

			if(!isRandomDraw) {
				drawRect.p1 = baseDrawPoint + paletteColorOffset;
				drawRect.p2 = baseDrawPoint + paletteColorOffset + paletteColorSize;

				BGRAColor palColor = 0xFF00FF00;

				if(paletteColorMap.contains(charID)) {
					int tempIndex = (((palX + x) % 6) * 6) + y + offsetPaletteVal;
					tempIndex = CLAMP(tempIndex, 0, 63);
					palColor = paletteColorMap[charID][tempIndex];
				}

				if(x != 0) {
					// melty seems to do col *= (1 / (x+1)). this works,, but i have some weird shit
					// nope, this is wrong!!!,, maybe? or something else is fucking up my transforms??
					// its doing some other type of transform. that ratio is correct, but not consistent
					// it was for warc 27 though?
					//palColor.r = (BYTE)(palColor.r * (1.0f / (x + 1.0f)));
					//palColor.g = (BYTE)(palColor.g * (1.0f / (x + 1.0f)));
					//palColor.b = (BYTE)(palColor.b * (1.0f / (x + 1.0f)));

					palColor.r = (BYTE)(palColor.r * (1.0f / (2.0f)));
					palColor.g = (BYTE)(palColor.g * (1.0f / (2.0f)));
					palColor.b = (BYTE)(palColor.b * (1.0f / (2.0f)));
				}

				RectDrawTex(drawRect, palColor);
			}

			BorderDrawTex(drawRectOrig, borderColor);

			if(x == 0) {
				int dispPaletteIndex = (palX * 6) + y;
				Point temp = shouldReverseDraws ? Point(2, 0) : Point(0, 0);
				if(y == 6) {
					TextDraw(textPoint + paletteFontOffset + temp - Point(8, 0), fontSize, 0xFFFFFFFF, "RND");
				} else {
					TextDraw(textPoint + paletteFontOffset + temp, fontSize, 0xFFFFFFFF, "%02d", dispPaletteIndex + offsetPaletteVal + 1);
				}
			}
		
			startOffset.y -= paletteSize.y;
		}
	}
};

std::function<void(int playerIndex, Point p)> drawReadySelect = [](int playerIndex, Point p) mutable -> void {

	if(ourCSSData[playerIndex].randomPalette) { // we are here, pick a random palette
		ourCSSData[playerIndex].randomPalette = false;
		ourCSSData[playerIndex].paletteIndex = rand() % 36;
	}

	TextDraw(p, cssMenuFontSize, 0xFF42e5f4, "READY");
};

std::function<void(int playerIndex, Point p)> DrawFuncs[] = { // i could pass in more params, but tbh ill just let each function do its own vibe
	drawCharSelect,
	drawMoonSelect,
	drawPaletteSelect,
	drawReadySelect
};

void drawCharSelectRect(int playerIndex) {

	bool shouldReverseDrawsBackup = shouldReverseDraws;
	shouldReverseDraws = false;

	int gridPos = charGridList[ourCSSData[playerIndex].idIndex];

	int x = gridPos % gridWidth;
	int y = gridPos / gridWidth;

	static Point basePoint(137, 42);
	static float width = 40;
	static float height = 45;
	static float boxWidth = 46;
	static float boxHeight = 55;

	//UIManager::add("base", &basePoint);
	//UIManager::add("width", &width);
	//UIManager::add("height", &height);
	//UIManager::add("boxWidth", &boxWidth);
	//UIManager::add("boxHeight", &boxHeight);

	Point drawPoint = basePoint + Point(x * width, y * height);

	DWORD boxColor = 0xFFFFFFFF;
	switch(playerIndex) {
		case 0:
			boxColor = 0xFFff0000;
			break;
		case 1:
			boxColor = 0xFF00ca50;
			break;
		case 2:
			boxColor = 0xFF0000ff;
			break;
		case 3:
			boxColor = 0xFFff9d00;
			break;
		default:
			break;
	}

	Rect r(drawPoint, drawPoint + Point(boxWidth, boxHeight));

	addCharRect(r, boxColor);

	shouldReverseDraws = shouldReverseDrawsBackup;

}

// ------

const int menuOptionCount = sizeof(ControlFuncs) / sizeof(ControlFuncs[0]);
static_assert(sizeof(DrawFuncs) / sizeof(DrawFuncs[0]) == sizeof(ControlFuncs) / sizeof(ControlFuncs[0]), "each DrawFuncs must have a controlfunc!");

bool inStageSelect = false;

void cssInit() {

	log("css inited");

	static bool firstInit = true;
	if(firstInit) {
		firstInit = false;
		ourCSSData[0].idIndex = 0; // sion
		ourCSSData[1].idIndex = 10; // vsion
		ourCSSData[2].idIndex = 1; // arc
		ourCSSData[3].idIndex = 11; // warc
		srand(time(NULL));
	}

	// on init, set all css phases to 1 so that caster doesnt block B inputs
	for(int i=0; i<4; i++) {
		dispPlayers[i]->cssPhase = 1;
		dispPlayers[i]->cssPhase = 1;
		ourCSSData[i].selectIndex = 0;
	}

	inStageSelect = false;

}

extern "C" {
    DWORD stageSelEAX = 0;
}

void stageSelCallback() {

	if(!inStageSelect) {
		stageSelEAX = 0xFFFFFFFF;
		inStageSelect = false;
	}

}

void enterStageSel() {

	if(inStageSelect) {
		return;
	}

	inStageSelect = true;

	dispPlayers[0]->cssPhase = 5;
	dispPlayers[1]->cssPhase = 5;

}

void leaveStageSel() {

	if(!inStageSelect) {
		return;
	}

	log("leaveStageSel");

	inStageSelect = false;

	dispPlayers[0]->cssPhase = 1;
	dispPlayers[1]->cssPhase = 1;

}

void updateControls() {

	static bool copyInputs = false;
	if(F12PRESS) {
		copyInputs = !copyInputs;
		//log("copyInputs: %d", copyInputs);
	}

	// handle controls
	// idk where i should grab controls from either, what reads the stuff melty writes to? or i could be safe with a direct melty write, but have to keep track 
	// of presses myself
	// maybe one less patch tho

	for(int i=0; i<4; i++) {

		if(copyInputs) {
			ourCSSData[i].input.set(0);
		} else {
			ourCSSData[i].input.set(i);
		}
		
		BYTE pressDir = ourCSSData[i].pressDir();
		pressDir |= ourCSSData[i].holdDir();
		BYTE pressBtn = ourCSSData[i].pressBtn();

		// this needs refactoring

		if(*(BYTE*)(0x00773158 + 0x3C) == 0x01) {
			if(pressBtn == 0x20) { // B press
				leaveStageSel(); // does this even work.
				return; // this return might do some weird shit, but not having it would be worse
			}
		} else {
			if(pressBtn) {
				if(pressBtn & 0x10) { // A
					ourCSSData[i].selectIndex++;
					if(ourCSSData[i].selectIndex >= menuOptionCount) {
						ourCSSData[i].selectIndex = menuOptionCount - 1;
					}
				} else if(pressBtn & 0x20) { // B. B Always moves you back to start 
					ourCSSData[i].selectIndex = 0;

					// reset moon positions
					for(int m=0; m<3; m++) {
						moonPositions[i][m].current = Point(0, 0);
						moonPositions[i][m].goal = Point(0, 0);
					}

					// reset palette positions
					for(int pal=0; pal<36; pal++) {
						palettePositions[i][pal].current = Point(0, 0);
						palettePositions[i][pal].goal = Point(0, 0);
					}

				} else if((pressBtn & 0x0C) && ((ourCSSData[i].input.btn & 0x0C) == 0x0C)) { // caster is fucking with p/2/3 on subsequent loads. am i not resetting correctly? its randomly having 0x02 btn
					ourCSSData[i].offsetPalette = !ourCSSData[i].offsetPalette;
				}
			} else {
				switch(pressDir) {
					case 2:
					case 8:
					case 4:
					case 6:
						ControlFuncs[ourCSSData[i].selectIndex](i, pressDir);
						break;
					default:
						break;
				}
			}
		}
	
		ourCSSData[i].prevInput = ourCSSData[i].input;
	}

	// check if i should move ppl to css
	bool shouldMoveToStageSel = true;
	for(int i=0; i<4; i++) {
		if(ourCSSData[i].selectIndex != menuOptionCount - 1) {
			shouldMoveToStageSel = false;
			break;
		}
	}

	if(shouldMoveToStageSel) {
		enterStageSel();
	}

}

void updateMeltyState() {
	for(int playerIndex=0; playerIndex<4; playerIndex++) {

		if(!ourCSSData[playerIndex].randomPalette) {
			players[playerIndex]->palette = (ourCSSData[playerIndex].offsetPalette ? 36 : 0) + ourCSSData[playerIndex].paletteIndex;
			players[playerIndex]->palette = CLAMP(players[playerIndex]->palette, 0, 63);
		}
		
		dispPlayers[playerIndex]->gridPos = charGridList[ourCSSData[playerIndex].idIndex];
		players[playerIndex]->charID = charIDList[ourCSSData[playerIndex].idIndex];
		dispPlayers[playerIndex]->charID = players[playerIndex]->charID;
		dispPlayers[playerIndex]->palette = players[playerIndex]->palette;

		players[playerIndex]->moon = ourCSSData[playerIndex].moonIndex;
		dispPlayers[playerIndex]->moon = ourCSSData[playerIndex].moonIndex;
	}	
}

void drawCSS() {

	if(*(BYTE*)(0x00773158 + 0x3C) == 0x01) { // in stagesel, dont draw
		return;
	}
 
	constexpr Point anchors[4] = { // coords grabbed from _naked_modifyCSSPixelDraw
		Point(60 + 80, 448),
		Point(60 + 80, 448),
		//Point(640 - 60 - 80, 448),
		Point(60, 300),
		Point(60, 300)
		//Point(640 - 60, 300)
	};

	for(int i=0; i<4; i++) {

		shouldReverseDraws = (i & 1);

		Point p = anchors[i];

		drawCharSelectRect(i);

		DrawFuncs[ourCSSData[i].selectIndex](i, p);
	}
	
}

void updateCSSStuff() {

	if(!(*((uint8_t*) 0x0054EEE8) == 0x14 && DllOverlayUi::isDisabled())) { // check if in css
		return;
	}

	shouldReverseDraws = false;

	updateControls();

	updateMeltyState();

	drawCSS();

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
