#pragma once
#include <vector>

#include <glm/glm.hpp>

#include "../dungeon/generate.h"

namespace storm {

// World state the host simulates. Pure data + pure functions; no engine, no
// SDL, so specs link clean. The host session (net/hostSession) wires these to
// the wire.

struct Monster { float x = 0, y = 0; int hp = 30; int id = 0; bool alive = true; };
struct Chest { float x = 0, y = 0; bool opened = false; int id = 0; };

struct WorldState {
    std::vector<Monster> monsters;
    std::vector<Chest> chests;
    float lampX = 0, lampY = 0;
    int lampCarrier = 1;  // 0 ground, 1 host, 2 joiner
    float hostX = 0, hostY = 0, joinX = 0, joinY = 0;
    int hostHp = 100, joinHp = 100, ammo = 12;
};

// Tiles are 16px drawn at scale 3.
inline constexpr float kWorldTile = 48.0f;

// The joiner must start on floor, next to the host if the host's cell is
// taken. Scanning the whole grid and picking the closest free floor cell
// keeps the answer a pure function of (dungeon, host position).
inline glm::vec2 JoinerSpawn(const Dungeon &d, float hostX, float hostY) {
    const int hc = static_cast<int>(hostX / kWorldTile);
    const int hr = static_cast<int>(hostY / kWorldTile);
    float best = -1.0f;
    glm::vec2 spawn{hostX, hostY};
    for (int r = 0; r < d.rows_; ++r)
        for (int c = 0; c < d.cols_; ++c) {
            if (c == hc && r == hr) continue;
            if (d.IsSolid(c, r)) continue;
            const float dx = c - hc, dy = r - hr;
            const float dist = dx * dx + dy * dy;
            if (best < 0 || dist < best) {
                best = dist;
                spawn = {(c + 0.5f) * kWorldTile, (r + 0.5f) * kWorldTile};
            }
        }
    return spawn;
}

// Place players, lamp, monsters and chests into generated rooms. Deterministic
// by room order, which Generate fixes per seed -- no extra RNG needed, and a
// second draw order would be a second thing to keep in sync.
inline WorldState SpawnWorld(const Dungeon &d) {
    WorldState w;
    if (d.rooms_.empty()) return w;
    const Room &first = d.rooms_[0];
    w.hostX = first.cx() * kWorldTile;
    w.hostY = first.cy() * kWorldTile;
    const glm::vec2 js = JoinerSpawn(d, w.hostX, w.hostY);
    w.joinX = js.x;
    w.joinY = js.y;
    w.lampX = w.hostX;
    w.lampY = w.hostY;
    for (size_t i = 0; i < d.rooms_.size(); ++i) {
        const Room &r = d.rooms_[i];
        if (i % 2 == 0 && w.monsters.size() < 24)
            for (int k = 0; k < 2; ++k)
                w.monsters.push_back(Monster{(r.x + 1 + k) * kWorldTile,
                                             (r.y + 1) * kWorldTile, 30,
                                             static_cast<int>(w.monsters.size()), true});
        if (w.chests.size() < 8)
            w.chests.push_back(Chest{(r.x + r.w / 2) * kWorldTile,
                                     (r.y + r.h / 2) * kWorldTile, false,
                                     static_cast<int>(w.chests.size())});
    }
    return w;
}

// Chase the nearer player, capped so far monsters idle. The wake radius is
// a little past the lamp's 200px light, and the snapshot cull (480px) is
// wider still, so monsters appear out of the dark just before reaching you.
inline constexpr float kMonsterWakeRadius = 260.0f;
inline constexpr float kMonsterSpeed = 140.0f;   // below the player's 180
inline constexpr float kMonsterTouchDps = 30.0f;

inline void SteerMonster(Monster &m, float px, float py, float speed, float dt) {
    float dx = px - m.x, dy = py - m.y;
    float d2 = dx * dx + dy * dy;
    if (d2 < 1.0f || d2 > kMonsterWakeRadius * kMonsterWakeRadius) return;
    float d = std::sqrt(d2);
    m.x += dx / d * speed * dt;
    m.y += dy / d * speed * dt;
}

// One tick of one monster: steer, and touch damage against both players.
// meleeDamage per tick is damagePerSecond * dt out at the call site.
inline void TickMonster(Monster &m, WorldState &w, float speed, float touchDps, float dt) {
    if (!m.alive) return;
    float dH = (m.x - w.hostX) * (m.x - w.hostX) + (m.y - w.hostY) * (m.y - w.hostY);
    float dJ = (m.x - w.joinX) * (m.x - w.joinX) + (m.y - w.joinY) * (m.y - w.joinY);
    if (dH < dJ) SteerMonster(m, w.hostX, w.hostY, speed, dt);
    else SteerMonster(m, w.joinX, w.joinY, speed, dt);
    if (dH < 24 * 24) w.hostHp -= static_cast<int>(touchDps * dt);
    if (dJ < 24 * 24) w.joinHp -= static_cast<int>(touchDps * dt);
}

// Open a chest; opened chests stay open (a lost Open msg would desync both
// sides, which is why Open is sent vital). Returns ammo granted.
inline int TryOpenChest(WorldState &w, int id, int grant = 6) {
    if (id < 0 || id >= static_cast<int>(w.chests.size())) return 0;
    Chest &c = w.chests[id];
    if (c.opened) return 0;
    c.opened = true;
    w.ammo += grant;
    return grant;
}

// Drop or pick up the lamp. Host carries on death must also ground it here.
inline void DropLamp(WorldState &w, int carrier) {
    if (w.lampCarrier != carrier) return;
    w.lampCarrier = 0;
    if (carrier == 1) { w.lampX = w.hostX; w.lampY = w.hostY; }
    else if (carrier == 2) { w.lampX = w.joinX; w.lampY = w.joinY; }
}

// Pick up a grounded lamp within reach. Returns true if it flipped.
inline bool TryPickUpLamp(WorldState &w, int carrier, float reach = 72.0f) {
    if (w.lampCarrier != 0) return false;
    float px = carrier == 1 ? w.hostX : w.joinX;
    float py = carrier == 1 ? w.hostY : w.joinY;
    float dx = px - w.lampX, dy = py - w.lampY;
    if (dx * dx + dy * dy > reach * reach) return false;
    w.lampCarrier = carrier;
    return true;
}

// Nearest unopened chest within reach of a position, or -1.
inline int ChestInReach(const WorldState &w, float px, float py, float reach = 64.0f) {
    for (const Chest &c : w.chests) {
        if (c.opened) continue;
        float dx = px - c.x, dy = py - c.y;
        if (dx * dx + dy * dy < reach * reach) return c.id;
    }
    return -1;
}

} // namespace storm
