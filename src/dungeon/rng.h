#pragma once
#include <cstdint>

namespace storm {

// Ours, deliberately. std::uniform_int_distribution and std::shuffle are not
// portable across standard library versions -- same seed, different sequence --
// and a divergence here means the two players generate different dungeons and
// never find out. SDL-free, so the specs link without the renderer.
struct Rng {
    std::uint32_t s;
    explicit Rng(std::uint32_t seed) : s(seed ? seed : 0x9E3779B9u) {}
    std::uint32_t Next() {           // xorshift32
        s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s;
    }
    // Rejection sampling, so the result is uniform and identical everywhere.
    std::uint32_t Below(std::uint32_t n) {
        const std::uint32_t limit = 0xFFFFFFFFu - (0xFFFFFFFFu % n);
        std::uint32_t v;
        do { v = Next(); } while (v >= limit);
        return v % n;
    }
};

} // namespace storm
