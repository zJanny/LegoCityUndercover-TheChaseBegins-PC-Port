#include "frontend/hud.hpp"
#include "runtime/studs.hpp"
#include <algorithm>
#include <cmath>

namespace chase::frontend {

namespace {

constexpr float kPi = 3.14159265f;
constexpr int kStudNumberSpacingFitted = 0;   // fitted to the oracle (mean abs 5.8 against 38 at -1, 109 at -2); the call that sets it is not identified
constexpr float kStudIconBase = 22.0f;
constexpr float kStudIconGap = 32.0f;

bool findElement(const assets::UiElement& element, const std::string& name, float parentX, float parentY, float& x, float& y,
                 float& width, float& height) {
    const float ownX = parentX + element.position[0], ownY = parentY + element.position[1];
    if (element.name == name) {
        x = ownX;
        y = ownY;
        width = element.size[0];
        height = element.size[1];
        return true;
    }
    for (const auto& child : element.children)
        if (findElement(child, name, ownX, ownY, x, y, width, height)) return true;
    return false;
}

float evaluateTarget(const assets::UiLayout& layout, const std::string& storyboard, const std::string& target,
                     unsigned property, float time, float fallback) {
    for (const auto& board : layout.storyboards) {
        if (board.name != storyboard) continue;
        for (const auto& track : board.tracks)
            if (track.target == target && track.property == property) return evaluateTrack(track, time);
    }
    return fallback;
}

float storyboardLength(const assets::UiLayout& layout, const std::string& storyboard) {
    float length = 0;
    for (const auto& board : layout.storyboards) {
        if (board.name != storyboard) continue;
        for (const auto& track : board.tracks)
            length = std::max(length, track.repeat == 0 ? track.duration : track.duration * float(track.repeat));
    }
    return length;
}

std::string heartContainer(int index) {
    return "UI_Hearts_" + std::to_string(index + 1) + "_3DS";
}

}

Hud::Hud(Resources& resources, const HudConfig& config) : resources_(resources), config_(config), superBrickDraw_(resources) {
    const assets::UiLayout& layout = resources_.layout(kHudLayout);
    onLength_ = storyboardLength(layout, "Studs_ON");
    offLength_ = storyboardLength(layout, "Studs_OFF");
    findElement(layout.root, "Studs_Count", 0, 0, baseX_, baseY_, width_, height_);
    sink_ = {baseX_ + width_ / 2, baseY_ + height_ / 2};
    setHearts(config.hearts);
}

float Hud::boardLength(const std::string& layoutPath, const std::string& storyboard) {
    return storyboardLength(resources_.layout(layoutPath), storyboard);
}

void Hud::setHearts(unsigned hearts, bool vehicleImage) {
    vehicleHearts_ = vehicleImage;
    const unsigned previous = hearts_;
    for (unsigned i = 0; i < unsigned(kHudHeartSlots); ++i) {
        if (i < hearts && i >= previous) heart_[i] = {HeartBoard::Gain, 0};
        else if (i >= hearts && i < previous) heart_[i] = {HeartBoard::Lose, 0};
    }
    hearts_ = std::min<unsigned>(hearts, kHudHeartSlots);
}

void Hud::startBar(float hold) {
    barHold_ = hold;
    switch (barState_) {
    case StudBarState::Hidden:
        barState_ = StudBarState::TurningOn;
        barStart_ = seconds_;
        break;
    case StudBarState::Shown:
        barStart_ = seconds_;
        break;
    case StudBarState::TurningOff: {
        const float elapsed = offLength_ > 0 ? std::clamp((seconds_ - barStart_) / offLength_, 0.0f, 1.0f) : 1.0f;
        barState_ = StudBarState::TurningOn;
        barStart_ = seconds_ - (1.0f - elapsed) * onLength_;
        break;
    }
    case StudBarState::TurningOn: break;
    }
}

void Hud::studsBanked(unsigned type, std::uint64_t total) {
    if (type < 4) studType_ = type;
    studValue_ = total;
    changeUpdate_ = updates_;
    startBar(kStudBarHoldSeconds);
}

void Hud::update(float dt) {
    seconds_ += dt;
    ++updates_;
    superBrick_.update(dt);
    for (auto& heart : heart_) heart.time += dt;
    const float gainLength = boardLength(kHeartControlLayout, "Gain_Heart"), loseLength = boardLength(kHeartControlLayout, "Lose_Heart");
    const int last = int(hearts_) - 1;
    for (int i = 0; i < kHudHeartSlots; ++i) {
        Heart& heart = heart_[std::size_t(i)];
        const bool idle = heart.board == HeartBoard::None || heart.board == HeartBoard::Loop ||
                          (heart.board == HeartBoard::Gain && heart.time >= gainLength) ||
                          (heart.board == HeartBoard::Lose && heart.time >= loseLength);
        if (i == last) {
            if (idle && heart.board != HeartBoard::Loop) heart = {HeartBoard::Loop, 0};
        } else if (i < last && heart.board == HeartBoard::Loop) {
            heart = {HeartBoard::None, 0};
        }
    }
    switch (barState_) {
    case StudBarState::TurningOn:
        if (seconds_ - barStart_ >= onLength_) {
            barState_ = StudBarState::Shown;
            barStart_ = seconds_;
        }
        break;
    case StudBarState::Shown:
        if (seconds_ - barStart_ >= barHold_) {
            barState_ = StudBarState::TurningOff;
            barStart_ = seconds_;
        }
        break;
    case StudBarState::TurningOff:
        if (seconds_ - barStart_ >= offLength_) barState_ = StudBarState::Hidden;
        break;
    case StudBarState::Hidden: break;
    }
}

float Hud::pageOffsetY() const {
    const assets::UiLayout& layout = resources_.layout(kHudLayout);
    switch (barState_) {
    case StudBarState::Hidden: return evaluateTarget(layout, "Studs_OFF", "Stud_Count", 2, offLength_, 0);
    case StudBarState::TurningOn: return evaluateTarget(layout, "Studs_ON", "Stud_Count", 2, seconds_ - barStart_, 0);
    case StudBarState::Shown: return evaluateTarget(layout, "Studs_ON", "Stud_Count", 2, onLength_, 0);
    case StudBarState::TurningOff: return evaluateTarget(layout, "Studs_OFF", "Stud_Count", 2, seconds_ - barStart_, 0);
    }
    return 0;
}

float Hud::bounce() const {
    if (updates_ - kStudBounceUpdates >= changeUpdate_) return 0;
    return 8.0f * std::sin(float(updates_ - changeUpdate_) * 0.2f * kPi);
}

Hud::BarLayout Hud::barLayout() const {
    const BitmapFont& font = resources_.font("frontendfont");
    const auto& extended = resources_.extendedCharacters();
    BarLayout out;
    out.centreX = baseX_ + width_ / 2;
    out.topY = baseY_ + pageOffsetY() + height_ / 2;
    const float shift = bounce();
    const float textWidthPixels = float(textWidth(font, extended, std::to_string(studValue_), kStudNumberSpacingFitted));
    out.iconX = out.centreX - (textWidthPixels * 0.5f + kStudIconGap + shift);
    out.iconY = out.topY - shift * 0.5f + 2.0f;
    out.iconSize = kStudIconBase + shift;
    return out;
}

void Hud::drawStudBar(Canvas& top, double spinSeconds) {
    if (studValue_ == 0 || barState_ == StudBarState::Hidden) return;
    const BitmapFont& font = resources_.font("frontendfont");
    const auto& extended = resources_.extendedCharacters();
    const BarLayout bar = barLayout();
    const float centre = bounce() * 0.5f + 11.0f;
    sink_ = {bar.iconX + centre, bar.iconY + centre};
    drawText(top, font, extended, std::to_string(studValue_), bar.centreX, bar.topY, Align::Centre, {1, 1, 1, 1}, kStudNumberSpacingFitted, false);
    const int frame = spinSeconds > 0 ? int(spinSeconds * 15.0) : 0;
    const int column = frame & 3, row = (frame & 15) >> 2;
    const std::uint32_t tint = runtime::kStudTint[studType_];
    const Colour colour{float(tint & 0xff) / 255.0f, float((tint >> 8) & 0xff) / 255.0f, float((tint >> 16) & 0xff) / 255.0f,
                        float(tint >> 24) / 255.0f};
    top.drawScaled(resources_.image(kStudSheet), {column * 32, row * 32, 32, 32}, bar.iconX, bar.iconY, bar.iconSize,
                   bar.iconSize, colour);
}

void Hud::draw(Canvas& top, double spinSeconds) {
    LayoutScene scene;
    scene.storyboard = "TransitionOn";
    scene.hidden = {"userControl", "UI_Strike", "Struggle_Button"};
    scene.time = seconds_;
    const bool inVehicle = !vehiclePortrait_.empty();
    scene.images["UI_HUD_Image_3DS"] = "sprites/ui_hud/blend/images/heartsbg_" + (inVehicle ? std::string("generic") : config_.heartsBackground) + ".bpng";
    scene.images["UI_portrait"] = inVehicle ? vehiclePortrait_ : "sprites/portraits/" + config_.portrait + ".bpng";
    if (vehicleHearts_) scene.images["UI_Heart_Image_3DS"] = "sprites/ui_hud/blend/images/ui_heartvehicle_3ds.bpng";
    for (int i = 0; i < kHudHeartSlots; ++i) {
        const std::string name = heartContainer(i);
        scene.attachedLayouts[name] = kHeartControlLayout;
        const Heart& heart = heart_[std::size_t(i)];
        switch (heart.board) {
        case HeartBoard::None: break;
        case HeartBoard::Loop:
            scene.referenceStoryboards[name] = "Loop_Heart";
            scene.referenceTimes[name] = std::fmod(heart.time, boardLength(kHeartControlLayout, "Loop_Heart"));
            break;
        case HeartBoard::Lose:
            scene.referenceStoryboards[name] = "Lose_Heart";
            scene.referenceTimes[name] = heart.time;
            break;
        case HeartBoard::Gain:
            scene.referenceStoryboards[name] = "Gain_Heart";
            scene.referenceTimes[name] = heart.time;
            break;
        }
    }
    scene.laterStoryboards.push_back({barState_ == StudBarState::TurningOff || barState_ == StudBarState::Hidden ? "Studs_OFF" : "Studs_ON",
                                      barState_ == StudBarState::Hidden ? offLength_
                                      : barState_ == StudBarState::Shown ? onLength_
                                                                          : seconds_ - barStart_});
    drawLayout(top, resources_, kHudLayout, scene, 0, 0);
    drawStudBar(top, spinSeconds);
    superBrickDraw_.draw(top, superBrick_.view());
    if (cameraLockAlpha_ > 0.0f) {
        const assets::Image& icon = resources_.image(kCameraLockedSprite);
        top.drawScaled(icon, {0, 0, icon.width, icon.height}, 10.0f, float(kScreenHeight) - 10.0f - float(icon.height), float(icon.width),
                       float(icon.height), {1, 1, 1, float(int(cameraLockAlpha_ * 255.0f) & 0xff) / 255.0f});
    }
}

}
