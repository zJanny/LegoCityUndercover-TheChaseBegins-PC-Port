#pragma once

#include "qol/features.hpp"
#include "qol/settings.hpp"
#include "qol/ui_nav.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace qol::ui {

struct PageHelp {
    std::string text;
};

enum class QolOutcomeKind { Nothing, Changed, NeedsConfirm };

struct QolPageOutcome {
    QolOutcomeKind kind = QolOutcomeKind::Nothing;
    std::string question;
};

class QolPage {
public:
    static constexpr int masterRow = 0;
    static constexpr int firstFeatureRow = 1;
    static constexpr int resetRow = firstFeatureRow + static_cast<int>(featureCount);
    static constexpr int rowCount = resetRow + 1;

    explicit QolPage(Settings& settings) : settings_(settings) {}

    std::vector<RowSpec> rows() const;
    DropdownInfo dropdown(int row) const;
    QolPageOutcome handle(const Action& action);
    QolPageOutcome reset();
    PageHelp help(int row) const;

private:
    QolPageOutcome stepMaster(int direction);
    QolPageOutcome flipFeature(Feature feature);

    Settings& settings_;
};

}
