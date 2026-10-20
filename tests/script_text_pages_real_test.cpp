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
    // Oracle screenshot of the tip "Press <L> or <R> to rotate the camera." (comparison picture tip_oracle_left_port_right of agent X,
    // border columns 20..299 and rows 72..167 of the 320x240 bottom screen); layout code 0x15b518 / 0x23c780 / 0x229128, docs/re/script-text.md.
    require(tipRow.first == 20 && tipRow.last == 299, "tip box columns 20..299");
    require(tipColumn.first == 72 && tipColumn.last == 167, "tip box rows 72..167");

    int blueText = 0, textLeft2 = 999, textRight2 = -1, textTop2 = 999, textBottom2 = -1, iconWhite = 0, iconLeft = 999, iconRight = -1, iconTop = 999, iconBottom = -1;
    Canvas tipOnBlack(320, 240);
    tipOnBlack.clear(0, 0, 0);
    drawTipPage(tipOnBlack, resources, "Press <L> or <R> to rotate the camera.", 1.0f);
    for (int y = 100; y < 150; ++y)
        for (int x = 30; x < 300; ++x) {
            const auto pixel = tipOnBlack.pixel(x, y);
            if (pixel[2] > 200 && pixel[1] > 130 && pixel[1] < 200 && pixel[0] > 40 && pixel[0] < 120) {
                ++blueText;
                textLeft2 = std::min(textLeft2, x);
                textRight2 = std::max(textRight2, x);
                textTop2 = std::min(textTop2, y);
                textBottom2 = std::max(textBottom2, y);
            }
            if (x >= 100 && x < 200 && std::min({pixel[0], pixel[1], pixel[2]}) > 225) {
                ++iconWhite;
                iconLeft = std::min(iconLeft, x);
                iconRight = std::max(iconRight, x);
                iconTop = std::min(iconTop, y);
                iconBottom = std::max(iconBottom, y);
            }
        }
    // Oracle screenshot agentHUD2-out/h2.png (bottom screen = image columns 40..359, rows 240..479; second tip of mission 1-1): pixels with
    // blue > 200, 130 < green < 200, 40 < red < 120 are 521 with bounding box x 31..289, y 117..128; pixels above 225 in all channels for
    // x 100..199 (the button icons' white borders) are 72 with bounding box x 116..131, y 116..129. Layout code 0x21bc2c (line height 23 - 6),
    // 0x195408 (line top = centre - lines * pitch / 2), 0x2544e0 (icon at line top + (23 - 16) / 2), icons dsbuttonl / dsbuttonr 16x16.
    require(blueText == 521 && textLeft2 == 31 && textRight2 == 289 && textTop2 == 117 && textBottom2 == 128, "tip text pixels equal the oracle");
    require(iconWhite == 72 && iconLeft == 116 && iconRight == 131 && iconTop == 116 && iconBottom == 129, "tip button icon pixels equal the oracle");

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
    // x 103..216, y 24..36; the box spans y 6..53 and x 92..227 (border columns 92 and 93, 226 and 227: the oracle pixel columns 132/133 and 266/267 of h2.png equal the port's, difference to the earlier measure was the alpha). Box centre (160, 30),
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
    // Oracle h2.png pixel (132, 270) = (141, 202, 233) over the scene background (2, 18, 54): the box alpha is the square of 240 / 255 (0x229128
    // writes the alpha into the vertex colour and into the material), over black the red channel is 141 on columns 92 and 93.
    require(heading.pixel(92, 30)[0] == 141 && heading.pixel(93, 30)[0] == 141 && heading.pixel(91, 30)[0] < 20, "heading box border at columns 92, 93 with squared alpha");
    require(levelHeadingKey(resources, "lvl_01_cherrytreehills") == 2004168849u && levelHeadingKey(resources, "lvl_missing") == 0,
            "level heading key: TablesLevels field +8 of the row (oracle memory 0x20a4a8 call, gametext 2004168849 = Cherry Tree Hills)");
    require(characterNameKey(resources, 9) == 0xe1924407u && characterNameKey(resources, 10) == 0x8003758fu && characterNameKey(resources, 9999) == 0,
            "character name key: tablescharacters column 3 (struct +8 read by 0x1b1314), gametext e1924407 = Chase McCain, 8003758f = Police Chief Gleeson (tools/convert_loc.py on gametext_uk.loc)");
    Canvas information(400, 240);
    information.clearTransparent();
    drawInformationBox(information, resources, "You need to complete more missions before you can enter this area of LEGO City.", "OK");
    const Extent informationRow = rowExtent(information, 170, 200);
    const Extent informationColumn = columnExtent(information, 200, 200);
    // Oracle (state info-barrier, walked into No_Entry_Auburn_OnFoot, agentOC-out/ib3.png top screen): border light blue (159, 224, 255) at columns
    // 56..57 and 342..343 of row 170, rows 126..127 and 216..217 of column 200 (box x 56..343, y 126..217); the draw call 0x193c4c gives centre
    // (200, 172) and half size (135.5, 34) (three lines plus the empty line of the two newlines appended before the buttons).
    require(informationRow.first == 56 && informationRow.last == 343, "information box columns 56..343");
    require(informationColumn.first == 126 && informationColumn.last == 217, "information box rows 126..217");
    int okLeft = 999, okRight = -1, okTop = 999, okBottom = -1;
    Canvas informationOnBlack(400, 240);
    informationOnBlack.clear(0, 0, 0);
    drawInformationBox(informationOnBlack, resources, "You need to complete more missions before you can enter this area of LEGO City.", "OK");
    for (int y = 190; y < 216; ++y)
        for (int x = 280; x < 318; ++x) {
            const auto pixel = informationOnBlack.pixel(x, y);
            if (pixel[0] <= 200 || pixel[1] <= 120 || pixel[1] >= 210 || pixel[2] >= 90) continue;
            okLeft = std::min(okLeft, x);
            okRight = std::max(okRight, x);
            okTop = std::min(okTop, y);
            okBottom = std::max(okBottom, y);
        }
    // Oracle ib3.png: orange pixels (red > 200, 120 < green < 210, blue < 90) of the "OK" row span x 298..310, y 198..208 at rows 190..215; the
    // oracle's glyph rows sit about 0.2 pixel lower, so the port's last row 209 carries two pixels the oracle does not.
    require(okLeft == 298 && okRight == 310 && okTop == 198 && okBottom <= 209 && okBottom >= 208, "information box OK text pixels x 298..310, y 198..208");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
