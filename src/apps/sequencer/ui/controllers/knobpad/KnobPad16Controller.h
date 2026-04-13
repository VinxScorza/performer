#pragma once

#include "ui/Controller.h"

#include "Config.h"
#include "model/NoteSequence.h"

#include <array>
#include <cstdint>

class KnobPad16Controller : public Controller {
public:
    enum class Profile : uint8_t {
        LaunchControlXL,
        BeatStepPro,
    };

    KnobPad16Controller(ControllerManager &manager, Model &model, Engine &engine, Profile profile);
    virtual ~KnobPad16Controller();

    virtual void update() override;
    virtual void recvMidi(uint8_t cable, const MidiMessage &message) override;
    virtual void uiPageChanged() override;

private:
    enum class KnobMode : uint8_t {
        Absolute,
        RelativeTwosComplement,
    };

    enum class InputActionKind : uint8_t {
        None,
        ToggleArmed,
        MoveSection,
        SetNoteAbsolute,
        AdjustNoteRelative,
        ToggleGate,
    };

    struct InputAction {
        InputActionKind kind = InputActionKind::None;
        int slot = -1;
        int value = 0;
    };

    struct ProfileMapping {
        KnobMode knobMode = KnobMode::Absolute;
        int prevSectionCc = -1;
        int nextSectionCc = -1;
        int channel = -1;
        uint8_t padOnValue = 63;
    };

    struct LoopSnapshot {
        bool valid = false;
        int firstStep = 0;
        int lastStep = CONFIG_STEP_COUNT - 1;
    };

    bool shouldEnableMode() const;
    void refreshMode();

    void enterModeForTrack(int trackIndex);
    void exitMode();
    void restoreTrackLoopSnapshots();
    void ensureTrackSnapshot(int trackIndex);
    void applyVisibleWindowLoop();
    void moveSection(int direction);

    int findKnobSlot(uint8_t cc) const;
    int findPadSlot(uint8_t note) const;
    int decodeRelativeDelta(uint8_t value) const;
    bool isInControllerChannel(const MidiMessage &message) const;
    bool decodeControlAction(const MidiMessage &message, InputAction &action);
    bool decodePadAction(const MidiMessage &message, InputAction &action);
    void applyInputAction(const InputAction &action);
    void resetComboState();
    int mapAbsoluteToNoteValue(const NoteSequence &sequence, uint8_t value) const;
    int clampToKnobNoteRange(const NoteSequence &sequence, int note) const;

    void applyKnobAbsolute(int slot, uint8_t value);
    void applyKnobRelative(int slot, int delta);
    void applyPadToggle(int slot);
    bool supportsPadFunctionFeedback() const;
    void syncFeedback(bool force = false);
    void clearFeedback();
    void sendPadLed(int slot, bool on, bool force = false);
    void sendSectionLed(bool prev, bool on, bool force = false);

    ProfileMapping buildProfileMapping(Profile profile) const;

    Profile _profile;
    ProfileMapping _profileMapping;
    bool _armed = false;
    bool _modeActive = false;
    int _activeTrackIndex = -1;
    bool _prevPressed = false;
    bool _nextPressed = false;
    bool _prevTapCandidate = false;
    bool _nextTapCandidate = false;
    bool _comboTriggered = false;
    std::array<LoopSnapshot, CONFIG_TRACK_COUNT> _trackLoopSnapshots;
    std::array<int, 16> _padLedCache;
    int _prevSectionLedCache = -1;
    int _nextSectionLedCache = -1;
};
