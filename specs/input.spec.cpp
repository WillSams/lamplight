#include <igloo/igloo_alt.h>

#include "../src/input/input.h"

using namespace igloo;
using namespace storm;

Describe(MoveDirectionFor) {
  Describe(keyboard_only) {
    It(moves_left) {
      MoveInput in;
      in.left = true;
      Assert::That(MoveDirection(in).x, Is().LessThan(0));
    };
  };
  Describe(a_diagonal) {
    It(is_normalised_not_doubled) {
      MoveInput in;
      in.right = true;
      in.down = true;
      Assert::That(glm::length(MoveDirection(in)), Is().LessThanOrEqualTo(1.0f + 0.0001f));
    };
  };
  Describe(stick_and_dpad_together) {
    It(ignores_the_dpad_while_the_stick_is_live) {
      MoveInput in;
      in.stickX = 1.0f;
      in.dpadLeft = true;
      Assert::That(MoveDirection(in).x, Is().GreaterThan(0));
    };
  };
};

Describe(FacingResolvedFrom) {
  Describe(the_right_stick_aimed) {
    It(wins_over_movement) {
      MoveInput in;
      in.stickX = 1;
      glm::vec2 f = FacingFrom(glm::vec2(1, 0), 0, 1, glm::vec2(1, 0));
      Assert::That(f.y, Is().GreaterThan(0.9f));
    };
  };
  Describe(no_aim_and_no_move) {
    It(keeps_the_previous_facing) {
      MoveInput in;
      glm::vec2 f = FacingFrom(glm::vec2(0, 0), 0, 0, glm::vec2(1, 0));
      Assert::That(f.x, Is().EqualTo(1.0f));
    };
  };
};
