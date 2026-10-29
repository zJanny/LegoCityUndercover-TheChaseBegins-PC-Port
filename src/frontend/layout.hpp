#pragma once
// Draws .bxaml UI layouts (docs/re/ui-layouts.md) into a Canvas. No SDL dependency.
// STAND-INS: the original's property evaluator (storyboard playback, 0x1dbb80 and the code that applies the track
// property ids) is not recovered. Track properties follow the data fit of ui-layouts.md (1/2 translate x/y,
// 3/4 scale x/y, 5 rotation, 6 opacity) and keyframes use the spline stored in the file as a cubic Bezier.
#include "frontend/resources.hpp"
#include <array>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace chase::frontend {

struct TextSlot {
    std::string text;
    Align align = Align::Centre;
    bool wrap = false;
    int pitch = 0;
    int spacing = kSpacingDefault;
    // Vertical shift in pixels. STAND-IN: measured per element; the rule behind it (font size field) is not recovered.
    int offsetY = 0;
    // Horizontal shift in pixels. STAND-IN (FE-S31): measured for the slot list percent, the rule behind it is not recovered.
    int offsetX = 0;
    bool tinted = false;
    float tint[3] = {1, 1, 1};
    static constexpr int kSpacingDefault = 1 << 20;
};

struct PlayingStoryboard {
    std::string name;
    float time = 0;
};

struct LayoutScene {
    std::map<std::string, TextSlot> text;
    std::set<std::string> hidden;
    std::map<std::string, std::string> images;
    std::map<std::string, std::string> referenceStoryboards;
    std::map<std::string, float> referenceTimes;
    std::string storyboard;
    float time = 0;
    std::vector<PlayingStoryboard> laterStoryboards;
    std::map<std::string, std::string> attachedLayouts;
    // Element positions set by code (the layout node's x and y replaced, as 0x20354c does for one axis); storyboard offsets still apply.
    std::map<std::string, std::array<float, 2>> positions;
    // Element opacities set by code: replace the file value and every storyboard track of that element. Empty for every port screen.
    std::map<std::string, float> opacities;
    float opacity = 1;
    // 0x2b4dc4 sets the font spacing to the node byte +0x141 (0 for every layout node) clamped by the font minimum.
    int textSpacing = 0;
    // STAND-IN (SCAN-S5): the scanner overlay draws rotation (element rotation field and track property 5) and lets scale tracks
    // replace the element scale; the original's property evaluator is not recovered. Off for every other layout.
    bool rotations = false;
    // Text drawn through the node scale (glyphs, pitch and offsets scaled with the element frame, as 0x2b4dc4 draws through the node
    // matrix). Off by default: the port's screens draw layout text at glyph scale 1 whatever the frame scale.
    bool scaleText = false;
};

std::string layoutTexturePath(const std::string& layoutPath, const std::string& source);
std::string layoutFolder(const std::string& layoutPath);

void drawLayout(Canvas& canvas, Resources& resources, const std::string& layoutPath, const LayoutScene& scene, float x,
                float y, float scale = 1.0f);

// The box of a named element in layout pixels: the positions of its parents summed and its size, without storyboards or scale. A path
// "Reference/Child" looks for Child inside the layout a reference element named Reference points to. Empty without such an element.
std::optional<std::array<float, 4>> layoutElementBox(Resources& resources, const std::string& layoutPath, const std::string& path);

float evaluateTrack(const assets::UiTrack& track, float time);

// A child of the layout's root after the storyboards played in order: the position the node holds (+0xf0: the layout position, the translate
// and the shift of a scale around its origin) and its layout size (+0x54, +0x58, not scaled). Empty without such a child.
std::optional<std::array<float, 4>> layoutChildBox(const assets::UiLayout& layout, const std::vector<PlayingStoryboard>& storyboards,
                                                   const std::string& name);
std::vector<std::string> wrapText(const BitmapFont& font, const std::vector<std::string>& extended,
                                  const std::string& text, int width, int spacing);

}
