#pragma once
// The pedestrians' vehicle test and room layer table in chase-game (runtime/pedestrian_blocked.hpp, runtime/room_layers.hpp): the
// vehicle list 0x5863b4 + 0x305c, the dodge ray 0x22b108 on the collision world and the set-up table *0x376ad4.
#include "runtime/collision.hpp"
#include "runtime/game_random.hpp"
#include "runtime/pedestrian_blocked.hpp"
#include "runtime/pedestrian_system.hpp"

#include <vector>

namespace game_pedestrian_blocking {

namespace runtime = chase::runtime;

struct Blocking {
    std::vector<runtime::PedestrianBlockVehicle> vehicles;
    const runtime::CollisionWorld* collision = nullptr;
};

// The manager update: the spawn test 0x26cd38 and the layer table. STAND-IN PED-S10: the set-up table also during a cutscene, whose
// start pushes the cutscene table (0x196734, 0x16c060) and whose end pops it (0x2fae68).
void wireSystemStep(runtime::PedestrianSystemStep& step, const Blocking* blocking, const runtime::PedestrianSystem& system,
                    runtime::GameRandom& random);
// The characters' walk: the vehicle loop 0x2b9340 .. 0x2b93a4 with the dodge ray.
void wireCharacterStep(runtime::PedestrianCharacterStep& step, const Blocking* blocking, const runtime::PedestrianSystem& system,
                       runtime::GameRandom& random);

}
