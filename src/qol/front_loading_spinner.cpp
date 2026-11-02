#include "qol/front_loading_spinner.hpp"

#include "frontend/pages.hpp"
#include "qol/active_settings.hpp"
#include "qol/front_page_rules.hpp"

namespace qol {

namespace fe = chase::frontend;

void drawFrontLoadingSpinner(fe::Canvas& bottom, fe::Resources& resources, const fe::Flow& flow) {
    const auto place = loadingSpinnerFor(activeSettings(), flow.screen(), flow.blackout());
    if (!place) return;
    fe::drawSaveSpinnerAt(bottom, resources, flow.screenSeconds(), 1.0f, place->x, place->y, place->size);
}

}
