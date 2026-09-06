#include <igloo/igloo_alt.h>

#include "../src/game/world.h"

using namespace igloo;
using namespace storm;

Describe(SpawningTheWorld) {
  Describe(a_fresh_spawn) {
    Dungeon d = Generate(7);
    WorldState w = SpawnWorld(d);
    It(places_the_players_in_the_first_room) {
      const Room &r = d.rooms_[0];
      Assert::That(w.hostX, Is().EqualTo(r.cx() * kWorldTile));
      Assert::That(w.hostY, Is().EqualTo(r.cy() * kWorldTile));
    };
    It(arms_the_lamp_with_the_host) {
      Dungeon d2 = Generate(7);
      WorldState w2 = SpawnWorld(d2);
      Assert::That(w2.lampCarrier, Is().EqualTo(1));
    };
    It(spawns_monsters) {
      Assert::That(w.monsters.size(), Is().GreaterThan(0));
    };
    It(spawns_the_joiner_on_floor) {
      Dungeon d = Generate(77);
      WorldState w = SpawnWorld(d);
      int jc = static_cast<int>(w.joinX / kWorldTile);
      int jr = static_cast<int>(w.joinY / kWorldTile);
      Assert::That(d.IsSolid(jc, jr), Is().False());
    };
  };
};

Describe(MonsterSteeringRules) {
  Describe(a_monster_far_from_both_players) {
    It(does_not_move) {
      Monster m{2000.0f, 2000.0f};
      WorldState w;
      w.hostX = 0; w.hostY = 0; w.joinX = 48; w.joinY = 48;
      SteerMonster(m, 0.0f, 0.0f, 60.0f, 1.0f);
      Assert::That(m.x, Is().EqualTo(2000.0f));
    };
  };
  Describe(a_monster_next_to_a_player) {
    It(closes_the_gap) {
      Monster m{100.0f, 0.0f};
      SteerMonster(m, 40.0f, 0.0f, 60.0f, 1.0f);
      Assert::That(m.x, Is().LessThan(100.0f));
    };
  };
};

Describe(Chests) {
  Describe(opening_a_closed_chest) {
    It(grants_ammo_and_marks_it_open) {
      WorldState w;
      w.chests.push_back(Chest{0, 0, false, 0});
      Assert::That(TryOpenChest(w, 0) , Is().EqualTo(6));
      Assert::That(w.chests[0].opened, Is().True());
    };
  };
  Describe(opening_the_same_chest_twice) {
    It(grants_nothing_the_second_time) {
      WorldState w;
      w.chests.push_back(Chest{0, 0, false, 0});
      TryOpenChest(w, 0);
      Assert::That(TryOpenChest(w, 0), Is().EqualTo(0));
    };
  };
  Describe(an_out_of_range_chest_id) {
    It(does_nothing) {
      WorldState w;
      Assert::That(TryOpenChest(w, 99), Is().EqualTo(0));
    };
  };
};

Describe(TheLamp) {
  Describe(dropped_by_the_carrier) {
    It(lands_on_the_world_at_the_carrier) {
      WorldState w;
      w.hostX = 100; w.hostY = 50;
      DropLamp(w, 1);
      Assert::That(w.lampCarrier, Is().EqualTo(0));
      Assert::That(w.lampX, Is().EqualTo(100.0f));
    };
  };
  Describe(dropped_by_someone_who_is_not_carrying) {
    It(stays_where_it_was) {
      WorldState w;
      w.lampX = 33; w.lampY = 44;
      DropLamp(w, 2);  // host has it
      Assert::That(w.lampCarrier, Is().EqualTo(1));
      Assert::That(w.lampX, Is().EqualTo(33.0f));
    };
  };
  Describe(picked_up_in_reach) {
    It(switches_the_carrier) {
      WorldState w;
      w.lampCarrier = 0;
      w.lampX = 100; w.lampY = 100;
      w.joinX = 120; w.joinY = 100;
      Assert::That(TryPickUpLamp(w, 2), Is().True());
      Assert::That(w.lampCarrier, Is().EqualTo(2));
    };
    It(refuses_a_carried_or_distant_lamp) {
      WorldState w;
      w.lampX = 9000; w.lampY = 9000; w.joinX = 0; w.joinY = 0;
      Assert::That(TryPickUpLamp(w, 2), Is().False());  // out of reach
      w.lampCarrier = 1;
      Assert::That(w.lampCarrier, Is().EqualTo(1));
    };
  };
  Describe(opening_a_chest_uses_reach) {
    It(finds_only_a_closed_chest_in_range) {
      WorldState w;
      w.chests.push_back(Chest{0, 0, true, 0});
      w.chests.push_back(Chest{40, 0, false, 1});
      Assert::That(ChestInReach(w, 20, 0), Is().EqualTo(1));
      Assert::That(ChestInReach(w, 900, 900), Is().EqualTo(-1));
    };
  };
};
