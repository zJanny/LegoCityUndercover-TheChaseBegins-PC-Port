#pragma once

#include <array>
#include <cstddef>

namespace qol {

enum class Feature : std::size_t {
    WidenedView,
    PcControls,
    Count
};

struct FeatureInfo {
    Feature feature;
    const char* name;
    const char* label;
};

inline constexpr std::array<FeatureInfo, static_cast<std::size_t>(Feature::Count)> featureTable{{
    {Feature::WidenedView, "widened_view", "Widened view"},
    {Feature::PcControls, "pc_controls", "PC controls"},
}};

inline constexpr std::size_t featureCount = featureTable.size();

}
