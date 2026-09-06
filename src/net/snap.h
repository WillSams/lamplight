#pragma once
#include <cstdint>

namespace storm {

// SDL-free snapshot payload packing. Positions at centipixel precision so a
// delta for a player that stood still is empty, not full of float jitter.
inline void PackPosHp(std::int32_t *out, float x, float y, int hp) {
    out[0] = static_cast<std::int32_t>(x * 100.0f);
    out[1] = static_cast<std::int32_t>(y * 100.0f);
    out[2] = hp;
}
inline void UnpackPosHp(const std::int32_t *in, float &x, float &y, int &hp) {
    x = in[0] / 100.0f;
    y = in[1] / 100.0f;
    hp = in[2];
}

} // namespace storm
