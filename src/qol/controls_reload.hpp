#pragma once

#include "qol/active_settings.hpp"
#include "qol/controls_port.hpp"

#include <functional>
#include <memory>

namespace qol {

class ReloadingControls final : public ControlsPort {
public:
    using Make = std::function<std::unique_ptr<ControlsPort>()>;
    using MakeSet = std::function<std::unique_ptr<ControlsPort>(ControlsSet)>;

    explicit ReloadingControls(Make make) : make_([make = std::move(make)](ControlsSet) { return make(); }), current_(make_(set_)) {}
    explicit ReloadingControls(MakeSet make)
        : make_(std::move(make)), setsAvailable_([] { return enabled(Feature::PcControls); }), current_(make_(set_)) {}

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
    void reload() override { current_ = make_(set_); }
    bool hasSets() const override { return setsAvailable_ && setsAvailable_(); }
    ControlsSet set() const override { return set_; }
    void selectSet(ControlsSet set) override {
        if (!hasSets() || set == set_ || current_->tab().waiting() || current_->tab().conflictPending() || current_->tab().confirming()) return;
        set_ = set;
        current_ = make_(set_);
    }

private:
    MakeSet make_;
    std::function<bool()> setsAvailable_;
    ControlsSet set_ = ControlsSet::OnFoot;
    std::unique_ptr<ControlsPort> current_;
};

}
