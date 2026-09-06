#pragma once
#include <cmath>

namespace storm {

// SDL-free hit tests. All distances world pixels. Host resolves; the
// authority rule: position is the joiner's, damage is the host's.

// Melee: short arc centred on the facing direction.
inline bool MeleeHits(float ax, float ay, float fx, float fy,
                      float tx, float ty, float range = 48.0f, float cosHalf = 0.5f) {
    float dx = tx - ax, dy = ty - ay;
    float dist = std::sqrt(dx * dx + dy * dy);
    if (dist > range) return false;
    if (dist < 0.001f) return true;
    float fl = std::sqrt(fx * fx + fy * fy);
    if (fl < 0.001f) return true;
    return (dx * fx + dy * fy) / (dist * fl) >= cosHalf;
}

// Ranged: ray from origin along (dx,dy); hit if the target is within maxDist
// and within hitRadius of the ray.
inline bool RangedHits(float ox, float oy, float dx, float dy,
                       float tx, float ty, float maxDist = 320.0f, float hitRadius = 14.0f) {
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.001f) return false;
    dx /= len; dy /= len;
    float rx = tx - ox, ry = ty - oy;
    float t = rx * dx + ry * dy;
    if (t < 0 || t > maxDist) return false;
    float px = ox + dx * t, py = oy + dy * t;
    float ex = tx - px, ey = ty - py;
    return (ex * ex + ey * ey) <= hitRadius * hitRadius;
}

} // namespace storm
