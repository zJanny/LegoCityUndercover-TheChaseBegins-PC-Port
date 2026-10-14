#include "frontend/frontend.hpp"
#include "frontend/pages.hpp"
#include <algorithm>
#include <cmath>

namespace chase::frontend {

namespace {

// Keys from the page builders of the original (docs/re/front-end.md).
constexpr std::uint32_t kKeyStreetPass = 0x4617a2ca, kKeyYes = 0x2c187527, kKeyNo = 0x4e1381, kKeyOk = 0x4f13bc,
                        kKeySelect = 0x715c59fc, kKeyDelete = 0x24d3ab6b, kKeyAutosave = 0xa5e2f63f, kKeyHome = 0x65213108,
                        kKeyAgreement = 0x62ab3f48, kKeyAgreementHelp = 0x62ab3f49, kKeyParental = 0x64301deb,
                        kKeyTooMany = 0x71ba2807, kKeyStreetPassOn = 0xe10321ee, kKeyTip = 0x9e318a31;
constexpr std::uint32_t kSystemContinue = 0xe3f70996, kSystemBack = 0x8be4916, kSystemSelectSlot = 0xe2117397,
                        kSystemNewGame = 0xd7476f00, kSystemSaving = 0xcbafe6a8, kSystemDoNotTurnOff = 0x2c824a40,
                        kSystemPreparingCard = 0x12e3d762, kSystemPreparingSave = 0x84828812;
constexpr std::uint32_t kHintLoadingTip = 0x926950e8, kHintMissionTitle = 0x42ef3429;

// STAND-INS: the first loading tip and the first mission card are those of a new game in the oracle; the choice of tip
// is not read. Fade-in of the screens (the original shows a few updates of ramp-up, docs/re/front-end.md).
constexpr float kFadeInSeconds = 0.25f;
constexpr float kSpinnerOnSeconds = 0.6f;
constexpr float kSpinnerOffSeconds = 0.3f;
constexpr float kBootGameCardSeconds = 1.0f, kBootSaveSeconds = 1.5f, kBootFadeSeconds = 0.5f;

Colour pulseColour(unsigned tick) {
    const float pulse = (std::sin(float(tick) * 0.2f) + 2.0f) * 0.333333f;
    return {pulse * 0.290196f, pulse * 0.678431f, pulse, 1.0f};
}

}

FrontEnd::FrontEnd(Resources& resources, PictureHost* movies, FlowConfig config)
    : resources_(resources), movies_(movies), flow_(movies, config) {}

std::string FrontEnd::suffix() const {
    return localisationSuffix(flow_.language());
}

std::string FrontEnd::textFile(const char* table) const {
    return std::string("localisation/") + table + "_" + suffix() + ".loc";
}

std::vector<std::uint8_t> FrontEnd::takeMovieAudio() {
    return movies_ ? movies_->takeAudio() : std::vector<std::uint8_t>();
}

void FrontEnd::startSubtitles() {
    cues_.clear();
    const std::string base = "cutscenes/01_gameintro_" + suffix();
    for (const std::string& path : {base + ".srt", base + "_" + suffix() + ".srt"}) {
        if (!resources_.has(path)) continue;
        cues_ = assets::parseSubtitles(resources_.bytes(path), resources_.extendedCharacters());
        return;
    }
}

void FrontEnd::update(float dt, const FrontInput& input) {
    flow_.update(dt, input);
    if (flow_.screen() != shown_) {
        shown_ = flow_.screen();
        if (shown_ == Screen::IntroMovie) startSubtitles();
    }
}

void FrontEnd::drawBoot(Canvas& bottom) {
    const float t = flow_.screenSeconds();
    AlertPage page;
    page.secondTransition = false;
    page.seconds = t;
    const std::string system = textFile("systemtext");
    if (t < kBootGameCardSeconds) {
        page.header = resources_.textForKey(system, kSystemPreparingCard);
    } else {
        page.header = resources_.textForKey(system, kSystemPreparingSave);
        page.body = resources_.textForKey(system, kSystemDoNotTurnOff);
    }
    drawAlertPage(bottom, resources_, page);
    drawSaveSpinner(bottom, resources_, t, std::min(1.0f, t / kSpinnerOnSeconds));
    const float fadeStart = kBootGameCardSeconds + kBootSaveSeconds;
    if (t > fadeStart) bottom.dim(std::max(0.0f, 1.0f - (t - fadeStart) / kBootFadeSeconds));
}

void FrontEnd::drawLanguage(Canvas& bottom) {
    const std::string file = textFile("gametext");
    const BitmapFont& font = resources_.font("frontendfont");
    const auto& extended = resources_.extendedCharacters();
    const float level = std::min(1.0f, flow_.screenSeconds() / kFadeInSeconds);
    for (std::size_t row = 0; row < kLanguageOfRow.size(); ++row) {
        Colour colour{0.174118f, 0.407059f, 0.6f, 1.0f};
        if (row == flow_.selectedRow()) colour = pulseColour(flow_.tick());
        colour.a = level;
        drawText(bottom, font, extended, resources_.textForKey(file, kLanguageRowKeys[row]), kBottomWidth / 2.0f,
                 20.0f + 24.0f * float(row), Align::Centre, colour, -2, true);
    }
    drawSelectShortcut(bottom, resources_, resources_.textForKey(file, kKeySelect));
}

void FrontEnd::drawPrompt(Canvas& bottom) {
    const std::string game = textFile("gametext"), system = textFile("systemtext");
    AlertPage page;
    page.seconds = flow_.pageSeconds();
    const PageButton cont{resources_.textForKey(system, kSystemContinue), PadSymbol::A};
    switch (flow_.prompt()) {
    case Prompt::StreetPassQuestion:
        page.body = resources_.textForKey(game, kKeyStreetPass);
        page.buttons = {{resources_.textForKey(game, kKeyYes), PadSymbol::A}, {resources_.textForKey(game, kKeyNo), PadSymbol::B}};
        break;
    case Prompt::AgreementMissing:
        page.body = resources_.textForKey(game, kKeyAgreement);
        page.buttons = {cont};
        break;
    case Prompt::AgreementHelp:
        page.body = resources_.textForKey(game, kKeyAgreementHelp);
        page.buttons = {cont, {resources_.textForKey(system, kSystemBack), PadSymbol::B}};
        break;
    case Prompt::ParentalControls:
        page.body = resources_.textForKey(game, kKeyParental);
        page.buttons = {cont};
        break;
    case Prompt::TooManyTitles:
        page.body = resources_.textForKey(game, kKeyTooMany);
        page.buttons = {cont};
        break;
    case Prompt::StreetPassOn:
        page.body = resources_.textForKey(game, kKeyStreetPassOn);
        page.buttons = {cont};
        break;
    case Prompt::HomeNotice:
    case Prompt::None:
        page.body = resources_.textForKey(game, kKeyHome);
        page.buttons = {cont};
        break;
    }
    drawAlertPage(bottom, resources_, page);
}

void FrontEnd::drawAutosave(Canvas& bottom) {
    const std::string game = textFile("gametext");
    AlertPage page;
    page.seconds = flow_.screenSeconds();
    page.body = resources_.textForKey(game, kKeyAutosave);
    page.buttons = {{resources_.textForKey(game, kKeyOk), PadSymbol::A}};
    drawAlertPage(bottom, resources_, page);
    float opacity = std::min(1.0f, flow_.screenSeconds() / kSpinnerOnSeconds);
    if (flow_.confirming()) opacity *= std::max(0.0f, 1.0f - flow_.confirmSeconds() / kSpinnerOffSeconds);
    drawSaveSpinner(bottom, resources_, flow_.screenSeconds(), opacity);
}

void FrontEnd::drawSlots(Canvas& top, Canvas& bottom) {
    drawTitleTop(top, resources_, "uk", 1.0f);
    std::vector<SlotRow> rows;
    const std::string system = textFile("systemtext");
    for (unsigned i = 0; i < flow_.config().slotCount; ++i) {
        SlotRow row{int(i) + 1, resources_.textForKey(system, kSystemNewGame), flow_.slotUsed(i), {}, {}, {}};
        if (row.used) {
            const SlotSummary& summary = flow_.config().slots[i];
            row.time = summary.time;
            row.percent = summary.percent;
            row.location = resources_.textForKey(textFile("gametext"), summary.locationKey);
        }
        rows.push_back(row);
    }
    drawSlotPage(bottom, resources_, resources_.textForKey(system, kSystemSelectSlot),
                 resources_.textForKey(textFile("gametext"), kKeySelect), resources_.textForKey(textFile("gametext"), kKeyDelete), rows, flow_.selectedSlot(), flow_.screenSeconds(),
                 std::min(flow_.selectionSeconds(), 0.267f), flow_.confirming() ? flow_.confirmSeconds() : -1.0f);
}

void FrontEnd::drawSaving(Canvas& top, Canvas& bottom) {
    drawTitleTop(top, resources_, "uk", 1.0f);
    const std::string system = textFile("systemtext");
    AlertPage page;
    page.secondTransition = false;
    page.seconds = flow_.screenSeconds();
    page.header = resources_.textForKey(system, kSystemSaving);
    page.body = resources_.textForKey(system, kSystemDoNotTurnOff);
    drawAlertPage(bottom, resources_, page);
    drawSaveSpinner(bottom, resources_, flow_.screenSeconds(), std::min(1.0f, flow_.screenSeconds() / kSpinnerOnSeconds));
}

void FrontEnd::drawIntro(Canvas& top, Canvas& bottom) {
    std::vector<std::string> lines;
    const auto milliseconds = std::int64_t(flow_.movieSeconds() * 1000);
    for (const auto& cue : cues_)
        if (milliseconds >= std::int64_t(cue.startMs) && milliseconds < std::int64_t(cue.endMs)) lines = cue.lines;
    if (movies_ && flow_.screenUpdates() > 0) {
        const assets::Image& frame = movies_->frame();
        top.draw(frame, {0, 0, std::min(frame.width, kTopWidth), std::min(frame.height, kScreenHeight)}, 0, 0);
    }
    drawSubtitleBubble(bottom, resources_, lines);
}

void FrontEnd::draw(Canvas& top, Canvas& bottom) {
    top.clear(0, 0, 0);
    bottom.clear(0, 0, 0);
    if (flow_.blackout()) return;
    switch (flow_.screen()) {
    case Screen::Boot:
        drawBoot(bottom);
        break;
    case Screen::Logos: {
        const assets::Image& up = resources_.image("sprites/splashscreens/splash2top.btga");
        top.draw(up, {0, 0, kTopWidth, kScreenHeight}, 0, 0);
        std::string path = "sprites/splashscreens/splash2bottom_" + suffix() + ".btga";
        if (!resources_.has(path)) path = "sprites/splashscreens/splash2bottom.btga";
        if (resources_.has(path)) {
            const assets::Image& down = resources_.image(path);
            bottom.draw(down, {0, 0, kBottomWidth, kScreenHeight}, 0, 0);
        }
        break;
    }
    case Screen::FusionMovie:
        if (movies_ && flow_.screenUpdates() > 0) {
            const assets::Image& frame = movies_->frame();
            top.draw(frame, {0, 0, std::min(frame.width, kTopWidth), std::min(frame.height, kScreenHeight)}, 0, 0);
        }
        break;
    case Screen::Language:
        drawLanguage(bottom);
        break;
    case Screen::Prompt:
        drawPrompt(bottom);
        break;
    case Screen::Autosave:
        drawAutosave(bottom);
        break;
    case Screen::SlotSelect:
        drawSlots(top, bottom);
        break;
    case Screen::SavingGame:
        drawSaving(top, bottom);
        break;
    case Screen::Loading: {
        drawLoadingTop(top, resources_);
        const std::string tip = resources_.textForKey(textFile("gametext"), kKeyTip) + " " +
                                resources_.textForKey(textFile("hinttext"), kHintLoadingTip);
        drawLoadingBottom(bottom, resources_, tip);
        break;
    }
    case Screen::IntroMovie:
        drawIntro(top, bottom);
        break;
    case Screen::MissionCard:
        drawMissionCard(bottom, resources_, resources_.textForKey(textFile("hinttext"), kHintMissionTitle));
        break;
    case Screen::Done:
        break;
    }
}

}
