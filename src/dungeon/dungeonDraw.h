#pragma once
#include <SDL2/SDL.h>

#include <stormengine2/assetStore.h>

#include "generate.h"

namespace storm {

// Draws only the dungeon cells inside the camera. Kept out of generate.h so
// the generator and its specs never touch a renderer; only the game links
// this. Window size stays the Project's 800x600 5x scale.
void DrawDungeon(SDL_Renderer *renderer, const AssetStore &assets,
                 const Dungeon &dungeon, const SDL_Rect &camera, int scale);

} // namespace storm
