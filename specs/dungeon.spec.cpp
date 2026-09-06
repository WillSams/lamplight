#include <igloo/igloo_alt.h>

#include "../src/dungeon/generate.h"

using namespace igloo;
using namespace storm;

// Same seed, same dungeon -- the property the whole netcode rests on.
// A divergence here is silent: both players walk different dungeons and
// never find out.
Describe(DungeonGenerator) {
  Describe(two_runs_on_one_seed) {
    It(produce_identical_grids) {
      Dungeon a = Generate(12345);
      Dungeon b = Generate(12345);
      Assert::That(a.cells_ == b.cells_, Is().True());
      Assert::That(a.rooms_.size() == b.rooms_.size(), Is().True());
    };
    It(produce_identical_rooms) {
      Dungeon a = Generate(12345);
      Dungeon b = Generate(12345);
      for (size_t i = 0; i < a.rooms_.size(); ++i) {
        Assert::That(a.rooms_[i].x, Is().EqualTo(b.rooms_[i].x));
        Assert::That(a.rooms_[i].y, Is().EqualTo(b.rooms_[i].y));
        Assert::That(a.rooms_[i].w, Is().EqualTo(b.rooms_[i].w));
        Assert::That(a.rooms_[i].h, Is().EqualTo(b.rooms_[i].h));
      }
    };
  };

  Describe(two_different_seeds) {
    It(produce_different_grids) {
      Dungeon a = Generate(12345);
      Dungeon b = Generate(999);
      Assert::That(a.cells_ != b.cells_, Is().True());
    };
  };

  Describe(a_generated_dungeon) {
    It(has_every_floor_reachable) {
      Dungeon d = Generate(12345);
      Assert::That(DungeonConnected(d), Is().True());
    };
    It(has_no_overlapping_rooms) {
      Dungeon d = Generate(12345);
      for (size_t i = 0; i < d.rooms_.size(); ++i)
        for (size_t j = i + 1; j < d.rooms_.size(); ++j) {
          const Room &r = d.rooms_[i];
          const Room &s = d.rooms_[j];
          const bool overlap = r.x < s.x + s.w && r.x + r.w > s.x &&
                               r.y < s.y + s.h && r.y + r.h > s.y;
          Assert::That(overlap, Is().False());
        }
    };
    It(reports_out_of_bounds_as_solid) {
      Dungeon d = Generate(12345);
      Assert::That(d.IsSolid(-1, 0), Is().True());
      Assert::That(d.IsSolid(0, -1), Is().True());
      Assert::That(d.IsSolid(d.cols_, 0), Is().True());
      Assert::That(d.IsSolid(0, d.rows_), Is().True());
    };
  };
};
