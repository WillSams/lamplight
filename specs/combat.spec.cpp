#include <igloo/igloo_alt.h>

#include "../src/combat/hits.h"

using namespace igloo;
using namespace storm;

Describe(MeleeArc) {
  Describe(a_target_straight_ahead_in_range) {
    It(is_hit) {
      Assert::That(MeleeHits(0, 0, 1, 0, 30, 0), Is().True());
    };
  };
  Describe(a_target_behind_the_attacker) {
    It(is_not_hit) {
      Assert::That(MeleeHits(0, 0, 1, 0, -30, 0), Is().False());
    };
  };
  Describe(a_target_past_range) {
    It(is_not_hit) {
      Assert::That(MeleeHits(0, 0, 1, 0, 300, 0), Is().False());
    };
  };
  Describe(a_target_off_the_arc) {
    It(is_not_hit) {
      // 90 degrees off the facing, way outside cosHalf=0.5 (60 deg half-angle).
      Assert::That(MeleeHits(0, 0, 1, 0, 0, 30), Is().False());
    };
  };
};

Describe(RangedTrace) {
  Describe(a_target_on_the_ray) {
    It(is_hit) {
      Assert::That(RangedHits(0, 0, 1, 0, 200, 0), Is().True());
    };
  };
  Describe(a_target_beside_the_ray) {
    It(is_not_hit) {
      Assert::That(RangedHits(0, 0, 1, 0, 200, 60), Is().False());
    };
  };
  Describe(a_shot_with_no_direction) {
    It(is_not_a_hit) {
      Assert::That(RangedHits(0, 0, 0, 0, 50, 0), Is().False());
    };
  };
  Describe(a_target_beyond_max_distance) {
    It(is_not_hit) {
      Assert::That(RangedHits(0, 0, 1, 0, 400, 0), Is().False());
    };
  };
};
