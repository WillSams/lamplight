#include <igloo/igloo_alt.h>

#include <chrono>
#include <thread>

#include <stormengine2/net/netServer.h>
#include <stormengine2/net/netClient.h>

#include "../src/net/hostSession.h"
#include "../src/net/joinSession.h"

using namespace igloo;
using namespace storm;

// Loopback specs: real UDP sockets in one process, the pattern the engine's
// netLoopback.spec.cpp uses. Drives the full handshake, seed sync, world
// spawn, and snapshot round trip without two machines.

namespace {

const int kPumpDeadlineMs = 4000;
const int kPumpStepMs = 1;

bool Pump(HostSession &host, JoinSession &join, float mx, std::function<bool()> done) {
    (void)mx;
    uint32_t start = NetNowMs();
    while (NetNowMs() - start < (uint32_t)kPumpDeadlineMs) {
        host.Tick(1.0 / 60.0);
        join.Tick(1.0 / 60.0, join.myX, join.myY, 1.0f, 0.0f, 0);
        if (done()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(kPumpStepMs));
    }
    return false;
}

} // namespace

Describe(HostJoinHandShake) {
  Describe(a_host_and_joiner_on_loopback) {
    It(establishes_the_connection) {
      HostSession host;
      Assert::That(host.Start(0, 12345), Is().True());
      JoinSession join;
      Assert::That(join.Connect("127.0.0.1", host.server.GetPort()), Is().True());
      Assert::That(Pump(host, join, 0.0f, [&] { return join.connected; }), Is().True());
    };
  };
};

Describe(SeedSync) {
  Describe(a_host_with_a_connected_joiner) {
    It(sends_the_seed_and_the_joiner_generates_the_dungeon) {
      HostSession host;
      Assert::That(host.Start(0, 4242), Is().True());
      JoinSession join;
      Assert::That(join.Connect("127.0.0.1", host.server.GetPort()), Is().True());
      Assert::That(Pump(host, join, 0.0f, [&] { return join.ready; }), Is().True());
      Assert::That(join.dungeon.cells_, Is().EqualTo(host.dungeon.cells_));
    };
  };
};

Describe(SnapshotRoundTrip) {
  Describe(a_moving_joiner) {
    It(mirrors_its_position_back_to_the_host_world) {
      HostSession host;
      Assert::That(host.Start(0, 77), Is().True());
      JoinSession join;
      Assert::That(join.Connect("127.0.0.1", host.server.GetPort()), Is().True());
      Assert::That(Pump(host, join, 1.0f, [&] { return join.ready; }), Is().True());
      // Session sends an absolute authoritative position; keep feeding a
      // rising one and the host mirrors it.
      // (px would start at join.myX if driving; spec teleports to a floor cell instead.)
      // Pick any floor cell far from the joiner's start -- the authority
      // model only requires the host accept a legal position, so distance is
      // the point of the test (it proves join-> Tick is absolute, not motion).
      const Dungeon &d = join.dungeon;
      const int sc = static_cast<int>(join.myX / 48);
      const int sr = static_cast<int>(join.myY / 48);
      int tc = sc, tr = sr;
      float best = -1.0f;
      for (int rr = 1; rr < d.rows_ - 1; ++rr)
          for (int cc = 1; cc < d.cols_ - 1; ++cc) {
              if (d.IsSolid(cc, rr)) continue;
              float dx = cc - sc, dy = rr - sr;
              float dist = dx * dx + dy * dy;
              if (dist > best) { best = dist; tc = cc; tr = rr; }
          }
      Assert::That(best, Is().GreaterThan(0.0f));
      const float targetX = (tc + 0.5f) * 48;
      const float targetY = (tr + 0.5f) * 48;
      Assert::That(d.IsSolid(tc, tr), Is().False());
      uint32_t start = NetNowMs();
      while (NetNowMs() - start < (uint32_t)3000) {
          host.Tick(1.0 / 60.0);
          join.Tick(1.0 / 60.0, targetX, targetY, 1.0f, 0.0f, 0);
          if (std::abs(host.world.joinX - targetX) < 0.5f) break;
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      Assert::That(std::abs(host.world.joinX - targetX), Is().LessThan(0.5f));
      Assert::That(host.world.joinY, Is().EqualTo(targetY));
    };
  };
  Describe(host_world_data) {
    It(reaches_the_joiner_as_a_monster_list) {
      HostSession host;
      Assert::That(host.Start(0, 77), Is().True());
      JoinSession join;
      Assert::That(join.Connect("127.0.0.1", host.server.GetPort()), Is().True());
      Assert::That(Pump(host, join, 0.0f, [&] { return join.ready && !join.world.monsters.empty(); }),
                   Is().True());
      Assert::That(join.world.monsters.empty(), Is().False());
    };
  };
  Describe(the_moving_host) {
    It(delivers_the_host_position_to_the_client) {
      HostSession host;
      Assert::That(host.Start(0, 77), Is().True());
      JoinSession join;
      Assert::That(join.Connect("127.0.0.1", host.server.GetPort()), Is().True());
      Assert::That(Pump(host, join, 0.0f, [&] { return join.ready; }), Is().True());
      host.world.hostX += 200.0f;
      Assert::That(Pump(host, join, 0.0f, [&] { return join.world.hostX > 0.0f; }), Is().True());
      Assert::That(join.world.hostX, Is().GreaterThan(0.0f));
    };
  };
  Describe(the_joiner_spawn_cell) {
    // Regression: host and joiner used to compute the joiner's start cell
    // independently. Divergence meant the client spawned in a wall while the
    // host rendered them next to the host. Both sides must agree, on floor.
    It(is_identical_on_both_sides_and_on_floor) {
      HostSession host;
      Assert::That(host.Start(0, 55), Is().True());
      JoinSession join;
      Assert::That(join.Connect("127.0.0.1", host.server.GetPort()), Is().True());
      Assert::That(Pump(host, join, 0.0f, [&] { return join.ready; }), Is().True());
      Assert::That(join.myX, Is().EqualTo(host.world.joinX));
      Assert::That(join.myY, Is().EqualTo(host.world.joinY));
      Assert::That(join.dungeon.IsSolid(static_cast<int>(join.myX / 48),
                                        static_cast<int>(join.myY / 48)), Is().False());
    };
  };
};
