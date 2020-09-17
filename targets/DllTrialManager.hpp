#pragma once
#include <vector>

#include <d3dx9.h>

using namespace std;

namespace TrialManager {

extern wstring comboName;

extern vector<wstring> comboTrialText;

extern vector<wstring> fullStrings;

extern string dtext;

extern int comboTrialTextAlign;

extern int comboTrialLength;

extern int comboTrialPosition;

extern int currentTrial;

extern bool hideText;
    
extern LPDIRECT3DTEXTURE9 trialTextures;

extern int trialTextures2;
} // namespace TrialManager

class DllTrialManager
{
public:

    void frameStepTrial();
    void loadTrialFile();
    void clear();
    void loadCombo( int comboId );

    bool initialized = false;

private:

    bool comboDrop = false;
    bool comboStart = false;

    int currentHitcount = 0;

    int numCombos;

    vector<wstring> comboNames;
    vector<vector<wstring>> comboText;
    vector<vector<int>> comboSeq;
    vector<vector<int>> comboHit;

    int getHitcount();

};
