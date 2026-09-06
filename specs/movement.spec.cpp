#include <igloo/igloo_alt.h>

#include <glm/glm.hpp>

#include "../src/arena/camera.h"
#include "../src/arena/collision.h"

using namespace igloo;
using namespace storm;

Describe(Camera) {
  Describe(centred_on_a_target) {
    It(stays_inside_the_world) {
      CameraRect c = CameraFor(glm::vec2(1500.0f, 1500.0f), 800, 600,
                               glm::vec2(64 * 48.0f, 64 * 48.0f));
      Assert::That(c.x, Is().GreaterThanOrEqualTo(0));
      Assert::That(c.y, Is().GreaterThanOrEqualTo(0));
      Assert::That(c.x + c.w, Is().LessThanOrEqualTo(64 * 48));
      Assert::That(c.y + c.h, Is().LessThanOrEqualTo(64 * 48));
    };
  };
  Describe(target_pinned_to_a_corner) {
    It(pins_the_camera_to_zero) {
      CameraRect c = CameraFor(glm::vec2(0.0f, 0.0f), 800, 600,
                               glm::vec2(64 * 48.0f, 64 * 48.0f));
      Assert::That(c.x, Is().EqualTo(0));
      Assert::That(c.y, Is().EqualTo(0));
    };
  };
  Describe(a_world_smaller_than_the_view) {
    It(does_not_scroll) {
      CameraRect c = CameraFor(glm::vec2(400.0f, 300.0f), 800, 600,
                               glm::vec2(400.0f, 300.0f));
      Assert::That(c.x, Is().EqualTo(0));
      Assert::That(c.y, Is().EqualTo(0));
    };
  };
};

Describe(GridCollision) {
  // Wall column at x >= 96px, all walkable rows otherwise. Solid in pixel
  // space, like the game's lambda.
  static bool WallFn(float x, float y) {
    (void)y;
    return x >= 96.0f && x < 192.0f;
  };
  Describe(sliding_along_a_wall) {
    It(lets_the_free_axis_advance) {
      glm::vec2 pos(50.0f, 50.0f);
      pos = ResolveGridMove(pos, glm::vec2(20.0f, 20.0f),
                            glm::vec2(100.0f, 0.0f), WallFn);  // x blocked
      pos = ResolveGridMove(pos, glm::vec2(20.0f, 20.0f),
                            glm::vec2(0.0f, 30.0f), WallFn);   // y free
      Assert::That(pos.x, Is().LessThan(96.0f));
      Assert::That(pos.y, Is().EqualTo(80.0f));
    };
  };
  Describe(crossing_an_empty_floor) {
    It(moves_freely) {
      glm::vec2 pos = ResolveGridMove(glm::vec2(0.0f, 400.0f),
                                      glm::vec2(20.0f, 20.0f),
                                      glm::vec2(10.0f, 10.0f), WallFn);
      Assert::That(pos.x, Is().EqualTo(10.0f));
      Assert::That(pos.y, Is().EqualTo(410.0f));
    };
  };
};
