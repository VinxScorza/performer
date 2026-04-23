#include "KnobPad16Controller.h"

#include "ui/ControllerManager.h"

#include "core/math/Math.h"

#include <algorithm>

KnobPad16Controller::KnobPad16Controller(ControllerManager &manager, Model &model, Engine &engine, Profile profile) :
    Controller(manager, model, engine),
    _profile(profile),
    _profileMapping(buildProfileMapping(profile))
{
    _padLedCache.fill(-1);
    _model.setKnobPad16Armed(false);
}

KnobPad16Controller::~KnobPad16Controller() {
    clearFeedback();
    if (_modeActive) {
        exitMode();
    }
    _model.setKnobPad16Armed(false);
}

void KnobPad16Controller::update() {
    refreshMode();
    syncFeedback();
}

void KnobPad16Controller::uiPageChanged() {
    refreshMode();
}

void KnobPad16Controller::recvMidi(uint8_t, const MidiMessage &message) {
    refreshMode();

    if (!shouldEnableMode()) {
        return;
    }

    InputAction action;
    if (decodeControlAction(message, action) || decodePadAction(message, action)) {
        applyInputAction(action);
    }
}

bool KnobPad16Controller::shouldEnableMode() const {
    if (_manager.uiPageKind() != ControllerManager::UiPageKind::NoteSequenceEdit) {
        return false;
    }

    if (_model.project().selectedTrack().trackMode() != Track::TrackMode::Note) {
        return false;
    }

    // Keep controller mode active on NOTE tracks even when switching layer:
    // pads must still toggle gates in step view, while knobs remain NOTE-layer only.
    return true;
}

void KnobPad16Controller::refreshMode() {
    const bool enable = shouldEnableMode();
    const int selectedTrack = _model.project().selectedTrackIndex();

    if (!enable) {
        if (_modeActive) {
            exitMode();
        }
        resetComboState();
        return;
    }

    if (!_armed) {
        if (_modeActive) {
            exitMode();
        }
        return;
    }

    if (!_modeActive) {
        enterModeForTrack(selectedTrack);
        return;
    }

    if (_activeTrackIndex != selectedTrack) {
        enterModeForTrack(selectedTrack);
    }
}

bool KnobPad16Controller::isInControllerChannel(const MidiMessage &message) const {
    return _profileMapping.channel < 0 || message.channel() == uint8_t(_profileMapping.channel);
}

bool KnobPad16Controller::decodeControlAction(const MidiMessage &message, InputAction &action) {
    if (!message.isControlChange() || !isInControllerChannel(message)) {
        return false;
    }

    const uint8_t cc = message.controlNumber();
    const uint8_t value = message.controlValue();

    const bool isPrevCc = _profileMapping.prevSectionCc >= 0 && cc == uint8_t(_profileMapping.prevSectionCc);
    const bool isNextCc = _profileMapping.nextSectionCc >= 0 && cc == uint8_t(_profileMapping.nextSectionCc);

    if (isPrevCc || isNextCc) {
        const bool pressed = value > 0;

        bool &pressedState = isPrevCc ? _prevPressed : _nextPressed;
        bool &tapCandidate = isPrevCc ? _prevTapCandidate : _nextTapCandidate;

        if (pressed == pressedState) {
            return false;
        }

        pressedState = pressed;

        if (_armed && supportsPadFunctionFeedback()) {
            // Keep these lit while armed, but blink OFF while physically pressed.
            sendSectionLed(isPrevCc, !pressed, true);
        }

        if (pressed) {
            tapCandidate = true;
            if (_prevPressed && _nextPressed && !_comboTriggered) {
                _comboTriggered = true;
                action.kind = InputActionKind::ToggleArmed;
                return true;
            }
            return false;
        }

        const bool shouldMoveSection = tapCandidate && !_comboTriggered;
        tapCandidate = false;

        if (!_prevPressed && !_nextPressed) {
            _comboTriggered = false;
        }

        if (shouldMoveSection && _modeActive) {
            action.kind = InputActionKind::MoveSection;
            action.value = isPrevCc ? -1 : 1;
            return true;
        }

        return false;
    }

    const int slot = findKnobSlot(cc);
    if (slot < 0 || slot >= 16) {
        return false;
    }

    action.slot = slot;

    if (_profileMapping.knobMode == KnobMode::Absolute) {
        action.kind = InputActionKind::SetNoteAbsolute;
        action.value = value;
        return true;
    }

    const int delta = decodeRelativeDelta(value);
    if (delta == 0) {
        return false;
    }

    action.kind = InputActionKind::AdjustNoteRelative;
    action.value = delta;
    return true;
}

bool KnobPad16Controller::decodePadAction(const MidiMessage &message, InputAction &action) {
    if (!message.isNoteOn() || message.velocity() == 0 || !isInControllerChannel(message)) {
        return false;
    }

    const int slot = findPadSlot(message.note());
    if (slot < 0 || slot >= 16) {
        return false;
    }

    action.kind = InputActionKind::ToggleGate;
    action.slot = slot;
    return true;
}

void KnobPad16Controller::applyInputAction(const InputAction &action) {
    switch (action.kind) {
    case InputActionKind::ToggleArmed:
        _armed = !_armed;
        _model.setKnobPad16Armed(_armed);
        if (!_armed) {
            if (_modeActive) {
                exitMode();
            }
            clearFeedback();
            return;
        }
        if (shouldEnableMode()) {
            const int selectedTrack = _model.project().selectedTrackIndex();
            if (!_modeActive) {
                enterModeForTrack(selectedTrack);
            } else if (_activeTrackIndex != selectedTrack) {
                enterModeForTrack(selectedTrack);
            }
        }
        syncFeedback(true);
        return;
    case InputActionKind::MoveSection:
        if (_modeActive) {
            moveSection(action.value < 0 ? -1 : 1);
            syncFeedback(true);
        }
        return;
    case InputActionKind::SetNoteAbsolute:
        if (_modeActive && _model.project().selectedNoteSequenceLayer() == NoteSequence::Layer::Note) {
            applyKnobAbsolute(action.slot, uint8_t(clamp(action.value, 0, 127)));
            syncFeedback();
        }
        return;
    case InputActionKind::AdjustNoteRelative:
        if (_modeActive && _model.project().selectedNoteSequenceLayer() == NoteSequence::Layer::Note) {
            applyKnobRelative(action.slot, action.value);
            syncFeedback();
        }
        return;
    case InputActionKind::ToggleGate:
        if (_modeActive) {
            applyPadToggle(action.slot);
            syncFeedback();
        }
        return;
    case InputActionKind::None:
        return;
    }
}

void KnobPad16Controller::resetComboState() {
    _prevPressed = false;
    _nextPressed = false;
    _prevTapCandidate = false;
    _nextTapCandidate = false;
    _comboTriggered = false;
}

void KnobPad16Controller::enterModeForTrack(int trackIndex) {
    _modeActive = true;
    _activeTrackIndex = trackIndex;

    ensureTrackSnapshot(trackIndex);
    applyVisibleWindowLoop();
    syncFeedback(true);
}

void KnobPad16Controller::exitMode() {
    restoreTrackLoopSnapshots();
    _modeActive = false;
    _activeTrackIndex = -1;
    clearFeedback();
}

void KnobPad16Controller::restoreTrackLoopSnapshots() {
    const int selectedPattern = _model.project().selectedPatternIndex();

    for (int trackIndex = 0; trackIndex < CONFIG_TRACK_COUNT; ++trackIndex) {
        auto &snapshot = _trackLoopSnapshots[trackIndex];
        if (!snapshot.valid) {
            continue;
        }

        auto &track = _model.project().track(trackIndex);
        if (track.trackMode() != Track::TrackMode::Note) {
            snapshot.valid = false;
            continue;
        }

        auto &sequence = track.noteTrack().sequence(selectedPattern);
        sequence.setLastStep(snapshot.lastStep);
        sequence.setFirstStep(snapshot.firstStep);

        snapshot.valid = false;
    }
}

void KnobPad16Controller::ensureTrackSnapshot(int trackIndex) {
    if (trackIndex < 0 || trackIndex >= CONFIG_TRACK_COUNT) {
        return;
    }

    auto &snapshot = _trackLoopSnapshots[trackIndex];
    if (snapshot.valid) {
        return;
    }

    auto &track = _model.project().track(trackIndex);
    if (track.trackMode() != Track::TrackMode::Note) {
        return;
    }

    auto &sequence = _model.project().selectedNoteSequence();
    snapshot.valid = true;
    snapshot.firstStep = sequence.firstStep();
    snapshot.lastStep = sequence.lastStep();
}

void KnobPad16Controller::applyVisibleWindowLoop() {
    auto &sequence = _model.project().selectedNoteSequence();
    const int windowFirst = clamp(sequence.section() * 16, 0, CONFIG_STEP_COUNT - 1);
    const int windowLast = std::min(windowFirst + 15, CONFIG_STEP_COUNT - 1);

    sequence.setLastStep(windowLast);
    sequence.setFirstStep(windowFirst);
}

void KnobPad16Controller::moveSection(int direction) {
    auto &sequence = _model.project().selectedNoteSequence();
    constexpr int sectionCount = CONFIG_STEP_COUNT / 16;

    int section = sequence.section();
    section = (section + sectionCount + direction) % sectionCount;
    sequence.setSecion(section);
    applyVisibleWindowLoop();
}

bool KnobPad16Controller::supportsPadFunctionFeedback() const {
    switch (_profile) {
    case Profile::LaunchControlXL:
    case Profile::BeatStepPro:
        return _profileMapping.channel >= 0;
    default:
        return false;
    }
}

void KnobPad16Controller::syncFeedback(bool force) {
    if (!supportsPadFunctionFeedback()) {
        return;
    }

    if (!_armed || !_modeActive || !shouldEnableMode()) {
        clearFeedback();
        return;
    }

    auto &sequence = _model.project().selectedNoteSequence();
    const int firstStep = sequence.section() * 16;
    const int playheadSlot = visiblePlayheadSlot(sequence);

    for (int slot = 0; slot < 16; ++slot) {
        const int stepIndex = firstStep + slot;
        if (stepIndex < 0 || stepIndex >= CONFIG_STEP_COUNT) {
            sendPadLed(slot, 0, force);
            continue;
        }

        const auto &step = sequence.step(stepIndex);
        const bool isPlayhead = slot == playheadSlot;
        const int value = isPlayhead ? int(_profileMapping.padPlayheadValue) : (step.gate() ? int(_profileMapping.padOnValue) : 0);
        sendPadLed(slot, value, force);
    }

    sendSectionLed(true, !_prevPressed, force);
    sendSectionLed(false, !_nextPressed, force);
}

void KnobPad16Controller::clearFeedback() {
    if (!supportsPadFunctionFeedback()) {
        return;
    }

    for (int slot = 0; slot < 16; ++slot) {
        sendPadLed(slot, 0, false);
    }
    sendSectionLed(true, false, false);
    sendSectionLed(false, false, false);
}

int KnobPad16Controller::visiblePlayheadSlot(const NoteSequence &sequence) const {
    const auto &trackEngine = _engine.selectedTrackEngine().as<NoteTrackEngine>();
    if (!trackEngine.isActiveSequence(sequence)) {
        return -1;
    }

    const int currentStep = trackEngine.currentStep();
    const int firstStep = sequence.section() * 16;
    const int lastStep = firstStep + 15;
    if (currentStep < firstStep || currentStep > lastStep) {
        return -1;
    }

    return currentStep - firstStep;
}

void KnobPad16Controller::sendPadLed(int slot, int value, bool force) {
    if (slot < 0 || slot >= 16 || !supportsPadFunctionFeedback()) {
        return;
    }

    static constexpr uint8_t padNotes[16] = {
        41, 42, 43, 44,
        57, 58, 59, 60,
        73, 74, 75, 76,
        89, 90, 91, 92,
    };
    value = clamp(value, 0, 127);
    if (!force && _padLedCache[slot] == value) {
        return;
    }

    sendMidi(0, MidiMessage::makeNoteOn(uint8_t(_profileMapping.channel), padNotes[slot], uint8_t(value)));
    _padLedCache[slot] = value;
}

void KnobPad16Controller::sendSectionLed(bool prev, bool on, bool force) {
    if (!supportsPadFunctionFeedback()) {
        return;
    }

    const int ccValue = prev ? _profileMapping.prevSectionCc : _profileMapping.nextSectionCc;
    if (ccValue < 0) {
        return;
    }
    const uint8_t cc = uint8_t(ccValue);
    const int value = on ? 127 : 0;
    int &cache = prev ? _prevSectionLedCache : _nextSectionLedCache;
    if (!force && cache == value) {
        return;
    }

    sendMidi(0, MidiMessage::makeControlChange(uint8_t(_profileMapping.channel), cc, uint8_t(value)));
    cache = value;
}

int KnobPad16Controller::findKnobSlot(uint8_t cc) const {
    switch (_profile) {
    case Profile::LaunchControlXL:
    case Profile::BeatStepPro:
        // LCXL-compatible PERFORMERstep16 mapping:
        // CC 13..20 and 29..36 map to step 1..16 encoders.
        if (cc >= 13 && cc <= 20) { return cc - 13; }
        if (cc >= 29 && cc <= 36) { return 8 + (cc - 29); }
        return -1;
    }

    return -1;
}

int KnobPad16Controller::findPadSlot(uint8_t note) const {
    switch (_profile) {
    case Profile::LaunchControlXL:
    case Profile::BeatStepPro:
        // LCXL-compatible PERFORMERstep16 mapping:
        // 41..44, 57..60, 73..76, 89..92 -> step slots 0..15
        if (note >= 41 && note <= 44) { return note - 41; }
        if (note >= 57 && note <= 60) { return 4 + (note - 57); }
        if (note >= 73 && note <= 76) { return 8 + (note - 73); }
        if (note >= 89 && note <= 92) { return 12 + (note - 89); }
        return -1;
    }

    return -1;
}

int KnobPad16Controller::decodeRelativeDelta(uint8_t value) const {
    if (value == 64) {
        return 0;
    }
    if (value < 64) {
        return value;
    }
    return int(value) - 128;
}

int KnobPad16Controller::mapAbsoluteToNoteValue(const NoteSequence &sequence, uint8_t value) const {
    constexpr int semitoneMin = -12;
    constexpr int semitoneMax = 12;
    constexpr int semitoneSpan = semitoneMax - semitoneMin;

    const int semitone = semitoneMin + (int(value) * semitoneSpan + 63) / 127;
    const auto &scale = sequence.selectedScale(_model.project().scale());
    const int note = scale.noteFromVolts(semitone * (1.f / 12.f));

    return clampToKnobNoteRange(sequence, note);
}

int KnobPad16Controller::clampToKnobNoteRange(const NoteSequence &sequence, int note) const {
    const auto &scale = sequence.selectedScale(_model.project().scale());

    int noteMin = scale.noteFromVolts(-1.f);
    int noteMax = scale.noteFromVolts(1.f);
    if (noteMin > noteMax) {
        std::swap(noteMin, noteMax);
    }

    noteMin = NoteSequence::Note::clamp(noteMin);
    noteMax = NoteSequence::Note::clamp(noteMax);

    return clamp(note, noteMin, noteMax);
}

void KnobPad16Controller::applyKnobAbsolute(int slot, uint8_t value) {
    auto &sequence = _model.project().selectedNoteSequence();
    const int stepIndex = sequence.section() * 16 + slot;
    if (stepIndex < 0 || stepIndex >= CONFIG_STEP_COUNT) {
        return;
    }

    auto &step = sequence.step(stepIndex);
    step.setNote(mapAbsoluteToNoteValue(sequence, value));
}

void KnobPad16Controller::applyKnobRelative(int slot, int delta) {
    auto &sequence = _model.project().selectedNoteSequence();
    const int stepIndex = sequence.section() * 16 + slot;
    if (stepIndex < 0 || stepIndex >= CONFIG_STEP_COUNT) {
        return;
    }

    if (delta == 0) {
        return;
    }

    auto &step = sequence.step(stepIndex);
    step.setNote(clampToKnobNoteRange(sequence, step.note() + delta));
}

void KnobPad16Controller::applyPadToggle(int slot) {
    auto &sequence = _model.project().selectedNoteSequence();
    const int stepIndex = sequence.section() * 16 + slot;
    if (stepIndex < 0 || stepIndex >= CONFIG_STEP_COUNT) {
        return;
    }

    sequence.step(stepIndex).toggleGate();
}

KnobPad16Controller::ProfileMapping KnobPad16Controller::buildProfileMapping(Profile profile) const {
    ProfileMapping mapping;

    switch (profile) {
    case Profile::LaunchControlXL:
        // LaunchControl XL Factory Preset #1.
        mapping.knobMode = KnobMode::Absolute;
        mapping.prevSectionCc = 106;
        mapping.nextSectionCc = 107;
        mapping.channel = 8; // MIDI channel 9 (0-based index)
        mapping.padOnValue = 63; // amber-like ON
        mapping.padPlayheadValue = 15; // red-like playhead
        return mapping;
    case Profile::BeatStepPro:
        // BSP template aligned to LCXL Factory #1 map:
        // - Absolute knobs
        // - CC 106 / 107 for prev/next section (Ptn1 / Ptn2)
        mapping.knobMode = KnobMode::Absolute;
        mapping.prevSectionCc = 106;
        mapping.nextSectionCc = 107;
        mapping.channel = 8; // keep same logical channel as LCXL template
        mapping.padOnValue = 127; // stronger ON level for BSP pads
        mapping.padPlayheadValue = 15; // fallback distinct level for playhead
        return mapping;
    }

    return mapping;
}
