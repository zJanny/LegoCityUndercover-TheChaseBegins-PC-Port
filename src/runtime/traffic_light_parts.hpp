#pragma once
// The look of the traffic lights (docs/re/traffic-junction.md): the node parts red, yellow and green of props/prop_trafficlight,
// hidden at the light's create 0x1afae8 and changed by 0x20b19c(light, new, old) from the junction set-up 0x1ca5a4 (with its slot index
// quirk) and from the light cycle 0x1d8924. Pure steps on the light pairs of a junction; the part flags are the bitset at node +0x114 that
// 0x2363d4(node, part, show) writes. No SDL, no rendering.
#include "runtime/traffic_junction_fill.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::size_t kTrafficLightPartCount = 3;
constexpr std::array<const char*, kTrafficLightPartCount> kTrafficLightPartNames = {"red", "yellow", "green"};  // by phase: 0x20b19c

// One 0x2363d4(node, part, show) of the light `member` (0 the light, 1 its slave) of `pair`. A part name is found case-insensitively (0x25ad90).
struct TrafficLightPartOp {
    std::size_t pair = 0;
    std::size_t member = 0;
    std::uint8_t part = 0;
    bool show = false;
};

// 0x20b19c(light, phase, previous) for each light of `pair` that exists (0x1d89c8 loop): hide the part of `previous`, show the part of `phase`.
std::vector<TrafficLightPartOp> trafficLightPhaseOps(const std::vector<std::array<std::optional<TrafficLight>, 2>>& lightPairs, std::size_t pair,
                                                     std::uint8_t phase, std::uint8_t previous);

// The part changes of 0x1ca5a4 after the pairs are found. `slots` holds the 0-based slot of each pair in order. For each found pair n at slot s
// 0x20b19c(word[2 n + s], red, red) runs twice, where word[w] is the light of pair w / 2 (member w % 2) when that pair is already written and
// the light exists, else 0 and skipped (0x1ca860: the index is the slot, not the pair). Then both lights of pair 0 get (green, red).
// Throws when `slots` and `lightPairs` differ in size or a slot is not in 0..7.
std::vector<TrafficLightPartOp> trafficLightSetupOps(const std::vector<std::array<std::optional<TrafficLight>, 2>>& lightPairs,
                                                     const std::vector<std::size_t>& slots);

// The part flags of the lights of one junction: bit p set = part p hidden. A light starts with all three hidden (0x1afae8).
class TrafficJunctionLightParts {
public:
    explicit TrafficJunctionLightParts(std::size_t pairs) : hidden_(pairs, {kAllHidden, kAllHidden}) {}
    void apply(const TrafficLightPartOp& op);
    void apply(const std::vector<TrafficLightPartOp>& ops);
    bool shown(std::size_t pair, std::size_t member, std::uint8_t part) const;
    std::size_t pairs() const { return hidden_.size(); }

private:
    static constexpr std::uint8_t kAllHidden = 7;
    std::vector<std::array<std::uint8_t, 2>> hidden_;
};

// The index of the node of `names` that 0x23645c finds for `part` (case-insensitive, first match), or -1.
std::ptrdiff_t trafficLightPartNode(const std::vector<std::string>& names, std::uint8_t part);

}
