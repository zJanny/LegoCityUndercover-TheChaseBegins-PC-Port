#include "runtime/traffic_light_parts.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace chase::runtime {
namespace {

using LightPairs = std::vector<std::array<std::optional<TrafficLight>, 2>>;

bool exists(const LightPairs& lightPairs, std::size_t word) {
    const std::size_t pair = word / 2;
    return pair < lightPairs.size() && lightPairs[pair][word % 2].has_value();
}

void phaseOps(std::vector<TrafficLightPartOp>& ops, std::size_t pair, std::size_t member, std::uint8_t phase, std::uint8_t previous) {
    ops.push_back({pair, member, previous, false});
    ops.push_back({pair, member, phase, true});
}

bool equalNoCase(const std::string& a, const char* b) {
    std::size_t i = 0;
    for (; i < a.size() && b[i] != 0; ++i)
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) return false;
    return i == a.size() && b[i] == 0;
}

}

std::vector<TrafficLightPartOp> trafficLightPhaseOps(const LightPairs& lightPairs, std::size_t pair, std::uint8_t phase, std::uint8_t previous) {
    if (phase >= kTrafficLightPartCount || previous >= kTrafficLightPartCount) throw std::runtime_error("traffic light: phase outside 0..2");
    std::vector<TrafficLightPartOp> ops;
    if (pair >= lightPairs.size()) return ops;
    for (std::size_t member = 0; member < 2; ++member)
        if (lightPairs[pair][member]) phaseOps(ops, pair, member, phase, previous);
    return ops;
}

std::vector<TrafficLightPartOp> trafficLightSetupOps(const LightPairs& lightPairs, const std::vector<std::size_t>& slots) {
    if (slots.size() != lightPairs.size()) throw std::runtime_error("traffic light: slots and pairs differ in size");
    std::vector<TrafficLightPartOp> ops;
    for (std::size_t n = 0; n < slots.size(); ++n) {
        if (slots[n] >= kTrafficJunctionLightSlots) throw std::runtime_error("traffic light: slot outside 0..7");
        const std::size_t word = 2 * n + slots[n];
        if (word >= 2 * (n + 1) || !exists(lightPairs, word)) continue;
        for (int pass = 0; pass < 2; ++pass) phaseOps(ops, word / 2, word % 2, kTrafficLightRed, kTrafficLightRed);
    }
    if (!lightPairs.empty())
        for (std::size_t member = 0; member < 2; ++member)
            if (lightPairs[0][member]) phaseOps(ops, 0, member, kTrafficLightGreen, kTrafficLightRed);
    return ops;
}

void TrafficJunctionLightParts::apply(const TrafficLightPartOp& op) {
    if (op.pair >= hidden_.size() || op.member >= 2 || op.part >= kTrafficLightPartCount) throw std::out_of_range("traffic light part op");
    std::uint8_t& flags = hidden_[op.pair][op.member];
    const std::uint8_t bit = static_cast<std::uint8_t>(1u << op.part);
    flags = op.show ? static_cast<std::uint8_t>(flags & ~bit) : static_cast<std::uint8_t>(flags | bit);
}

void TrafficJunctionLightParts::apply(const std::vector<TrafficLightPartOp>& ops) {
    for (const TrafficLightPartOp& op : ops) apply(op);
}

bool TrafficJunctionLightParts::shown(std::size_t pair, std::size_t member, std::uint8_t part) const {
    return (hidden_.at(pair).at(member) & (1u << part)) == 0;
}

void TrafficLightPartStates::add(const TrafficJunctionFill& fill) {
    TrafficJunctionLightParts parts(fill.record.lightPairs.size());
    parts.apply(trafficLightSetupOps(fill.record.lightPairs, fill.lightSlots));
    junctions_.push_back({fill.record.lightPairs, std::move(parts)});
}

void TrafficLightPartStates::applyChange(std::size_t junction, const TrafficLightChange& change) {
    Junction& target = junctions_.at(junction);
    target.parts.apply(trafficLightPhaseOps(target.pairs, change.pair, change.phase, change.previous));
}

bool TrafficLightPartStates::shown(std::size_t junction, std::size_t pair, std::size_t member, std::uint8_t part) const {
    return junctions_.at(junction).parts.shown(pair, member, part);
}

std::ptrdiff_t trafficLightPartNode(const std::vector<std::string>& names, std::uint8_t part) {
    if (part >= kTrafficLightPartCount) return -1;
    for (std::size_t i = 0; i < names.size(); ++i)
        if (equalNoCase(names[i], kTrafficLightPartNames[part])) return static_cast<std::ptrdiff_t>(i);
    return -1;
}

}
