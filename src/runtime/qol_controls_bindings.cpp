#include "runtime/qol_controls_bindings.hpp"

#include "qol/controls_file.hpp"

#include <SDL.h>
#include <fstream>
#include <stdexcept>

namespace chase::runtime {

using qol::BindingEntry, qol::ControlsTab, qol::AnyInput, qol::controlActions, qol::fileInputValidator, qol::readBindingEntries, qol::writeBindingFile;
namespace {

std::vector<BindingEntry> readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot open controls file " + path);
    return readBindingEntries(file, sdlKeyResolver, sdlKeyNamer);
}

}

int sdlKeyResolver(const std::string& name) {
    return SDL_GetKeyFromName(name.c_str());
}

std::string sdlKeyNamer(int code) {
    return SDL_GetKeyName(code);
}

std::string shippedPathNextTo(const std::string& path) {
    auto slash = path.find_last_of('/');
    return (slash == std::string::npos ? std::string() : path.substr(0, slash + 1)) + "controls.default.cfg";
}

QolControlsBindings::QolControlsBindings(const std::string& path) : QolControlsBindings(path, shippedPathNextTo(path)) {}

QolControlsBindings::QolControlsBindings(const std::string& path, const std::string& shippedPath)
    : path_(path), tab_(std::make_unique<ControlsTab>(AnyInput{}, controlActions(), readFile(path), readFile(shippedPath),
          fileInputValidator(sdlKeyResolver))) {}

ControlsResult QolControlsBindings::saved(ControlsResult result) {
    if (result == ControlsResult::Changed) writeBindingFile(path_, tab_->bindings(), sdlKeyResolver);
    return result;
}

ControlsResult QolControlsBindings::activate(std::size_t row, std::size_t slot) { return tab_->activate(row, slot); }
ControlsResult QolControlsBindings::assignKey(const std::string& keyName) { return saved(tab_->assignKey(keyName)); }
ControlsResult QolControlsBindings::assignInput(const std::string& source, const std::string& code) {
    return saved(tab_->assignInput(source, code));
}
ControlsResult QolControlsBindings::toggleMode(std::size_t row, std::size_t slot) { return saved(tab_->toggleMode(row, slot)); }
ControlsResult QolControlsBindings::clearSlot(std::size_t row, std::size_t slot) { return saved(tab_->clearSlot(row, slot)); }
ControlsResult QolControlsBindings::cancel() { return tab_->cancel(); }
ControlsResult QolControlsBindings::requestReset() { return tab_->requestReset(); }
ControlsResult QolControlsBindings::confirmReset() { return saved(tab_->confirmReset()); }

}
