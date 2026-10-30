#include "qol/menu_redraw.hpp"

#include <algorithm>
#include <sstream>

namespace qol {

namespace fe = chase::frontend;

std::string sceneFingerprint(const fe::LayoutScene& scene) {
    std::ostringstream out;
    out.precision(9);
    out << "t" << scene.text.size();
    for (const auto& entry : scene.text) {
        const fe::TextSlot& slot = entry.second;
        out << '|' << entry.first << '=' << slot.text.size() << ':' << slot.text << ',' << int(slot.align) << ',' << slot.wrap << ','
            << slot.pitch << ',' << slot.spacing << ',' << slot.offsetY << ',' << slot.offsetX << ',' << slot.tinted << ',' << slot.tint[0]
            << ',' << slot.tint[1] << ',' << slot.tint[2];
    }
    out << "\nh" << scene.hidden.size();
    for (const std::string& name : scene.hidden) out << '|' << name;
    out << "\ni" << scene.images.size();
    for (const auto& entry : scene.images) out << '|' << entry.first << '=' << entry.second;
    out << "\nrs" << scene.referenceStoryboards.size();
    for (const auto& entry : scene.referenceStoryboards) out << '|' << entry.first << '=' << entry.second;
    out << "\nrt" << scene.referenceTimes.size();
    for (const auto& entry : scene.referenceTimes) out << '|' << entry.first << '=' << entry.second;
    out << "\ns" << scene.storyboard << '@' << scene.time << "\nl" << scene.laterStoryboards.size();
    for (const fe::PlayingStoryboard& later : scene.laterStoryboards) out << '|' << later.name << '@' << later.time;
    out << "\na" << scene.attachedLayouts.size();
    for (const auto& entry : scene.attachedLayouts) out << '|' << entry.first << '=' << entry.second;
    out << "\np" << scene.positions.size();
    for (const auto& entry : scene.positions) out << '|' << entry.first << '=' << entry.second[0] << ',' << entry.second[1];
    out << "\no" << scene.opacities.size();
    for (const auto& entry : scene.opacities) out << '|' << entry.first << '=' << entry.second;
    out << "\nf" << scene.opacity << ',' << scene.textSpacing << ',' << scene.rotations << ',' << scene.scaleText;
    return out.str();
}

MenuRedraw::MenuRedraw(std::size_t capacity) : capacity_(std::max<std::size_t>(capacity, 1)) {}

RedrawnPage MenuRedraw::draw(int windowWidth, int windowHeight, fe::Resources& resources, const MenuModel& model,
    const MenuPageSetup& setup, const ListForm& list, const GameOptions* game) {
    std::ostringstream keyStream;
    keyStream << windowWidth << 'x' << windowHeight << '\n' << sceneFingerprint(pageScene(model, setup, list, game)) << '\n'
              << sceneFingerprint(menuShortcutsScene(setup));
    if (usesListForm(model.tab()) && list.overlay) {
        const std::optional<fe::LayoutScene> overlay = list.overlay(model);
        keyStream << '\n' << (overlay ? sceneFingerprint(*overlay) : std::string("-"));
    }
    const std::string key = keyStream.str();
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->key != key) continue;
        entries_.splice(entries_.begin(), entries_, it);
        ++reuses_;
        return {&entries_.front().pixels, entries_.front().id};
    }
    fe::Canvas window(std::max(windowWidth, 1), std::max(windowHeight, 1));
    drawMenuPage(window, resources, model, setup, list, game);
    ++renders_;
    entries_.push_front({key, nextId_++, std::move(window)});
    while (entries_.size() > capacity_) entries_.pop_back();
    return {&entries_.front().pixels, entries_.front().id};
}

}
