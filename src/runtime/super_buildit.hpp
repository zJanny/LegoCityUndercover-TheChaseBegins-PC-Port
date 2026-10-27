#pragma once
// Super build-its (class props/prop_superbuildit, kind 98): the plate that asks for super bricks, the "needs N super bricks" prompt, the
// confirmation, the movie and the build that tells the story script. Rules and evidence: docs/re/super-buildit.md. No SDL dependency.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/script_runner.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr const char* kSuperBuilditClass = "props/prop_superbuildit";
constexpr std::uint8_t kSuperBuilditKind = 98;
// Kind of the holder classes (props/prop_sbhelipad*, prop_sbvehicledrop*; create 0x29c964 only): a super build-it nested in one takes the
// holder's SuperBuildValue (link 0x1af224).
constexpr std::uint8_t kSuperBuilditHolderKind = 253;
// Use zone (link 0x1af224 -> 0x23e130): the node bounds with each half extent widened by these; SuperBuildType 12 narrows x and z instead.
constexpr std::array<float, 3> kSuperBuilditZoneMargin = {0.5f, 0.2f, 0.5f};
constexpr std::array<float, 3> kSuperBuilditZoneMarginType12 = {-1.5f, 0.2f, -1.5f};
constexpr int kSuperBuilditTypeNarrowZone = 12;

// Class data halfwords +2 (state) and +4 (requested state), stepped by 0x26d550.
enum class SuperBuilditPhase : std::uint16_t { Created = 0, Off = 1, Waiting = 2, Built = 3, Confirm = 4 };

// Results of the confirmation dialog (object 0x375f00, word +0x28), as read by 0x26d550.
constexpr int kSuperBuilditDialogNone = 0;
constexpr int kSuperBuilditDialogCancel = 1;
constexpr int kSuperBuilditDialogConfirm = 2;

// Text keys of the prompt (0x213df4) and of the dialog (0x1bbec4 -> 0x20ffe8).
constexpr std::uint32_t kSuperBuilditFreeKey = 0xc415b30d;        // gametext "FREE", value 0
constexpr std::uint32_t kSuperBuilditNotEnoughKey = 0x8580a309;   // gametext "Not enough Super Bricks!"
constexpr std::uint32_t kSuperBuilditBuildKey = 0x473818f3;       // gametext "Build"
constexpr std::uint32_t kSuperBuilditConfirmBodyKey = 0x968ebefd; // gametext "Are you sure?"
constexpr std::uint32_t kSuperBuilditConfirmOkKey = 0xd12f028b;   // systemtext "OK"
constexpr std::uint32_t kSuperBuilditConfirmCancelKey = 0x994b0cc9;  // systemtext "Cancel"

struct SuperBuilditObject {
    std::string instance;   // lower case; "parent.child" for one nested in a class file
    std::string parent;     // lower case holder instance, empty for a placed one
    std::uint32_t value = 0;                 // +0x80 SuperBuildValue (the holder's when nested in kind 253)
    int type = 0;                            // +0x8c SuperBuildType: row of tables/tablessuperbuildit (picture of the prompt)
    std::uint32_t textPopup = 0xffffffffu;   // TextPopup: text key of the name, gametext then hinttext
    std::string fmv;                         // +0x88 FMVToPlay
    std::uint16_t soundBuilt = 0;            // +0x90 ATTR_SFXBuilt
    bool playerFaceObject = false;           // +0x92 bit 0
    std::string propToReveal, useObject, hideObject, buildCompleteTrigger;  // +0x1c, +0x2c, +0x24, +0x28 (lower case, empty when unset)
    std::string parentTrigger;               // the holder's TriggerObject, activated by the build (0x26d550 state 3)
    std::string mesh;                        // model of the plate
    assets::Matrix world{};                  // node world matrix
    float zoneCentre[3] = {0, 0, 0};         // node frame
    float zoneHalf[3] = {0, 0, 0};
    bool hasZone = false;
    std::uint16_t state = std::uint16_t(SuperBuilditPhase::Created);
    std::uint16_t requested = std::uint16_t(SuperBuilditPhase::Waiting);  // create 0x1af05c
    bool built = false;        // +0x92 bit 1
    bool saveDisabled = false; // +0x92 bit 3: DisableSuperBuiltItSave (0x26d530)
    bool saveState = true;     // entity +0x82 bit 0: the class writes a save record and has a restore callback (SaveState)
    bool zoneActive = true;    // the use zone record (0x236624): switched off by the answer OK, on again by the reset
    bool promptShown = false;  // +0x92 bit 2
    std::uint32_t countSeen = 0xffffffffu;  // +0x84: the count read by the last accepted use query
};

// The shared prompt object (0x375858 page, owner 0x37588c) as 0x213df4 fills it.
struct SuperBuilditPrompt {
    int type = 0;
    std::uint32_t value = 0;
    std::size_t object = 0;
    bool enough = false;  // value <= the super brick count
};

enum class SuperBuilditActionKind {
    PromptShown,   // 0x1cc994 took the page: TransitionOn
    PromptHidden,  // 0x1cc4d4 released it: TransitionOff
    ConfirmOpened, // use query with enough bricks: "Are you sure?" pushed (0x25d26c), the game pauses
    MovieStarted,  // dialog answer 2: FMVToPlay starts (0x20f29c), the game pauses
    Built,         // change to state 3
};

struct SuperBuilditAction {
    SuperBuilditActionKind kind = SuperBuilditActionKind::PromptShown;
    std::size_t index = 0;
    bool countsBricks = false;  // Built from a state other than 0: TriggerObject of the holder, bricks deducted
};

struct SuperBuilditInput {
    float player[3] = {0, 0, 0};
    // STAND-IN LEVOBJ-S94: the player gate of the update 0x1af3d0 (globals 0x375ee8, 0x5189dc, 0x375848, the player's state +0x6c and 0x24255c,
    // kind 159) is the port's "player free" flag; the characters standing in the zone that block the prompt (0x237f30 list 0x578dfc) are not tested.
    // From the OK answer on (built bit) the gate is closed: the answer stores the plate at player +0x130, which 0x24255c returns in the player
    // states 0x11c and 0x19b; who sets that state is not read, and the oracle shows no prompt after the movie.
    bool playerFree = false;
    bool playerCarrying = false;  // player record +0x13c: the use query is refused
    bool actionHeld = false;      // the action flag of the use query (LEVOBJ-S41: read as A)
    std::uint32_t superBricks = 0;  // 0x593430
};

class SuperBuilditSet {
public:
    // Every props/prop_superbuildit placed in the level and every one nested in the class file of a placed object.
    void load(assets::FuseArchive& archive, const assets::Level& level, const assets::ClassLookup& classOf);
    void add(SuperBuilditObject object) { objects_.push_back(std::move(object)); }
    // One update: for each enabled super build-it its update 0x1af3d0 (zone, prompt) with the state change 0x26d550, then the use query (message 3,
    // 0x1bbec4) of the one whose zone holds the player.
    std::vector<SuperBuilditAction> update(const assets::PlacedObjectState& state, const SuperBuilditInput& input);
    bool confirmOpen() const { return confirmOpen_; }
    // Closes the dialog with kSuperBuilditDialogCancel or kSuperBuilditDialogConfirm; the next update reads it.
    void answer(int result);
    const std::optional<SuperBuilditPrompt>& prompt() const { return prompt_; }
    const std::vector<SuperBuilditObject>& objects() const { return objects_; }
    const SuperBuilditObject* find(const std::string& lowerInstance) const;
    SuperBuilditObject* findMutable(const std::string& lowerInstance);
    static bool inZone(const SuperBuilditObject& object, const float player[3]);

private:
    void stepState(std::size_t index, std::vector<SuperBuilditAction>& actions);
    std::vector<SuperBuilditObject> objects_;
    std::optional<SuperBuilditPrompt> prompt_;
    std::optional<std::size_t> owner_;  // 0x37588c
    bool confirmOpen_ = false;
    int dialogResult_ = kSuperBuilditDialogNone;
};

// The tables/tablessuperbuildit picture of a SuperBuildType ("objecthelipad.png" for 1), as the prompt loads it from
// sprites/blend/breakandbuildmenu/images/ (setup 0x290b1c).
std::string superBuilditPicture(const std::vector<std::string>& tableNames, int type);

// The links at level load: 0x1af224 disables PropToReveal, 0x19fa90 disables UseObject.
void applySuperBuilditLink(const SuperBuilditSet& set, assets::PlacedObjectState& state);

// What Built does (0x26d550 state 3): the plate and HideObject disabled, PropToReveal and UseObject enabled, BuildCompleteTriggerObject and, unless the
// change came from state 0, the holder's TriggerObject activated (0x244f40), the value taken from the super brick count (0x593430) and a save
// request 0x21cf38(0) made after it.
struct SuperBuilditSpend {
    std::uint32_t bricks = 0;   // to subtract from the count
    unsigned saveRequests = 0;  // 0x21cf38(0) calls, after the subtraction
};
SuperBuilditSpend applySuperBuilditActions(const SuperBuilditSet& set, const std::vector<SuperBuilditAction>& actions, assets::PlacedObjectState& state,
                                           ScriptRunner& scripts);

}
