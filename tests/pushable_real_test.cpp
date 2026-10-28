// The push cart of mission 1-3 (1-3_Push_cart, leprops/leprop_pushable, kind 71) loaded from lvl_01_cherrytreehills by PushableSet
// (src/runtime/pushable.cpp, docs/re/pushables.md): its parameters (instance values over the class defaults of leprop_pushable.blvl), its four
// nested handles of which only the back one is active, and the push from its placement along its own z axis to 1-3_Push_cart_to, which it
// reaches within TriggerDistance and then fires 1-3_Push_Done. Values from the level data with the C++ loader (20261026-crane-B, 2026-10-26);
// the 90 moves from the placement are the port's own count. The second run starts at the cart position measured in the original (recorded from
// a save state in front of the push cart of mission 1-3, RPC 20261027-mission13-C-2, servant, 2026-10-28, New 3DS, OpenGL, fixed dt 0.033, trace of the cart node at
// node+0x8c per update; first move in update 11, state 1 in update 82, state 2 in update 83): 72 moves of 0.06, state 1 in the update of the 72nd move, rest position (1019.4844, 33.4108, 1034.3561). Needs the extracted RomFS; exits with 77 (skipped) without it.
#include "assets/assets.hpp"
#include "runtime/pushable.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: pushable-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "pushable-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);

        runtime::PushableSet set;
        set.load(archive, level, classOf);
        require(set.objects().size() == 1, "one pushable in lvl_01, got " + std::to_string(set.objects().size()));
        const runtime::PushableObject& cart = set.objects()[0];
        require(cart.instance == "1-3_push_cart", "the push cart: " + cart.instance);
        require(cart.destination == "1-3_push_cart_to" && cart.triggerObject == "1-3_push_done", "Destination and TriggerObject");
        require(near(cart.triggerDistance, 0.1f, 1e-6f), "TriggerDistance 0.1 (instance; class 0.75)");
        require(near(cart.pushSpeed, 0.06f, 1e-6f), "PushSpeed: the create's default 0.06 (neither class nor instance sets it)");
        require(cart.sinkHeight == 0.0f, "SinkHeight 0 (instance; class 0.5)");
        require(cart.autoPush && cart.noGravity && !cart.strong && cart.canPush && !cart.canPull, "AutoPush 1, NoGravity 1, Strong 0, MoveType 0");
        require(cart.soundPush == 337 && cart.soundDestroyed == 549, "ATTR_SFX_PUSH 337 (instance), ATTR_SFX_DESTROYED 549 (class)");
        require(cart.handles.size() == 4, "four handles nested in leprop_pushable.blvl");
        for (std::size_t h = 0; h < 4; ++h) require(cart.handles[h].active == (h == 1), "only ActiveHandles_Back is set: handle " + std::to_string(h));
        require(cart.handles[1].instance == "1-3_push_cart.handle02", "the back handle is handle02");
        const float useCentre[3] = {0, -1, 0.75f}, useHalf[3] = {1, 2, 0.75f};
        for (int a = 0; a < 3; ++a)
            require(cart.handles[1].useCentre[a] == useCentre[a] && cart.handles[1].useHalf[a] == useHalf[a], "UseBound of leprop_pushhandle.blvl");

        const runtime::PushMatrix back = runtime::pushHandleWorld(cart, cart.handles[1]);
        require(near(back[12], 1022.02f, 0.01f) && near(back[13], 33.638f, 0.01f) && near(back[14], 1027.41f, 0.01f), "the back handle stands at the bounds face");

        runtime::PushableSet pushed;
        pushed.add(cart);
        const auto noneDisabled = [](const std::string&) { return false; };
        const auto destination = [&](const std::string& name, float out[3]) {
            const std::optional<assets::Placement> placed = assets::objectStart(level, name);
            if (!placed) return false;
            std::copy_n(placed->position, 3, out);
            return true;
        };
        const float direction[3] = {cart.node[8], cart.node[9], cart.node[10]};
        int moves = 0;
        bool done = false;
        for (int update = 0; update < 200 && !done; ++update) {
            if (pushed.objects()[0].state == 0) pushed.push(0, direction);
            for (const runtime::PushableEvent& event : pushed.update(noneDisabled, destination)) {
                if (event.kind == runtime::PushableEventKind::Moved) ++moves;
                done = done || event.kind == runtime::PushableEventKind::Done;
            }
        }
        require(done, "pushed along its z axis the cart reaches 1-3_Push_cart_to within 0.1 and reaches state 2");
        require(moves == 90, "90 moves of 0.06 (the tilted z axis also lowers it by 0.17), got " + std::to_string(moves));

        runtime::PushableSet measured;
        measured.add(cart);
        measured.objects()[0].node[12] = 1020.9684f;
        measured.objects()[0].node[13] = 33.5452f;
        measured.objects()[0].node[14] = 1030.2870f;
        int measuredMoves = 0;
        int arrivedAt = 0, doneAt = 0;
        for (int update = 1; update <= 200 && !doneAt; ++update) {
            if (measured.objects()[0].state == 0) measured.push(0, direction);
            for (const runtime::PushableEvent& event : measured.update(noneDisabled, destination)) {
                if (event.kind == runtime::PushableEventKind::Moved) ++measuredMoves;
                if (event.kind == runtime::PushableEventKind::Arrived) arrivedAt = measuredMoves;
                if (event.kind == runtime::PushableEventKind::Done) doneAt = update;
            }
        }
        require(measuredMoves == 72 && arrivedAt == 72, "from the measured start the cart arrives after 72 moves, got " + std::to_string(measuredMoves));
        const float* rest = measured.objects()[0].node.data() + 12;
        require(near(rest[0], 1019.4844f, 0.003f) && near(rest[1], 33.4108f, 0.003f) && near(rest[2], 1034.3561f, 0.003f),
                "rest position of the oracle (state 2): " + std::to_string(rest[0]) + " " + std::to_string(rest[1]) + " " + std::to_string(rest[2]));
    } catch (const std::exception& error) {
        std::cerr << "pushable-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "pushable-real-test: ok\n";
    return 0;
}
