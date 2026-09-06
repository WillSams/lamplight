#pragma once
#include <glm/glm.hpp>

namespace storm {

// Move a size-wide axis box one axis at a time, refusing solid cells. Tests
// the leading edge's two corners; no normal is computed, so sliding along a
// wall never catches on the seam the way per-tile colliders do. Same pattern
// as nightshift's collision.h. SolidFn takes pixel coords -> bool.
template <typename SolidFn>
inline glm::vec2 ResolveGridMove(glm::vec2 pos, glm::vec2 size,
                                 glm::vec2 delta, SolidFn solid) {
    if (delta.x != 0.0f) {
        float nx = pos.x + delta.x;
        float edgeX = delta.x > 0.0f ? nx + size.x - 0.001f : nx;
        if (!solid(edgeX, pos.y) && !solid(edgeX, pos.y + size.y - 0.001f))
            pos.x = nx;
    }
    if (delta.y != 0.0f) {
        float ny = pos.y + delta.y;
        float edgeY = delta.y > 0.0f ? ny + size.y - 0.001f : ny;
        if (!solid(pos.x, edgeY) && !solid(pos.x + size.x - 0.001f, edgeY))
            pos.y = ny;
    }
    return pos;
}

} // namespace storm
