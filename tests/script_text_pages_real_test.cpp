// Tip panel and dialogue box geometry against the real game data. Skips (exit 77) without the extracted RomFS.
// Usage: script-text-pages-real-test FIB CSV
#include "frontend/script_text_pages.hpp"
#include <algorithm>
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

    Canvas framed(400, 240);
    framed.clearTransparent();
    drawDialogueBox(framed, resources, 10, "This police force doesn't run on hopes and dreams alone!", 1.0f);
    auto litOverBlack = [&](int x, int y) {
        const auto pixel = framed.pixel(x, y);
        const int sum = (pixel[0] + pixel[1] + pixel[2]) * pixel[3] / 255;
        return sum > 150 && pixel[0] > pixel[2];
    };
    int frameLeft = -1, frameTop = -1, frameBottom = -1;
    for (int x = 0; x < 40 && frameLeft < 0; ++x)
        if (litOverBlack(x, 185)) frameLeft = x;
    for (int y = 140; y < 230; ++y)
        if (litOverBlack(20, y)) {
            if (frameTop < 0) frameTop = y;
            frameBottom = y;
        }
    // Oracle (state agent-intro-dialog, line "This police force doesn't run on hopes and dreams alone!", agentHUD2-out/i12.png, pixels over
    // black with brightness > 150 and red > blue): hexagon frame heartsbg_generic left tip at row 185 is x 8, column 20 spans y 163..206.
    // Formula from the draw code 0x1cc550: centre x = 200 - half text width - 24 - 7 (measure call without the +10 offset), centre y = line box centre - 1.
    require(frameLeft == 8, "portrait frame left tip x 8");
    require(frameTop == 163 && frameBottom == 206, "portrait frame column 20 spans 163..206");

    Canvas heading(320, 240);
    heading.clear(0, 0, 0);
    drawHeadingBox(heading, resources, "Cherry Tree Hills", 1.0f);
    int textLeft = 999, textRight = -1, textTop = 999, textBottom = -1;
    for (int y = 0; y < 58; ++y)
        for (int x = 60; x < 260; ++x) {
            const auto pixel = heading.pixel(x, y);
            if (pixel[0] <= 200 || pixel[1] <= 200 || pixel[2] <= 200) continue;
            textLeft = std::min(textLeft, x);
            textRight = std::max(textRight, x);
            textTop = std::min(textTop, y);
            textBottom = std::max(textBottom, y);
        }
    // Oracle (state agent-doughnut-title, first tip of mission 1-1, agentHUD2-out/h2.png bottom screen): text pixels above 200 in all channels span
    // x 103..216, y 24..36; the box spans y 6..53 and x 92..227 (the port box is one pixel narrower on each side, HUD-23). Box centre (160, 30),
    // half height 12 and half width min(text width / 2, 145): draw code 0x193f00.
    require(textLeft == 103 && textRight == 216 && textTop == 24 && textBottom == 36, "heading text pixels x 103..216, y 24..36");
    int borderTop = 999, borderBottom = -1;
    for (int y = 0; y < 58; ++y) {
        const auto pixel = heading.pixel(160, y);
        if (pixel[2] <= 180 || pixel[0] >= 150 || pixel[1] <= 120) continue;
        borderTop = std::min(borderTop, y);
        borderBottom = std::max(borderBottom, y);
    }
    require(borderTop == 6 && borderBottom == 53, "heading box border rows 6..53 at column 160");
    require(levelHeadingKey(resources, "lvl_01_cherrytreehills") == 2004168849u && levelHeadingKey(resources, "lvl_missing") == 0,
            "level heading key: TablesLevels field +8 of the row (oracle memory 0x20a4a8 call, gametext 2004168849 = Cherry Tree Hills)");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
