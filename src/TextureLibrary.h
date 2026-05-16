#pragma once
#include <raylib.h>

// Owns every PNG referenced by the game. Loaded once after the window exists,
// unloaded once before the window dies. World + Enemy hold raw pointers into
// this library; the library outlives them so the pointers are stable.
class TextureLibrary {
public:
    TextureLibrary();

    void Load();
    void Unload();

    Texture2D floor;
    Texture2D structure;
    Texture2D enemies[3];   // [0]=CHASER, [1]=SHOOTER, [2]=TANK
    Texture2D skies[3];

    bool loaded;
};
