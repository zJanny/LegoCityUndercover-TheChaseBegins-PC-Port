#pragma once

#include "qol/controls_port.hpp"

#include <functional>
#include <memory>

namespace qol {

class ReloadingControls final : public ControlsPort {
public:
    using Make = std::function<std::unique_ptr<ControlsPort>()>;

    explicit ReloadingControls(Make make) : make_(std::move(make)), current_(make_()) {}

    const ControlsTab& tab() const override { return current_->tab(); }
    ControlsResult activate(std::size_t row, std::size_t slot) override { return current_->activate(row, slot); }
    ControlsResult assignInput(const std::string& source, const std::string& code) override {
        return current_->assignInput(source, code);
    }
    ControlsResult toggleMode(std::size_t row, std::size_t slot) override { return current_->toggleMode(row, slot); }
    ControlsResult clearSlot(std::size_t row, std::size_t slot) override { return current_->clearSlot(row, slot); }
    ControlsResult cancel() override { return current_->cancel(); }
    ControlsResult confirmSwap() override { return current_->confirmSwap(); }
    ControlsResult cancelConflict() override { return current_->cancelConflict(); }
    ControlsResult requestReset() override { return current_->requestReset(); }
    ControlsResult confirmReset() override { return current_->confirmReset(); }
    void reload() override { current_ = make_(); }

private:
    Make make_;
    std::unique_ptr<ControlsPort> current_;
};

}
