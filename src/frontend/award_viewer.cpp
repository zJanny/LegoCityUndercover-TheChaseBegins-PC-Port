#include "frontend/award_viewer.hpp"
#include "frontend/pages.hpp"

#include <stdexcept>

namespace chase::frontend {

AwardViewerSetup awardViewerSetup(Resources& resources, const std::string& gameTextFile, const std::string& systemTextFile) {
    AwardViewerSetup setup;
    setup.shared = pauseMenuSetup(resources, gameTextFile, systemTextFile);
    setup.title = resources.textForKey(gameTextFile, kAwardTitleKey);
    const assets::FuseEntry* entry = resources.archive().find("tables/tableslevels.bxls");
    if (!entry) throw std::runtime_error("award viewer: tables/tableslevels.bxls missing");
    const assets::Table table = assets::parseTable(resources.archive().read(*entry));
    if (table.sheets.empty() || table.sheets[0].rows <= runtime::kAwardLastLevel || table.sheets[0].columns.size() <= kAwardLevelNameColumn)
        throw std::runtime_error("award viewer: TablesLevels sheet 0 is too short");
    const assets::TableSheet& sheet = table.sheets[0];
    for (unsigned index = 0; index < runtime::kAwardViewerItems; ++index) {
        const unsigned item = runtime::awardViewerItem(index);
        RoundMenuItem& entryItem = setup.items[index];
        if (item == runtime::kAwardGeneralItem) {
            entryItem.text = resources.textForKey(gameTextFile, kAwardGeneralKey);
            entryItem.image = kAwardGeneralImage;
        } else {
            entryItem.text = resources.textForKey(gameTextFile, assets::tableValue(sheet, item, kAwardLevelKeyColumn).bits);
            entryItem.image = std::string(kAwardPortraitFolder) + assets::tableValue(sheet, item, kAwardLevelNameColumn).text + ".png";
        }
    }
    return setup;
}

AwardViewer::AwardViewer(const AwardViewerSetup& setup) : setup_(setup), round_(setup.shared.round, setup.shared.centres) {}

void AwardViewer::open(const std::array<runtime::LevelAwardData, runtime::kAwardLevels>& levels) {
    levels_ = levels;
    counts_ = runtime::awardCountsOfItem(runtime::kAwardGeneralItem, levels_);
    opened_ = true;
    finished_ = closing_ = false;
    round_ = RoundMenu(setup_.shared.round, setup_.shared.centres);
    round_.create();
    round_.setTitle(setup_.title);
    pageRequested_ = true;
    page_ = justSwitched_ = false;
    back_ = {};
}

void AwardViewer::recount() {
    counts_ = runtime::awardCountsOfItem(runtime::awardViewerItem(unsigned(round_.selected())), levels_);
}

void AwardViewer::enterPage() {
    round_.open(kAwardItemFolder, std::vector<RoundMenuItem>(setup_.items.begin(), setup_.items.end()), savedSelection_);
    back_ = {true, 0};
}

void AwardViewer::requestClose() {
    closing_ = true;
    pageRequested_ = false;
    if (page_) {
        round_.close(true);
        back_ = {false, 0};
    }
}

void AwardViewer::update(float dt, const AwardViewerInput& input) {
    if (!isOpen()) return;
    back_.time += dt;
    RoundMenuInput menuInput;
    menuInput.left = input.left;
    menuInput.right = input.right;
    menuInput.a = input.a;
    menuInput.b = input.b;
    menuInput.touch = input.touch;
    const int result = round_.update(dt, menuInput);
    for (const int id : round_.takeSounds()) sounds_.push_back(id);
    if (!closing_) {
        if (justSwitched_) {
            justSwitched_ = false;
        } else if (page_ == pageRequested_) {
            if (page_) recount();
            if (page_ && result == 1) {
                savedSelection_ = round_.selected();
            } else if (page_ && result == -1) {
                sounds_.push_back(kAwardSoundBack);
                requestClose();
            }
        } else if (round_.state() == RoundMenuState::Idle) {
            if (pageRequested_) {
                enterPage();
                recount();
            }
            page_ = pageRequested_;
            justSwitched_ = true;
        }
    }
    for (const int id : round_.takeSounds()) sounds_.push_back(id);
    if (closing_ && round_.state() == RoundMenuState::Idle) finished_ = true;
}

std::vector<int> AwardViewer::takeSounds() {
    std::vector<int> out;
    out.swap(sounds_);
    return out;
}

std::array<std::string, 3> AwardViewer::countTexts() const {
    return {std::to_string(counts_.bronze) + "/" + std::to_string(counts_.bronzeTotal),
            std::to_string(counts_.silver) + "/" + std::to_string(counts_.silverTotal),
            std::to_string(counts_.gold) + "/" + std::to_string(counts_.goldTotal)};
}

void AwardViewer::drawTop(Canvas& top, Resources& resources) const {
    top.clear(0, 0, 0);
    const assets::Image& picture = resources.image(kAwardTopTexture);
    top.draw(picture, {0, 0, top.width(), top.height()}, 0, 0);
    const BitmapFont& font = resources.font("frontendfont");
    const auto& extended = resources.extendedCharacters();
    const std::array<std::string, 3> texts = countTexts();
    for (int i = 0; i < 3; ++i) {
        const float x = float(top.width()) * 0.5f + kAwardCountSpacing * float(i - 1);
        drawText(top, font, extended, texts[std::size_t(i)], x, kAwardCountY, Align::Centre, Colour{1, 1, 1, 1}, 1, true);
    }
}

void AwardViewer::drawBottom(Canvas& bottom, Resources& resources) const {
    bottom.clear(0, 0, 0);
    drawLayout(bottom, resources, kRoundMenuLayout, round_.scene(), 0, 0);
    ShortcutView back, select;
    back.label = setup_.shared.texts.back;
    back.storyboard = back_.shown ? "TransitionOn" : "TransitionOff";
    back.time = back_.time;
    back.drawn = page_ || closing_;
    drawBackSelectShortcuts(bottom, resources, back, select);
}

}
