// The final prop's scene transform belongs to the build-it only while the prop rises (src/runtime/final_prop_rise.cpp). Rule: the height of
// docs/re/buildit.md is non-zero from the check of state 10 to the change to state 11, so no write happens before the build-it's rise, one
// write per update while it lasts and one write when the height returns to 0; a prop that a pushable moves afterwards keeps its own transform
// (docs/re/pushables.md, 20261101-mission-B).
#include "runtime/final_prop_rise.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main() {
    try {
        chase::runtime::FinalPropRise rise;
        for (int update = 0; update < 5; ++update) require(!rise.writes(0.0f), "no write before the rise");
        require(rise.writes(0.25f) && rise.writes(1.0f) && rise.writes(0.5f), "a write per update while the height is non-zero");
        require(rise.writes(0.0f), "one write when the height returns to 0");
        for (int update = 0; update < 5; ++update) require(!rise.writes(0.0f), "no write after the rise, a pushed prop keeps its own transform");
        chase::runtime::FinalPropRise other;
        require(!other.writes(0.0f), "each build-it keeps its own height");
    } catch (const std::exception& error) {
        std::cerr << "final-prop-rise-test: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
