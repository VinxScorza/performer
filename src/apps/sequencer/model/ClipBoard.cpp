#include "ClipBoard.h"

#include "Model.h"
#include "ModelUtils.h"

ClipBoard::ClipBoard(Project &project) :
    _project(project)
{
    clear();
}

void ClipBoard::clear() {
    _type = Type::None;
}

void ClipBoard::copyTrack(const Track &track) {
    _type = Type::Track;
    _container.as<Track>().setTrackMode(track.trackMode());
    _container.as<Track>() = track;
}

void ClipBoard::copyNoteSequence(const NoteSequence &noteSequence) {
    _type = Type::NoteSequence;
    _container.as<NoteSequence>().setName(noteSequence.name());
    _container.as<NoteSequence>() = noteSequence;
}

void ClipBoard::copyNoteSequenceSteps(const NoteSequence &noteSequence, const SelectedSteps &selectedSteps) {
    _type = Type::NoteSequenceSteps;
    auto &noteSequenceSteps = _container.as<NoteSequenceSteps>();
    noteSequenceSteps.sequence = noteSequence;
    noteSequenceSteps.selected = selectedSteps;
}

void ClipBoard::copyNoteSequenceSteps(NoteSequence &noteSequence, const SelectedSteps &selectedSteps) {
    _type = Type::NoteSequenceSteps;
    auto &noteSequenceSteps = _container.as<NoteSequenceSteps>();
    noteSequenceSteps.sequence = noteSequence;
    noteSequenceSteps.selected = selectedSteps;
}

void ClipBoard::copyCurveSequence(const CurveSequence &curveSequence) {
    _type = Type::CurveSequence;
    _container.as<CurveSequence>().setName(curveSequence.name());
    _container.as<CurveSequence>() = curveSequence;
}

void ClipBoard::copyCurveSequenceSteps(const CurveSequence &curveSequence, const SelectedSteps &selectedSteps) {
    _type = Type::CurveSequenceSteps;
    auto &curveSequenceSteps = _container.as<CurveSequenceSteps>();
    curveSequenceSteps.sequence = curveSequence;
    curveSequenceSteps.selected = selectedSteps;
}

void ClipBoard::copyStochasticSequence(const StochasticSequence &sequence) {
    _type = Type::StochasticSequence;
    _container.as<StochasticSequence>() = sequence;
}

void ClipBoard::copyStochasticSequenceSteps(const StochasticSequence &sequence, const SelectedSteps &selectedSteps) {
    _type = Type::StochasticSequenceSteps;
    auto &stochasticSequenceSteps = _container.as<StochasticSequenceSteps>();
    stochasticSequenceSteps.sequence = sequence;
    stochasticSequenceSteps.selected = selectedSteps;
}

void ClipBoard::copyLogicSequence(const LogicSequence &sequence) {
    _type = Type::LogicSequence;
    _container.as<LogicSequence>().setName(sequence.name());
    _container.as<LogicSequence>() = sequence;
}

void ClipBoard::copyLogicSequenceSteps(const LogicSequence &sequence, const SelectedSteps &selectedSteps) {
    _type = Type::LogicSequenceSteps;
    auto &logicSequenceSteps = _container.as<LogicSequenceSteps>();
    logicSequenceSteps.sequence = sequence;
    logicSequenceSteps.selected = selectedSteps;
}

void ClipBoard::copyArpSequence(const ArpSequence &sequence) {
    _type = Type::ArpSequence;
    _container.as<ArpSequence>().setName(sequence.name());
    _container.as<ArpSequence>() = sequence;
}

void ClipBoard::copyArpSequenceSteps(const ArpSequence &sequence, const SelectedSteps &selectedSteps) {
    _type = Type::ArpSequenceSteps;
    auto &arpSequenceSteps = _container.as<ArpSequenceSteps>();
    arpSequenceSteps.sequence = sequence;
    arpSequenceSteps.selected = selectedSteps;
}

void ClipBoard::copyPattern(int patternIndex) {
    _type = Type::Pattern;
    auto &pattern = _container.as<Pattern>();
    for (int trackIndex = 0; trackIndex < CONFIG_TRACK_COUNT; ++trackIndex) {
        const auto &track = _project.track(trackIndex);
        pattern.sequences[trackIndex].trackMode = track.trackMode();
        switch (track.trackMode()) {
        case Track::TrackMode::Note:
            pattern.sequences[trackIndex].data.note = track.noteTrack().sequence(patternIndex);
            break;
        case Track::TrackMode::Curve:
            pattern.sequences[trackIndex].data.curve = track.curveTrack().sequence(patternIndex);
            break;
        case Track::TrackMode::Stochastic:
            pattern.sequences[trackIndex].data.stochastic = track.stochasticTrack().sequence(patternIndex);
            break;
        case Track::TrackMode::Logic:
            pattern.sequences[trackIndex].data.logic = track.logicTrack().sequence(patternIndex);
            break;
        case Track::TrackMode::Arp:
            pattern.sequences[trackIndex].data.arp = track.arpTrack().sequence(patternIndex);
            break;
        default:
            break;
        }
    }
}

void ClipBoard::copyUserScale(const UserScale &userScale) {
    _type = Type::UserScale;
    _container.as<UserScale>() = userScale;
}

void ClipBoard::pasteTrack(Track &track) const {
    if (canPasteTrack()) {
        Model::ConfigLock lock;
        _project.setTrackMode(track.trackIndex(), _container.as<Track>().trackMode());
        track = _container.as<Track>();
    }
}

void ClipBoard::pasteNoteSequence(NoteSequence &noteSequence) const {
    if (canPasteNoteSequence()) {
        Model::WriteLock lock;
        noteSequence.setName(_container.as<NoteSequence>().name());
        noteSequence = _container.as<NoteSequence>();
    }
}

void ClipBoard::pasteNoteSequenceSteps(NoteSequence &noteSequence, const SelectedSteps &selectedSteps) const {
    if (canPasteNoteSequenceSteps() && !pasteNoteSequenceStepsRequiresDestination(selectedSteps)) {
        const auto &noteSequenceSteps = _container.as<NoteSequenceSteps>();
        ModelUtils::copySteps(noteSequenceSteps.sequence.steps(), noteSequenceSteps.selected, noteSequence.steps(), selectedSteps);
    }
}

void ClipBoard::pasteCurveSequence(CurveSequence &curveSequence) const {
    if (canPasteCurveSequence()) {
        Model::WriteLock lock;
        curveSequence.setName(_container.as<CurveSequence>().name());
        curveSequence = _container.as<CurveSequence>();
    }
}

void ClipBoard::pasteCurveSequenceSteps(CurveSequence &curveSequence, const SelectedSteps &selectedSteps) const {
    if (canPasteCurveSequenceSteps() && !pasteCurveSequenceStepsRequiresDestination(selectedSteps)) {
        const auto &curveSequenceSteps = _container.as<CurveSequenceSteps>();
        ModelUtils::copySteps(curveSequenceSteps.sequence.steps(), curveSequenceSteps.selected, curveSequence.steps(), selectedSteps);
    }
}


void ClipBoard::pasteStochasticSequence(StochasticSequence &sequence) const {
    if (canPasteStochasticSequence()) {
        Model::WriteLock lock;
        sequence = _container.as<StochasticSequence>();
    }
}

void ClipBoard::pasteStochasticSequenceSteps(StochasticSequence &sequence, const SelectedSteps &selectedSteps) const {
    if (canPasteStochasticSequenceSteps() && !pasteStochasticSequenceStepsRequiresDestination(selectedSteps)) {
        const auto &stochasticSequenceSteps = _container.as<StochasticSequenceSteps>();
        ModelUtils::copySteps(stochasticSequenceSteps.sequence.steps(), stochasticSequenceSteps.selected, sequence.steps(), selectedSteps);
    }
}

void ClipBoard::pasteLogicSequence(LogicSequence &sequence) const {
    if (canPasteLogicSequence()) {
        Model::WriteLock lock;
        sequence.setName(_container.as<LogicSequence>().name());
        sequence = _container.as<LogicSequence>();
    }
}

void ClipBoard::pasteLogicSequenceSteps(LogicSequence &sequence, const SelectedSteps &selectedSteps) const {
    if (canPasteLogicSequenceSteps() && !pasteLogicSequenceStepsRequiresDestination(selectedSteps)) {
        const auto &logicSequenceSteps = _container.as<LogicSequenceSteps>();
        ModelUtils::copySteps(logicSequenceSteps.sequence.steps(), logicSequenceSteps.selected, sequence.steps(), selectedSteps);
    }
}

void ClipBoard::pasteArpSequence(ArpSequence &sequence) const {
    if (canPasteArpSequence()) {
        Model::WriteLock lock;
        sequence = _container.as<ArpSequence>();
    }
}

void ClipBoard::pasteArpSequenceSteps(ArpSequence &sequence, const SelectedSteps &selectedSteps) const {
    if (canPasteArpSequenceSteps() && !pasteArpSequenceStepsRequiresDestination(selectedSteps)) {
        const auto &arpSequenceSteps = _container.as<ArpSequenceSteps>();
        ModelUtils::copySteps(arpSequenceSteps.sequence.steps(), arpSequenceSteps.selected, sequence.steps(), selectedSteps);
    }
}

void ClipBoard::pastePattern(int patternIndex) const {
    if (canPastePattern()) {
        Model::WriteLock lock;
        const auto &pattern = _container.as<Pattern>();
        for (int trackIndex = 0; trackIndex < CONFIG_TRACK_COUNT; ++trackIndex) {
            auto &track = _project.track(trackIndex);
            if (track.trackMode() == pattern.sequences[trackIndex].trackMode) {
                switch (track.trackMode()) {
                case Track::TrackMode::Note:
                    track.noteTrack().sequence(patternIndex) = pattern.sequences[trackIndex].data.note;
                    break;
                case Track::TrackMode::Curve:
                    track.curveTrack().sequence(patternIndex) = pattern.sequences[trackIndex].data.curve;
                    break;
                case Track::TrackMode::Stochastic:
                    track.stochasticTrack().sequence(patternIndex) = pattern.sequences[trackIndex].data.stochastic;
                    break;
                case Track::TrackMode::Logic:
                    track.logicTrack().sequence(patternIndex) = pattern.sequences[trackIndex].data.logic;
                    break;
                case Track::TrackMode::Arp:
                    track.arpTrack().sequence(patternIndex) = pattern.sequences[trackIndex].data.arp;
                    break;
                default:
                    break;
                }
            }
        }
    }
}

void ClipBoard::pasteUserScale(UserScale &userScale) const {
    if (canPasteUserScale()) {
        userScale = _container.as<UserScale>();
    }
}

bool ClipBoard::canPasteTrack() const {
    return _type == Type::Track;
}

bool ClipBoard::canPasteNoteSequence() const {
    return _type == Type::NoteSequence;
}

bool ClipBoard::canPasteNoteSequenceSteps() const {
    return _type == Type::NoteSequenceSteps;
}

bool ClipBoard::pasteNoteSequenceStepsRequiresDestination(const SelectedSteps &selectedSteps) const {
    return canPasteNoteSequenceSteps() && _container.as<NoteSequenceSteps>().selected.any() && selectedSteps.none();
}

bool ClipBoard::canPasteCurveSequence() const {
    return _type == Type::CurveSequence;
}

bool ClipBoard::canPasteCurveSequenceSteps() const {
    return _type == Type::CurveSequenceSteps;
}

bool ClipBoard::pasteCurveSequenceStepsRequiresDestination(const SelectedSteps &selectedSteps) const {
    return canPasteCurveSequenceSteps() && _container.as<CurveSequenceSteps>().selected.any() && selectedSteps.none();
}

bool ClipBoard::canPasteStochasticSequence() const {
    return _type == Type::StochasticSequence;
}

bool ClipBoard::canPasteStochasticSequenceSteps() const {
    return _type == Type::StochasticSequenceSteps;
}

bool ClipBoard::pasteStochasticSequenceStepsRequiresDestination(const SelectedSteps &selectedSteps) const {
    return canPasteStochasticSequenceSteps() && _container.as<StochasticSequenceSteps>().selected.any() && selectedSteps.none();
}

bool ClipBoard::canPasteLogicSequence() const {
    return _type == Type::LogicSequence;
}

bool ClipBoard::canPasteLogicSequenceSteps() const {
    return _type == Type::LogicSequenceSteps;
}

bool ClipBoard::pasteLogicSequenceStepsRequiresDestination(const SelectedSteps &selectedSteps) const {
    return canPasteLogicSequenceSteps() && _container.as<LogicSequenceSteps>().selected.any() && selectedSteps.none();
}

bool ClipBoard::canPasteArpSequence() const {
    return _type == Type::ArpSequence;
}

bool ClipBoard::canPasteArpSequenceSteps() const {
    return _type == Type::ArpSequenceSteps;
}

bool ClipBoard::pasteArpSequenceStepsRequiresDestination(const SelectedSteps &selectedSteps) const {
    return canPasteArpSequenceSteps() && _container.as<ArpSequenceSteps>().selected.any() && selectedSteps.none();
}

bool ClipBoard::canPastePattern() const {
    return _type == Type::Pattern;
}

bool ClipBoard::canPasteUserScale() const {
    return _type == Type::UserScale;
}
