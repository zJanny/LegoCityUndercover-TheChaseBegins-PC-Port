#pragma once

#include "frontend/layout.hpp"
#include "qol/menu_hit.hpp"
#include "qol/menu_model.hpp"

#include <vector>

namespace qol {

inline constexpr const char* listPageLayout = "sprites/ui_roundaboutmenu/main.bxaml";
inline constexpr const char* noticePageLayout = "sprites/blend/savegameflow/page_alerttext.bxaml";
inline constexpr std::size_t listBarCount = 5;

bool controlsListShown(const MenuModel& model);
bool controlsNoticeShown(const MenuModel& model);
bool controlsQuestionShown(const MenuModel& model);

chase::frontend::LayoutScene listPageScene(const MenuModel& model);
chase::frontend::LayoutScene noticeScene(const MenuModel& model);
chase::frontend::LayoutScene questionScene(const MenuModel& model);
std::vector<HitArea> listHitAreas(const MenuModel& model);

}
