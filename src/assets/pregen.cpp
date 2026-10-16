#include "assets.hpp"
#include <stdexcept>

namespace chase::assets {

CollectableLists parseCollectableLists(const std::vector<std::uint8_t>& data) {
    constexpr std::size_t kFileSize = 1 + 2 * (20 + 10 + 5);
    if (data.size() != kFileSize) throw std::runtime_error("collectables file is not 71 bytes");
    if (data[0] != 2) throw std::runtime_error("collectables file version is not 2");
    auto at = [&](std::size_t index) {
        return std::int16_t(std::uint16_t(data[1 + index * 2] | (data[2 + index * 2] << 8)));
    };
    CollectableLists lists;
    for (std::size_t i = 0; i < lists.groupA.size(); ++i) lists.groupA[i] = at(i);
    for (std::size_t i = 0; i < lists.groupB.size(); ++i) lists.groupB[i] = at(20 + i);
    for (std::size_t i = 0; i < lists.groupC.size(); ++i) lists.groupC[i] = at(30 + i);
    return lists;
}

std::vector<std::uint16_t> parseLevelObjectCounts(const std::vector<std::uint8_t>& data) {
    if (data.empty() || data.size() % 2) throw std::runtime_error("level object counts file is not a list of 16-bit values");
    std::vector<std::uint16_t> counts(data.size() / 2);
    for (std::size_t i = 0; i < counts.size(); ++i) counts[i] = std::uint16_t(data[i * 2] | (data[i * 2 + 1] << 8));
    return counts;
}

}
