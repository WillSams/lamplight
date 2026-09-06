#pragma once
#include <cstdint>
#include <cstring>

#include <stormengine2/net/netServer.h>

#include "../combat/hits.h"
#include "../dungeon/generate.h"
#include "../dungeon/rng.h"
#include "../game/world.h"
#include "protocol.h"
#include "snap.h"

namespace storm {

// Host simulates the world — monsters, chests, lamp, damage — and owns
// everything except the joiner's position. Thin wire layer over the pure
// WorldState in game/world.h: it times Ticks, validates chunk payloads, and
// encodes snapshots. All rules live in world.h + hits.h; only the byte
// marshalling here touches the engine.
class HostSession {
public:
    std::uint32_t seed = 0;
    Dungeon dungeon;
    WorldState world;
    int tick = 0;
    NetServer server;

    bool Start(std::uint16_t port, std::uint32_t s) {
        seed = s;
        dungeon = Generate(seed);
        world = SpawnWorld(dungeon);
        if (!server.Start(port, 2)) return false;
        server.SetOnClientConnect([this](int id) {
            HelloMsg h{static_cast<std::uint8_t>(MsgId::Hello), seed,
                       dungeon.cols_, dungeon.rows_};
            // Vital, own datagram: losing the seed means no dungeon.
            server.Send(id, &h, sizeof(h), true);
        });
        server.SetOnChunk([this](int id, const NetChunk &c) { HandleChunk(id, c); });
        return true;
    }

    void Tick(double dt) {
        server.Update();  // Update() before Poll() everywhere: a Poll-only loop
        server.Poll();    // freezes NetConnection's clock (RTT 0, no timeouts).
        ++tick;
        for (Monster &m : world.monsters)
            TickMonster(m, world, kMonsterSpeed, kMonsterTouchDps, static_cast<float>(dt));
        if (world.lampCarrier == 1) {
            world.lampX = world.hostX;
            world.lampY = world.hostY;
        }
        SendSnapshot();
    }

    // Host-side player actions. Damage resolved here no matter who originated
    // the input — a client never claims a kill.
    void HostSwing(float x, float y, float fx, float fy) {
        for (Monster &m : world.monsters)
            if (m.alive && MeleeHits(x, y, fx, fy, m.x, m.y)) {
                m.hp -= 15;
                if (m.hp <= 0) m.alive = false;
            }
    }
    void HostShoot(float x, float y, float dx, float dy) {
        if (world.ammo <= 0) return;
        --world.ammo;
        for (Monster &m : world.monsters)
            if (m.alive && RangedHits(x, y, dx, dy, m.x, m.y)) {
                m.hp -= 25;
                if (m.hp <= 0) m.alive = false;
                return;
            }
    }

private:
    void HandleChunk(int id, const NetChunk &c) {
        if (c.size < 1) return;
        // Copy inside the callback: chunk data points into connection scratch
        // that the next Feed() overwrites.
        std::uint8_t buf[64];
        int n = c.size < 64 ? c.size : 64;
        std::memcpy(buf, c.data, n);
        switch (static_cast<MsgId>(buf[0])) {
        case MsgId::Input: {
            if (n < static_cast<int>(sizeof(InputMsg))) return;
            InputMsg im;
            std::memcpy(&im, buf, sizeof(im));
            // Position is the joiner's — clamp against walls, accept otherwise.
            if (!dungeon.IsSolid(static_cast<int>(im.x / kWorldTile),
                                 static_cast<int>(im.y / kWorldTile))) {
                world.joinX = im.x;
                world.joinY = im.y;
            }
            if (im.actions & ActSwing) HostSwing(world.joinX, world.joinY, im.fx, im.fy);
            if (im.actions & ActDrop) DropLamp(world, 2);
            break;
        }
        case MsgId::Shoot: {
            if (n < static_cast<int>(sizeof(ShootMsg))) return;
            ShootMsg sm;
            std::memcpy(&sm, buf, sizeof(sm));
            HostShoot(sm.ox, sm.oy, sm.dx, sm.dy);
            break;
        }
        case MsgId::OpenChest: {
            if (n < static_cast<int>(sizeof(OpenMsg))) return;
            OpenMsg om;
            std::memcpy(&om, buf, sizeof(om));
            TryOpenChest(world, static_cast<int>(om.chestId));
            break;
        }
        case MsgId::Ready:
            (void)id;
            break;
        default:
            break;
        }
    }

    void SendSnapshot() {
        // Full snapshot every tick, unreliable. Deltas would be cheaper, but a
        // single dropped chunk poisons every later Apply against a broken
        // base -- the client would freeze with no error. A full frame
        // self-corrects on the next tick; ~23 items x 15 bytes sits far under
        // the 1200 chunk cap, so the whole world fits one datagram.
        std::uint8_t out[1200];
        out[0] = static_cast<std::uint8_t>(MsgId::Snapshot);
        std::memcpy(out + 1, &tick, 4);
        int off = 5;
        auto item = [&](std::uint16_t type, std::uint16_t id, float x, float y, int v) {
            if (off + 15 > static_cast<int>(sizeof(out))) return;
            std::int32_t d[3];
            PackPosHp(d, x, y, v);
            out[off++] = static_cast<std::uint8_t>(type & 0xFF);
            out[off++] = static_cast<std::uint8_t>(id & 0xFF);
            out[off++] = 3;
            std::memcpy(out + off, d, 12);
            off += 12;
        };
        item(SnapPlayer, 1, world.hostX, world.hostY, world.hostHp);
        item(SnapPlayer, 2, world.joinX, world.joinY, world.joinHp);
        for (const Monster &m : world.monsters) {
            float dx = m.x - world.joinX, dy = m.y - world.joinY;
            if (!m.alive || dx * dx + dy * dy > 480 * 480) continue;  // cull radius = visibility model
            item(SnapMonster, static_cast<std::uint16_t>(m.id), m.x, m.y, m.hp);
        }
        item(SnapLamp, 0, world.lampX, world.lampY, world.lampCarrier);
        for (const Chest &ch : world.chests)
            item(SnapChest, static_cast<std::uint16_t>(ch.id), ch.x, ch.y, ch.opened ? 1 : 0);

        int ids[16];
        int n = server.GetConnectedClientIds(ids, 16);
        for (int i = 0; i < n; ++i) server.Send(ids[i], out, off, false);
    }
};

} // namespace storm
