#pragma once

#include "qol/text_check.hpp"

#include <string>
#include <vector>

namespace qol {

struct MenuTextInputs {
    std::vector<std::string> keyNames;
    std::vector<std::string> languageNames;
    std::vector<std::string> resolutions;
};

const std::vector<std::string>& mouseButtonNames();
const std::vector<std::string>& padInputNames();
const std::vector<std::string>& actionNames();
std::vector<MenuText> menuTexts(const MenuTextInputs& inputs);

}
