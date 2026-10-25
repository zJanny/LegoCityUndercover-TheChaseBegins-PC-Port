#include "runtime/script_runner.hpp"
#include "runtime/director_hold.hpp"
#include "runtime/level_start.hpp"
#include "runtime/script_triggers.hpp"
#include "runtime/script_word.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

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
    WarningDialogBox, InformationBox, FadeScreen, PositionObject, PositionPlayer, SceneChange,
    KillScript, ObjectNative, VehicleNative, ProgressNative, MusicNative, SoundNative, TextNative, ObjectiveNative, AiNative, AnimationNative, CutsceneNative, EffectNative, WorldNative, PartyNative,
    StartDCam, StartDCamTrack, DCamActive, WaitForDCam, EndDCam, SnapCamera, ShakeScreen, Set3DFocusObject, SetDCamStartWait, SetDCamEndWait, CameraFollow
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
    {"getgamecomplete", Word::GetGameComplete, false},
    {"islevelunlocked", Word::IsLevelUnlocked, false},
    {"isredbrickextraactive", Word::IsRedBrickExtraActive, false},
    {"warningdialogbox", Word::WarningDialogBox, false},
    {"informationbox", Word::InformationBox, true},
    {"fadescreen", Word::FadeScreen, false},
    {"positionobject", Word::PositionObject, false},
    {"positionplayer", Word::PositionPlayer, true},
    {"scenechange", Word::SceneChange, false},
    {"killscript", Word::KillScript, false},
    {"startdcam", Word::StartDCam, false},
    {"startdcamtrack", Word::StartDCamTrack, true},
    {"dcamactive", Word::DCamActive, true},
    {"waitfordcam", Word::WaitForDCam, false},
    {"enddcam", Word::EndDCam, false},
    {"snapcamera", Word::SnapCamera, true},
    {"shakescreen", Word::ShakeScreen, true},
    {"set3dfocusobject", Word::Set3DFocusObject, true},
    {"setdcamstartwait", Word::SetDCamStartWait, false},
    {"setdcamendwait", Word::SetDCamEndWait, false},
    {"camerafollow", Word::CameraFollow, true},
};

Word wordOf(const std::string& function) {
    const std::string name = lower(function);
    for (const ScriptFunction& entry : kScriptFunctions)
        if (name == entry.name) return entry.word;
    if (objectNativeKind(name)) return Word::ObjectNative;
    if (vehicleNativeKind(name)) return Word::VehicleNative;
    if (progressNativeKind(name)) return Word::ProgressNative;
    if (musicNativeKind(name)) return Word::MusicNative;
    if (soundNativeKind(name)) return Word::SoundNative;
    if (textNativeKind(name)) return Word::TextNative;
    if (objectiveNativeKind(name)) return Word::ObjectiveNative;
    if (aiNativeKind(name)) return Word::AiNative;
    if (animationNativeKind(name)) return Word::AnimationNative;
    if (effectNativeKind(name)) return Word::EffectNative;
    if (worldNativeKind(name)) return Word::WorldNative;
    if (partyNativeKind(name)) return Word::PartyNative;
    return cutsceneNativeKind(name) ? Word::CutsceneNative : Word::Unhandled;
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
    if (const auto kind = objectNativeKind(key))
        return objectNativeIsStandIn(*kind) ? ScriptFunctionSupport::StandIn : ScriptFunctionSupport::Implemented;
    if (vehicleNativeKind(key)) return ScriptFunctionSupport::Implemented;
    if (const auto kind = progressNativeKind(key))
        return progressNativeIsStandIn(*kind) ? ScriptFunctionSupport::StandIn : ScriptFunctionSupport::Implemented;
    if (musicNativeKind(key)) return ScriptFunctionSupport::StandIn;
    if (soundNativeKind(key)) return ScriptFunctionSupport::StandIn;
    if (textNativeKind(key)) return ScriptFunctionSupport::StandIn;
    if (const auto kind = objectiveNativeKind(key))
        return objectiveNativeIsStandIn(*kind) ? ScriptFunctionSupport::StandIn : ScriptFunctionSupport::Implemented;
    if (aiNativeKind(key)) return ScriptFunctionSupport::StandIn;
    if (animationNativeKind(key)) return ScriptFunctionSupport::StandIn;
    if (cutsceneNativeKind(key)) return ScriptFunctionSupport::StandIn;
    if (effectNativeKind(key)) return ScriptFunctionSupport::StandIn;
    if (const auto kind = worldNativeKind(key)) return worldNativeIsStandIn(*kind) ? ScriptFunctionSupport::StandIn : ScriptFunctionSupport::Implemented;
    if (const auto kind = partyNativeKind(key)) return partyNativeIsStandIn(*kind) ? ScriptFunctionSupport::StandIn : ScriptFunctionSupport::Implemented;
    return ScriptFunctionSupport::None;
}

std::vector<std::string> handledScriptFunctions() {
    std::vector<std::string> names;
    for (const ScriptFunction& entry : kScriptFunctions) names.push_back(entry.name);
    for (const std::string& name : objectNativeNames()) names.push_back(name);
    for (const std::string& name : vehicleNativeNames()) names.push_back(name);
    for (const std::string& name : progressNativeNames()) names.push_back(name);
    for (const std::string& name : musicNativeNames()) names.push_back(name);
    for (const std::string& name : soundNativeNames()) names.push_back(name);
    for (const std::string& name : textNativeNames()) names.push_back(name);
    for (const std::string& name : objectiveNativeNames()) names.push_back(name);
    for (const std::string& name : aiNativeNames()) names.push_back(name);
    for (const std::string& name : animationNativeNames()) names.push_back(name);
    for (const std::string& name : cutsceneNativeNames()) names.push_back(name);
    for (const std::string& name : effectNativeNames()) names.push_back(name);
    for (const std::string& name : worldNativeNames()) names.push_back(name);
    for (const std::string& name : partyNativeNames()) names.push_back(name);
    return names;
}

ScriptRunner::ScriptRunner(const Level& level, assets::PlacedObjectState& state)
    : level_(level), state_(state), triggers_(level.triggers.size()) {
    for (const assets::LevelBound& bound : level.bounds) boundEnabled_.push_back(bound.enabled);
    ai_.setPositionLookup([this](const std::string& name, float out[3]) { return objectPosition_ && objectPosition_(name, out); });
    ai_.setPathLookup([this](const std::string& name, SimplePathFollow& out) {
        const std::uint32_t hash = assets::nameHash(name);
        for (const assets::LevelPath& path : level_.paths)
            if (path.nameHash == hash) {
                out.nodes = path.points;
                out.closed = path.closed;
                return true;
            }
        return false;
    });
    ai_.setUpdateGate([this](const std::string& name) {
        return characterControl(director_.holdsControl(), name == "player1", true, director_.objectActiveDuringDirector(name)) == CharacterControl::Ai;
    });
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
    text_.update(dt, objectives_.mapBlackout());
    effectNatives_.update(dt);
    ai_.update(dt);
    screenFade_.update(dt);
    cutscenes_.update(dt, screenFade_.running());
    if (cutscenes_.takeModeEntered()) screenFade_.fadeIn(kCutsceneModeFadeSeconds, kCutsceneFadeScreens);
    switch (cutscenes_.takeFadeRequest()) {
    case CutsceneFadeRequest::Out: screenFade_.fadeOut(kCutsceneFadeSeconds, kCutsceneFadeScreens); break;
    case CutsceneFadeRequest::In: screenFade_.fadeIn(kCutsceneFadeSeconds, kCutsceneFadeScreens); break;
    case CutsceneFadeRequest::None: break;
    }
    fireAiMoveEvents();
    for (auto it = threads_.begin(); it != threads_.end();) {
        const auto next = std::next(it);
        execute(*it);
        it = next;
    }
}

std::vector<std::string> ScriptRunner::updateWaypoints(float dt, const float player[3], const WaypointVehicle* vehicle) {
    const WaypointUpdate update = waypoints_.update(dt, player, vehicle, [this](const std::string& room) {
        return allRoomsLoaded_ || loadedRooms_.count(lower(room)) > 0;
    });
    for (const WaypointVehicleAction& action : update.vehicleActions) {
        vehicles_.apply(VehicleNativeKind::SetForcedStop, action.vehicle, action.stop ? 1.0f : 0.0f);
        if (action.stop) vehicles_.limitSpeed(action.vehicle, kWaypointVehicleSpeedLimit);
    }
    if (update.vehicleHint) text_.showTip(kWaypointVehicleHintKey, kWaypointVehicleHintSeconds);
    for (const std::string& name : update.deactivated) objectives_.setWaypointActive(name, false);
    for (const std::string& name : update.startScripts)
        if (const LevelScript* script = find(name)) create(*script);
    return update.startScripts;
}

void ScriptRunner::setCarryGroupTarget(const std::string& lowerTarget, bool enable) {
    objectives_.setSubObjective(waypoints_.carryItWaypoint(), lowerTarget, 2, enable, !state_.isHidden(lowerTarget));
    objectives_.setArrow(2, lowerTarget, enable);
}

void ScriptRunner::updateObjectiveMarkers(float dt, SubObjectiveWorld world) {
    if (!world.hidden) world.hidden = [this](const std::string& name) { return state_.isHidden(name); };
    if (!world.destroyed) world.destroyed = [this](const std::string& name) { return state_.destroyed.count(lower(name)) != 0; };
    if (!world.position) world.position = [this](const std::string& name, float out[3]) { return objectPosition_ && objectPosition_(name, out); };
    objectives_.update(dt, world);
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

float ScriptRunner::variable(const std::string& name) const { return variable(assets::nameHash(name)).value_or(0.0f); }

std::optional<float> ScriptRunner::variable(std::uint32_t nameHash) const {
    const auto found = variables_.find(nameHash);
    return found == variables_.end() ? std::nullopt : std::optional<float>(found->second);
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

std::vector<ScriptThreadEvent> ScriptRunner::takeThreadEvents() {
    std::vector<ScriptThreadEvent> taken;
    taken.swap(threadEvents_);
    return taken;
}

ScriptThread* ScriptRunner::create(const LevelScript& script) {
    threadEvents_.push_back({script.nameHash, true});
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
    threadEvents_.push_back({thread.script->nameHash, false});
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
    if (!script)
        throw std::runtime_error(call.function + " of a script the level does not have: " + (call.arguments.empty() ? std::string("(no argument)") : call.arguments[0]) +
                                 " (the original 0x1c3730 reads the null script at address 4 and faults)");
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
        if (!args.empty()) variables_[assets::nameHash(args[0])] = progress_.gameComplete ? 1.0f : 0.0f;
        break;
    case Word::IsLevelUnlocked:
        if (args.size() >= 2) {
            const int level = int(read(args[0]));
            variables_[assets::nameHash(args[1])] = level >= 0 && level < 9 && (progress_.levelUnlocked >> level & 1) ? 1.0f : 0.0f;
        }
        break;
    case Word::IsRedBrickExtraActive:
        if (args.size() >= 2) {
            const int index = int(read(args[0]));
            variables_[assets::nameHash(args[1])] = index >= 0 && index < 32 && (progress_.redBrickExtrasActive >> index & 1) ? 1.0f : 0.0f;
        }
        break;
    case Word::PositionObject:
    case Word::PositionPlayer:
        if (args.size() >= 2) {
            placements_.push_back({lower(args[0]), lower(args[1])});
            float where[3];
            if (objectPosition_ && objectPosition_(lower(args[1]), where)) ai_.setPosition(lower(args[0]), where);
        }
        break;
    case Word::SceneChange:
        if (!args.empty()) sceneChanges_.push_back(args[0]);
        break;
    case Word::KillScript:
        if (!args.empty()) killScript(thread, args[0]);
        break;
    case Word::ObjectNative:
        if (const auto kind = objectNativeKind(lower(call.function)); kind && !args.empty()) {
            if (*kind == ObjectNativeKind::DestroyProp || *kind == ObjectNativeKind::KillObject) {
                destroyNative(lower(args[0]));
                break;
            }
            if (*kind == ObjectNativeKind::EnableObjectFade) {
                if (args.size() >= 2 && objectNatives_.enableObjectFadeFallsBack(lower(args[0]))) {
                    if (read(args[1]) == 0.0f) state_.disabled.insert(lower(args[0]));
                    else state_.disabled.erase(lower(args[0]));
                }
                break;
            }
            const bool needsValue = objectNativeTakesValue(*kind);
            if (needsValue && args.size() < 2) break;
            const float value = needsValue ? read(args[1]) : 0.0f;
            objectNatives_.apply(*kind, lower(args[0]), value);
            if (*kind == ObjectNativeKind::ResetObject || (*kind == ObjectNativeKind::EnableSwitch && value > 0.0f)) resetEntity(lower(args[0]));
        }
        break;
    case Word::VehicleNative:
        if (const auto kind = vehicleNativeKind(lower(call.function)); kind && !args.empty()) {
            const VehicleNativeArgs shape = vehicleNativeArgs(*kind);
            const bool second = shape == VehicleNativeArgs::NameValue || shape == VehicleNativeArgs::NameName;
            if (!second || args.size() >= 2) {
                const std::string name = shape == VehicleNativeArgs::Value ? std::string() : lower(args[0]);
                const float value = shape == VehicleNativeArgs::NameValue ? read(args[1]) : shape == VehicleNativeArgs::Value ? read(args[0]) : 0.0f;
                const std::string other = shape == VehicleNativeArgs::NameName ? lower(args[1]) : std::string();
                if (!vehicles_.apply(*kind, name, value, other)) return Result::Block;
            }
        }
        break;
    case Word::StartDCam: startDCamNative(director_, level_, args, [this](const std::string& token) { return read(token); }); break;
    case Word::StartDCamTrack: startDCamTrackNative(director_, level_, args, [this](const std::string& token) { return read(token); }); break;
    case Word::DCamActive: dcamActiveNative(director_, args, [this](const std::string& token) { return read(token); }); break;
    case Word::WaitForDCam: return director_.busy() || cutscenes_.busy("") ? Result::Block : Result::Continue;
    case Word::EndDCam: director_.requestEnd(); break;
    case Word::SnapCamera: director_.snapFollowCamera(); break;
    case Word::ShakeScreen: shakeScreenNative(director_, args, [this](const std::string& token) { return read(token); }); break;
    case Word::Set3DFocusObject: break;
    case Word::CameraFollow: break;
    case Word::SetDCamStartWait:
        if (!args.empty()) director_.setStartWait(read(args[0]));
        break;
    case Word::SetDCamEndWait:
        if (!args.empty()) director_.setEndWait(read(args[0]));
        break;
    case Word::ProgressNative:
        progressNative(call);
        break;
    case Word::MusicNative:
        musicNative(call);
        break;
    case Word::SoundNative:
        soundNative(call);
        break;
    case Word::TextNative:
        textNative(call);
        break;
    case Word::ObjectiveNative:
        objectiveNative(call);
        break;
    case Word::AiNative:
        aiNative(call);
        break;
    case Word::AnimationNative:
        if (!animationNative(call)) return Result::Block;
        break;
    case Word::CutsceneNative:
        return cutsceneNative(thread, call);
    case Word::EffectNative:
        effectNative(call);
        break;
    case Word::WorldNative:
        return worldNative(call);
    case Word::PartyNative:
        partyNative(call);
        break;
    case Word::InformationBox:
        if (!(thread.flags & kScriptBlocked)) {
            if (args.empty()) break;
            dialog_ = ScriptDialog{textKeyOf(args[0]), ScriptDialog{}.confirmKey, kInformationBoxCancelKey};
            dialogOpen_ = true;
            dialogAnswered_ = false;
            dialogResult_ = 0;
            return Result::Block;
        }
        if (!dialogAnswered_) return Result::Block;
        dialogOpen_ = false;
        dialogAnswered_ = false;
        break;
    case Word::WarningDialogBox:
        if (!(thread.flags & kScriptBlocked)) {
            dialog_ = ScriptDialog{};
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
            const FadeScreens screens = fadeScreenScreens(text_.dialogBgFlag());
            if (read(args[0]) == 0.0f) screenFade_.fadeOut(kFadeSeconds, screens);
            else screenFade_.fadeIn(kFadeSeconds, screens);
        }
        if (screenFade_.running()) return Result::Block;
        break;
    default: break;
    }
    return Result::Continue;
}

void ScriptRunner::progressNative(const ScriptCall& call) {
    const auto kind = progressNativeKind(lower(call.function));
    if (!kind) return;
    const ProgressNativeShape shape = progressNativeShape(*kind);
    const std::size_t first = shape.object ? 1 : 0;
    const auto& args = call.arguments;
    if (args.size() < first + shape.values + (shape.output ? 1 : 0)) return;
    std::vector<float> values;
    for (unsigned i = 0; i < shape.values; ++i) values.push_back(read(args[first + i]));
    const ProgressNativeResult result = progressNatives_.run(*kind, values, shape.object ? lower(args[0]) : std::string());
    if (result.unlockedLevel) progress_.levelUnlocked |= 1u << *result.unlockedLevel;
    if (!shape.output) return;
    float output = result.output.value_or(-1.0f);
    if (result.languageParameter) {
        const auto found = variables_.find(assets::nameHash(*result.languageParameter));
        if (found != variables_.end()) output = float(static_cast<std::int8_t>(static_cast<std::int32_t>(found->second)));
    }
    variables_[assets::nameHash(args[first + shape.values])] = output;
}

void ScriptRunner::musicNative(const ScriptCall& call) {
    const auto kind = musicNativeKind(lower(call.function));
    if (!kind || call.arguments.size() < musicNativeArguments(*kind)) return;
    std::vector<float> values;
    for (unsigned i = 0; i < musicNativeArguments(*kind); ++i) values.push_back(read(call.arguments[i]));
    musicNatives_.run(*kind, values);
}

void ScriptRunner::soundNative(const ScriptCall& call) {
    const auto kind = soundNativeKind(lower(call.function));
    const unsigned count = kind ? soundNativeArguments(*kind) : 0;
    if (!kind || call.arguments.size() < count) return;
    const int objectArgument = soundNativeObjectArgument(*kind);
    std::vector<float> values(count, 0.0f);
    std::vector<std::string> objects(count);
    for (unsigned i = 0; i < count; ++i) {
        if (int(i) == objectArgument) objects[i] = lower(call.arguments[i]);
        else values[i] = read(call.arguments[i]);
    }
    soundNatives_.run(*kind, values, objects, [this](const std::string& name, float out[3]) { return objectPosition_ && objectPosition_(name, out); });
}

std::uint32_t ScriptRunner::textKeyOf(const std::string& token) {
    if (!token.empty() && std::all_of(token.begin(), token.end(), [](unsigned char c) { return std::isdigit(c); }))
        return std::uint32_t(std::stoull(token));
    return std::uint32_t(read(token));
}

void ScriptRunner::textNative(const ScriptCall& call) {
    const auto kind = textNativeKind(lower(call.function));
    if (!kind) return;
    const auto& args = call.arguments;
    if (args.size() < textNativeArguments(*kind)) return;
    const auto key = [&](std::size_t index) { return textKeyOf(args[index]); };
    switch (*kind) {
    case TextNativeKind::ShowTipTextOnce: text_.showTip(key(0), kTipOnceSeconds); break;
    case TextNativeKind::ShowTipTextTimed: text_.showTip(key(0), read(args[1])); break;
    case TextNativeKind::ShowDialogueText: text_.showDialogue(key(0), std::uint16_t(read(args[1])), lower(args[2]), kDialogueDefaultSeconds); break;
    case TextNativeKind::ShowDialogueTextTimed: text_.showDialogue(key(0), std::uint16_t(read(args[1])), lower(args[2]), read(args[3])); break;
    case TextNativeKind::ShowDialogBG: text_.setDialogBgFlag(scriptWordIsSet(read(args[0]))); break;
    case TextNativeKind::ShowLevelText:
        if (levelHeadingKey_) text_.showHeading(levelHeadingKey_);
        else text_.record(*kind, {});
        break;
    case TextNativeKind::ShowCharacterIntro: text_.showCharacterIntro(std::uint16_t(read(args[0]))); break;
    case TextNativeKind::ShowIntroText: text_.showHeading(key(0), true); break;
    case TextNativeKind::SetMissionTitle: text_.setMissionTitle(key(0)); break;
    case TextNativeKind::SetMissionSummary: text_.setMissionSummary(key(0)); break;
    default: {
        std::vector<float> values;
        for (const std::string& token : args) values.push_back(read(token));
        text_.record(*kind, values);
    }
    }
}

void ScriptRunner::objectiveNative(const ScriptCall& call) {
    const auto kind = objectiveNativeKind(lower(call.function));
    if (!kind) return;
    const auto& args = call.arguments;
    if (args.size() < objectiveNativeArguments(*kind)) return;
    switch (*kind) {
    case ObjectiveNativeKind::SetActiveWayPoint: {
        const std::string waypoint = lower(args[0]);
        const bool enable = read(args[1]) != 0.0f;
        objectives_.setWaypointActive(waypoint, waypoints_.known(waypoint) ? waypoints_.setActive(waypoint, enable) : enable);
        break;
    }
    case ObjectiveNativeKind::SubObjectiveGoToEnable:
        objectives_.setSubObjective(lower(args[0]), lower(args[1]), 0, read(args[2]) != 0.0f, !state_.isHidden(lower(args[1])));
        break;
    case ObjectiveNativeKind::SubObjectiveEnemyEnable: break;
    case ObjectiveNativeKind::SubObjectiveArrowGoToEnable: objectives_.setArrow(0, lower(args[0]), read(args[1]) != 0.0f); break;
    case ObjectiveNativeKind::SubObjectiveArrowEnemyEnable: objectives_.setArrow(1, lower(args[0]), read(args[1]) != 0.0f); break;
    default: {
        std::vector<float> values;
        for (const std::string& token : args) values.push_back(read(token));
        objectives_.record(*kind, values, lower(args[0]));
    }
    }
}

void ScriptRunner::aiNative(const ScriptCall& call) {
    const auto kind = aiNativeKind(lower(call.function));
    if (!kind) return;
    const auto& args = call.arguments;
    if (args.size() < aiNativeArguments(*kind)) return;
    const std::size_t objects = aiNativeObjects(*kind);
    std::vector<std::string> names;
    std::vector<float> numbers;
    for (std::size_t i = 0; i < aiNativeArguments(*kind); ++i) {
        if (i < objects) names.push_back(lower(args[i]));
        else numbers.push_back(read(args[i]));
    }
    ai_.run(*kind, names, numbers);
}

void ScriptRunner::loadAnimationOffsets(const assets::ClassLookup& classOf) {
    const std::uint32_t hash = assets::nameHash("AnimationOffset");
    std::map<std::string, std::uint32_t> offsets;
    for (const assets::LevelObject& object : level_.objects) {
        const assets::Level* cls = classOf(object.className);
        if (!cls) continue;
        if (const assets::LevelParam* param = effectiveParam(object, *cls, hash)) offsets[lower(object.instance)] = param->value;
    }
    animation_.setAnimationOffsets(std::move(offsets));
}

bool ScriptRunner::animationNative(const ScriptCall& call) {
    const auto kind = animationNativeKind(lower(call.function));
    if (!kind) return true;
    std::vector<std::string> args;
    std::vector<float> numbers;
    for (const std::string& token : call.arguments) {
        args.push_back(unquoted(token));
        numbers.push_back(read(token));
    }
    return animation_.apply(*kind, args, numbers, animationDriver_);
}

ScriptRunner::Result ScriptRunner::cutsceneNative(ScriptThread& thread, const ScriptCall& call) {
    const auto kind = cutsceneNativeKind(lower(call.function));
    const auto& args = call.arguments;
    if (!kind || args.size() < cutsceneNativeArguments(*kind)) return Result::Continue;
    const std::string name = args.empty() ? std::string() : lower(unquoted(args[0]));
    switch (*kind) {
    case CutsceneNativeKind::Load: cutscenes_.load(name); break;
    case CutsceneNativeKind::Play:
        if (cutscenes_.play(name, {read(args[1]) != 0.0f, read(args[2]) != 0.0f, read(args[3]) != 0.0f, read(args[4]) != 0.0f}))
            screenFade_.fadeOut(read(args[1]) != 0.0f ? kCutsceneModeFadeSeconds : kCutscenePlayFadeWithoutFlagA, kCutsceneFadeScreens);
        break;
    case CutsceneNativeKind::Wait: return cutscenes_.busy(name) ? Result::Block : Result::Continue;
    case CutsceneNativeKind::Free: cutscenes_.free(name); break;
    case CutsceneNativeKind::FadeDisable: break;
    case CutsceneNativeKind::Movie:
    case CutsceneNativeKind::MovieNoFadeBackIn:
        if (!(thread.flags & kScriptBlocked)) {
            cutscenes_.startMovie(unquoted(args[0]), *kind == CutsceneNativeKind::Movie);
            return cutscenes_.movieOpen() ? Result::Block : Result::Continue;
        }
        if (cutscenes_.movieOpen()) return Result::Block;
        break;
    }
    return Result::Continue;
}

void ScriptRunner::fireAiMoveEvents() {
    for (const AiMoveEnd& end : ai_.takeMovesEnded())
        for (std::size_t i = 0; i < level_.triggers.size(); ++i)
            if (triggers_[i].enabled && lower(level_.triggers[i].event) == "aimove" && lower(level_.triggers[i].objectName) == end.character) {
                queueTrigger(i);
                break;
            }
}

void ScriptRunner::killScript(const ScriptThread& caller, const std::string& name) {
    const LevelScript* script = find(name);
    if (!script) return;
    for (ScriptThread& thread : threads_)
        if (&thread != &caller && thread.script == script) thread.pc = static_cast<std::uint16_t>(script->calls.size());
}

void ScriptRunner::answerDialog(int result) {
    if (!dialogOpen_ || dialogAnswered_) return;
    if (result == kDialogCancel && dialog_.cancelKey == kInformationBoxCancelKey) return;
    dialogResult_ = result;
    dialogAnswered_ = true;
}

std::vector<std::string> ScriptRunner::takeSceneChanges() {
    std::vector<std::string> taken;
    taken.swap(sceneChanges_);
    return taken;
}

bool ScriptRunner::playerInNamedBound(const std::string& name, const float player[3]) const {
    const int index = boundIndex(name);
    return index >= 0 && playerInBound(std::size_t(index), player);
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

void ScriptRunner::checkTriggers(const float player[3], bool playerInVehicle) {
    for (std::size_t i = 0; i < level_.triggers.size(); ++i) {
        const assets::LevelTrigger& trigger = level_.triggers[i];
        ScriptTriggerState& state = triggers_[i];
        if (!state.enabled || (!trigger.objectName.empty() && state_.isDisabled(lower(trigger.objectName)))) continue;
        const std::string event = lower(trigger.event);
        const TriggerEvent kind = classifyTriggerEvent(event);
        if (kind == TriggerEvent::Unmodelled) {
            if (event != "objecttriggered" && event != "objectuntriggered" && event != "playervehicleentered" && event != "playervehicleexiting" &&
                std::find(unhandledEvents_.begin(), unhandledEvents_.end(), event) == unhandledEvents_.end())
                unhandledEvents_.push_back(event);
            continue;
        }
        const int bound = boundIndex(trigger.firstArgument);
        TriggerSample sample;
        sample.playerInside = bound >= 0 && playerInBound(std::size_t(bound), player);
        sample.playerInVehicle = playerInVehicle;
        float position[3] = {0, 0, 0};
        const bool objectBound = kind == TriggerEvent::ObjectBoundEntered || kind == TriggerEvent::ObjectBoundLeft ||
                                 kind == TriggerEvent::ObjectBoundInside;
        if (objectBound) {
            sample.objectUsable = bound >= 0 && boundEnabled_[std::size_t(bound)] && objectPosition_ &&
                                  objectPosition_(lower(trigger.objectName), position);
            sample.objectInside = sample.objectUsable && pointInBound(std::size_t(bound), position, false);
        }
        TriggerRecord record{state.skip, state.inside};
        const bool fire = applyTriggerCheck(kind, record, sample);
        state.skip = record.skip;
        state.inside = record.inside;
        if (fire) queueTrigger(i);
    }
    while (!queued_.empty()) {
        const std::size_t index = queued_.back();
        queued_.pop_back();
        if (const LevelScript* script = find(level_.triggers[index].scriptName)) create(*script);
    }
}

void ScriptRunner::fireObjectTriggered(const std::string& lowerInstance) { fireObjectEvent("objecttriggered", lowerInstance); }

void ScriptRunner::triggerObjectEnabling(const std::string& lowerInstance) {
    enableObject(lowerInstance);
    sendObjectMessage(lowerInstance, kMessageTrigger);
    fireObjectTriggered(lowerInstance);
}

void ScriptRunner::enableObject(const std::string& lowerInstance) { state_.disabled.erase(lowerInstance); }

void ScriptRunner::sendObjectMessage(const std::string& lowerInstance, std::uint8_t message) { messages_.push_back({lowerInstance, message}); }

void ScriptRunner::fireObjectEvent(const std::string& lowerEvent, const std::string& lowerInstance) {
    for (std::size_t i = 0; i < level_.triggers.size(); ++i)
        if (triggers_[i].enabled && lower(level_.triggers[i].event) == lowerEvent && lower(level_.triggers[i].objectName) == lowerInstance) {
            queueTrigger(i);
            break;
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

bool ScriptRunner::pointInBound(std::size_t index, const float player[3], bool requireRoom) const {
    const assets::LevelBound& bound = level_.bounds[index];
    if (!boundEnabled_[index] || (requireRoom && !boundActive(index))) return false;
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
    knownRooms_.insert(lower(room));
    if (loaded) loadedRooms_.insert(lower(room));
    else loadedRooms_.erase(lower(room));
}

void ScriptRunner::setAllRoomsLoaded() { allRoomsLoaded_ = true; }

void ScriptRunner::setSceneRooms(const std::set<std::string>& rooms) {
    sceneRooms_.emplace();
    for (const std::string& room : rooms) sceneRooms_->insert(lower(room));
}

bool ScriptRunner::boundActive(std::size_t index) const {
    const std::string& room = level_.bounds[index].room;
    return room.empty() || allRoomsLoaded_ || (sceneRooms_ && sceneRooms_->count(lower(room)) == 0) || loadedRooms_.count(lower(room)) > 0;
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
