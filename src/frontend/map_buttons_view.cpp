#include "frontend/map_buttons_view.hpp"
#include <algorithm>
#include <cctype>

namespace chase::frontend {

namespace {

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

std::vector<PlayingStoryboard> playing(const runtime::MapButtons& buttons) {
    std::vector<PlayingStoryboard> boards;
    for (const runtime::MapStoryboard& board : buttons.storyboards()) boards.push_back({board.name, board.time});
    return boards;
}

}

runtime::MapButtonsSetup mapButtonsSetup(Resources& resources) {
    runtime::MapButtonsSetup setup;
    const assets::UiLayout& layout = resources.layout(runtime::kMapButtonsLayout);
    for (const assets::UiStoryboard& board : layout.storyboards) {
        float length = 0;
        for (const assets::UiTrack& track : board.tracks)
            length = std::max(length, track.repeat == 0 ? track.duration : track.duration * float(track.repeat));
        setup.storyboardLengths[board.name] = length;
    }
    for (std::size_t i = 0; i < runtime::kMapButtonCount; ++i)
        if (const auto box = layoutChildBox(layout, {}, runtime::mapButtonInfo()[i].element))
            setup.boxes[i] = {(*box)[0], (*box)[1], (*box)[2], (*box)[3]};
    return setup;
}

std::array<runtime::MapButtonBox, runtime::kMapButtonCount> mapButtonBoxes(Resources& resources, const runtime::MapButtons& buttons) {
    std::array<runtime::MapButtonBox, runtime::kMapButtonCount> boxes{};
    const assets::UiLayout& layout = resources.layout(runtime::kMapButtonsLayout);
    const std::vector<PlayingStoryboard> boards = playing(buttons);
    for (std::size_t i = 0; i < runtime::kMapButtonCount; ++i)
        if (const auto box = layoutChildBox(layout, boards, runtime::mapButtonInfo()[i].element))
            boxes[i] = {(*box)[0], (*box)[1], (*box)[2], (*box)[3]};
    return boxes;
}

LayoutScene mapButtonsScene(const runtime::MapButtons& buttons) {
    LayoutScene scene;
    const std::vector<PlayingStoryboard> boards = playing(buttons);
    if (!boards.empty()) {
        scene.storyboard = boards.front().name;
        scene.time = boards.front().time;
        scene.laterStoryboards.assign(boards.begin() + 1, boards.end());
    }
    for (std::size_t i = 0; i < runtime::kMapButtonCount; ++i) {
        const runtime::MapButtonInfo& info = runtime::mapButtonInfo()[i];
        const runtime::MapButtonState& state = buttons.button(runtime::MapButton(i));
        scene.attachedLayouts[info.element] = runtime::kMapButtonLayout;
        if (!state.storyboard.name.empty()) {
            scene.referenceStoryboards[info.element] = state.storyboard.name;
            scene.referenceTimes[info.element] = state.storyboard.time;
        }
        scene.images[std::string(info.element) + "/icon"] = "sprites/blend/mapbuttons/images/" + lower(state.icon) + ".bpng";
        if (info.originOffset) scene.positions[std::string(info.element) + "/LayoutRoot"] = {runtime::kMapButtonOriginOffset, runtime::kMapButtonOriginOffset};
    }
    for (std::size_t i = 0; i < runtime::kMapPortraitCount; ++i) scene.hidden.insert("portrait_" + std::to_string(i));
    scene.hidden.insert("textWindow");
    return scene;
}

}
