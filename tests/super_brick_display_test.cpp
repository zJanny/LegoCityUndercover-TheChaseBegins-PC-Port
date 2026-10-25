// Tests of the super brick count display (src/runtime/super_brick_display.*, src/frontend/super_brick_display.*), the brick flight and the rainbow tint
// (src/runtime/super_brick.*), docs/re/super-bricks.md.
// Read with REA (Ghidra 12.1.2) and llvm-objdump of artifacts/game/game.arm.elf: the HUD draw callback 0x193604 (its part from 0x2a5f80), the fader draw
// 0x27b3e4, the fader state machine 0x23cc00 / 0x2459d8 / 0x235e2c, the creator 0x290b1c, the sink table 0x2cee00, the brick update 0x1acc10 (flight 0x1acfc0..0x1ad0b4)
// and the tint writer 0x2600dc. Oracle (Azahar OpenGL, user state m13-1-roof-superbrick, New 3DS, RPC screenshots 400x480 of the walk to the brick,
// 2026-10-19): the display appeared about 3 vblanks after the pickup, the number read 96 and became 296 at the destroy, the display ended about 160 vblanks
// (80 updates) after it appeared; the digits "96" cover the pixels x 203..216, y 16..26 (top row at y 16); the icon lay in x 175..196, y 11..33.
// Optional arguments 1 and 2: fib and names for the drawing check.
#include "assets/assets.hpp"
#include "frontend/canvas.hpp"
#include "frontend/resources.hpp"
#include "frontend/super_brick_display.hpp"
#include "runtime/super_brick.hpp"
#include "runtime/super_brick_display.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace chase;
using namespace chase::runtime;

constexpr float kDt = 1.0f / 30.0f;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance = 1e-4f) { return std::fabs(a - b) <= tolerance; }

void displayTiming() {
    SuperBrickDisplay display;
    display.setCount(96);
    require(!display.view().iconVisible && !display.view().textVisible, "nothing is shown before a pickup");
    display.show();
    require(display.view().textVisible && display.view().iconVisible && near(display.view().iconAlpha, 0.0f), "a pickup starts the fade-in");
    for (int i = 0; i < 9; ++i) display.update(kDt);
    require(near(display.view().iconAlpha, 1.0f), "the fader is fully in after 0.25 s (8 updates at 1/30)");
    const SuperBrickDisplayView shown = display.view();
    require(near(shown.textX, 202.0f) && near(shown.textY, 11.0f) && shown.count == 96 && near(shown.textScale, 1.0f),
            "the number stands at the anchor (186, 21) + (16, -10), one update behind the fader (0x375898 is written by the fader draw after the number)");
    require(near(shown.iconX, 186.0f) && near(shown.iconY, 29.0f), "the icon position is the anchor + (0, 4) + 32 x fade - 28");

    unsigned updates = 9;
    while (display.view().textVisible && updates < 200) {
        display.update(kDt);
        ++updates;
    }
    require(updates == 80, "the number is shown for 80 updates from the pickup, got " + std::to_string(updates));
    require(display.view().iconVisible, "the icon fades out after the hide");
    for (int i = 0; i < 5; ++i) display.update(kDt);
    require(!display.view().iconVisible, "the next draw hides the fader and its 0.1 s fade-out ends within 4 more updates");
}

void bank() {
    SuperBrickDisplay display;
    display.setCount(96);
    display.show();
    for (int i = 0; i < 31; ++i) display.update(kDt);
    require(display.view().count == 96, "the number still reads the old count before the destroy");
    display.banked(296);
    require(display.view().count == 296, "0x262778 stores the new count");
    require(near(display.view().textScale, 0.0f, 1e-3f), "the bounce scale is 2 sin(0) at the destroy update");
    display.update(kDt);
    require(near(display.view().textScale, 2.0f * std::sin(0.2f * 3.14159265f), 1e-3f), "2 sin(0.2 pi) one update later");
    for (int i = 0; i < 4; ++i) display.update(kDt);
    require(near(display.view().textScale, 1.0f), "the bounce lasts 5 updates");
}

// The helipad build lowers the shown count +0x2a8 together with the counter 0x593430 (0x26d778..0x26d780 by llvm-objdump, 20261025-mission13-C).
// Oracle: the user state m13-4-helipad-300-bricks read 302 in both words and 10 s after the build 2 in both (docs/re/super-buildit.md, entry
// 2026-10-19T19:23:11, RPC reads of the round that wrote it). The build does not show the display and does not bounce the number.
void spend() {
    SuperBrickDisplay display;
    display.setCount(302);
    display.spend(300);
    require(display.count() == 2, "the build lowers the shown count by the value (302 -> 2 in the oracle)");
    require(!display.view().textVisible && !display.view().iconVisible, "the build alone shows nothing");
    display.show();
    for (int i = 0; i < 10; ++i) display.update(kDt);
    require(display.view().count == 2 && near(display.view().textScale, 1.0f), "the number reads the lowered count without a bounce");
}

// While a super build-it owns the prompt page (word 0x37588c, written by 0x1cc994 and 0x1cc4d4) the draw callback 0x2a5f80 calls 0x2144f8 in every
// update (0x2a5fa0..0x2a5fa8): the stamp keeps moving, so the display stays. After the last call it lasts the usual 80 updates
// (20261025-mission13-C, llvm-objdump of artifacts/game/game.arm.elf).
void heldByPrompt() {
    SuperBrickDisplay display;
    display.setCount(296);
    for (int i = 0; i < 300; ++i) {
        display.show();
        display.update(kDt);
        require(display.view().textVisible && display.view().iconVisible, "the display stays while the prompt page is owned");
    }
    require(near(display.view().iconAlpha, 1.0f) && display.view().count == 296, "the held display is fully in and shows the count");
    unsigned updates = 1;
    while (display.view().textVisible && updates < 200) {
        display.update(kDt);
        ++updates;
    }
    require(updates == 80, "the number lasts 80 updates after the last call, got " + std::to_string(updates));
}

// The spin angle 0x375890 and the tint phase 0x3743f4 are static data with the initial value 0 (bytes 00 00 00 00 in artifacts/game/code.bin at
// both addresses) and no level load resets them: a run that continues in a new level session with the carried values equals an unbroken run.
void globalsCarry() {
    const SuperBrickGlobals initial;
    require(initial.spin == 0.0f && initial.tintPhase == 0.0f, "both globals start at 0");
    SuperBrickDisplay whole, first, second;
    SuperBrickTint tintWhole, tintFirst, tintSecond;
    for (int i = 0; i < 45; ++i) {
        whole.update(kDt);
        first.update(kDt);
        tintWhole.update(kDt);
        tintFirst.update(kDt);
    }
    const SuperBrickGlobals carried = {first.spin(), tintFirst.phase()};
    second.setSpin(carried.spin);
    tintSecond.setPhase(carried.tintPhase);
    for (int i = 0; i < 40; ++i) {
        whole.update(kDt);
        second.update(kDt);
        tintWhole.update(kDt);
        tintSecond.update(kDt);
    }
    require(near(whole.spin(), second.spin()) && whole.spin() > 0.0f, "the spin continues across the level load");
    require(near(tintWhole.phase(), tintSecond.phase()) && tintWhole.bytes() == tintSecond.bytes(), "the tint phase continues across the level load");
}

void flight() {
    SuperBrickField field;
    field.add("b", {0, 0, 0}, 200);
    const auto always = [](std::size_t) { return true; };
    field.update(kDt, {0, 0, 0}, false, always);
    const std::array<float, 3> target = {30, 60, 90};
    field.update(kDt, {0, 0, 0}, false, always, target);
    const float timer = 1.0f - kDt;
    const float rate = 1.0f / (30.0f * timer);
    require(near(field.bricks()[0].position[0], 30.0f * rate, 1e-3f) && near(field.bricks()[0].position[2], 90.0f * rate, 1e-3f),
            "a step moves (target - position) / (30 x timer) (0x1ad04c..0x1ad07c)");
    for (int i = 0; i < 40; ++i) field.update(kDt, {0, 0, 0}, false, always, target);
    require(field.bricks()[0].state == SuperBrickState::Done, "the destroy still comes 31 updates after the pickup");
}

void tint() {
    SuperBrickTint tint;
    tint.update(kDt);
    require(tint.bytes() == std::array<std::uint8_t, 3>{0x8a, 0x2f, 0xa1}, "phase 0 is table entry 0");
    SuperBrickTint half;
    for (int i = 0; i < 30; ++i) half.update(kDt);
    require(near(half.phase(), 0.5f, 1e-3f), "0.5 per second");
    half.update(kDt);
    const auto& bytes = half.bytes();
    require(bytes[0] >= 0xfe && bytes[1] <= 0x5d && bytes[1] >= 0x5a && bytes[2] <= 0x0b, "after 1 s the phase 0.5 x 14 = 7 lies between the entries 6 and 7 (ff d3 0b, ff 5c 00), got " + std::to_string(bytes[0]) + " " + std::to_string(bytes[1]) + " " + std::to_string(bytes[2]) + " phase " + std::to_string(half.phase()));
    SuperBrickTint wrap;
    for (int i = 0; i < 70; ++i) wrap.update(kDt);
    require(wrap.phase() < 0.5f, "the phase wraps above 1.0");
}

void drawing(const std::string& fib, const std::string& names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    frontend::Resources resources(archive);
    frontend::SuperBrickDisplayDraw draw(resources);
    SuperBrickDisplay display;
    display.setCount(96);
    display.show();
    for (int i = 0; i < 20; ++i) display.update(kDt);
    frontend::Canvas canvas(400, 240);
    canvas.clearTransparent();
    draw.draw(canvas, display.view());
    int minX = 1000, maxX = -1, minY = 1000, maxY = -1, yellow = 0, yellowTop = 1000;
    for (int y = 0; y < 60; ++y)
        for (int x = 160; x < 230; ++x) {
            const std::uint8_t* pixel = canvas.pixel(x, y);
            if (pixel[3] < 128) continue;
            if (pixel[0] > 200 && pixel[1] > 200 && pixel[2] < 100) {
                if (x >= 199) {
                    ++yellow;
                    yellowTop = std::min(yellowTop, y);
                }
            } else if (x < 199 && pixel[2] > pixel[0]) {
                minX = std::min(minX, x);
                maxX = std::max(maxX, x);
                minY = std::min(minY, y);
                maxY = std::max(maxY, y);
            }
        }
    require(yellowTop == 16, "the digits' top row is y 16 as in the oracle, got " + std::to_string(yellowTop));
    require(yellow >= 72 && yellow <= 90, "the digits cover 72 (oracle) to about 79 yellow pixels, got " + std::to_string(yellow));
    require(minX >= 173 && maxX <= 198 && minY >= 9 && maxY <= 35, "the icon lies where the oracle's lies (x 175..196, y 11..33)");
    require(maxX - minX >= 12 && maxY - minY >= 15, "the icon has the oracle's size");
}

}

int main(int argc, char** argv) {
    try {
        if (argc >= 3 && !std::filesystem::exists(argv[1])) return 77;
        displayTiming();
        bank();
        spend();
        heldByPrompt();
        globalsCarry();
        flight();
        tint();
        if (argc >= 3) drawing(argv[1], argv[2]);
        std::cout << "super-brick-display: ok\n";
    } catch (const std::exception& error) {
        std::cerr << "super-brick-display: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
