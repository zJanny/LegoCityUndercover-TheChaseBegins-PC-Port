#pragma once
// Stereo gains of a positional sound, replicating the emitter update 0x107fa0 (distance volume and pan from the
// listener matrix 0x52ac10) and the mix matrix set by 0x25c388. Evidence: docs/re/audio.md. No SDL dependency.
#include <array>

namespace chase::audio {

// The listener matrix at 0x52ac10: rows 0..2 are the right, up and forward axes, row 3 the position; the velocity is the
// vector at 0x52ac50 that the Doppler factor of 0x25c388 reads (writer unread, AUDIO-S35).
struct Listener {
    std::array<float, 3> right{1, 0, 0}, up{0, 1, 0}, forward{0, 0, 1};
    std::array<float, 3> position{0, 0, 0};
    std::array<float, 3> velocity{0, 0, 0};
};

// Right/up/forward axes of a camera looking along `forward` with row 0 = normalize(fz, 0, -fx) (camera.md, 0x22859c)
// and row 1 = row 2 x row 0 (the listener matrix read in the oracle satisfies both).
Listener listenerFromForward(const std::array<float, 3>& position, const std::array<float, 3>& forward);

struct StereoGains {
    float left = 0;
    float right = 0;
};

// Emitter parameters of a sound effect row: volume (percent / 100), minimum and maximum distance (table paramA and
// paramB, emitter slot +0x20 and +0x24).
struct EmitterShape {
    float volume = 1.0f;
    float minimumDistance = 0;
    float maximumDistance = 0;
};

// Volume factor of the distance: 1 up to the minimum distance, 0 from the maximum distance on, linear between.
float distanceVolume(float distance, float minimumDistance, float maximumDistance);

// Pan in -1..1 (positive = right): x of the emitter in listener space with y dropped, normalised in the xz plane (0x257ea0);
// 0 when the horizontal offset is not above 2^-23.
float panOf(const Listener& listener, const std::array<float, 3>& emitter);

// Left/right gains of a mono source: the far side is attenuated by 1 - |pan|; both sides are scaled by the volume,
// the distance factor and the master volume. A sound whose distance volume is 0 is silent.
StereoGains positionalGains(const EmitterShape& shape, const Listener& listener, const std::array<float, 3>& emitter,
                            float masterVolume);

// Gains of a non-positional sound: volume * master on both sides.
StereoGains flatGains(const EmitterShape& shape, float masterVolume);

} // namespace chase::audio
