#include "runtime/level_start.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace chase::runtime {
namespace {
using assets::Level;
using assets::LevelObject;
using assets::LevelParam;
using assets::LevelScript;
using assets::ScriptCall;

constexpr int kMaxScriptDepth = 32;

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

enum class Word {
    Unhandled, If, Else, ElseIf, EndIf, Assign, AssignArithmetic, CallScript, SpawnScript,
    EnableObject, EnableCollision, GetGameComplete, IsLevelUnlocked, IsRedBrickExtraActive
};

struct ScriptFunction {
    const char* name;
    Word word;
    bool standIn;
};

constexpr ScriptFunction kScriptFunctions[] = {
    {"?", Word::If, false},
    {"!?", Word::Else, false},
    {"?!", Word::ElseIf, false},
    {"??", Word::EndIf, false},
    {"=", Word::Assign, false},
    {"=+", Word::AssignArithmetic, false},
    {"callscript", Word::CallScript, false},
    {"spawnscript", Word::SpawnScript, false},
    {"enableobject", Word::EnableObject, false},
    {"enablecollision", Word::EnableCollision, false},
    {"getgamecomplete", Word::GetGameComplete, true},
    {"islevelunlocked", Word::IsLevelUnlocked, true},
    {"isredbrickextraactive", Word::IsRedBrickExtraActive, true},
};

Word wordOf(const std::string& function) {
    const std::string name = lower(function);
    for (const ScriptFunction& entry : kScriptFunctions)
        if (name == entry.name) return entry.word;
    return Word::Unhandled;
}

std::string unquoted(const std::string& text) {
    if (text.size() >= 2 && text.front() == '"' && text.back() == '"') return text.substr(1, text.size() - 2);
    return text;
}

bool isNumber(const std::string& token) {
    if (token.empty() || !(std::isdigit(static_cast<unsigned char>(token[0])) || token[0] == '-')) return false;
    return std::all_of(token.begin() + 1, token.end(), [](unsigned char c) { return std::isdigit(c) || c == '.'; });
}

float floatOf(std::uint32_t bits) {
    float value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}

class ScriptRun {
public:
    ScriptRun(const Level& level, assets::PlacedObjectState& state) : level_(level), state_(state) {
        for (const LevelParam& param : level.defaults) {
            if (param.type == 0) variables_[param.hash] = floatOf(param.value);
            else if (param.type == 5 || param.type == 7) variables_[param.hash] = float(param.value);
        }
    }

    void run(const LevelScript& script, int depth) {
        const std::vector<ScriptCall>& calls = script.calls;
        bool elseIfPending = false;
        std::size_t pc = 0;
        while (pc < calls.size()) {
            const ScriptCall& call = calls[pc];
            const Word word = wordOf(call.function);
            switch (word) {
            case Word::ElseIf:
                if (!elseIfPending) {
                    pc = skipToEnd(calls, pc);
                    continue;
                }
                elseIfPending = false;
                [[fallthrough]];
            case Word::If:
                if (!holds(call)) {
                    pc = skipBranch(calls, pc, elseIfPending);
                    continue;
                }
                break;
            case Word::Else:
                pc = skipToEnd(calls, pc);
                continue;
            case Word::EndIf:
                break;
            case Word::Assign:
                if (call.arguments.size() >= 2) variables_[assets::nameHash(call.arguments[0])] = read(call.arguments[1]);
                break;
            case Word::AssignArithmetic:
                assignArithmetic(call);
                break;
            case Word::CallScript:
            case Word::SpawnScript:
                runNamed(call, depth);
                break;
            default:
                native(word, call);
                break;
            }
            ++pc;
        }
    }

private:
    float read(const std::string& token) const {
        if (isNumber(token)) return std::strtof(token.c_str(), nullptr);
        const auto found = variables_.find(assets::nameHash(token));
        return found == variables_.end() ? 0.0f : found->second;
    }

    bool holds(const ScriptCall& call) const {
        if (call.arguments.size() < 3) return false;
        const float left = read(call.arguments[0]), right = read(call.arguments[2]);
        const std::string op = unquoted(call.arguments[1]);
        if (op == "<") return left < right;
        if (op == "<=") return left <= right;
        if (op == "==") return left == right;
        if (op == "!=") return left != right;
        if (op == ">=") return left >= right;
        if (op == ">") return left > right;
        return false;
    }

    void assignArithmetic(const ScriptCall& call) {
        if (call.arguments.size() < 4) return;
        const float left = read(call.arguments[1]), right = read(call.arguments[3]);
        const std::string op = unquoted(call.arguments[2]);
        float result = left;
        if (op == "+") result = left + right;
        else if (op == "-") result = left - right;
        else if (op == "*") result = left * right;
        else if (op == "/") result = left / right;
        variables_[assets::nameHash(call.arguments[0])] = result;
    }

    // Scan of the if handler 0x16354c: past the matching else, at the matching else-if, or past the matching end if.
    static std::size_t skipBranch(const std::vector<ScriptCall>& calls, std::size_t pc, bool& elseIfPending) {
        int depth = 0;
        for (std::size_t i = pc + 1; i < calls.size(); ++i) {
            switch (wordOf(calls[i].function)) {
            case Word::If: ++depth; break;
            case Word::Else:
                if (depth == 0) return i + 1;
                break;
            case Word::ElseIf:
                if (depth == 0) {
                    elseIfPending = true;
                    return i;
                }
                break;
            case Word::EndIf:
                if (depth == 0) return i + 1;
                --depth;
                break;
            default: break;
            }
        }
        return calls.size();
    }

    // Scan of the else handler 0x174428 and of the else-if handler 0x18a7e4 without a pending branch: past the matching end if.
    static std::size_t skipToEnd(const std::vector<ScriptCall>& calls, std::size_t pc) {
        int depth = 0;
        for (std::size_t i = pc + 1; i < calls.size(); ++i) {
            const Word word = wordOf(calls[i].function);
            if (word == Word::If) ++depth;
            else if (word == Word::EndIf) {
                if (depth == 0) return i + 1;
                --depth;
            }
        }
        return calls.size();
    }

    void runNamed(const ScriptCall& call, int depth) {
        if (call.arguments.empty() || depth >= kMaxScriptDepth) return;
        const std::uint32_t hash = assets::nameHash(call.arguments[0]);
        for (const LevelScript& script : level_.scripts)
            if (script.nameHash == hash) {
                run(script, depth + 1);
                return;
            }
    }

    void native(Word word, const ScriptCall& call) {
        const auto& args = call.arguments;
        switch (word) {
        case Word::EnableObject:
            if (args.size() < 2) break;
            if (read(args[1]) == 0.0f) state_.disabled.insert(lower(args[0]));
            else state_.disabled.erase(lower(args[0]));
            break;
        case Word::EnableCollision:
            if (args.size() >= 2) state_.collisionOverride[lower(args[0])] = read(args[1]) != 0.0f;
            break;
        case Word::GetGameComplete:
            if (!args.empty()) variables_[assets::nameHash(args[0])] = 0.0f;
            break;
        case Word::IsLevelUnlocked:
        case Word::IsRedBrickExtraActive:
            if (args.size() >= 2) variables_[assets::nameHash(args[1])] = 0.0f;
            break;
        default: break;
        }
    }

    const Level& level_;
    assets::PlacedObjectState& state_;
    std::map<std::uint32_t, float> variables_;
};

const LevelParam* findParam(const std::vector<LevelParam>& params, std::uint32_t hash) {
    for (const LevelParam& param : params)
        if (param.hash == hash) return &param;
    return nullptr;
}

const LevelParam* effectiveParam(const LevelObject& object, const Level& cls, std::uint32_t hash) {
    const LevelParam* classParam = findParam(cls.defaults, hash);
    if (!classParam) classParam = findParam(cls.classParams, hash);
    if (!classParam) return nullptr;
    if (classParam->flags & 2)
        if (const LevelParam* instance = findParam(object.params, hash)) return instance;
    return classParam;
}

void hideRevealedObjects(const Level& level, const assets::ClassLookup& classOf, assets::PlacedObjectState& state) {
    const std::uint32_t links[] = {assets::nameHash("BuilditFinalProp"), assets::nameHash("PropToReveal"), assets::nameHash("RevealObject")};
    std::map<std::uint32_t, std::string> instanceByHash;
    for (const LevelObject& object : level.objects) instanceByHash[assets::nameHash(object.instance)] = lower(object.instance);
    for (const LevelObject& object : level.objects) {
        const Level* cls = classOf(object.className);
        if (!cls) continue;
        for (const std::uint32_t link : links) {
            const LevelParam* param = effectiveParam(object, *cls, link);
            if (!param) continue;
            const auto target = instanceByHash.find(assets::objectReferenceHash(*param));
            if (target != instanceByHash.end()) state.disabled.insert(target->second);
        }
    }
}
}

ScriptFunctionSupport scriptFunctionSupport(const std::string& name) {
    const std::string key = lower(name);
    for (const ScriptFunction& entry : kScriptFunctions)
        if (key == entry.name) return entry.standIn ? ScriptFunctionSupport::StandIn : ScriptFunctionSupport::Implemented;
    return ScriptFunctionSupport::None;
}

std::vector<std::string> handledScriptFunctions() {
    std::vector<std::string> names;
    for (const ScriptFunction& entry : kScriptFunctions) names.push_back(entry.name);
    return names;
}

assets::PlacedObjectState levelStartState(const Level& level, const assets::ClassLookup& classOf) {
    assets::PlacedObjectState state;
    hideRevealedObjects(level, classOf, state);
    const std::uint32_t startup = assets::nameHash("startup");
    ScriptRun run(level, state);
    for (const LevelScript& script : level.scripts)
        if (script.nameHash == startup) {
            run.run(script, 0);
            break;
        }
    return state;
}

}
