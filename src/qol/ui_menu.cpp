#include "qol/ui_menu.hpp"

#include "qol/controls_swap.hpp"

#include <stdexcept>
#include <utility>

namespace qol::ui {

const char* const resetYesText = "Yes";
const char* const resetNoText = "No";

enum class MenuStore { Game, Display, Controls, Qol };

struct MenuOutcome {
    bool changed = false;
    std::optional<std::string> confirm;
    std::optional<KeepSettingsChange> keep;
    std::string message;
    bool quit = false;
    bool settingsChanged = false;
};

struct MenuPageEntry {
    std::string title;
    MenuStore store;
    std::function<std::vector<RowSpec>()> rows;
    std::function<DropdownInfo(int)> dropdown;
    std::function<std::vector<std::string>(int)> dropdownItems;
    std::function<std::string(int)> help;
    std::function<std::string()> pageHelp;
    std::vector<std::string> columnTitles;
    std::function<MenuOutcome(const Action&)> handle;
    std::function<MenuOutcome()> reset;
};

namespace {

constexpr int swapButton = 0;
constexpr int confirmButton = 0;
constexpr int cancelButton = 1;

DisplayValues valuesOf(const DisplayState& state) { return {state.windowMode, state.resolution}; }

bool isPadKey(Key key) {
    switch (key) {
    case Key::PadUp:
    case Key::PadDown:
    case Key::PadLeft:
    case Key::PadRight:
    case Key::PadA:
    case Key::PadB:
    case Key::PadLB:
    case Key::PadRB: return true;
    default: return false;
    }
}

MenuOutcome outcomeOf(const GamePageResult& r) { return {r.changed, r.needsConfirm, std::nullopt, {}, r.quitAsked}; }

MenuOutcome outcomeOf(const DisplayPageResult& r) { return {r.changed, r.needsConfirm, r.needsKeepSettings, {}, false, r.settingsChanged}; }

MenuOutcome outcomeOf(const QolPageOutcome& r) {
    MenuOutcome out;
    out.changed = r.kind == QolOutcomeKind::Changed;
    if (r.kind == QolOutcomeKind::NeedsConfirm) out.confirm = r.question;
    return out;
}

MenuOutcome outcomeOf(const ControlsPageOutcome& r) {
    MenuOutcome out;
    out.changed = r.event == ControlsPageEvent::Changed;
    if (r.event == ControlsPageEvent::NeedsConfirm) out.confirm = r.text;
    if (r.event == ControlsPageEvent::Rejected) out.message = r.text;
    return out;
}

std::string helpText(const PageHelp& help) { return help.text; }
std::string helpText(std::string help) { return help; }

template <class PageType>
MenuPageEntry entryFor(std::string title, MenuStore store, PageType& page) {
    MenuPageEntry entry;
    entry.title = std::move(title);
    entry.store = store;
    entry.rows = [&page] { return page.rows(); };
    entry.dropdown = [&page](int row) { return page.dropdown(row); };
    entry.dropdownItems = [](int) { return std::vector<std::string>{}; };
    entry.help = [&page](int row) { return helpText(page.help(row)); };
    entry.handle = [&page](const Action& action) { return outcomeOf(page.handle(action)); };
    entry.reset = [&page] { return outcomeOf(page.reset()); };
    return entry;
}

}

MenuSetup menuSetupFor(MenuStores& stores, std::vector<std::string> languageNames) {
    MenuSetup setup;
    setup.settings = &stores.settings();
    setup.game = stores.game();
    setup.languageNames = std::move(languageNames);
    setup.startLanguage = stores.startLanguage();
    setup.controls = stores.controls();
    setup.quitTab = stores.quitTab();
    if (stores.hasDisplay()) setup.display = DisplaySetup{stores.modes(), stores.desktop()};
    return setup;
}

UiMenu::UiMenu(TextMeasure measure, LineHeight lineHeight) : measure_(std::move(measure)), lineHeight_(std::move(lineHeight)) {
    if (!measure_ || !lineHeight_) throw std::invalid_argument("the settings menu needs a text measure and a line height");
}

UiMenu::~UiMenu() = default;

void UiMenu::open(const MenuSetup& setup) {
    if (!setup.settings) throw std::invalid_argument("the settings menu needs the settings store");
    pages_.clear();
    game_.reset();
    display_.reset();
    controls_.reset();
    qol_.reset();
    keep_.reset();
    confirm_.reset();
    confirmQuit_ = false;
    message_.clear();
    pending_ = {};
    gameOptions_ = setup.game;
    controlsPort_ = setup.controls;
    if (setup.game) game_.emplace(*setup.game, setup.languageNames, setup.startLanguage);
    else if (setup.quitTab) game_.emplace();
    if (game_) pages_.push_back(entryFor("Game", MenuStore::Game, *game_));
    if (setup.display) {
        display_.emplace(*setup.settings, setup.display->choices, setup.display->desktop);
        keep_.emplace(*setup.settings, setup.display->desktop, [this](const Resolution&, WindowMode) {
            pending_.settingsChanged = true;
            pending_.windowChanged = true;
        });
        MenuPageEntry entry = entryFor("Display", MenuStore::Display, *display_);
        entry.dropdownItems = [this](int row) { return display_->dropdownItems(row); };
        pages_.push_back(std::move(entry));
    }
    if (setup.controls) {
        controls_.emplace(*setup.controls);
        controls_->setPadConnected(padConnected_);
        MenuPageEntry entry = entryFor("Controls", MenuStore::Controls, *controls_);
        entry.columnTitles = controls_->columnTitles();
        entry.pageHelp = [this] { return controls_->setHint(); };
        pages_.push_back(std::move(entry));
    }
    qol_.emplace(*setup.settings);
    pages_.push_back(entryFor("Quality of life", MenuStore::Qol, *qol_));

    nav_ = std::make_unique<UiNav>(measure_);
    nav_->setWindow(width_, height_);
    nav_->setTabs(tabTitles());
    if (tabTitles() == lastTabs_) nav_->selectTab(lastTab_);
    nav_->setDropdownInfo([this](int row) {
        const MenuPageEntry* current = page();
        return current ? current->dropdown(row) : DropdownInfo{};
    });
    open_ = true;
    showPage();
}

void UiMenu::setWindow(int width, int height) {
    width_ = width;
    height_ = height;
    if (nav_) nav_->setWindow(width, height);
}

void UiMenu::setPadConnected(bool connected) {
    padConnected_ = connected;
    if (controls_) controls_->setPadConnected(connected);
}

std::vector<std::string> UiMenu::tabTitles() const {
    std::vector<std::string> titles;
    for (const MenuPageEntry& entry : pages_) titles.push_back(entry.title);
    return titles;
}

const MenuPageEntry* UiMenu::page() const {
    if (!nav_) return nullptr;
    const int tab = nav_->screen().selectedTab;
    if (tab < 0 || tab >= static_cast<int>(pages_.size())) return nullptr;
    return &pages_[static_cast<std::size_t>(tab)];
}

const Screen& UiMenu::screen() const {
    if (!nav_) throw std::logic_error("the settings menu was never opened");
    return nav_->screen();
}

Modal UiMenu::modal() const { return nav_ ? nav_->modal() : Modal::None; }

void UiMenu::showPage() {
    const MenuPageEntry* current = page();
    if (!current) return;
    nav_->setColumnTitles(current->columnTitles);
    nav_->setRows(current->rows());
}

void UiMenu::refresh() {
    const MenuPageEntry* current = page();
    if (current) nav_->updateRows(current->rows());
    syncModal();
}

void UiMenu::syncModal() {
    Modal modal = Modal::None;
    if (confirm_ || (keep_ && keep_->active())) modal = Modal::Dialog;
    else if (controlsPort_ && controlsPort_->tab().conflictPending()) modal = Modal::SwapDialog;
    else if (controlsPort_ && controlsPort_->tab().waiting()) modal = Modal::Prompt;
    if (!confirm_ && modal != Modal::SwapDialog) dialogFocus_ = cancelButton;
    if (modal != Modal::Prompt && modal != Modal::SwapDialog) message_.clear();
    if (modal != nav_->modal()) nav_->setModal(modal);
}

void UiMenu::track(const Event& event) {
    switch (event.kind) {
    case EventKind::Key: device_ = isPadKey(event.key) ? InputDevice::Pad : InputDevice::Keyboard; break;
    case EventKind::PromptInput:
        device_ = event.source == InputSource::Pad ? InputDevice::Pad : event.source == InputSource::Mouse ? InputDevice::Mouse : InputDevice::Keyboard;
        break;
    default: device_ = InputDevice::Mouse; break;
    }
}

bool UiMenu::handleDialogKeys(const Event& event, MenuEffects& effects, double now) {
    if (nav_->modal() != Modal::SwapDialog && !confirm_) return false;
    if (event.kind == EventKind::MouseMove) {
        const std::optional<MenuDialog> shown = dialog();
        const int button = shown ? hitDialogButton(computeDialogLayout(*shown, nav_->layout().scale, measure_, lineHeight_), event.x, event.y) : -1;
        if (button >= 0) dialogFocus_ = button;
        return true;
    }
    if (event.kind != EventKind::Key) return false;
    const Command command = commandOf(event.key);
    if (command == Command::Left || command == Command::Right) {
        dialogFocus_ = command == Command::Left ? 0 : cancelButton;
        return true;
    }
    if (command != Command::Confirm) return false;
    if (nav_->modal() == Modal::SwapDialog) resolveSwap(dialogFocus_ == swapButton, effects);
    else if (dialogFocus_ == confirmButton) confirmDialog(effects, now);
    else cancelDialog(effects);
    return true;
}

MenuEffects UiMenu::handle(const Event& event, double nowSeconds) {
    MenuEffects effects;
    if (!open_) return effects;
    track(event);
    if (!handleDialogKeys(event, effects, nowSeconds))
        for (const Action& action : nav_->handle(event)) {
            apply(action, effects, nowSeconds);
            if (!open_) break;
        }
    if (open_) refresh();
    effects.merge(takePending());
    return effects;
}

MenuEffects UiMenu::tick(double nowSeconds) {
    if (open_ && keep_ && keep_->active()) {
        keep_->tick(nowSeconds);
        refresh();
    }
    return takePending();
}

MenuEffects UiMenu::takePending() {
    MenuEffects out = pending_;
    pending_ = {};
    return out;
}

void UiMenu::close(MenuEffects& effects) {
    open_ = false;
    lastTabs_ = tabTitles();
    lastTab_ = nav_->screen().selectedTab;
    effects.closed = true;
    if (game_ && gameOptions_ && game_->language() != gameOptions_->language()) effects.language = game_->language();
}

void UiMenu::apply(const Action& action, MenuEffects& effects, double now) {
    const MenuPageEntry* current = page();
    switch (action.kind) {
    case ActionKind::Close: close(effects); return;
    case ActionKind::TabChanged:
        confirm_.reset();
        confirmQuit_ = false;
        showPage();
        return;
    case ActionKind::DialogConfirmed: confirmDialog(effects, now); return;
    case ActionKind::DialogCancelled: cancelDialog(effects); return;
    case ActionKind::ModalClick:
        if (nav_->modal() == Modal::Prompt) break;
        clickDialog(action, effects, now);
        return;
    case ActionKind::Scrolled:
    case ActionKind::DropdownOpened:
    case ActionKind::DropdownClosed: return;
    default: break;
    }
    if (current) applyOutcome(current->handle(action), effects, now);
}

void UiMenu::applyOutcome(const MenuOutcome& outcome, MenuEffects& effects, double now) {
    const MenuPageEntry* current = page();
    if (outcome.changed && current) {
        switch (current->store) {
        case MenuStore::Game: effects.gameChanged = true; break;
        case MenuStore::Display:
            effects.settingsChanged = true;
            effects.windowChanged = true;
            break;
        case MenuStore::Controls: effects.controlsChanged = true; break;
        case MenuStore::Qol: effects.settingsChanged = true; break;
        }
    }
    if (outcome.settingsChanged) effects.settingsChanged = true;
    if (outcome.confirm) {
        confirm_ = outcome.confirm;
        confirmQuit_ = outcome.quit;
    }
    if (outcome.keep && keep_) keep_->begin(valuesOf(outcome.keep->previous), valuesOf(outcome.keep->current), now);
    message_ = outcome.message;
}

void UiMenu::confirmDialog(MenuEffects& effects, double now) {
    if (confirm_) {
        confirm_.reset();
        if (confirmQuit_) {
            confirmQuit_ = false;
            effects.quitGame = true;
            return;
        }
        if (const MenuPageEntry* current = page()) applyOutcome(current->reset(), effects, now);
        return;
    }
    if (keep_ && keep_->active()) keep_->confirm();
}

void UiMenu::cancelDialog(MenuEffects& effects) {
    if (confirm_) {
        confirm_.reset();
        confirmQuit_ = false;
        Action cancelled;
        cancelled.kind = ActionKind::DialogCancelled;
        if (const MenuPageEntry* current = page()) current->handle(cancelled);
        return;
    }
    if (keep_ && keep_->active()) {
        keep_->cancel();
        return;
    }
    if (nav_->modal() == Modal::SwapDialog) resolveSwap(false, effects);
}

void UiMenu::resolveSwap(bool swap, MenuEffects& effects) {
    if (!controls_) return;
    if (outcomeOf(controls_->resolveSwap(swap)).changed) effects.controlsChanged = true;
}

void UiMenu::clickDialog(const Action& action, MenuEffects& effects, double now) {
    const std::optional<MenuDialog> shown = dialog();
    if (!shown) return;
    const int button = hitDialogButton(computeDialogLayout(*shown, nav_->layout().scale, measure_, lineHeight_), action.x, action.y);
    if (button < 0) return;
    if (nav_->modal() == Modal::SwapDialog) {
        resolveSwap(button == swapButton, effects);
        return;
    }
    if (confirm_) {
        if (button == confirmButton) confirmDialog(effects, now);
        else cancelDialog(effects);
        return;
    }
    if (keep_ && keep_->active()) keep_->choose(button == confirmButton ? KeepButton::Keep : KeepButton::Revert);
}

std::optional<MenuDialog> UiMenu::dialog() const {
    if (!nav_) return std::nullopt;
    if (confirm_) return MenuDialog{*confirm_, "", {resetYesText, resetNoText}, dialogFocus_, ""};
    if (keep_) {
        if (const std::optional<KeepDialog> keep = keep_->dialog())
            return MenuDialog{keep->title, "", keep->buttons, static_cast<int>(keep->defaultFocus), keep->body};
    }
    if (controlsPort_) {
        if (const std::optional<SwapDialog> swap = swapDialog(controlsPort_->tab()))
            return MenuDialog{swap->title, swap->body, swap->buttons, dialogFocus_, ""};
        if (controlsPort_->tab().waiting() && controls_) return MenuDialog{controls_->promptText(), message_, {}, 0, ""};
    }
    return std::nullopt;
}

std::string UiMenu::helpText() const {
    const MenuPageEntry* current = page();
    if (!current || !nav_) return {};
    if (nav_->screen().focusOnTabs || nav_->screen().focusRow < 0) return current->pageHelp ? current->pageHelp() : std::string();
    return current->help(nav_->screen().focusRow);
}

MenuChrome UiMenu::chrome() const {
    MenuChrome chrome;
    chrome.helpText = helpText();
    chrome.device = device_;
    chrome.measure = measure_;
    chrome.lineHeight = lineHeight_;
    chrome.dialog = dialog();
    const MenuPageEntry* current = page();
    const int row = nav_ ? nav_->screen().openDropdownRow : -1;
    if (current && row >= 0) {
        chrome.dropdownItems = current->dropdownItems(row);
        chrome.dropdownFocus = nav_->dropdownFocus();
        chrome.dropdownSelected = current->dropdown(row).selected;
    }
    return chrome;
}

void UiMenu::paint(chase::render::UiDrawList& list) const {
    if (!open_ || !nav_) return;
    paintMenu(list, nav_->screen(), nav_->layout(), chrome());
}

}
