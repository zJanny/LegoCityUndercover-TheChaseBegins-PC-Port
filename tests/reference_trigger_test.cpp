// Reference-trigger step 0x23211c in activate mode (src/runtime/reference_trigger.cpp). Values: docs/re/buildit.md (oracle ctrace of
// 0x23cee8 over the build of 1-1_BI_DonutVanParts: the call 0x2321dc with the build-it as object and instigator, then the call 0x23243c
// with the target as object and the build-it as instigator) and the disassembly of 0x23211c.
#include "runtime/reference_trigger.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main() {
    try {
        {
            ReferenceTriggers refs;
            const std::size_t only = refs.add("build", "van_fixed");
            const ReferenceActivation first = refs.activate(only);
            require(first.fired && first.target == "van_fixed" && first.owners.size() == 1, "a single reference fires its target");
            require(refs.kind(only) == ReferenceKind::Fired, "the reference becomes Fired");
            require(refs.activate(only).fired, "a Fired reference activates again (only kind 1 returns early)");
        }
        {
            ReferenceTriggers refs;
            const std::size_t a = refs.add("build_a", "gate");
            const std::size_t b = refs.add("build_b", "gate");
            const ReferenceActivation first = refs.activate(a);
            require(!first.fired && first.joinWaiting, "the target waits for the other reference (0x232234)");
            require(refs.kind(a) == ReferenceKind::Registered, "the waiting reference is Registered");
            const ReferenceActivation again = refs.activate(a);
            require(!again.fired && !again.joinWaiting, "a Registered reference returns at once (0x2321b4)");
            const ReferenceActivation second = refs.activate(b);
            require(second.fired && second.owners.size() == 2, "the last reference fires the target and marks both Fired");
        }
        {
            ReferenceTriggers refs;
            const std::size_t unset = refs.add("build", "");
            const ReferenceActivation result = refs.activate(unset);
            require(!result.fired && !result.joinWaiting, "an unset reference fires nothing");
            require(refs.kind(unset) == ReferenceKind::Registered, "an unset reference is still Registered");
        }
    } catch (const std::exception& error) {
        std::cerr << "reference-trigger-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "reference-trigger-test: ok\n";
    return 0;
}
