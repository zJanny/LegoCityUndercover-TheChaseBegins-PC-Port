#include "frontend/round_menu.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>

namespace chase::frontend {
namespace {

constexpr const char* kTransitionOn = "TransitionOn";
constexpr const char* kTransitionOff = "TransitionOff";
constexpr const char* kTransitionOnRound = "TransitionOn_Round";
constexpr const char* kTransitionOffRound = "TransitionOff_Round";
constexpr const char* kLeftMove = "Left_Move";
constexpr const char* kRightMove = "Right_Move";
constexpr const char* kLoop = "Loop";
constexpr const char* kButtonImage[kRoundMenuSlots] = {"Button_1_Image", "Button_2_Image", "Button_3_Image", "Button_4_Image", "Button_5_Image"};
constexpr const char* kButtonNew[kRoundMenuSlots] = {"Button_1_New", "Button_2_New", "Button_3_New", "Button_4_New", "Button_5_New"};
// Children of the round menu layout that the pause screen of the original does not show (oracle state gameplay-start, request r38-B-1):
// STAND-IN UI-S3, hidden by hand; the code that hides them is not found.
constexpr const char* kHiddenByHand[] = {"backButton", "selectButton"};

float storyboardLength(const assets::UiLayout& layout, const std::string& name) {
    float length = 0;
    for (const assets::UiStoryboard& board : layout.storyboards)
        if (board.name == name)
            for (const assets::UiTrack& track : board.tracks)
                length = std::max(length, track.repeat == 0 ? track.duration : track.duration * float(track.repeat));
    return length;
}

const assets::UiElement* findElement(const assets::UiElement& element, const std::string& name, float originX, float originY, float out[2]) {
    const float x = originX + element.position[0], y = originY + element.position[1];
    if (element.name == name) {
        out[0] = x;
        out[1] = y;
        return &element;
    }
    for (const assets::UiElement& child : element.children)
        if (const assets::UiElement* found = findElement(child, name, x, y, out)) return found;
    return nullptr;
}

std::string lowerCase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

std::string itemTexture(const std::string& folder, const std::string& image) {
    std::string path = lowerCase(folder + "/" + image);
    const std::string png = ".png";
    if (path.size() > png.size() && path.compare(path.size() - png.size(), png.size(), png) == 0) path.replace(path.size() - png.size(), png.size(), ".bpng");
    return path;
}

TextSlot textSlot(const std::string& text, Align align) {
    TextSlot slot;
    slot.text = text;
    slot.align = align;
    return slot;
}

}

RoundMenuTimings roundMenuTimings(const assets::UiLayout& layout) {
    RoundMenuTimings timings;
    timings.transitionOn = storyboardLength(layout, kTransitionOn);
    timings.transitionOff = storyboardLength(layout, kTransitionOff);
    timings.transitionOnRound = storyboardLength(layout, kTransitionOnRound);
    timings.transitionOffRound = storyboardLength(layout, kTransitionOffRound);
    timings.leftMove = storyboardLength(layout, kLeftMove);
    timings.rightMove = storyboardLength(layout, kRightMove);
    timings.loop = storyboardLength(layout, kLoop);
    return timings;
}

std::array<std::array<float, 2>, 3> roundMenuButtonCentres(const assets::UiLayout& layout) {
    std::array<std::array<float, 2>, 3> centres{};
    const char* names[3] = {"Button_2", "Button_3", "Button_4"};
    for (int i = 0; i < 3; ++i) {
        float position[2] = {0, 0};
        if (const assets::UiElement* button = findElement(layout.root, names[i], 0, 0, position))
            centres[std::size_t(i)] = {position[0] + button->size[0] / 2, position[1] + button->size[1] / 2};
    }
    return centres;
}

RoundMenu::RoundMenu(const RoundMenuTimings& timings, const std::array<std::array<float, 2>, 3>& centres) : timings_(timings), centres_(centres) {}

void RoundMenu::start(const std::string& name, float length, bool loop, bool atEnd) {
    boards_.erase(std::remove_if(boards_.begin(), boards_.end(), [&](const Playing& board) { return board.name == name; }), boards_.end());
    boards_.push_back({name, atEnd ? length : 0.0f, length, loop, true});
}

void RoundMenu::stop(const std::string& name) {
    for (Playing& board : boards_)
        if (board.name == name) board.running = false;
}

bool RoundMenu::finished(const std::string& name) const {
    for (const Playing& board : boards_)
        if (board.name == name) return !board.running || (!board.loop && board.time >= board.length);
    return true;
}

void RoundMenu::create() {
    start(kTransitionOn, timings_.transitionOn, false, true);
    shown_ = true;
    state_ = RoundMenuState::Idle;
}

void RoundMenu::open(const std::string& folder, const std::vector<RoundMenuItem>& items, int selected) {
    folder_ = folder;
    items_ = items;
    for (RoundMenuItem& item : items_) {
        item.enabled = true;
        item.isNew = false;
    }
    if (!shown_) create();
    transitionOnPending_ = true;
    state_ = RoundMenuState::Opening;
    const int count = int(items_.size());
    selected_ = selected >= 0 && selected < count ? selected : 0;
    move_ = 0;
    refreshTexts();
}

void RoundMenu::close(bool transitionOff) {
    stop(kTransitionOnRound);
    stop(kTransitionOn);
    stop(kLoop);
    if (state_ == RoundMenuState::Active || state_ == RoundMenuState::Opening) {
        sounds_.push_back(kRoundMenuSoundClose);
        start(kTransitionOffRound, timings_.transitionOffRound);
    }
    if (transitionOff && shown_) start(kTransitionOff, timings_.transitionOff);
    transitionOnPending_ = false;
    state_ = RoundMenuState::Closing;
}

void RoundMenu::setItemText(int item, const std::string& text) {
    if (item < 0 || item >= int(items_.size())) return;
    items_[std::size_t(item)].text = text;
    if (item == selected_) refreshTexts();
}

void RoundMenu::refreshTexts() {
    iconOutText_ = iconText_;
    iconText_ = items_.empty() ? std::string() : items_[std::size_t(selected_)].text;
}

void RoundMenu::endMove() {
    move_ = 0;
    stop(kLeftMove);
    stop(kRightMove);
    start(kTransitionOnRound, timings_.transitionOnRound, false, true);
    refreshTexts();
    start(kLoop, timings_.loop, true);
}

float RoundMenu::distance(const std::array<float, 2>& a, const std::array<float, 2>& b) {
    return std::hypot(a[0] - b[0], a[1] - b[1]);
}

int RoundMenu::touchDirection(const TouchInput& touch, bool& select, bool buttonsPressed) {
    int direction = 0;
    if (!touch.down) {
        if (!buttonsPressed && touchUpdates_ != 0 && swipeUpdate_ == 0 && touchUpdates_ < kRoundMenuTapUpdates) {
            const std::array<float, 2>& last = touchHistory_[0];
            if (distance(centres_[1], last) <= kRoundMenuTouchRadius) {
                select = true;
            } else {
                if (distance(centres_[0], last) <= kRoundMenuTouchRadius) direction = -1;
                if (distance(centres_[2], last) <= kRoundMenuTouchRadius) direction = 1;
            }
        }
        touchUpdates_ = 0;
        swipeUpdate_ = 0;
        return direction;
    }
    const std::array<float, 2> current = {touch.x, touch.y};
    if (touchUpdates_ > kRoundMenuSwipeUpdates && (swipeUpdate_ == 0 || swipeUpdate_ + kRoundMenuSwipeGapUpdates < touchUpdates_) &&
        distance(current, touchHistory_[4]) > kRoundMenuSwipeDistance) {
        swipeUpdate_ = touchUpdates_;
        direction = current[0] <= touchHistory_[4][0] ? 1 : -1;
    }
    ++touchUpdates_;
    for (std::size_t i = touchHistory_.size() - 1; i > 0; --i) touchHistory_[i] = touchHistory_[i - 1];
    touchHistory_[0] = current;
    return direction;
}

int RoundMenu::update(float dt, const RoundMenuInput& input) {
    for (Playing& board : boards_) {
        if (!board.running) continue;
        board.time += dt;
        if (board.loop && board.length > 0) board.time = std::fmod(board.time, board.length);
    }
    if (transitionOnPending_ && finished(kTransitionOn)) {
        transitionOnPending_ = false;
        start(kTransitionOnRound, timings_.transitionOnRound);
    }
    if (state_ == RoundMenuState::Opening) {
        if (!transitionOnPending_ && finished(kTransitionOnRound)) {
            state_ = RoundMenuState::Active;
            start(kLoop, timings_.loop, true);
        }
    } else if (state_ == RoundMenuState::Closing && finished(kTransitionOffRound)) {
        stop(kLeftMove);
        stop(kRightMove);
        state_ = RoundMenuState::Idle;
    }
    if (move_ != 0 && finished(move_ > 0 ? kLeftMove : kRightMove) && state_ != RoundMenuState::Closing && state_ != RoundMenuState::Idle) endMove();

    bool select = false, back = false;
    if (state_ == RoundMenuState::Active) {
        select = input.a;
        back = input.b;
        int direction = touchDirection(input.touch, select, input.a);
        if (input.left) direction = -1;
        if (input.right) direction = 1;
        if (direction != 0 && !items_.empty()) {
            sounds_.push_back(kRoundMenuSoundMove);
            start(direction < 0 ? kRightMove : kLeftMove, direction < 0 ? timings_.rightMove : timings_.leftMove);
            const int count = int(items_.size());
            selected_ = ((selected_ + direction) % count + count) % count;
            move_ = direction;
            refreshTexts();
        }
    }
    if (!items_.empty() && !items_[std::size_t(selected_)].enabled) select = false;
    if (select) return 1;
    return back ? -1 : 0;
}

int RoundMenu::itemAt(int slot) const {
    const int count = int(items_.size());
    if (count == 0) return -1;
    return ((selected_ + slot - move_ - 2) % count + count) % count;
}

std::vector<int> RoundMenu::takeSounds() {
    std::vector<int> out;
    out.swap(sounds_);
    return out;
}

LayoutScene RoundMenu::scene() const {
    LayoutScene scene;
    for (const Playing& board : boards_) {
        if (scene.storyboard.empty()) {
            scene.storyboard = board.name;
            scene.time = board.time;
        } else {
            scene.laterStoryboards.push_back({board.name, board.time});
        }
    }
    for (const char* name : kHiddenByHand) scene.hidden.insert(name);
    for (int slot = 0; slot < kRoundMenuSlots; ++slot) {
        const int item = itemAt(slot);
        if (item < 0 || items_[std::size_t(item)].image.empty()) scene.hidden.insert(kButtonImage[slot]);
        else scene.images[kButtonImage[slot]] = itemTexture(folder_, items_[std::size_t(item)].image);
        if (item < 0 || !items_[std::size_t(item)].isNew) scene.hidden.insert(kButtonNew[slot]);
    }
    scene.text["TEXT_Title"] = textSlot(title_, Align::Left);
    scene.text["TEXT_Subtitle"] = textSlot(subtitle_, Align::Left);
    scene.text["TEXT_Icon"] = textSlot(iconText_, Align::Centre);
    scene.text["TEXT_Icon_In"] = textSlot(iconText_, Align::Centre);
    scene.text["TEXT_Icon_Out"] = textSlot(iconOutText_, Align::Centre);
    return scene;
}

}
