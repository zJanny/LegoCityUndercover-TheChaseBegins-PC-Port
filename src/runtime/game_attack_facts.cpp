#include "game_level.hpp"
#include "runtime/attack_facts.hpp"

namespace game_level {
using namespace chase;

void LevelSession::installAttackFacts() {
    attackTables.lineOfSight = [this](const runtime::AttackBody& attacker, const std::vector<runtime::AttackCandidate>& candidates, std::size_t target) {
        return runtime::attackLineClear(collision, attacker, candidates, target);
    };
}

}
