#include "runtime/script_triggers.hpp"

#include <map>

namespace chase::runtime {

TriggerEvent classifyTriggerEvent(const std::string& lowerCaseName) {
    static const std::map<std::string, TriggerEvent> events = {
        {"boundentered", TriggerEvent::BoundEntered},
        {"boundbothentered", TriggerEvent::BoundEntered},
        {"player1boundentered", TriggerEvent::BoundEntered},
        {"boundleft", TriggerEvent::BoundLeft},
        {"boundinside", TriggerEvent::BoundInside},
        {"boundoutside", TriggerEvent::BoundOutside},
        {"playeronfootboundentered", TriggerEvent::PlayerOnFootBoundEntered},
        {"playervehicleboundentered", TriggerEvent::PlayerVehicleBoundEntered},
        {"player2boundentered", TriggerEvent::Player2BoundEntered},
        {"objectboundentered", TriggerEvent::ObjectBoundEntered},
        {"objectboundleft", TriggerEvent::ObjectBoundLeft},
        {"objectboundinside", TriggerEvent::ObjectBoundInside},
    };
    const auto found = events.find(lowerCaseName);
    return found == events.end() ? TriggerEvent::Unmodelled : found->second;
}

namespace {

bool settle(TriggerRecord& record, bool state, bool fire) {
    record.inside = state;
    record.skip = false;
    return fire;
}

}

bool applyTriggerCheck(TriggerEvent event, TriggerRecord& record, const TriggerSample& sample) {
    const bool wasInside = record.inside;
    const bool ready = !record.skip;
    switch (event) {
    case TriggerEvent::BoundEntered:
        return settle(record, sample.playerInside, ready && sample.playerInside && !wasInside);
    case TriggerEvent::PlayerOnFootBoundEntered:
        if (sample.playerInVehicle) return false;
        return settle(record, sample.playerInside, ready && sample.playerInside && !wasInside);
    case TriggerEvent::PlayerVehicleBoundEntered:
        if (!sample.playerInVehicle) return false;
        return settle(record, sample.playerInside, ready && sample.playerInside && !wasInside);
    case TriggerEvent::BoundLeft:
        return settle(record, sample.playerInside, ready && !sample.playerInside && wasInside);
    case TriggerEvent::BoundInside:
        return settle(record, sample.playerInside, sample.playerInside);
    case TriggerEvent::BoundOutside:
        return settle(record, sample.playerInside, !sample.playerInside);
    case TriggerEvent::Player2BoundEntered:
        return settle(record, false, false);
    case TriggerEvent::ObjectBoundEntered:
        if (!sample.objectUsable) return false;
        return settle(record, sample.objectInside, ready && sample.objectInside && !wasInside);
    case TriggerEvent::ObjectBoundLeft:
        if (!sample.objectUsable) return false;
        return settle(record, !sample.objectInside, ready && !sample.objectInside && !wasInside);
    case TriggerEvent::ObjectBoundInside:
        if (!sample.objectUsable) return false;
        return settle(record, sample.objectInside, ready && sample.objectInside);
    case TriggerEvent::Unmodelled:
        return false;
    }
    return false;
}

}
