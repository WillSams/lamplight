#include "dungeonDraw.h"

#include "../render/sheet.h"

namespace storm {

void DrawDungeon(SDL_Renderer *renderer, const AssetStore &assets,
                 const Dungeon &dungeon, const SDL_Rect &camera, int scale) {
    SDL_Texture *tex = assets.GetTexture("sheet");
    if (!tex) return;
    const int tilePx = kTile * scale;
    int c0 = camera.x / tilePx;
    int c1 = (camera.x + camera.w) / tilePx;
    int r0 = camera.y / tilePx;
    int r1 = (camera.y + camera.h) / tilePx;
    if (c0 < 0) c0 = 0;
    if (r0 < 0) r0 = 0;
    if (c1 >= dungeon.cols_) c1 = dungeon.cols_ - 1;
    if (r1 >= dungeon.rows_) r1 = dungeon.rows_ - 1;
    for (int r = r0; r <= r1; ++r) {
        for (int c = c0; c <= c1; ++c) {
            // Verified packed-sheet cells: floor (4,3) blue stone,
            // wall (0,4) cream stone. TileAt uses no margin (see sheet.h).
            const TileRect src = dungeon.CellAt(c, r) == Cell::Floor
                                 ? TileAt(6, 3) : TileAt(4, 3);  // stone floor / blue brick wall
            const SDL_Rect srcRect{src.x, src.y, src.w, src.h};
            const SDL_Rect dst{c * tilePx - camera.x, r * tilePx - camera.y, tilePx, tilePx};
            SDL_RenderCopy(renderer, tex, &srcRect, &dst);
        }
    }
}

} // namespace storm
