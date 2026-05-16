#include "TextureLibrary.h"

TextureLibrary::TextureLibrary() : loaded(false) {
    floor = { 0 };
    structure = { 0 };
    for (int i = 0; i < 3; ++i) { enemies[i] = { 0 }; skies[i] = { 0 }; }
}

void TextureLibrary::Load() {
    if (loaded) return;
    floor      = LoadTexture("assets/floor.png");
    structure  = LoadTexture("assets/structure.png");
    enemies[0] = LoadTexture("assets/enemie1.png");
    enemies[1] = LoadTexture("assets/enemie2.png");
    enemies[2] = LoadTexture("assets/enemie3.png");
    skies[0]   = LoadTexture("assets/sky1.png");
    skies[1]   = LoadTexture("assets/sky2.png");
    skies[2]   = LoadTexture("assets/sky3.png");

    // Tiling textures: repeat wrap + mipmaps for sharp distant detail.
    SetTextureWrap(floor,     TEXTURE_WRAP_REPEAT);
    SetTextureWrap(structure, TEXTURE_WRAP_REPEAT);
    GenTextureMipmaps(&floor);
    GenTextureMipmaps(&structure);
    SetTextureFilter(floor,     TEXTURE_FILTER_TRILINEAR);
    SetTextureFilter(structure, TEXTURE_FILTER_TRILINEAR);

    // Sprites + skies: bilinear is enough (no mipmaps needed — drawn at fixed sizes).
    for (int i = 0; i < 3; ++i) {
        GenTextureMipmaps(&enemies[i]);
        SetTextureFilter(enemies[i], TEXTURE_FILTER_TRILINEAR);
        SetTextureFilter(skies[i],   TEXTURE_FILTER_BILINEAR);
    }

    loaded = true;
}

void TextureLibrary::Unload() {
    if (!loaded) return;
    UnloadTexture(floor);
    UnloadTexture(structure);
    for (int i = 0; i < 3; ++i) UnloadTexture(enemies[i]);
    for (int i = 0; i < 3; ++i) UnloadTexture(skies[i]);
    loaded = false;
}
