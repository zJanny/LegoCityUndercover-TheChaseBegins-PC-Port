#pragma once
// Entities a cutscene holds while it plays. Cutscene setup 0x2d1524 enables a disabled actor entity and shows its node (0x23f734), and
// saves the node's 64-byte local matrix (0x22c2ec); the teardown 0x2d135c puts the matrix back (0x241540) and disables the entity
// again. Evidence: docs/re/cutscene-actors.md. No SDL dependency.
#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace chase::runtime {

class CutsceneHold {
public:
    using Node = std::array<float, 16>;

    struct Released {
        bool held = false;
        std::optional<Node> node;
    };

    void begin(std::size_t entity);
    void begin(std::size_t entity, const Node& node);
    bool holds(std::size_t entity) const;
    Released end(std::size_t entity);

private:
    struct Entry {
        std::size_t entity;
        std::optional<Node> node;
    };
    std::vector<Entry> held_;
};

bool drawnWhileHeld(bool drawnByState, bool held);

}
