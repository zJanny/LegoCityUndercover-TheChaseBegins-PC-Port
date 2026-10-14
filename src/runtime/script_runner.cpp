#include "runtime/script_runner.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace chase::runtime {
namespace {
using assets::Level;
using assets::LevelParam;
using assets::LevelScript;
using assets::ScriptCall;

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

enum class Word {
    Unhandled, If, Else, ElseIf, EndIf, Assign, AssignArithmetic, CallScript, SpawnScript, Wait,
    EnableObject, EnableCollision, TriggerObject, EnableTrigger, EnableBound, GetGameComplete, IsLevelUnlocked, IsRedBrickExtraActive,
    WarningDialogBox, FadeScreen, PositionObject
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
    {"wait", Word::Wait, false},
    {"enableobject", Word::EnableObject, false},
    {"enablecollision", Word::EnableCollision, false},
    {"triggerobject", Word::TriggerObject, false},
    {"enabletrigger", Word::EnableTrigger, false},
    {"enablebound", Word::EnableBound, false},
    {"getgamecomplete", Word::GetGameComplete, true},
    {"islevelunlocked", Word::IsLevelUnlocked, true},
    {"isredbrickextraactive", Word::IsRedBrickExtraActive, true},
    {"warningdialogbox", Word::WarningDialogBox, false},
    {"fadescreen", Word::FadeScreen, true},
    {"positionobject", Word::PositionObject, false},
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

// Scan of the if handler 0x16354c after a false condition: the call after the matching else, the matching else-if
// (with the else-if flag set), or the call after the matching end if.
std::size_t skipBranch(const std::vector<ScriptCall>& calls, std::size_t pc, bool& elseIfPending) {
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
std::size_t skipToEnd(const std::vector<ScriptCall>& calls, std::size_t pc) {
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

void jump(ScriptThread& thread, std::size_t next) { thread.pc = static_cast<std::uint16_t>(next - 1); }
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

ScriptRunner::ScriptRunner(const Level& level, assets::PlacedObjectState& state)
    : level_(level), state_(state), triggers_(level.triggers.size()) {
    for (const assets::LevelBound& bound : level.bounds) boundEnabled_.push_back(bound.enabled);
    for (const LevelParam& param : level.defaults) {
        if (param.type == 0) variables_[param.hash] = floatOf(param.value);
        else if (param.type == 5 || param.type == 7) variables_[param.hash] = float(param.value);
    }
}

bool ScriptRunner::start(const std::string& scriptName) {
    const LevelScript* script = find(scriptName);
    if (!script) return false;
    execute(*create(*script));
    return true;
}

void ScriptRunner::update(float dt) {
    dt_ = dt;
    for (auto it = threads_.begin(); it != threads_.end();) {
        const auto next = std::next(it);
        execute(*it);
        it = next;
    }
}

bool ScriptRunner::running(const std::string& scriptName, bool requireActive) const {
    const std::uint32_t hash = assets::nameHash(scriptName);
    for (const ScriptThread& thread : threads_)
        if (thread.script->nameHash == hash) {
            if (thread.pc >= thread.script->calls.size()) return false;
            return !(requireActive && (thread.flags & kScriptSuspended));
        }
    return false;
}

float ScriptRunner::variable(const std::string& name) const {
    const auto found = variables_.find(assets::nameHash(name));
    return found == variables_.end() ? 0.0f : found->second;
}

std::uint32_t ScriptRunner::unhandledCalls(const std::string& lowerCaseName) const {
    const auto found = unhandled_.find(lowerCaseName);
    return found == unhandled_.end() ? 0 : found->second;
}

std::vector<ScriptObjectMessage> ScriptRunner::takeObjectMessages() {
    std::vector<ScriptObjectMessage> taken;
    taken.swap(messages_);
    return taken;
}

ScriptThread* ScriptRunner::create(const LevelScript& script) {
    threads_.emplace_front();
    ScriptThread& thread = threads_.front();
    thread.script = &script;
    return &thread;
}

void ScriptRunner::execute(ScriptThread& thread) {
    const std::vector<ScriptCall>& calls = thread.script->calls;
    while (thread.pc < calls.size() && !(thread.flags & kScriptSuspended)) {
        if (call(thread, calls[thread.pc]) == Result::Block) {
            thread.flags |= kScriptBlocked;
            break;
        }
        ++thread.pc;
        if (thread.pc < calls.size()) thread.flags &= ~kScriptBlocked;
    }
    if (thread.pc >= calls.size() && !(thread.flags & kScriptSuspended)) finish(thread);
}

void ScriptRunner::finish(ScriptThread& thread) {
    if (thread.parent) thread.parent->flags ^= kScriptSuspended;
    threads_.remove_if([&](const ScriptThread& candidate) { return &candidate == &thread; });
}

ScriptRunner::Result ScriptRunner::call(ScriptThread& thread, const ScriptCall& call) {
    const std::vector<ScriptCall>& calls = thread.script->calls;
    const Word word = wordOf(call.function);
    switch (word) {
    case Word::ElseIf:
        if (!(thread.flags & kScriptElseIfPending)) {
            jump(thread, skipToEnd(calls, thread.pc));
            return Result::Continue;
        }
        thread.flags &= ~kScriptElseIfPending;
        [[fallthrough]];
    case Word::If:
        if (!holds(call)) {
            bool elseIfPending = false;
            jump(thread, skipBranch(calls, thread.pc, elseIfPending));
            if (elseIfPending) thread.flags |= kScriptElseIfPending;
        }
        return Result::Continue;
    case Word::Else:
        jump(thread, skipToEnd(calls, thread.pc));
        return Result::Continue;
    case Word::EndIf:
        return Result::Continue;
    case Word::Assign:
        if (call.arguments.size() >= 2) variables_[assets::nameHash(call.arguments[0])] = read(call.arguments[1]);
        return Result::Continue;
    case Word::AssignArithmetic:
        assignArithmetic(call);
        return Result::Continue;
    case Word::CallScript:
        return callScript(thread, call, true);
    case Word::SpawnScript:
        return callScript(thread, call, false);
    case Word::Wait:
        return wait(thread, call);
    case Word::Unhandled: {
        const std::string name = lower(call.function);
        if (unhandled_[name]++ == 0) unhandledOrder_.push_back(name);
        return Result::Continue;
    }
    default:
        return native(thread, call, int(word));
    }
}

ScriptRunner::Result ScriptRunner::callScript(ScriptThread& thread, const ScriptCall& call, bool suspendParent) {
    const LevelScript* script = call.arguments.empty() ? nullptr : find(call.arguments[0]);
    if (!script) {
        const std::string name = lower(call.function) + " " + (call.arguments.empty() ? std::string() : lower(call.arguments[0]));
        if (unhandled_[name]++ == 0) unhandledOrder_.push_back(name);
        return Result::Continue;
    }
    ScriptThread* child = create(*script);
    if (suspendParent) {
        child->parent = &thread;
        thread.flags |= kScriptSuspended;
    }
    execute(*child);
    return Result::Continue;
}

ScriptRunner::Result ScriptRunner::wait(ScriptThread& thread, const ScriptCall& call) {
    if (!(thread.flags & kScriptBlocked)) {
        thread.waitTimer = call.arguments.empty() ? 0.0f : read(call.arguments[0]);
        return Result::Block;
    }
    thread.waitTimer -= dt_;
    return thread.waitTimer <= 0.0f ? Result::Continue : Result::Block;
}

ScriptRunner::Result ScriptRunner::native(ScriptThread& thread, const ScriptCall& call, int word) {
    const auto& args = call.arguments;
    switch (Word(word)) {
    case Word::EnableObject:
        if (args.size() < 2) break;
        if (read(args[1]) == 0.0f) state_.disabled.insert(lower(args[0]));
        else state_.disabled.erase(lower(args[0]));
        break;
    case Word::EnableCollision:
        if (args.size() >= 2) state_.collisionOverride[lower(args[0])] = read(args[1]) != 0.0f;
        break;
    case Word::TriggerObject:
        if (args.size() >= 2) {
            const bool trigger = read(args[1]) != 0.0f;
            messages_.push_back({lower(args[0]), trigger ? kMessageTrigger : kMessageUntrigger});
            const std::string event = trigger ? "objecttriggered" : "objectuntriggered";
            for (std::size_t i = 0; i < level_.triggers.size(); ++i)
                if (triggers_[i].enabled && lower(level_.triggers[i].event) == event && lower(level_.triggers[i].objectName) == lower(args[0])) {
                    queueTrigger(i);
                    break;
                }
        }
        break;
    case Word::EnableTrigger:
        if (args.size() >= 2)
            if (const int index = triggerIndex(args[0]); index >= 0) triggers_[std::size_t(index)].enabled = read(args[1]) > 0.0f;
        break;
    case Word::EnableBound:
        if (args.size() >= 2)
            if (const int index = boundIndex(args[0]); index >= 0) boundEnabled_[std::size_t(index)] = read(args[1]) > 0.0f ? 1 : 0;
        break;
    case Word::GetGameComplete:
        if (!args.empty()) variables_[assets::nameHash(args[0])] = 0.0f;
        break;
    case Word::IsLevelUnlocked:
    case Word::IsRedBrickExtraActive:
        if (args.size() >= 2) variables_[assets::nameHash(args[1])] = 0.0f;
        break;
    case Word::PositionObject:
        if (args.size() >= 2) placements_.push_back({lower(args[0]), lower(args[1])});
        break;
    case Word::WarningDialogBox:
        if (!(thread.flags & kScriptBlocked)) {
            dialogOpen_ = true;
            dialogAnswered_ = false;
            dialogResult_ = 0;
            return Result::Block;
        }
        if (!dialogAnswered_) return Result::Block;
        dialogOpen_ = false;
        dialogAnswered_ = false;
        if (!args.empty()) variables_[assets::nameHash(args[0])] = float(dialogResult_);
        break;
    case Word::FadeScreen:
        if (!(thread.flags & kScriptBlocked)) {
            if (args.empty()) break;
            fadeTarget_ = read(args[0]) == 0.0f ? 0.0f : 1.0f;
            fadeStart_ = fadeLevel_;
            fadeElapsed_ = 0;
            return Result::Block;
        }
        fadeElapsed_ += dt_;
        if (fadeElapsed_ < kFadeSeconds) {
            fadeLevel_ = fadeStart_ + (fadeTarget_ - fadeStart_) * (fadeElapsed_ / kFadeSeconds);
            return Result::Block;
        }
        fadeLevel_ = fadeTarget_;
        break;
    default: break;
    }
    return Result::Continue;
}

void ScriptRunner::answerDialog(int result) {
    if (!dialogOpen_ || dialogAnswered_) return;
    dialogResult_ = result;
    dialogAnswered_ = true;
}

std::vector<ScriptPlacement> ScriptRunner::takePlacements() {
    std::vector<ScriptPlacement> taken;
    taken.swap(placements_);
    return taken;
}

float ScriptRunner::read(const std::string& token) const {
    if (isNumber(token)) return std::strtof(token.c_str(), nullptr);
    const auto found = variables_.find(assets::nameHash(token));
    return found == variables_.end() ? 0.0f : found->second;
}

bool ScriptRunner::holds(const ScriptCall& call) const {
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

void ScriptRunner::assignArithmetic(const ScriptCall& call) {
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

void ScriptRunner::checkTriggers(const float player[3]) {
    for (std::size_t i = 0; i < level_.triggers.size(); ++i) {
        const assets::LevelTrigger& trigger = level_.triggers[i];
        ScriptTriggerState& state = triggers_[i];
        if (!state.enabled || (!trigger.objectName.empty() && state_.isDisabled(lower(trigger.objectName)))) continue;
        const std::string event = lower(trigger.event);
        const bool entered = event == "boundentered" || event == "player1boundentered" || event == "playeronfootboundentered";
        const bool left = event == "boundleft";
        if (!entered && !left) {
            if (event != "objecttriggered" && event != "objectuntriggered" &&
                std::find(unhandledEvents_.begin(), unhandledEvents_.end(), event) == unhandledEvents_.end())
                unhandledEvents_.push_back(event);
            continue;
        }
        const int bound = boundIndex(trigger.firstArgument);
        const bool inside = bound >= 0 && playerInBound(std::size_t(bound), player);
        if (!state.skip && inside != state.inside && inside == entered) queueTrigger(i);
        state.inside = inside;
        state.skip = false;
    }
    while (!queued_.empty()) {
        const std::size_t index = queued_.back();
        queued_.pop_back();
        if (const LevelScript* script = find(level_.triggers[index].scriptName)) create(*script);
    }
}

void ScriptRunner::queueTrigger(std::size_t index) {
    const assets::LevelTrigger& trigger = level_.triggers[index];
    if (!triggers_[index].enabled) return;
    if (!trigger.objectName.empty() && state_.isDisabled(lower(trigger.objectName))) return;
    const LevelScript* script = find(trigger.scriptName);
    if (script && scriptActive(*script)) return;
    queued_.push_back(index);
}

bool ScriptRunner::scriptActive(const LevelScript& script) const {
    for (const ScriptThread& thread : threads_)
        if (thread.script == &script) return thread.pc < script.calls.size() && !(thread.flags & kScriptSuspended);
    return false;
}

bool ScriptRunner::playerInBound(std::size_t index, const float player[3]) const {
    const assets::LevelBound& bound = level_.bounds[index];
    if (!boundEnabled_[index] || !boundActive(index)) return false;
    float x = player[0] - bound.centre[0], y = player[1] - bound.centre[1], z = player[2] - bound.centre[2];
    if (bound.node != 0xff) {
        if (bound.node >= level_.boundMatrices.size()) return false;
        const std::array<float, 16>& m = level_.boundMatrices[bound.node];
        const float rx = x * m[0] + y * m[1] + z * m[2], ry = x * m[4] + y * m[5] + z * m[6], rz = x * m[8] + y * m[9] + z * m[10];
        x = rx;
        y = ry;
        z = rz;
    }
    switch (bound.shape) {
    case 2: return std::sqrt(x * x + y * y + z * z) < bound.half[0];
    case 3: return std::fabs(x) < bound.half[0] && std::fabs(y) < bound.half[1] && std::fabs(z) < bound.half[2];
    case 4:
    case 6: return std::sqrt(x * x + z * z) < bound.half[0] && std::fabs(y) < bound.half[1];
    default: return false;
    }
}

void ScriptRunner::setRoomLoaded(const std::string& room, bool loaded) {
    if (loaded) loadedRooms_.insert(lower(room));
    else loadedRooms_.erase(lower(room));
}

void ScriptRunner::setAllRoomsLoaded() { allRoomsLoaded_ = true; }

bool ScriptRunner::boundActive(std::size_t index) const {
    const std::string& room = level_.bounds[index].room;
    return room.empty() || allRoomsLoaded_ || loadedRooms_.count(lower(room)) > 0;
}

int ScriptRunner::triggerIndex(const std::string& name) const {
    const std::uint32_t hash = assets::nameHash(name);
    for (std::size_t i = 0; i < level_.triggers.size(); ++i)
        if (level_.triggers[i].nameHash == hash) return int(i);
    return -1;
}

int ScriptRunner::boundIndex(const std::string& name) const {
    const std::string key = lower(name);
    for (std::size_t i = 0; i < level_.bounds.size(); ++i)
        if (lower(level_.bounds[i].name) == key) return int(i);
    return -1;
}

const LevelScript* ScriptRunner::find(const std::string& name) const {
    const std::uint32_t hash = assets::nameHash(name);
    for (const LevelScript& script : level_.scripts)
        if (script.nameHash == hash) return &script;
    return nullptr;
}

}
