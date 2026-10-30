#pragma once

#include "frontend/canvas.hpp"
#include "frontend/layout.hpp"
#include "frontend/resources.hpp"
#include "qol/menu_page.hpp"

#include <cstddef>
#include <cstdint>
#include <list>
#include <string>

namespace qol {

std::string sceneFingerprint(const chase::frontend::LayoutScene& scene);

struct RedrawnPage {
    const chase::frontend::Canvas* pixels = nullptr;
    std::uint64_t id = 0;
};

class MenuRedraw {
public:
    explicit MenuRedraw(std::size_t capacity = 6);

    RedrawnPage draw(int windowWidth, int windowHeight, chase::frontend::Resources& resources, const MenuModel& model,
        const MenuPageSetup& setup, const ListForm& list, const GameOptions* game = nullptr);

    std::size_t renders() const { return renders_; }
    std::size_t reuses() const { return reuses_; }

private:
    struct Entry {
        std::string key;
        std::uint64_t id;
        chase::frontend::Canvas pixels;
    };

    std::size_t capacity_;
    std::list<Entry> entries_;
    std::uint64_t nextId_ = 1;
    std::size_t renders_ = 0, reuses_ = 0;
};

}
