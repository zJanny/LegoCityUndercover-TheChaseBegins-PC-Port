// The redraw cache of the settings menu (QOLSET-51, docs/design/qol-settings-drawing.md): every page it hands out is pixel-identical to a
// full redraw, a state seen before is reused without drawing, and a change costs the measured full draw only once.
// Provenance of the budgets: 20261030-qolstores-C, 2026-10-30, this test with its printed timing, run while other builds loaded the
// CPU: the full draw of one changed page took 291 to 710 ms at 960x600 and 1989 to 3265 ms at 1920x1080 in two runs, the reuse of a seen page 6 ms at most and a
// call with nothing changed 0.015 ms. The budgets leave a factor 4 to 7 over the full draws and are not a target for them: the cache
// removes the draw for a page seen before, the draw of a new page stays the cost of QOLSET-58. Needs the extracted RomFS; exits with 77 without it.
#include "qol/menu_redraw.hpp"
#include "qol/menu_window.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {
namespace fe = chase::frontend;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    ++failures;
    std::cerr << "FAIL: " << what << "\n";
}

double millisecondsSince(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

struct Size {
    int width, height;
    double drawBudgetMs, reuseBudgetMs;
    std::vector<std::string> keys;
};

void run(fe::Resources& resources, const qol::MenuPageSetup& setup, const Size& size) {
    const std::string name = std::to_string(size.width) + "x" + std::to_string(size.height);
    qol::Settings settings;
    qol::MenuModel model(settings);
    qol::MenuWindow window(model, setup, qol::controlsListForm(setup));
    window.open();

    std::uint64_t previous = window.page(size.width, size.height, resources).id;
    std::size_t changes = 0;
    double worstDraw = 0, worstReuse = 0;
    for (const std::string& key : size.keys) {
        window.keyDown(key);
        const std::size_t rendersBefore = window.redraw().renders();
        const auto start = std::chrono::steady_clock::now();
        const qol::RedrawnPage page = window.page(size.width, size.height, resources);
        const double elapsed = millisecondsSince(start);
        const bool drew = window.redraw().renders() != rendersBefore;
        (drew ? worstDraw : worstReuse) = std::max(drew ? worstDraw : worstReuse, elapsed);
        if (page.id != previous) ++changes;
        previous = page.id;
        fe::Canvas full(size.width, size.height);
        window.draw(full, resources);
        require(page.pixels->rgba() == full.rgba(), name + ": after " + key + " the cached page equals the full redraw");
    }
    require(changes > 0, name + ": the keys changed the page");
    require(window.redraw().reuses() > 0, name + ": a page seen before was reused");

    const std::size_t rendersBefore = window.redraw().renders();
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 20; ++i) window.page(size.width, size.height, resources);
    const double unchanged = millisecondsSince(start) / 20;
    require(window.redraw().renders() == rendersBefore, name + ": an unchanged page is not drawn again");

    std::cout << "qol-menu-redraw-real-test: " << name << " changed-page draw worst " << worstDraw << " ms, reuse worst " << worstReuse
              << " ms, unchanged call " << unchanged << " ms\n";
    require(worstDraw < size.drawBudgetMs, name + ": a changed page is drawn within its budget");
    require(worstReuse < size.reuseBudgetMs, name + ": a page seen before is reused within its budget");
    require(unchanged < size.reuseBudgetMs, name + ": an unchanged page costs next to nothing");
}

}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: qol-menu-redraw-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "qol-menu-redraw-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        fe::Resources resources(archive);
        const qol::MenuPageSetup setup = qol::menuPageSetup(resources, "localisation/gametext_uk.loc", "localisation/systemtext_uk.loc");
        run(resources, setup, {960, 600, 2000, 50, {"Down", "Down", "Up", "Right", "PageDown", "Return", "Return", "Down", "PageUp", "Up"}});
        run(resources, setup, {1920, 1080, 8000, 50, {"Down", "Up", "PageDown", "Return", "Return"}});
    } catch (const std::exception& error) {
        std::cerr << "qol-menu-redraw-real-test: " << error.what() << '\n';
        return 1;
    }
    if (failures) return 1;
    std::cout << "qol-menu-redraw-real-test: ok\n";
    return 0;
}
