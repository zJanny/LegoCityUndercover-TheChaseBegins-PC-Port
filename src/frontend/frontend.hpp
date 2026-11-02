#pragma once
// The front end: the Flow state machine plus the drawing of each screen into two CPU canvases (top 400x240, bottom
// 320x240). No SDL dependency: the caller presents the canvases, plays the sounds and the movie audio.
#include "assets/assets.hpp"
#include "frontend/canvas.hpp"
#include "frontend/flow.hpp"
#include "frontend/page_targets.hpp"
#include "frontend/pages.hpp"
#include "frontend/resources.hpp"
#include <memory>
#include <string>
#include <vector>

namespace chase::frontend {

class PictureHost : public MovieHost {
public:
    virtual const assets::Image& frame() const = 0;
    virtual std::vector<std::uint8_t> takeAudio() = 0;
};

class FrontEnd {
public:
    FrontEnd(Resources& resources, PictureHost* movies, FlowConfig config);
    void update(float dt, const FrontInput& input);
    void draw(Canvas& top, Canvas& bottom);
    Flow& flow() { return flow_; }
    const Flow& flow() const { return flow_; }
    std::vector<int> takeSounds() { return flow_.takeSounds(); }
    std::vector<std::uint8_t> takeMovieAudio();
    PageTargets targets() const;

private:
    std::string suffix() const;
    std::string textFile(const char* table) const;
    void startSubtitles();
    void drawBoot(Canvas& bottom);
    void drawLanguage(Canvas& bottom);
    void drawPrompt(Canvas& bottom);
    AlertPage promptPage() const;
    AlertPage autosavePage() const;
    void drawAutosave(Canvas& bottom);
    void drawSlots(Canvas& top, Canvas& bottom);
    void drawSaving(Canvas& top, Canvas& bottom);
    void drawIntro(Canvas& top, Canvas& bottom);

    Resources& resources_;
    PictureHost* movies_;
    Flow flow_;
    Screen shown_ = Screen::Boot;
    std::vector<assets::SubtitleCue> cues_;
};

}
