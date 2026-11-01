#pragma once
// The rise of a build-it's final prop (state 10, docs/re/buildit.md) is the only time the build-it moves the prop's node: the check
// 0x1d4320 sets the height and the change to state 11 clears it. Outside the rise the prop's node belongs to its own object (a pushed
// pushable, a carried or attached prop), so the scene transform is written only while the height is non-zero and once more when it returns to 0.
// No SDL dependency.

namespace chase::runtime {

class FinalPropRise {
public:
    // True when the build-it must write the final prop's transform for this height.
    bool writes(float rise);

private:
    float last_ = 0.0f;
};

}
