#pragma once

namespace AsmHacks {

struct PortraitConfig {
    float xPos;
    float yPos;
    float width;
    float height;
    float layer;
};

struct MoonConfig {
    float xPos;
    float yPos;
    float scale;
    float layer;
    int textureOffset;
};

extern PortraitConfig portraitConfigs[4];
extern MoonConfig moonConfigs[4];

} // namespace AsmHacks 