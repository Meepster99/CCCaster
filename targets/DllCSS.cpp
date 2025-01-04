
#include "DllDirectX.hpp"
#include "Version.hpp"
#include <stdlib.h>
#include <time.h>  
#include "aacc_2v2_ui_elements.h"

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

int getGoalGridPos(int startGrid, int dir) {

	const int width = 9;
	const int height = 6;

	int x = startGrid % width;
	int y = startGrid / width;

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
		x = width - 1;
	} else if(x >= width) {
		x = 0;
	}

	if(y < 0) {
		y = height - 1;
	} else if(y >= height) {
		y = 0;
	}

	int newGrid = x + (y * 9);

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
			// i could,, and maybe should, recurse here.
			return;
		}

		// reset the palette and moon of the char, now that a new one is here
		players[playerIndex]->moon = 0;
		ourCSSData[playerIndex].paletteIndex = 0;
		ourCSSData[playerIndex].randomPalette = false;

		ourCSSData[playerIndex].idIndex = newIndex;
	},

	[](int playerIndex, int dir) mutable -> void {

		int inc = 0;
		if(dir == 6) {
			inc = 1;
		} else if(dir == 4) {
			inc = -1;
		} else {
			return;
		}

		players[playerIndex]->moon = SAFEMOD(players[playerIndex]->moon + inc, 3);
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

std::function<void(int playerIndex, Point p)> drawCharSelect = [](int playerIndex, Point p) mutable -> void {
	/*
	bool mirror = playerIndex & 1;
	DWORD bgCol = selfIndex == ourCSSData[playerIndex].selectIndex ? 0xFFFF0000 : 0xFF000000;

	dispPlayers[playerIndex]->gridPos = charGridList[ourCSSData[playerIndex].idIndex];
	players[playerIndex]->charID = charIDList[ourCSSData[playerIndex].idIndex];
	dispPlayers[playerIndex]->charID = players[playerIndex]->charID;
	dispPlayers[playerIndex]->palette = players[playerIndex]->palette;

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
	*/


	//TextDraw(p, cssMenuFontSize, 0xFFFFFFFF, "CHOOSE CHAR");
};

std::function<void(int playerIndex, Point p)> drawMoonSelect = [](int playerIndex, Point p) mutable -> void {

	// upon entering moon select, if people have a random char selected, i need to give them a random char
	if(ourCSSData[playerIndex].idIndex == charIDCount - 1) {
		ourCSSData[playerIndex].idIndex = rand() % (charIDCount - 1);
	}

	if(ourCSSData[playerIndex].randomPalette) { // if we are in the moon state and randomPalette is still true, reset it
		ourCSSData[playerIndex].randomPalette = false;
	}

	constexpr const Rect* moonRects[] = {
		&UI::MoonCrescent,
		&UI::MoonFull,
		&UI::MoonHalf,
		&UI::MoonEclipse
	};

	static Point moonOffset(40, -180);
	static float moonScale = 4.0f;
	UIManager::add("moonOffset", &moonOffset);
	UIManager::add("moonScale", &moonScale);
	
	int moonIndex = CLAMP(players[playerIndex]->moon, 0, 3);
	UIDraw(*moonRects[moonIndex], p + moonOffset, moonScale, 0xFFFFFFFF);

};

std::function<void(int playerIndex, Point p)> drawPaletteSelect = [](int playerIndex, Point p) mutable -> void {

	const int width = 6;
	const int height = 6; // plus 1 for the random palette, but yea

	int palette = ourCSSData[playerIndex].paletteIndex;
	bool isRandomPalette = ourCSSData[playerIndex].randomPalette;
	bool offsetPalette = ourCSSData[playerIndex].offsetPalette;

	int x = palette / width;
	int y = palette % width;

	static Point paletteOffset(40, -180);
	static float paletteScale = 12.0f;
	//UIManager::add("paletteOffset", &paletteOffset);
	//UIManager::add("paletteScale", &paletteScale);

	Point basePoint = p + paletteOffset;

	for(int i=0; i<7; i++) {
		Rect highlightRect;
		highlightRect.p1 = basePoint - Point(0 * paletteScale, 0);
		highlightRect.p2 = basePoint + Point(2 * paletteScale, paletteScale);

		if(i < 6) {
			DWORD highlightColor = (((x * 6) + i == palette) && !isRandomPalette) ? 0x40FFFFFF : 0x40000000;

			int paletteDisplayIndex = (offsetPalette ? 36 : 0) + (x * 6) + i + 1;

			
			if(paletteDisplayIndex > 64) {
				highlightColor = 0x40FF0000;
			}

			RectDraw(highlightRect, highlightColor);
			TextDraw(basePoint, paletteScale, 0xFFFFFFFF, "%02d", paletteDisplayIndex);
		} else {

			highlightRect.p2 = basePoint + Point(6 * paletteScale, paletteScale);

			DWORD highlightColor = isRandomPalette ? 0x40FFFFFF : 0x40000000;
			RectDraw(highlightRect, highlightColor);
			TextDraw(basePoint, paletteScale, 0xFFFFFFFF, "RANDOM");
		}
		
		basePoint.y += paletteScale;
	}

	/*
	if(ourCSSData[playerIndex].randomPalette) {
		TextDraw(p, cssMenuFontSize, 0xFFFFFFFF, "CHOOSE PALETTE RANDOM");
		return;
	}

	TextDraw(p, cssMenuFontSize, 0xFFFFFFFF, "CHOOSE PALETTE %d", palette + 1);
	*/
};

std::function<void(int playerIndex, Point p)> drawReadySelect = [](int playerIndex, Point p) mutable -> void {

	if(ourCSSData[playerIndex].randomPalette) { // we are here, pick a random palette
		ourCSSData[playerIndex].randomPalette = false;
		ourCSSData[playerIndex].paletteIndex = rand() % 36;
	}

	/*
	bool mirror = playerIndex & 1;
	DWORD bgCol = selfIndex == ourCSSData[playerIndex].selectIndex ? 0xFFFF0000 : 0xFF000000;

	RectDraw(x, y, cssMenuSelectorWidth, cssMenuFontSize, bgCol);
	TextDraw(x, y, cssMenuFontSize, 0xFFFFFFFF, "Ready(notworking)");
	y += cssMenuFontSize;
	*/	  
	TextDraw(p, cssMenuFontSize, 0xFF42e5f4, "READY");
};

std::function<void(int playerIndex, Point p)> DrawFuncs[] = { // i could pass in more params, but tbh ill just let each function do its own vibe
	drawCharSelect,
	drawMoonSelect,
	drawPaletteSelect,
	drawReadySelect
};

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

	// handle controls
	// idk where i should grab controls from either, what reads the stuff melty writes to? or i could be safe with a direct melty write, but have to keep track 
	// of presses myself
	// maybe one less patch tho

	for(int i=0; i<4; i++) {

		ourCSSData[i].input.set(i);

		BYTE pressDir = ourCSSData[i].pressDir();
		BYTE pressBtn = ourCSSData[i].pressBtn();

		// this needs refactoring

	
		//if(inStageSelect) {
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
				} else if(pressBtn & 0x20) { // B
					ourCSSData[i].selectIndex--;
					if(ourCSSData[i].selectIndex < 0) {
						ourCSSData[i].selectIndex = 0;
					}
				} else if(pressBtn & 0x0C && (ourCSSData[i].input.btn == 0x0C)) {
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
	bool shouldMoveToCSS = true;
	for(int i=0; i<4; i++) {
		if(ourCSSData[i].selectIndex != menuOptionCount - 1) {
			shouldMoveToCSS = false;
			break;
		}
	}

	if(shouldMoveToCSS) {
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

		DrawFuncs[ourCSSData[i].selectIndex](i, p);
	}
	
}

void updateCSSStuff(IDirect3DDevice9 *device) {

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
