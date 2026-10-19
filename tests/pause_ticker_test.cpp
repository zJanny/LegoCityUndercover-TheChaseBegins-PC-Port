// Scroll rule and text of the pause top ticker (src/runtime/pause_ticker.hpp). Values from llvm-objdump of 0x29ec00 (literal pool 0x29ede4 = 35.0,
// 0x29ede8 = -350.0, 0x29edec / 0x29edf4 / 0x29edf8 = clip rectangle, 0x29ee00 = 17.0; sentinel 0xefed6b90 at 0x29edcc).
#include "runtime/pause_ticker.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace chase::runtime;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main() {
    try {
        require(!pauseTickerKeySet(0) && !pauseTickerKeySet(kPauseTickerEmptyKey) && pauseTickerKeySet(0x1234), "a key of 0 or the empty-text hash means unset");
        require(pauseTickerText("Cherry", 0, "t", 5, "s").empty(), "no title: nothing is drawn");
        require(pauseTickerText("Cherry", 7, "Title", 0, "") == "Cherry / Title", "title only");
        require(pauseTickerText("Cherry", 7, "Title", 9, "Sum") == "Cherry / Title / Sum", "title and summary");

        PauseTicker ticker;
        require(ticker.x() == 350.0f, "the text starts at x = 350");
        ticker.advance(1.0f, 1000.0f);
        require(std::fabs(ticker.x() - 287.0f) < 1e-3f, "63 pixels per second to the left: 1.05 per vblank (oracle C-1: scroll float of the pause object, 1.05 per vblank counter step 0x593d14, loads L2, L4, L5)");
        ticker.advance(10.0f, 10.0f);
        require(ticker.x() == 350.0f, "once the text has left (width below the scroll) it restarts at 350");
    } catch (const std::exception& error) {
        std::cerr << "pause-ticker-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "pause-ticker-test: ok\n";
    return 0;
}
