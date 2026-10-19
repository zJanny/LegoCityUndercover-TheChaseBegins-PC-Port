#include "game_stud_drops.hpp"

namespace game_stud_drops {
namespace runtime = chase::runtime;

void LevelStudDrops::load(runtime::GameRandom& random) { field_.emplace(random, planes_); }

std::optional<runtime::StudDropMode> LevelStudDrops::drop(const std::optional<runtime::StudDropRequest>& request, const runtime::DebrisLineTest& wall,
                                                          const runtime::DebrisLineTest& ground) {
    if (!request || !field_) return std::nullopt;
    return field_->drop(*request, wall, ground);
}

std::vector<runtime::SoundCall> LevelStudDrops::update(float frameDelta, const std::array<float, 3>& pickupPoint,
                                                       const std::optional<std::array<float, 3>>& flightTarget, runtime::StudField& counter) {
    std::vector<runtime::SoundCall> calls;
    if (!field_) return calls;
    for (const runtime::StudDropSound& sound : field_->update(frameDelta, pickupPoint, flightTarget, counter).sounds)
        calls.push_back({sound.id, sound.position});
    return calls;
}

void LevelStudDrops::appendSprites(std::vector<chase::render::StudSprite>& sprites) const {
    if (!field_) return;
    for (unsigned i = 0; i < runtime::kStudDropRecordCount; ++i) {
        const runtime::StudDropRecord& record = field_->records()[i];
        if (record.type >= runtime::kStudDropHeartType || !field_->drawn(i)) continue;
        sprites.push_back({{record.position[0], record.position[1], record.position[2]}, runtime::kStudTint[record.type]});
    }
}

}
