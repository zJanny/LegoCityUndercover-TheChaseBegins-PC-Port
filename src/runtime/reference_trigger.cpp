#include "runtime/reference_trigger.hpp"

namespace chase::runtime {

std::size_t ReferenceTriggers::add(const std::string& lowerOwner, const std::string& lowerTarget) {
    entries_.push_back({lowerOwner, lowerTarget, ReferenceKind::Idle});
    return entries_.size() - 1;
}

ReferenceActivation ReferenceTriggers::activate(std::size_t id) {
    ReferenceActivation result;
    Entry& entry = entries_[id];
    if (entry.kind == ReferenceKind::Registered) return result;
    entry.kind = ReferenceKind::Registered;
    result.target = entry.target;
    if (entry.target.empty()) return result;
    for (const Entry& other : entries_)
        if (other.target == entry.target && other.kind == ReferenceKind::Idle) {
            result.joinWaiting = true;
            return result;
        }
    for (std::size_t i = 0; i < entries_.size(); ++i)
        if (entries_[i].target == entry.target) {
            entries_[i].kind = ReferenceKind::Fired;
            result.owners.push_back(i);
        }
    result.fired = true;
    return result;
}

}
