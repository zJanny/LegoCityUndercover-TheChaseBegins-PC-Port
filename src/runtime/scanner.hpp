#pragma once
// The sound scanner (object type 201, props/prop_scannerzoneaudio, and the scannable objects it looks for): the use press on the
// zone, the player states "Scanner Enter" (0x156) and "Scanner Active" (0x157), the scanner view with aim, zoom, lock-on and scan,
// the exit, and the ObjectTriggered events of the zone and of the scanned object. Rules and measurements: docs/re/abilities-scanner.md.
// No SDL dependency.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/natives_sound.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr int kScannerZoneObjectType = 201;
// ScannerMode of the audio zones; the photo (0), physical, satellite and cannon zones share type 201 but use other mode values (SCAN-2).
constexpr int kScannerAudioMode = 1;

// Settings block of the scanner camera (initialiser 0x1e06ac, registered as "Game.Scanner.Audio"; read at 0x522440 in the save state on the roof at the sound scanner trigger of mission 1-2).
struct ScannerSettings {
    float yawLimitDegrees = 0.0f;  // [0]: 0 leaves the yaw free
    float pitchMinDegrees = -25.0f;
    float pitchMaxDegrees = 40.0f;
    float smoothing = 0.1f;         // [3]: step of the slerp 0x21a600 towards the aimed orientation
    float zoomTurnScale = 0.66f;    // [4]: turn speed factor 1 - 0.66 * (1 - zoom fraction)
    float zoomStart = 43.0f;        // [5]
    float zoomMin = 20.0f;          // [6]: fully zoomed in
    float zoomMax = 43.0f;          // [7]
    float turnDegreesPerSecond = 60.0f;  // [8]
    float zoomDegreesPerSecond = 35.0f;  // [9]
};

// Values the audio sub-mode (byte +0xd58 = 1) writes into the mode object 0x5216ac every update (0x2ef814, branch 0x2ef88c; read in the save state on the roof at the sound scanner trigger of mission 1-2).
struct ScannerModeParams {
    float scanRate = 0.75f;           // +0x34: scan progress per second
    float scanDecay = 4.0f;           // +0x38: progress lost per second after a cancel
    float reticleRadius = 80.0f;      // +0xcd0 / +0xcd4, screen pixels
    float angleZoomedOut = 50.0f;     // +0xcd8: largest angle to the view axis, degrees
    float angleZoomedIn = 30.0f;      // +0xcdc
    float rangeZoomedOut = 30.0f;     // +0xce0: distance limit
    float rangeZoomedIn = 30.0f;      // +0xce4
    float lockRate = 1.5f;            // +0xcf0: lock progress per second
};

// 0x29125c: a scannable object with a shown object while the player uses a zone gets the distance limit 100.
constexpr float kScannerLinkedRange = 100.0f;
// 0x2248c4: the alignment parameter grows by 1.5 per second; the facing turns by at most 0x800 per update.
constexpr float kScannerAlignRate = 1.5f;
constexpr int kScannerAlignTurnStep = 0x800;
// 0x1ebe88: the view sits at the player position plus 1.7 times the player height (+0x70) plus the zone's CameraYOffset.
constexpr float kScannerEyeHeightFactor = 1.7f;
// Real-time counts of the scanner transitions, in updates (the game clock advances 100 of 3000 ticks per second per update, so one update is
// 1/30 s). Measured in the oracle with the motion source (save state on the roof at the sound scanner trigger of mission 1-2, request B-1, docs/re/abilities-scanner.md): the
// press clears the toggle byte 0x375eb6 in its own update, state 0x157 is entered 49 updates later, the enter callback 0x2bee98 (mode index set,
// EnterScanTriggerObject) comes 20 updates after that, and the end callback 0x1a27e0 (toggle byte back to 1) together with the first camera update
// 0x26f0b4 (camera yaw valid) 56 updates after the entry. SCAN-S1: the counts are measured, the animation clocks behind them are not read.
constexpr unsigned kScannerTakeOutUpdates = 49;
constexpr unsigned kScannerEnterCallbackUpdates = 20;
constexpr unsigned kScannerEnterEndUpdates = 56;
constexpr unsigned kScannerCameraStartUpdates = 56;
// The exit (state 0x158, transition kind 1 with 0x2befc0 / 0x1a2824) cleared the flags 57 updates after the entry of the transition (the exit
// triggered by the script's EnableScanner 0, same run). SCAN-S1: the exit callback 0x2befc0 is placed at the enter callback's count (not observed).
constexpr unsigned kScannerExitCallbackUpdates = 20;
constexpr unsigned kScannerExitEndUpdates = 57;
// Player animation roles (tablesanimations rows): 0x208 "scanner_out" (state 0x156 enter 0x322ba8), 0x207 "scanner_in" (exit 0x2befc0).
constexpr int kAnimationScannerOut = 0x208;
constexpr int kAnimationScannerIn = 0x207;
// Sounds of the scanner (0x2247ac plays, 0x224cdc stops; ids as passed).
constexpr std::uint32_t kScannerSoundEnter = 0x19;      // 0x1bf2d0
constexpr std::uint32_t kScannerSoundExit = 0x1a;       // 0x2249e0 exit branch
constexpr std::uint32_t kScannerSoundAmbience = 0x27;   // 0x187f78 while active
constexpr std::uint32_t kScannerSoundLocked = 0x2b;     // 0x2ef4c8 lock reached
constexpr std::uint32_t kScannerSoundLockLost = 0x2c;   // 0x2ef4c8 lock lost
constexpr std::uint32_t kScannerSoundZoomLoop = 0x2d;   // 0x26f0b4
constexpr std::uint32_t kScannerSoundZoomStart = 0x2e;  // 0x26f0b4
constexpr std::uint32_t kScannerSoundScanning = 0xee;   // 0x1ebe18 loop in state 5, stopped by 0x1eb0e0
constexpr std::uint32_t kScannerSoundScanned = 0xef;    // 0x2ef310 scan complete
// AUDIO-S33: owner of the scanner sounds in the port's sound player; the original plays them on the 2D list 0x51a3ac (0x240944, not read).
constexpr std::int32_t kScannerSoundOwner = 0xfff0;

struct ScannerZone {
    std::string instance;  // lower case
    float position[3] = {0, 0, 0};
    float yawDegrees = 0;  // initialorientation y
    float useCentre[3] = {0, 0, 0};  // class bound "UseBound", relative to the position
    float useHalf[3] = {0, 0, 0};
    bool hasUseBound = false;
    int mode = 0;  // ScannerMode (+0x4c)
    float cameraYOffset = 0;
    std::string preEnterTrigger, enterTrigger, exitTrigger, completeExitTrigger, lockedOnTrigger, zoomedInTrigger;  // lower case, empty when unset
};

struct ScannableObject {
    std::string instance;  // lower case
    float point[3] = {0, 0, 0};  // SCAN-S3: placement position plus the model bounds centre
    float radius = 1.0f;         // component +0x1c, 1 when 0 (0x29125c)
    std::string triggerScanned;  // TriggerScanned, lower case
    bool hasShownObject = true;  // component +4: the model entity; gives kScannerLinkedRange
    int infoIcon = 0;            // class value InfoIcon: row of tables/tablespoi.bxls sheet 0 (SCAN-S12)
    bool scanned = false;        // component flag bit 1: TriggerScanned already fired
};

enum class ScannerPhase {
    Idle,
    TakingOut,  // player state 0x156: the scanner_out clip
    Entering,   // state 0x157 entered (0x1bf2d0), spinner transition running, view not started
    Active,     // view running (after 0x2bee98)
    Exiting,    // state 0x158 and the exit transition
};

struct ScannerInput {
    float dt = 1.0f / 30.0f;
    float player[3] = {0, 0, 0};
    std::int16_t facing = 0;       // ctrl+8
    float playerHeight = 0.85f;    // entity +0x70
    bool playerFree = true;        // player state 1 or 0x73 (0x2249e0)
    bool usePressed = false;       // the A press edge that walks the use registry (message 3 with pressed 1)
    bool usable = true;            // the zone's registry flag 0x10 (ToggleUseable)
    bool zoneEnabled = true;       // the zone object is not disabled
    bool exitPressed = false;      // slot 7 (B) press count above 0 (0x187f78)
    float scanHeld = 0;            // slot 6 (A) value (0x2ef310)
    bool cancelPressed = false;    // slot 9 (Y) press count above 0 (0x2ef310)
    float stickX = 0, stickY = 0;  // channels 0 and 1
    bool dpadUp = false, dpadDown = false, dpadLeft = false, dpadRight = false;  // slots 0..3
    float shoulderL = 0, shoulderR = 0;  // slots 12 and 13
    unsigned takeOutUpdates = kScannerTakeOutUpdates;  // updates from the press to the entry of state 0x157
    std::function<bool(const std::string&)> enabled;  // object not disabled (scannables)
    std::function<bool(std::uint32_t)> soundPlaying;  // 0x2247f0; without it a sound counts as playing from its play to its stop
};

enum class ScannerEventKind { Trigger, Sound, PlayerPose, PlayerAnimation };

struct ScannerEvent {
    ScannerEventKind kind = ScannerEventKind::Trigger;
    std::string instance;  // Trigger: the script object to fire (0x244f40)
    SoundRequest sound;
    int animation = 0;
};

// One screen-space record of a scannable (0x29125c, 0x28 bytes).
struct ScannerRecord {
    std::size_t object = 0;
    bool visible = false;  // +0x24 bit 0
    float distance = 0;    // +0x10
    float screen[2] = {0, 0};
    float radius = 0;      // +0x1c, pixels
    float distanceSquared = 0;  // +0x20 to the screen centre
};

struct ScannerView {
    float eye[3] = {0, 0, 0};
    float yaw = 0, pitch = 0;  // radians, the port's view() convention
    float zoomDegrees = 43.0f;
};

class Scanner {
public:
    void load(assets::FuseArchive& archive, const assets::Level& level, const assets::ClassLookup& classOf);
    void add(const ScannerZone& zone) { zones_.push_back(zone); }
    void add(const ScannableObject& object) { objects_.push_back(object); }

    std::vector<ScannerEvent> update(const ScannerInput& input);
    // EnableScanner (0x1cd9dc): a change of the active state toggles through 0x2249e0(1, 0).
    void requestEnabled(bool on) { request_ = on ? 1 : 0; }

    ScannerPhase phase() const { return phase_; }
    bool active() const { return activeFlag_; }  // 0x375ee8
    bool canToggle() const { return canToggle_; }  // 0x375eb6
    bool takesPlayerControl() const { return phase_ != ScannerPhase::Idle; }
    bool viewShown() const { return phase_ == ScannerPhase::Active; }
    ScannerView view() const;
    float zoom() const { return zoom_; }
    const ScannerSettings& settings() const { return settings_; }
    float yawOffset() const { return aim_[1]; }
    float pitchOffset() const { return aim_[0]; }
    int targetState() const { return state_; }  // mode +0x3c
    float lockProgress() const { return lockProgress_; }
    float scanProgress() const { return scanProgress_; }
    int target() const { return target_; }
    int current() const { return current_; }  // +0x44: the object being scanned or scanned last
    const std::vector<ScannerRecord>& records() const { return records_; }
    const std::vector<ScannerZone>& zones() const { return zones_; }
    const std::vector<ScannableObject>& objects() const { return objects_; }
    int zone() const { return zone_; }
    // The player while the scanner moves it (alignment 0x2248c4, exit facing 0x2befc0).
    const float* playerPosition() const { return player_; }
    std::int16_t playerFacing() const { return facing_; }
    // SCAN-S4: the player clip shown while the scanner holds the player (scanner_out until the exit callback, then scanner_in).
    int playerClip() const;

private:
    void startEnter(std::size_t zone, std::vector<ScannerEvent>& events);
    void startExit(std::vector<ScannerEvent>& events);
    void align(const ScannerInput& input, std::vector<ScannerEvent>& events);
    void updateCamera(const ScannerInput& input, std::vector<ScannerEvent>& events);
    void updateRecords();
    void updateTarget(const ScannerInput& input, std::vector<ScannerEvent>& events);
    void fire(const std::string& instance, std::vector<ScannerEvent>& events) const;
    void sound(SoundRequest::Kind kind, std::uint32_t id, std::vector<ScannerEvent>& events);
    float zoomFraction() const;
    bool playing(const ScannerInput& input, std::uint32_t id) const;

    std::vector<ScannerZone> zones_;
    std::vector<ScannableObject> objects_;
    std::vector<ScannerRecord> records_;
    ScannerSettings settings_;
    ScannerModeParams params_;
    ScannerPhase phase_ = ScannerPhase::Idle;
    unsigned phaseUpdates_ = 0;
    int zone_ = -1;
    bool activeFlag_ = false;
    bool canToggle_ = true;    // 0x375eb6
    bool cameraStarted_ = false;  // 0x375eb9
    int request_ = -1;
    bool aligning_ = false;    // 0x375e10
    float alignT_ = 0;         // ctrl+0x24c
    std::int16_t alignTarget_ = 0;
    float player_[3] = {0, 0, 0};
    std::int16_t facing_ = 0;
    float base_[3] = {0, 0, 0};  // eye of the view
    float baseYaw_ = 0;          // heading of the zone, radians
    float zoom_ = 43.0f;         // 0x375ec8
    int zoomSound_ = 0;          // 0x375ec0
    float aim_[2] = {0, 0};      // 0x375edc pitch, 0x375ee0 yaw (radians)
    float shown_[2] = {0, 0};    // the smoothed orientation
    int state_ = 0;              // +0x3c
    float lockProgress_ = 0;     // +0x28
    int lockObject_ = -1;        // +0x2c
    float scanProgress_ = 0;     // +0x30
    int target_ = -1;            // +0x48
    int current_ = -1;           // +0x44
    int scanning_ = -1;          // +0x40
    std::set<std::uint32_t> started_;  // loops started and not stopped, for runs without a sound player
};

}
