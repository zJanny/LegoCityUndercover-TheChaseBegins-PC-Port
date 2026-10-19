#include "frontend/super_build_prompt.hpp"
#include <algorithm>

namespace chase::frontend {
namespace {

const assets::UiElement* findElement(const assets::UiElement& element, const std::string& name) {
    if (element.name == name) return &element;
    for (const assets::UiElement& child : element.children)
        if (const assets::UiElement* found = findElement(child, name)) return found;
    return nullptr;
}

float storyboardLength(const assets::UiLayout& layout, const std::string& name) {
    float length = 0;
    for (const assets::UiStoryboard& board : layout.storyboards)
        if (board.name == name)
            for (const assets::UiTrack& track : board.tracks) length = std::max(length, track.duration * float(std::max<std::int16_t>(track.repeat, 1)));
    return length;
}

TextSlot whiteSlot(const std::string& text) {
    TextSlot slot;
    slot.text = text;
    slot.tinted = true;
    return slot;
}

}

bool superBuildPromptVisible(Resources& resources, const SuperBuildPromptView& view) {
    return view.on || view.time < storyboardLength(resources.layout(kSuperBuildPromptLayout), "TransitionOff");
}

LayoutScene superBuildPromptScene(Resources& resources, const SuperBuildPromptView& view) {
    const assets::UiLayout& layout = resources.layout(kSuperBuildPromptLayout);
    LayoutScene scene;
    scene.storyboard = view.on ? "TransitionOn" : "TransitionOff";
    scene.time = view.time;
    scene.text["objectName"] = whiteSlot(view.name);
    scene.text["objectCost"] = whiteSlot(view.cost);
    TextSlot prompt = whiteSlot(view.prompt);
    if (!view.enough)
        for (float& channel : prompt.tint) channel = float(kSuperBuildNotEnoughGrey) / 255.0f;
    scene.text["prompts"] = prompt;
    if (!view.picture.empty()) scene.images["objectImage"] = layoutTexturePath(kSuperBuildPromptLayout, std::string("images/") + view.picture);
    scene.hidden.insert("brickPlaceholder1");
    scene.hidden.insert("brickPlaceholder2");
    if (!view.enough) scene.hidden.insert("buyButton");
    const assets::UiElement* prompts = findElement(layout.root, "prompts");
    const assets::UiElement* button = findElement(layout.root, "buyButton");
    if (prompts && button) {
        const int width = textWidth(resources.font("frontendfont"), resources.extendedCharacters(), view.prompt, scene.textSpacing);
        const float x = prompts->position[0] - button->size[0] * 0.5f + prompts->size[0] * 0.5f - float(width) * 0.5f - kSuperBuildButtonGap;
        scene.positions["buyButton"] = {x, button->position[1]};
    }
    return scene;
}

void drawSuperBuildPrompt(Canvas& top, Resources& resources, const SuperBuildPromptView& view) {
    if (!superBuildPromptVisible(resources, view)) return;
    drawLayout(top, resources, kSuperBuildPromptLayout, superBuildPromptScene(resources, view), 0, 0);
}

}
