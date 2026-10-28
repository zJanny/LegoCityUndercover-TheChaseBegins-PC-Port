#pragma once

#include <array>
#include <cstddef>

namespace qol {

enum class Feature : std::size_t {
    WidenedView,
    Count
};

struct FeatureInfo {
    Feature feature;
    const char* name;
};

inline constexpr std::array<FeatureInfo, static_cast<std::size_t>(Feature::Count)> featureTable{{
    {Feature::WidenedView, "widened_view"},
}};

inline constexpr std::size_t featureCount = featureTable.size();

}
