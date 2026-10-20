#pragma once
// The alpha fade of a mesh node (docs/re/pedestrians-fade.md): the start 0x275434 that a spawn runs and the per-frame step
// 0x10eb70 that moves the node's alpha byte +0xdd by its step byte +0xdc. No SDL dependency.
#include "runtime/pedestrian_manager.hpp"

#include <cstdint>

namespace chase::runtime {

constexpr std::uint8_t kNodeFadeOpaque = 255;
constexpr std::int8_t kNodeFadeStepIn = 16;
constexpr std::int8_t kNodeFadeStepOut = -16;

struct NodeFade {
    std::uint8_t alpha = kNodeFadeOpaque;  // node +0xdd; 255 from the node's creation 0x10d2d4
    std::int8_t step = 0;                  // node +0xdc, a signed byte
};

// 0x275434 on a node whose type is the mesh node type: +0xdd = 1, +0xdc = 0xff (a step of -1).
void nodeFadeStart(NodeFade& fade);

struct NodeFadeFrame {
    int tier = -1;              // pedestrianFadeTier of the node; outside 0 .. 2 the step returns without a change
    float radius = 0;           // node +0x9c
    float distanceSquared = 0;  // squared distance of the node's world centre from the fade point 0x52a1b4
    bool jumpToTarget = false;  // byte 0x52a1cc
    bool forceFadeIn = false;   // byte 0x52a1cd
};

// 0x10eb70 after its gates (bit 2 of 0x529dc8, bit 9 of the node word, bit 4 of +0xde): a step below 0 turns to +16 when the
// squared distance beyond the radius is within the squared fade-in distance of the tier, a step of 0 or more turns to -16 when it
// reaches the squared fade-out distance, and the alpha moves by the step, clamped to 0 .. 255 (or jumps to 0 / 255).
void nodeFadeStep(NodeFade& fade, const NodeFadeFrame& frame, const DistanceFadeTable& table);

float nodeFadeOpacity(const NodeFade& fade);

}
