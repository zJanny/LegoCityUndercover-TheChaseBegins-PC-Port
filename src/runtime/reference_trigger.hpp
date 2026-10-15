#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace chase::runtime {

enum class ReferenceKind : std::uint8_t { Idle = 0, Registered = 1, Fired = 2 };

struct ReferenceActivation {
    bool fired = false;       // every reference to the target is active: the target is enabled, messaged and its ObjectTriggered raised
    bool joinWaiting = false; // another reference to the same target is still idle
    std::string target;       // lower case, empty when the reference is unset
    std::vector<std::size_t> owners;  // references switched to Fired (the original sends them message 0xff)
};

// The reference-trigger step 0x23211c in activate mode (r2 = 1). Each entry is one reference parameter (owner instance, referenced instance).
class ReferenceTriggers {
public:
    void clear() { entries_.clear(); }
    std::size_t add(const std::string& lowerOwner, const std::string& lowerTarget);
    // Returns nothing when the reference is already Registered (0x2321b4). Otherwise the reference becomes Registered; with a target set,
    // the target fires only when no other reference to it is Idle (0x232234); then all references to it become Fired.
    ReferenceActivation activate(std::size_t id);
    ReferenceKind kind(std::size_t id) const { return entries_[id].kind; }
    std::size_t size() const { return entries_.size(); }

private:
    struct Entry {
        std::string owner;
        std::string target;
        ReferenceKind kind = ReferenceKind::Idle;
    };
    std::vector<Entry> entries_;
};

}
