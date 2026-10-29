#include "qol/controls_file.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "input.hpp"

namespace qol {

namespace {

const char* const mouseNames[] = {"Left", "Middle", "Right", "X1", "X2"};

std::string mouseName(int code) {
    if (code < 1 || code > 5) throw std::runtime_error("Unknown mouse button code: " + std::to_string(code));
    return mouseNames[code - 1];
}

bool validMouse(const std::string& code) {
    return std::find(std::begin(mouseNames), std::end(mouseNames), code) != std::end(mouseNames);
}

bool validKeyName(const std::string& spaced, const KeyResolver& resolveKey) {
    if (spaced.empty() || spaced.find('#') != std::string::npos) return false;
    if (spaced.find_first_of("\t\r\n") != std::string::npos) return false;
    if (spaced.front() == ' ' || spaced.back() == ' ' || spaced.find("  ") != std::string::npos) return false;
    return resolveKey(spaced) != 0;
}

bool validPad(const std::string& code) {
    return chase::padButtonCode(code) >= 0;
}

std::string underscored(std::string code) {
    std::replace(code.begin(), code.end(), ' ', '_');
    return code;
}

std::string spaced(std::string code) {
    std::replace(code.begin(), code.end(), '_', ' ');
    return code;
}

}

std::vector<std::string> controlActions() {
    std::vector<std::string> names;
    for (std::size_t i = 0; i < chase::actionCount; ++i)
        names.push_back(chase::actionName(static_cast<chase::Action>(i)));
    return names;
}

InputValidator fileInputValidator(KeyResolver resolveKey) {
    if (!resolveKey) throw std::invalid_argument("Controls file needs a key resolver");
    return [resolveKey = std::move(resolveKey)](const std::string& source, const std::string& code) {
        if (source == "key") return validKeyName(code, resolveKey);
        if (source == "pad") return validPad(code);
        return source == "mouse" && validMouse(code);
    };
}

std::vector<BindingEntry> readBindingEntries(std::istream& stream, const KeyResolver& resolveKey, const KeyNamer& nameKey) {
    if (!resolveKey || !nameKey) throw std::invalid_argument("Controls file needs a key resolver and a key namer");
    auto bindings = chase::readBindings(stream, resolveKey);
    std::vector<BindingEntry> entries;
    for (const auto& binding : bindings) {
        BindingEntry entry;
        entry.action = chase::actionName(binding.action);
        entry.mode = binding.mode == chase::Mode::Hold ? "hold" : "toggle";
        if (binding.source == chase::Source::Mouse) {
            entry.source = "mouse";
            entry.code = mouseName(binding.code);
        } else if (binding.source == chase::Source::Pad) {
            entry.source = "pad";
            entry.code = chase::padButtonName(binding.code);
        } else {
            entry.source = "key";
            auto name = nameKey(binding.code);
            if (name.empty()) throw std::runtime_error("No key name for code " + std::to_string(binding.code));
            entry.code = underscored(name);
        }
        entries.push_back(std::move(entry));
    }
    return entries;
}

std::string formatBindingEntries(const std::vector<BindingEntry>& entries, const KeyResolver& resolveKey) {
    if (!resolveKey) throw std::invalid_argument("Controls file needs a key resolver");
    if (entries.empty()) throw std::invalid_argument("Controls file needs at least one binding");
    auto actions = controlActions();
    std::ostringstream out;
    std::size_t number = 0;
    for (const auto& entry : entries) {
        ++number;
        auto fail = [&](const std::string& why) {
            throw std::invalid_argument("Cannot write binding " + std::to_string(number) + " (" + entry.action + " " +
                entry.source + " " + entry.code + " " + entry.mode + "): " + why);
        };
        if (std::find(actions.begin(), actions.end(), entry.action) == actions.end()) fail("unknown action");
        if (entry.mode != "hold" && entry.mode != "toggle") fail("mode must be hold or toggle");
        if (entry.source == "key") {
            if (!validKeyName(spaced(entry.code), resolveKey)) fail("unknown or unwritable key name");
        } else if (entry.source == "mouse") {
            if (!validMouse(entry.code)) fail("unknown mouse button");
        } else if (entry.source == "pad") {
            if (!validPad(entry.code)) fail("unknown pad button");
        } else fail("the file format has no source " + entry.source);
        if (entry.code.find_first_of(" \t\r\n") != std::string::npos) fail("code contains whitespace");
        out << entry.action << ' ' << entry.source << ' ' << entry.code << ' ' << entry.mode << '\n';
    }
    auto text = out.str();
    std::istringstream check(text);
    if (chase::readBindings(check, resolveKey).size() != entries.size())
        throw std::logic_error("Written controls would not read back");
    return text;
}

void writeBindingFile(const std::string& path, const std::vector<BindingEntry>& entries, const KeyResolver& resolveKey) {
    auto text = formatBindingEntries(entries, resolveKey);
    auto temporary = path + ".tmp";
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        if (!file) throw std::runtime_error("Cannot open " + temporary + " for writing");
        file << text;
        file.flush();
        if (!file) {
            std::remove(temporary.c_str());
            throw std::runtime_error("Cannot write " + temporary);
        }
    }
    if (std::rename(temporary.c_str(), path.c_str()) != 0) {
        std::remove(temporary.c_str());
        throw std::runtime_error("Cannot replace " + path);
    }
}

}
