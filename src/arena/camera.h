#pragma once
#include <glm/glm.hpp>

namespace storm {

// Screen-space view rectangle, SDL-free. The game converts to SDL_Rect at the
// single point it hands the camera to RenderSystem.
struct CameraRect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

// Centre on target, clamp to the world; pin to zero where the world is
// smaller than the view so the clamp never produces negative max.
inline CameraRect CameraFor(glm::vec2 target, int viewW, int viewH,
                            glm::vec2 worldSize) {
    CameraRect c{0, 0, viewW, viewH};
    c.x = static_cast<int>(target.x) - viewW / 2;
    c.y = static_cast<int>(target.y) - viewH / 2;
    if (worldSize.x <= viewW) c.x = 0;
    else {
        if (c.x < 0) c.x = 0;
        if (c.x > static_cast<int>(worldSize.x) - viewW) c.x = static_cast<int>(worldSize.x) - viewW;
    }
    if (worldSize.y <= viewH) c.y = 0;
    else {
        if (c.y < 0) c.y = 0;
        if (c.y > static_cast<int>(worldSize.y) - viewH) c.y = static_cast<int>(worldSize.y) - viewH;
    }
    return c;
}

} // namespace storm
