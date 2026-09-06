#pragma once
#include <cstdint>
#include <vector>

namespace storm {

// Pure dungeon data and queries. NO SDL, NO engine headers: the specs link
// against this without touching a renderer. Draw lives in dungeonDraw.cpp.
enum class Cell : unsigned char { Wall = 0, Floor = 1 };

struct Room {
    int x, y, w, h;
    int cx() const { return x + w / 2; }
    int cy() const { return y + h / 2; }
};

class Dungeon {
public:
    Dungeon() = default;
    Dungeon(int cols, int rows)
        : cols_(cols), rows_(rows), cells_(cols * rows, Cell::Wall) {}

    Cell CellAt(int col, int row) const;
    bool IsSolid(int col, int row) const;
    int cols_ = 0;
    int rows_ = 0;
    std::vector<Cell> cells_;
    std::vector<Room> rooms_;
};

// Pure function seed -> grid. Both sims run it and must get the same answer.
Dungeon Generate(std::uint32_t seed);
// Every floor cell reachable from the first. The one property a missing
// corridor would silently break.
bool DungeonConnected(const Dungeon &d);

} // namespace storm
