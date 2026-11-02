#pragma once

#include "qol/front_panel.hpp"

#include <vector>

namespace qol {

ClickAction panelClick(const PageRect& panel, const std::vector<ClickTarget>& targets, float windowX, float windowY);

class FrontClickQueue {
public:
    void add(const ClickAction& action);
    ClickAction take();

private:
    ClickAction pending_;
};

}
