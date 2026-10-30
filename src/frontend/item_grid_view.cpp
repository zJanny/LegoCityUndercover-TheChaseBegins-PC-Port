#include "frontend/item_grid_view.hpp"
#include <algorithm>
#include <cctype>

namespace chase::frontend {
namespace {

constexpr const char* kPortrait = "itemSmallPortrait";

float storyboardLength(const assets::UiLayout& layout, const std::string& name) {
    float length = 0;
    for (const assets::UiStoryboard& board : layout.storyboards)
        if (board.name == name)
            for (const assets::UiTrack& track : board.tracks)
                length = std::max(length, track.repeat == 0 ? track.duration : track.duration * float(track.repeat));
    return length;
}

const assets::UiElement* findElement(const assets::UiElement& element, const std::string& name) {
    if (element.name == name) return &element;
    for (const assets::UiElement& child : element.children)
        if (const assets::UiElement* found = findElement(child, name)) return found;
    return nullptr;
}

runtime::ItemGridCircle centreOf(Resources& resources, const std::string& path, float radiusScale) {
    runtime::ItemGridCircle circle;
    if (const auto box = layoutElementBox(resources, kRoundMenuLayout, path)) {
        circle.x = (*box)[0] + (*box)[2] / 2;
        circle.y = (*box)[1] + (*box)[3] / 2;
        circle.radius = (*box)[2] * radiusScale;
    }
    return circle;
}

std::string texturePath(const std::string& picture) {
    std::string path = picture;
    std::transform(path.begin(), path.end(), path.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    const std::string png = ".png";
    if (path.size() > png.size() && path.compare(path.size() - png.size(), png.size(), png) == 0) path.replace(path.size() - png.size(), png.size(), ".bpng");
    return path;
}

TextSlot textSlot(const std::string& text, Align align) {
    TextSlot slot;
    slot.text = text;
    slot.align = align;
    return slot;
}

void addTile(LayoutScene& scene, const runtime::ItemGridTile& tile, const std::string& element) {
    if (!tile.boards.empty()) {
        scene.referenceStoryboards[element] = tile.boards.front().name;
        scene.referenceTimes[element] = tile.boards.front().time;
        std::vector<PlayingStoryboard>& later = scene.referenceLaterStoryboards[element];
        for (std::size_t i = 1; i < tile.boards.size(); ++i) later.push_back({tile.boards[i].name, tile.boards[i].time});
    }
    const std::string portrait = element + "/" + kPortrait;
    if (tile.picture.empty()) scene.hidden.insert(portrait);
    else scene.images[portrait] = texturePath(tile.picture);
    const runtime::ItemTileLook& look = tile.look;
    if (look.locked) scene.opacities[element + "/itemSmallIconLocked"] = *look.locked;
    if (look.isNew) scene.opacities[element + "/newItemSmall"] = *look.isNew;
    if (look.gift) scene.opacities[element + "/giftItemSmall"] = *look.gift;
    if (look.background) scene.opacities[element + "/itemSmallBG"] = *look.background;
}

}

runtime::ItemGridTimings itemGridTimings(const assets::UiLayout& main, const assets::UiLayout& itemSmall) {
    runtime::ItemGridTimings timings;
    timings.blockbusterOn = storyboardLength(main, runtime::kItemGridBlockbusterOn);
    timings.transitionOn = storyboardLength(main, runtime::kItemGridTransitionOn);
    timings.leftMove = storyboardLength(main, runtime::kItemGridLeftMove);
    timings.rightMove = storyboardLength(main, runtime::kItemGridRightMove);
    timings.show = storyboardLength(itemSmall, runtime::kItemTileShow);
    timings.hide = storyboardLength(itemSmall, runtime::kItemTileHide);
    timings.pressed = storyboardLength(itemSmall, runtime::kItemTilePressed);
    timings.outlined = storyboardLength(itemSmall, runtime::kItemTileOutlined);
    timings.defaultPose = storyboardLength(itemSmall, runtime::kItemTileDefault);
    return timings;
}

std::string itemGridTileElement(int tile, bool old) { return "itemSmall_" + std::to_string(tile) + (old ? "_Old" : ""); }

runtime::ItemGridGeometry itemGridGeometry(Resources& resources) {
    runtime::ItemGridGeometry geometry;
    for (int i = 0; i < runtime::kItemGridTiles; ++i)
        geometry.tiles[std::size_t(i)] = centreOf(resources, itemGridTileElement(i, false) + "/" + kPortrait, 0.5f);
    geometry.leftArrow = centreOf(resources, "Left_Arrow", 1.0f);
    geometry.rightArrow = centreOf(resources, "Right_Arrow", 1.0f);
    const assets::UiLayout& main = resources.layout(kRoundMenuLayout);
    if (const assets::UiElement* left = findElement(main.root, "Left_Arrow")) geometry.leftArrowOpacity = left->opacity;
    if (const assets::UiElement* right = findElement(main.root, "Right_Arrow")) geometry.rightArrowOpacity = right->opacity;
    return geometry;
}

void addItemGridScene(LayoutScene& scene, const runtime::ItemGrid& grid) {
    for (int i = 0; i < runtime::kItemGridTiles; ++i) {
        addTile(scene, grid.tile(i), itemGridTileElement(i, false));
        addTile(scene, grid.oldTile(i), itemGridTileElement(i, true));
    }
    scene.opacities["Left_Arrow"] = grid.leftArrowOpacity();
    scene.opacities["Right_Arrow"] = grid.rightArrowOpacity();
    scene.text["TEXT_ItemName"] = textSlot(grid.itemName(), Align::Centre);
    scene.text["PageNumber"] = textSlot(grid.pageNumber(), Align::Left);
}

}
