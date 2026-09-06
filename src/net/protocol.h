#pragma once
#include <cstdint>

// Wire protocol. SDL-free, fixed sizes. First byte of every chunk is msg id.
// Authority: position is joiner's, everything else (damage incl.) is host's.
enum class MsgId : std::uint8_t {
    Hello = 1,      // host->join vital: seed, cols, rows
    Ready = 2,      // join->host vital: dungeon generated
    Input = 3,      // join->host unreliable: x,y,facing,actions
    Swing = 4,      // join->host unreliable: "I swung"
    Shoot = 5,      // join->host unreliable: ox,oy,dx,dy
    OpenChest = 6,  // either->host vital: chest id
    Snapshot = 7,   // host->join unreliable: NetSnapshotDelta bytes + tick
    Event = 8,      // host->join vital: death/pickup/lamp events
};

enum ActionBits : std::uint32_t { ActSwing = 1u << 0, ActShoot = 1u << 1, ActDrop = 1u << 2, ActPickup = 1u << 3 };

#pragma pack(push, 1)
struct HelloMsg { std::uint8_t id; std::uint32_t seed; std::int32_t cols, rows; };
struct InputMsg { std::uint8_t id; float x, y, fx, fy; std::uint32_t actions; };
struct ShootMsg { std::uint8_t id; float ox, oy, dx, dy; };
struct SwingMsg { std::uint8_t id; float x, y, fx, fy; };
struct OpenMsg { std::uint8_t id; std::uint32_t chestId; };
#pragma pack(pop)

// Snapshot item types (uint16 type field).
enum SnapType : std::uint16_t { SnapPlayer = 1, SnapMonster = 2, SnapLamp = 3, SnapChest = 4, SnapShot = 5 };
// SnapPlayer/SnapMonster payload: x,y,hp packed as int32 (x*100,y*100,hp).
// SnapLamp: x,y,carriedBy (0 ground,1 host,2 joiner). SnapChest: opened flag.
