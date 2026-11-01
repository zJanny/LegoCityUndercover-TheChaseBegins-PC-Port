#pragma once

namespace qol {

enum class BackdropStep { Defer, Picture, Black };

BackdropStep backdropStepFor(bool frameKept, bool asked);

class Backdrop {
public:
    BackdropStep step();
    void draw(int width, int height) const;
    void release();
    bool asked() const { return asked_; }

private:
    bool asked_ = false;
};

}
