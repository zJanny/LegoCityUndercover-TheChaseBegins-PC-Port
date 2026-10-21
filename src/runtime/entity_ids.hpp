#pragma once
// The entity id u16 at entity +0x14, the owner key of the sounds an object plays and stops (0x23ae84, 0x23ae50, StopSoundGO 0x1d0d28)
// and of the door sounds: the object loader 0x1a4130 stores the scene's entity count (scene +0x1c) in it and appends the entity to the
// scene's list (+0x24), so the id is the creation index. The scene load 0x17e898 starts the count at 0 and creates the level file as
// the root (id 0); each object follows its parent depth first, the objects of its class file after it under "Parent.Child".
// Evidence: docs/re/script-natives-audio.md. No SDL dependency.
#include "assets/assets.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>

namespace chase::runtime {

class SceneEntityIds {
public:
    SceneEntityIds() = default;
    // An object record with byte +0xc bit 0 is never created, one with bit 1 only in the scene at 0x589a1c (`defaultScene`).
    // STAND-IN AUDIO-S36: every other object counts; class creates that return no entity and the loader filter *0x585924 are not modelled.
    // The absolute ids are unverified: the oracle's id of 1-8_Zip_Line05 and its lvl_01 entity count disagree with them (AUDIO-37).
    SceneEntityIds(const assets::Level& level, const assets::ClassLookup& classOf, bool defaultScene);

    std::optional<std::uint16_t> find(const std::string& lowerCaseName) const;
    std::size_t count() const { return count_; }

private:
    std::map<std::string, std::uint16_t> ids_;
    std::size_t count_ = 0;
};

}
