#pragma once
// Shadows of the original (docs/re/rendering-shadows.md): one light that casts shadows renders the caster mesh nodes into a 512 x 512
// shadow texture through an orthographic camera around a focus point in front of the game camera; receivers append two combiner
// stages that darken where the texture says "in shadow". This file holds the plain math; the passes are in src/render/shadow.*.
// No SDL or GL dependency.
#include "assets/assets.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace chase::runtime {

using ShadowMatrix = std::array<float, 16>;  // game layout: rows 0..2 right, up, forward; translation at 12..14

constexpr float kShadowRadiusDefault = 15.0f;     // ShadowRadius default of the level settings (0x2e6710) and of the light (0x1b22cc)
constexpr float kShadowOpacityDefault = 0.5f;     // ShadowOpacity default of 0x22dedc
constexpr float kShadowFocusAhead = 0.8f;         // 0x2e4d5c: the focus moves 0.8 radius ahead and the ray is clamped to it
constexpr float kShadowNear = -50.0f;             // 0x2e4d64, passed to 0x1c1b68
constexpr float kShadowFar = 25.0f;               // 0x2e4d60
constexpr int kShadowTextureSize = 512;           // 0x17e27c: width and height of the shadow texture of a light
// STAND-IN RENDER-S68: the camera values 0x52a17c and 0x52a178 the focus ray is built from, as read in the oracle outside the police
// station (0.0 and -1.0, so the ray is the view direction); their writer is not found.
constexpr float kShadowRayUp = 0.0f;
constexpr float kShadowRayForward = -1.0f;

// The light that casts the shadow: the first light object (registry order) whose CastShadows is set; 0x15dd50 reads CastShadows
// for types 1 and 4 only and type 5 always casts (0x23aa24).
struct ShadowLight {
    std::size_t lightIndex = 0;                  // index in the level's light objects
    float opacity = kShadowOpacityDefault;       // ShadowOpacity, clamped to 0..1 by 0x101864
    std::array<float, 3> colour = {0, 0, 0};     // ShadowColour (0..255)
    float radius = kShadowRadiusDefault;
};
// The radius: 0x1b20bc writes the ShadowRadius of a directional casting light (default 15) over the level's (0x2e6264, default 15).
std::optional<ShadowLight> shadowLight(const std::vector<assets::LightObject>& lights, const std::optional<float>& levelRadius);

// 0x265dd8: the constant colour of the shadow stage, per channel trunc((255 - colour) * opacity + 0.5).
std::array<std::uint8_t, 3> shadowConstant(const std::array<float, 3>& colour, float opacity);
// The two stages 0x12ecd4 appends: texture0 x constant + (1 - constant), then that times the colour of the material's stages; the
// factor is 1 where the shadow texture is 1 (lit) and 1 - constant / 255 where it is 0.
std::array<float, 3> shadowFactor(const std::array<std::uint8_t, 3>& constant, float lit);

// The focus of the frame pass 0x2e4858 (case 1), written by 0x1c1b68: the ray (0, frustumY, -frustumZ) of the camera (0x52a17c,
// 0x52a178) meets the player's height comp+0x240 at most 0.8 radius away, then the point moves 0.8 radius along the camera's
// horizontal forward. Nothing when the player is a radius or more above or below the camera or the ray does not point down; the
// previous focus then stays.
std::optional<std::array<float, 3>> shadowFocus(const ShadowMatrix& camera, float frustumY, float frustumZ, float playerY, float radius);

// 0x2ca6e8: the camera of the shadow pass. The focus moves forward until it is 0.8 radius from the camera; the light's orientation
// is kept and its position is focus + (near - 1) * forward. Orthographic, scale 1 / radius on both axes, depth 1 .. (far - near) + 1.
struct ShadowCamera {
    ShadowMatrix world{};
    float radius = kShadowRadiusDefault;
    float nearZ = 1.0f, farZ = kShadowFar - kShadowNear + 1.0f;
};
ShadowCamera shadowCamera(const ShadowMatrix& light, const std::array<float, 3>& focus, const ShadowMatrix& camera, float radius);
// World to clip space of the shadow camera (column-major like render::Mat4): x and y over [-radius, radius], depth from nearZ
// (-1) to farZ (+1) along the light's forward axis.
std::array<float, 16> shadowViewProjection(const ShadowCamera& camera);

// Shadow_Caster / Shadow_Receiver of a placed entity (0x1c22d0, 0x1cfb1c, 0x21db64): bool class parameters, 0 when the class does not
// list them; they set bits 0 and 1 of +0xde of every mesh node of the entity.
struct ShadowFlags {
    bool caster = false, receiver = false;
};
ShadowFlags shadowFlags(const assets::LevelObject& object, const assets::Level& cls);

}
