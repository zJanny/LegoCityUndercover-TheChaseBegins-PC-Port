#pragma once

#include "qol/features.hpp"
#include "qol/settings.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace qol {

enum class Tab { Game, Display, Controls, QualityOfLife };

inline constexpr std::size_t tabCount = 4;

const char* tabName(Tab tab);

enum class RowKind { Master, Feature, Placeholder };

struct Row {
    RowKind kind;
    std::string label;
    Feature feature = Feature::Count;
};

std::vector<Row> rowsOf(Tab tab);

enum class Key {
    Up, Down, Left, Right, Enter, Escape, PageUp, PageDown,
    PadUp, PadDown, PadLeft, PadRight, PadA, PadB, PadL, PadR
};

enum class EventKind {
    Up, Down, Left, Right, Activate, Back, TabNext, TabPrevious,
    RowMove, RowClick, TabMove, TabClick
};

struct Event {
    EventKind kind;
    std::size_t index = 0;
};

Event eventForKey(Key key);
Event rowMove(std::size_t row);
Event rowClick(std::size_t row);
Event tabMove(std::size_t tab);
Event tabClick(std::size_t tab);

enum class Result {
    Ignored,
    Rejected,
    Closed,
    TabChanged,
    RowChanged,
    MasterCycled,
    FeatureToggled,
    SwitchedToCustom
};

class MenuModel {
public:
    explicit MenuModel(Settings& settings) : settings_(settings) {}

    bool isOpen() const { return open_; }
    Tab tab() const { return tab_; }
    std::size_t row() const { return rows_[static_cast<std::size_t>(tab_)]; }
    std::vector<Row> rows() const { return rowsOf(tab_); }
    bool tabActive(Tab) const { return true; }

    void open();
    Result handle(const Event& event);

private:
    Result moveRow(std::ptrdiff_t delta);
    Result moveTab(std::ptrdiff_t delta);
    Result selectRow(std::size_t row);
    Result activateRow();

    Settings& settings_;
    bool open_ = false;
    Tab tab_ = Tab::Game;
    std::array<std::size_t, tabCount> rows_{};
};

}
