#pragma once

#include "qol/controls_tab.hpp"

#include <cstddef>
#include <string>

namespace qol {

class ControlsPort {
public:
    virtual ~ControlsPort() = default;
    virtual const ControlsTab& tab() const = 0;
    virtual ControlsResult activate(std::size_t row, std::size_t slot) = 0;
    virtual ControlsResult assignInput(const std::string& source, const std::string& code) = 0;
    virtual ControlsResult toggleMode(std::size_t row, std::size_t slot) = 0;
    virtual ControlsResult clearSlot(std::size_t row, std::size_t slot) = 0;
    virtual ControlsResult cancel() = 0;
    virtual ControlsResult confirmSwap() = 0;
    virtual ControlsResult cancelConflict() = 0;
    virtual ControlsResult requestReset() = 0;
    virtual ControlsResult confirmReset() = 0;
};

class ControlsTabPort final : public ControlsPort {
public:
    explicit ControlsTabPort(ControlsTab& tab) : tab_(tab) {}
    const ControlsTab& tab() const override { return tab_; }
    ControlsResult activate(std::size_t row, std::size_t slot) override { return tab_.activate(row, slot); }
    ControlsResult assignInput(const std::string& source, const std::string& code) override { return tab_.assignInput(source, code); }
    ControlsResult toggleMode(std::size_t row, std::size_t slot) override { return tab_.toggleMode(row, slot); }
    ControlsResult clearSlot(std::size_t row, std::size_t slot) override { return tab_.clearSlot(row, slot); }
    ControlsResult cancel() override { return tab_.cancel(); }
    ControlsResult confirmSwap() override { return tab_.confirmSwap(); }
    ControlsResult cancelConflict() override { return tab_.cancelConflict(); }
    ControlsResult requestReset() override { return tab_.requestReset(); }
    ControlsResult confirmReset() override { return tab_.confirmReset(); }

private:
    ControlsTab& tab_;
};

}
