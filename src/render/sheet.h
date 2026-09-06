#pragma once

namespace storm {

// tilemap_packed.png is 12x11 cells of 16px with NO margin. The pack's
// Tilesheet.txt documents a 1px margin -- that describes tilemap.png, the
// variant this project deleted. SDL-free so specs can include it.
//
// Cell layout confirmed from a labeled render of the sheet itself
// (assets regenerate with tools, never guess indices):
//   row 3           wall (4,3) and floor (6,3)
//   row 7           chest (5,7 closed, 8,7 open)
//   row 8 col 2/4   knights (heroes)
//   row 10          monster (1,10), lamp (8,10)
struct TileRect {
    int x, y, w, h;
};

inline constexpr int kTile = 16;
inline constexpr int kSheetCols = 12;
inline constexpr int kSheetRows = 11;

inline constexpr TileRect TileAt(int col, int row) {
    return TileRect{col * kTile, row * kTile, kTile, kTile};
}

} // namespace storm
