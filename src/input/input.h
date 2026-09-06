#pragma once
#include <glm/glm.hpp>

namespace storm {

// Keyboard + gamepad input, already sampled, with no SDL types so specs link
// with no renderer. The playState maps SDL key events and the engine Gamepad
// state into this once per frame; everything downstream consumes it.
struct MoveInput {
    bool left = false, right = false, up = false, down = false;  // keyboard
    float stickX = 0.0f, stickY = 0.0f;                          // left stick, -1..1
    bool dpadLeft = false, dpadRight = false, dpadUp = false, dpadDown = false;
};

// Resolved velocity direction, magnitude <= 1. The d-pad is ignored while the
// stick is live so one direction cannot double-count.
inline glm::vec2 MoveDirection(const MoveInput &in) {
    glm::vec2 v(0.0f);
    if (in.left) v.x -= 1.0f;
    if (in.right) v.x += 1.0f;
    if (in.up) v.y -= 1.0f;
    if (in.down) v.y += 1.0f;
    v.x += in.stickX;
    v.y += in.stickY;
    if (in.stickX == 0.0f && in.stickY == 0.0f) {
        if (in.dpadLeft) v.x -= 1.0f;
        if (in.dpadRight) v.x += 1.0f;
        if (in.dpadUp) v.y -= 1.0f;
        if (in.dpadDown) v.y += 1.0f;
    }
    if (glm::length(v) > 1.0f) v = glm::normalize(v);
    return v;
}

// Face the aim direction (right stick) when live, else the move direction.
// Front-facing art: we return a direction and the draw layer flips; rotation
// is never set.
inline glm::vec2 FacingFrom(glm::vec2 velocity,
                            float aimX, float aimY, glm::vec2 previous) {
    if (aimX != 0.0f || aimY != 0.0f) return glm::normalize(glm::vec2(aimX, aimY));
    if (velocity.x != 0.0f || velocity.y != 0.0f) return glm::normalize(velocity);
    return previous;
}

} // namespace storm
