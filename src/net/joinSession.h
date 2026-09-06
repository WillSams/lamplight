#pragma once
#include <cstring>
#include <string>

#include <stormengine2/net/netClient.h>
#include <stormengine2/net/netSnapshot.h>

#include "../dungeon/generate.h"
#include "../game/world.h"
#include "protocol.h"
#include "snap.h"

namespace storm {

// Joiner simulates only its own character and renders everything else from
// snapshots. Wire layer, mirroring HostSession: marshalling here, rules in
// game/world.h. Damage is the host's — this side never applies a hit.
class JoinSession {
public:
    Dungeon dungeon;
    WorldState world;  // mirrored world: host-driven fields only
    float myX = 0, myY = 0, fx = 1, fy = 0;
    bool ready = false;
    bool connected = false;
    NetClient client;
    NetSnapshot base;
    int baseTick = -1;

    bool Connect(const std::string &ip, std::uint16_t port) {
        client.SetOnConnect([this]() { connected = true; });
        client.SetOnChunk([this](const NetChunk &c) { HandleChunk(c); });
        return client.Connect(ip, port);
    }

    // The joiner's own position IS its authority. playState owns world
    // collision before it gets here (ResolveGridMove on the dungeon grid);
    // the session only reports the already-collided result, never integrates
    // itself. Sending an uncollided position would let the joiner clip walls.
    // Callers hold px,py from the previous frame; a spawn assigned in the
    // Hello handler would be stomped the very same tick if we copied it
    // unconditionally. Only trust px once the session was already ready.
    void Tick(double dt, float px, float py, float fxx, float fyy, std::uint32_t actions) {
        const bool wasReady = ready;
        client.Update();  // Update() before Poll() every frame.
        client.Poll();
        if (!ready) return;
        if (wasReady) { myX = px; myY = py; }
        fx = fxx;
        fy = fyy;
        InputMsg im{static_cast<std::uint8_t>(MsgId::Input), myX, myY, fx, fy, actions};
        client.Send(&im, sizeof(im), false);
    }

    void Shoot() {
        ShootMsg s{static_cast<std::uint8_t>(MsgId::Shoot), myX, myY, fx, fy};
        client.Send(&s, sizeof(s), false);
    }

    // Opening a chest is vital: dropping it would show the two players a
    // chest both opened and neither agreed on.
    void OpenChest(int id) {
        OpenMsg o{static_cast<std::uint8_t>(MsgId::OpenChest),
                  static_cast<std::uint32_t>(id)};
        client.Send(&o, sizeof(o), true);
    }

    static int ChestNearby(float x, float y, const WorldState &w) {
        return ChestInReach(w, x, y);
    }

private:
    void HandleChunk(const NetChunk &c) {
        if (c.size < 1) return;
        // Copy in the callback: data points into per-connection scratch the
        // next Feed() overwrites -- including the next datagram in this Poll().
        std::uint8_t buf[1300];
        int n = c.size < 1300 ? c.size : 1300;
        std::memcpy(buf, c.data, n);
        switch (static_cast<MsgId>(buf[0])) {
        case MsgId::Hello: {
            if (n < static_cast<int>(sizeof(HelloMsg))) return;
            HelloMsg h;
            std::memcpy(&h, buf, sizeof(h));
            dungeon = Generate(h.seed);
            world = SpawnWorld(dungeon);
            myX = world.joinX;  // JoinerSpawn picks a floor cell, not cx+1.
            myY = world.joinY;
            ready = true;
            std::uint8_t r = static_cast<std::uint8_t>(MsgId::Ready);
            client.Send(&r, 1, true);  // vital: host must know we're generating
            break;
        }
        case MsgId::Snapshot: {
            if (n < 5) return;
            int tick;
            std::memcpy(&tick, buf + 1, 4);
            if (tick <= baseTick) return;  // stale full snapshot, ignore
            baseTick = tick;
            int off = 5;
            world.monsters.clear();
            while (off + 3 <= n - 12) {
                const std::uint8_t t = buf[off];
                const std::uint8_t id = buf[off + 1];
                const std::uint8_t cnt = buf[off + 2];
                if (cnt != 3 || off + 3 + 12 > n) return;
                float x, y;
                int hp;
                UnpackPosHp(reinterpret_cast<const std::int32_t *>(buf + off + 3), x, y, hp);
                off += 15;
                switch (t) {
                case SnapPlayer:
                    if (id == 1) { world.hostX = x; world.hostY = y; world.hostHp = hp; }
                    else if (id == 2) { world.joinHp = hp; }
                    break;
                case SnapMonster:
                    world.monsters.push_back(Monster{x, y, hp, id, hp > 0});
                    break;
                case SnapLamp:
                    world.lampX = x; world.lampY = y; world.lampCarrier = hp;
                    break;
                case SnapChest:
                    if (id >= world.chests.size()) world.chests.resize(static_cast<size_t>(id) + 1);
                    world.chests[id] = Chest{x, y, hp == 1, id};
                    break;
                default: break;
                }
            }
            break;
        }
        default:
            break;
        }
    }

    // Full-snapshot deserialiser lives inline in HandleChunk; no delta base.
};

} // namespace storm
