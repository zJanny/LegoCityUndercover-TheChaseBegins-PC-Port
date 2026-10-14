// Tip panel and dialogue box geometry against the real game data. Skips (exit 77) without the extracted RomFS.
// Usage: script-text-pages-real-test FIB CSV
#include "frontend/script_text_pages.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>

namespace {
using namespace chase;
using namespace chase::frontend;

int failures = 0;

void require(bool ok, const char* what) {
    if (ok) return;
    ++failures;
    std::cerr << "FAIL: " << what << "\n";
}

struct Extent {
    int first = -1, last = -1;
};

Extent rowExtent(const Canvas& canvas, int y, int minAlpha) {
    Extent extent;
    for (int x = 0; x < canvas.width(); ++x) {
        if (canvas.pixel(x, y)[3] < minAlpha) continue;
        if (extent.first < 0) extent.first = x;
        extent.last = x;
    }
    return extent;
}

Extent columnExtent(const Canvas& canvas, int x, int minAlpha) {
    Extent extent;
    for (int y = 0; y < canvas.height(); ++y) {
        if (canvas.pixel(x, y)[3] < minAlpha) continue;
        if (extent.first < 0) extent.first = y;
        extent.last = y;
    }
    return extent;
}

}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: script-text-pages-real-test FIB CSV\n";
        return EXIT_FAILURE;
    }
    if (!std::ifstream(argv[1]).good() || !std::ifstream(argv[2]).good()) {
        std::cout << "skipped: game data not extracted\n";
        return 77;
    }
    assets::FuseArchive archive(argv[1]);
    archive.loadNames(argv[2]);
    Resources resources(archive);

    Canvas tip(320, 240);
    tip.clearTransparent();
    drawTipPage(tip, resources, "Press <L> or <R> to rotate the camera.", 1.0f);
    const Extent tipRow = rowExtent(tip, 120, 200);
    const Extent tipColumn = columnExtent(tip, 160, 200);
    // Oracle screenshot of the tip "Press <L> or <R> to rotate the camera." (artifacts/oracle/agentX-out/compare/tip_oracle_left_port_right.png,
    // border columns 20..299 and rows 72..167 of the 320x240 bottom screen); layout code 0x15b518 / 0x23c780 / 0x229128, docs/re/script-text.md.
    require(tipRow.first == 20 && tipRow.last == 299, "tip box columns 20..299");
    require(tipColumn.first == 72 && tipColumn.last == 167, "tip box rows 72..167");

    Canvas dialogue(400, 240);
    dialogue.clearTransparent();
    drawDialogueBox(dialogue, resources, 0, "This police force doesn't run on hopes and dreams alone!", 1.0f);
    const Extent dialogueRow = rowExtent(dialogue, 186, 200);
    const Extent dialogueColumn = columnExtent(dialogue, 210, 200);
    // Oracle screenshot of the dialogue box (agentX-out/compare/dialogue_oracle_left_port_right.png: border x 45..375, y 155..217, to one pixel);
    // layout code 0x1cc550 / 0x23c780.
    require(dialogueRow.first == 44 && dialogueRow.last == 375, "dialogue box columns 44..375");
    require(dialogueColumn.first == 154 && dialogueColumn.last == 217, "dialogue box rows 154..217");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
