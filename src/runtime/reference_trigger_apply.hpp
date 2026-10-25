#pragma once
// What the reference-trigger step 0x23211c does to the scripts (reference_trigger.hpp): the events go to the trigger records of the runner, the
// messages to its message queue, the enable clears the disabled bit. No SDL dependency.

#include "runtime/reference_trigger.hpp"
#include "runtime/script_runner.hpp"

#include <cstdint>
#include <vector>

namespace chase::runtime {

struct ReferenceNavNodeRequest {
    std::uint8_t node = 0;
    bool active = false;
};

// Performs the effects in order. STAND-IN SCRIPT-S289: the nav node requests (0x22c470 sets bit 15 of the node's halfword +0x34 to the inverse of
// `active`) are returned and no caller applies them, because the port has no nav node flags.
std::vector<ReferenceNavNodeRequest> applyReferenceActivation(const ReferenceActivation& activation, ScriptRunner& scripts);

}
