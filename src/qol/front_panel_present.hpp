#pragma once

#include "frontend/canvas.hpp"
#include "frontend/frontend.hpp"
#include "render/front_presenter.hpp"

namespace qol {

bool presentFrontPanels(chase::render::FrontPresenter& presenter, const chase::frontend::Canvas& top, const chase::render::Pixels& bottom,
    chase::frontend::Resources& resources, const chase::frontend::FrontEnd& front, int windowWidth, int windowHeight);

}
