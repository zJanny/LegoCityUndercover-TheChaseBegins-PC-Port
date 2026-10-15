#pragma once
// Several animation drivers behind the one the script runner knows: a play or stop goes to every driver (a driver ignores instances it does not
// animate), the wait test comes from the first driver that animates the instance. No SDL dependency.
#include "runtime/natives_animation.hpp"

#include <vector>

namespace chase::runtime {

class AnimationDriverChain : public AnimationDriver {
public:
    void add(AnimationDriver* driver) { drivers_.push_back(driver); }
    void play(const AnimationPlay& play) override {
        for (AnimationDriver* driver : drivers_) driver->play(play);
    }
    void stop(const std::string& lowerCaseInstance) override {
        for (AnimationDriver* driver : drivers_) driver->stop(lowerCaseInstance);
    }
    std::optional<bool> currentNearEnd(const std::string& lowerCaseInstance) const override {
        for (const AnimationDriver* driver : drivers_)
            if (const std::optional<bool> near = driver->currentNearEnd(lowerCaseInstance)) return near;
        return std::nullopt;
    }

private:
    std::vector<AnimationDriver*> drivers_;
};

}
