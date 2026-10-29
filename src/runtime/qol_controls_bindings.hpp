#pragma once
#include "qol/controls_tab.hpp"

#include <memory>
#include <string>

namespace chase::runtime {

using qol::ControlsTab, qol::ControlsResult;

class QolControlsBindings {
public:
    explicit QolControlsBindings(const std::string& path);
    QolControlsBindings(const std::string& path, const std::string& shippedPath);

    const ControlsTab& tab() const { return *tab_; }
    const std::string& path() const { return path_; }

    ControlsResult activate(std::size_t row, std::size_t slot);
    ControlsResult assignKey(const std::string& keyName);
    ControlsResult assignInput(const std::string& source, const std::string& code);
    ControlsResult toggleMode(std::size_t row, std::size_t slot);
    ControlsResult clearSlot(std::size_t row, std::size_t slot);
    ControlsResult cancel();
    ControlsResult requestReset();
    ControlsResult confirmReset();

private:
    ControlsResult saved(ControlsResult result);

    std::string path_;
    std::unique_ptr<ControlsTab> tab_;
};

int sdlKeyResolver(const std::string& name);
std::string sdlKeyNamer(int code);

}
