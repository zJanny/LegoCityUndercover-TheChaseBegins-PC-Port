#pragma once

#include "qol/controls_port.hpp"
#include "qol/display_tab.hpp"
#include "qol/game_tab.hpp"
#include "qol/keep_settings.hpp"
#include "qol/menu_stores.hpp"
#include "qol/settings.hpp"
#include "qol/ui_nav.hpp"
#include "qol/ui_page_controls.hpp"
#include "qol/ui_page_display.hpp"
#include "qol/ui_page_game.hpp"
#include "qol/ui_page_qol.hpp"
#include "qol/ui_paint.hpp"
#include "render/ui_draw.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace qol::ui {

extern const char* const resetYesText;
extern const char* const resetNoText;

struct DisplaySetup {
    ResolutionChoices choices;
    Resolution desktop{0, 0};
};

struct MenuSetup {
    Settings* settings = nullptr;
    GameOptions* game = nullptr;
    std::vector<std::string> languageNames;
    int startLanguage = firstLanguage;
    ControlsPort* controls = nullptr;
    std::optional<DisplaySetup> display;
    bool quitTab = false;
};

MenuSetup menuSetupFor(MenuStores& stores, std::vector<std::string> languageNames);

struct MenuPageEntry;
struct MenuOutcome;

class UiMenu {
public:
    UiMenu(TextMeasure measure, LineHeight lineHeight);
    ~UiMenu();
    UiMenu(const UiMenu&) = delete;
    UiMenu& operator=(const UiMenu&) = delete;

    void open(const MenuSetup& setup);
    bool isOpen() const { return open_; }
    void setWindow(int width, int height);
    void setPadConnected(bool connected);

    MenuEffects handle(const Event& event, double nowSeconds);
    MenuEffects tick(double nowSeconds);

    Modal modal() const;
    bool promptOpen() const { return modal() == Modal::Prompt; }
    const Screen& screen() const;
    std::vector<std::string> tabTitles() const;
    std::optional<MenuDialog> dialog() const;
    InputDevice device() const { return device_; }
    std::string helpText() const;
    MenuChrome chrome() const;
    void paint(chase::render::UiDrawList& list) const;

private:
    const MenuPageEntry* page() const;
    void showPage();
    void refresh();
    void syncModal();
    void track(const Event& event);
    bool handleDialogKeys(const Event& event, MenuEffects& effects, double now);
    void apply(const Action& action, MenuEffects& effects, double now);
    void applyOutcome(const MenuOutcome& outcome, MenuEffects& effects, double now);
    void clickDialog(const Action& action, MenuEffects& effects, double now);
    void confirmDialog(MenuEffects& effects, double now);
    void cancelDialog(MenuEffects& effects);
    void resolveSwap(bool swap, MenuEffects& effects);
    void close(MenuEffects& effects);
    MenuEffects takePending();

    TextMeasure measure_;
    LineHeight lineHeight_;
    std::unique_ptr<UiNav> nav_;
    std::vector<MenuPageEntry> pages_;
    std::optional<GamePage> game_;
    std::optional<DisplayPage> display_;
    std::optional<ControlsPage> controls_;
    std::optional<QolPage> qol_;
    std::optional<KeepSettings> keep_;
    GameOptions* gameOptions_ = nullptr;
    ControlsPort* controlsPort_ = nullptr;
    std::optional<std::string> confirm_;
    bool confirmQuit_ = false;
    std::string message_;
    int dialogFocus_ = 1;
    bool padConnected_ = false;
    bool open_ = false;
    InputDevice device_ = InputDevice::Keyboard;
    MenuEffects pending_;
    int width_ = 0;
    int height_ = 0;
    std::vector<std::string> lastTabs_;
    int lastTab_ = 0;
};

}
