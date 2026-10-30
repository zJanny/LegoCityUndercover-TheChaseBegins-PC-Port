#pragma once

#include "render/front_presenter.hpp"

namespace qol {

bool presentFrontPanels(chase::render::FrontPresenter& presenter, const chase::render::Pixels& top, const chase::render::Pixels& bottom,
    int windowWidth, int windowHeight);

}
