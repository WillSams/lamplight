#include "generate.h"

#include "rng.h"

namespace storm {

Cell Dungeon::CellAt(int col, int row) const {
    if (col < 0 || row < 0 || col >= cols_ || row >= rows_) return Cell::Wall;
    return cells_[row * cols_ + col];
}

bool Dungeon::IsSolid(int col, int row) const {
    return CellAt(col, row) == Cell::Wall;
}

namespace {
void CarveH(Dungeon &d, int x0, int x1, int y) {
    if (y < 1 || y >= d.rows_ - 1) return;
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    for (int x = x0; x <= x1; ++x)
        if (x >= 1 && x < d.cols_ - 1) d.cells_[y * d.cols_ + x] = Cell::Floor;
}
void CarveV(Dungeon &d, int y0, int y1, int x) {
    if (x < 1 || x >= d.cols_ - 1) return;
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    for (int y = y0; y <= y1; ++y)
        if (y >= 1 && y < d.rows_ - 1) d.cells_[y * d.cols_ + x] = Cell::Floor;
}
} // namespace

Dungeon Generate(std::uint32_t seed) {
    Dungeon d(64, 64);
    Rng rng(seed);
    const int attempts = (d.cols_ * d.rows_) / 8;
    for (int i = 0; i < attempts && d.rooms_.size() < 12; ++i) {
        int w = 4 + static_cast<int>(rng.Below(7));
        int h = 4 + static_cast<int>(rng.Below(6));
        if (w + 2 >= d.cols_ || h + 2 >= d.rows_) continue;
        int x = 1 + static_cast<int>(rng.Below(static_cast<std::uint32_t>(d.cols_ - w - 2)));
        int y = 1 + static_cast<int>(rng.Below(static_cast<std::uint32_t>(d.rows_ - h - 2)));
        bool overlap = false;
        for (const Room &r : d.rooms_) {
            if (x - 1 < r.x + r.w && x + w + 1 > r.x && y - 1 < r.y + r.h && y + h + 1 > r.y) {
                overlap = true;
                break;
            }
        }
        if (overlap) continue;
        d.rooms_.push_back(Room{x, y, w, h});
        for (int yy = y; yy < y + h; ++yy)
            for (int xx = x; xx < x + w; ++xx)
                d.cells_[yy * d.cols_ + xx] = Cell::Floor;
        if (d.rooms_.size() > 1) {
            const Room &a = d.rooms_[d.rooms_.size() - 2];
            const Room &b = d.rooms_[d.rooms_.size() - 1];
            if (rng.Below(2) == 0) { CarveH(d, a.cx(), b.cx(), a.cy()); CarveV(d, a.cy(), b.cy(), b.cx()); }
            else { CarveV(d, a.cy(), b.cy(), a.cx()); CarveH(d, a.cx(), b.cx(), b.cy()); }
        }
    }
    return d;
}

bool DungeonConnected(const Dungeon &d) {
    int start = -1;
    for (int i = 0; i < static_cast<int>(d.cells_.size()); ++i)
        if (d.cells_[i] == Cell::Floor) { start = i; break; }
    if (start < 0) return false;
    std::vector<char> seen(d.cells_.size(), 0);
    std::vector<int> stack{start};
    seen[start] = 1;
    const int dc[4] = {1, -1, 0, 0}, dr[4] = {0, 0, 1, -1};
    while (!stack.empty()) {
        int cur = stack.back(); stack.pop_back();
        int c = cur % d.cols_, r = cur / d.cols_;
        for (int k = 0; k < 4; ++k) {
            int nc = c + dc[k], nr = r + dr[k];
            if (nc < 0 || nr < 0 || nc >= d.cols_ || nr >= d.rows_) continue;
            int ni = nr * d.cols_ + nc;
            if (!seen[ni] && d.cells_[ni] == Cell::Floor) { seen[ni] = 1; stack.push_back(ni); }
        }
    }
    for (int i = 0; i < static_cast<int>(d.cells_.size()); ++i)
        if (d.cells_[i] == Cell::Floor && !seen[i]) return false;
    return true;
}

} // namespace storm
