#pragma once

#include "frontend/canvas.hpp"
#include "frontend/flow.hpp"
#include "frontend/resources.hpp"

namespace qol {

void drawFrontLoadingSpinner(chase::frontend::Canvas& bottom, chase::frontend::Resources& resources, const chase::frontend::Flow& flow);

}
