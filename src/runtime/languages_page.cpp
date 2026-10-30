#include "runtime/languages_page.hpp"

#include <stdexcept>
#include <string>

namespace chase::runtime {

LanguagesPageActions LanguagesPage::enter() {
    LanguagesPageActions actions;
    actions.backShortcut = true;
    pending_ = 0;
    return actions;
}

LanguagesPageActions LanguagesPage::selectionChanged(int selected) {
    LanguagesPageActions actions;
    if (pending_ == 0) actions.selectShortcut = selected != -1;
    return actions;
}

LanguagesPageActions LanguagesPage::a(int selected) {
    if (selected < 0 || selected >= kLanguageCount) throw std::out_of_range("language row out of range: " + std::to_string(selected));
    LanguagesPageActions actions;
    actions.sounds.push_back(kLanguagesSoundSelect);
    pending_ = kLanguageTable[std::size_t(selected)];
    actions.closeList = true;
    actions.selectShortcut = false;
    actions.backShortcut = false;
    return actions;
}

LanguagesPageActions LanguagesPage::b() {
    LanguagesPageActions actions;
    actions.sounds.push_back(kLanguagesSoundBack);
    actions.optionsPage = true;
    return actions;
}

LanguagesPageActions LanguagesPage::leaveRequest() {
    LanguagesPageActions actions;
    actions.closeList = true;
    actions.selectShortcut = false;
    actions.backShortcut = false;
    return actions;
}

LanguagesPageActions LanguagesPage::update(bool listBusy, std::uint8_t* options) {
    LanguagesPageActions actions;
    if (pending_ == 0 || listBusy) return actions;
    options[kLanguageOffset] = pending_;
    actions.languageChanged = pending_;
    actions.saveRequest = true;
    pending_ = 0;
    return actions;
}

}
