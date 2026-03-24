#pragma once

#include "Config.h"
#include "Observable.h"
#include "Scale.h"
#include "Types.h"
#include "TimeSignature.h"
#include "ClockSetup.h"
#include "Track.h"
#include "Song.h"
#include "PlayState.h"
#include "UserScale.h"
#include "Routing.h"
#include "MidiOutput.h"
#include "Serialize.h"
#include "FileDefs.h"

#include "core/math/Math.h"
#include "core/utils/StringBuilder.h"
#include "core/utils/StringUtils.h"
#include <algorithm>
#include <string>
#include <iostream>

class Project {
public:
    //----------------------------------------
    // Types
    //----------------------------------------

    static constexpr size_t NameLength = FileHeader::NameLength;

    typedef std::array<Track, CONFIG_TRACK_COUNT> TrackArray;
    typedef std::array<uint8_t, CONFIG_CHANNEL_COUNT> CvOutputTrackArray;
    typedef std::array<uint8_t, CONFIG_CHANNEL_COUNT> GateOutputArray;

    Project();

    //----------------------------------------
    // Properties
    //----------------------------------------

    // slot

    int slot() const { return _slot; }
    void setSlot(int slot) {
        _slot = slot;
    }
    bool slotAssigned() const {
        return _slot != uint8_t(-1);
    }

    // name

    const char *name() const { return _name; }
    void setName(const char *name) {
        StringUtils::copy(_name, name, sizeof(_name));
    }

    // autoLoaded

    bool autoLoaded() const { return _autoLoaded != 0; }
    void setAutoLoaded(bool autoLoaded) { _autoLoaded = autoLoaded ? 1 : 0; }

    // tempo

    float tempo() const { return _tempo.get(isRouted(Routing::Target::Tempo)); }
    void setTempo(float tempo, bool routed = false) {
        _tempo.set(clamp(tempo, 1.f, 1000.f), routed);
        _orinalTempo = _tempo.base;
    }

    float originalTempo() { return _orinalTempo;}

    void editTempo(int value, bool shift) {
        if (!isRouted(Routing::Target::Tempo)) {
            setTempo(tempo() + value * (shift ? 0.1f : 1.f));
        }
    }

    void printTempo(StringBuilder &str) const {
        printRouted(str, Routing::Target::Tempo);
        str("%.1f", tempo());
    }

    // swing

    int swing() const { return _swing.get(isRouted(Routing::Target::Swing)); }
    void setSwing(int swing, bool routed = false) {
        _swing.set(clamp(swing, 50, 75), routed);
    }

    void editSwing(int value, bool shift) {
        if (!isRouted(Routing::Target::Swing)) {
            setSwing(ModelUtils::adjustedByStep(swing(), value, 5, !shift));
        }
    }

    void printSwing(StringBuilder &str) const {
        printRouted(str, Routing::Target::Swing);
        int swingAmount = ((swing() - 50) * 99 + 12) / 25;
        str("%d%%", swingAmount);
    }

    // timeSignature

    TimeSignature timeSignature() const { return _timeSignature; }
    void setTimeSignature(TimeSignature timeSignature) {
        _timeSignature = timeSignature;
    }

    void editTimeSignature(int value, bool shift) {
        _timeSignature.edit(value, shift);
    }

    void printTimeSignature(StringBuilder &str) const {
        _timeSignature.print(str);
    }

    // syncMeasure

    int syncMeasure() const { return _syncMeasure; }
    void setSyncMeasure(int syncMeasure) {
        _syncMeasure = clamp(syncMeasure, 1, 128);
    }

    void editSyncMeasure(int value, bool shift) {
        setSyncMeasure(ModelUtils::adjustedByPowerOfTwo(syncMeasure(), value, shift));
    }

    void printSyncMeasure(StringBuilder &str) const {
        str("%d %s", syncMeasure(), syncMeasure() > 1 ? "bars" : "bar");
    }

    // scale

    int scale() const { return _scale; }
    void setScale(int s) {
        int previousScaleIndex = clamp(scale(), 0, Scale::Count - 1);
        int newScaleIndex = clamp(s, 0, Scale::Count - 1);

        auto &pScale = Scale::get(previousScaleIndex);
        auto &aScale = Scale::get(newScaleIndex);

        _scale = newScaleIndex;

        if (pScale == aScale) {
            return;
        }

        if (aScale.isChromatic() && pScale.isChromatic()) {

            for (int trackIndex = 0; trackIndex < 8; ++trackIndex) {    
                    auto &t = track(trackIndex);
                    switch (t.trackMode()) {
                        case Track::TrackMode::Note: {
                            for (auto &seq : t.noteTrack().sequences()) {
                                if (seq.scale()==-1) {
                                    for (int i = 0; i < 64; ++i) {
                                        auto pStep = seq.step(i);

                                        int rN = pScale.noteIndex(pStep.note(), rootNote());
                                        if (rN > 0) {
                                            if (aScale.isNotePresent(rN)) {
                                                int pNoteIndex = aScale.getNoteIndex(rN);
                                                seq.step(i).setNote(pNoteIndex);
                                            } else {
                                                // search nearest note
                                                while (!aScale.isNotePresent(rN)) {
                                                    rN--;
                                                }
                                                int pNoteIndex = aScale.getNoteIndex(rN);
                                                seq.step(i).setNote(pNoteIndex);
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        default:
                            break;
                    }                    
                }
        }
        
    }

    void editScale(int value, bool shift) {
        setScale(value);
    }

    void printScale(StringBuilder &str) const {
        str(Scale::name(scale()));
    }

    const Scale &selectedScale() const {
        return Scale::get(scale());
    }

    // rootNote

    int rootNote() const { return _rootNote; }
    void setRootNote(int rootNote) {
        _rootNote = clamp(rootNote, 0, 11);
    }

    void editRootNote(int value, bool shift) {
        setRootNote(rootNote() + value);
    }

    void printRootNote(StringBuilder &str) const {
        Types::printNote(str, _rootNote);
    }

    // monitorMode

    Types::MonitorMode monitorMode() const { return _monitorMode; }
    void setMonitorMode(Types::MonitorMode monitorMode) {
        _monitorMode = ModelUtils::clampedEnum(monitorMode);
    }

    void editMonitorMode(int value, bool shift) {
        _monitorMode = ModelUtils::adjustedEnum(_monitorMode, value);
    }

    void printMonitorMode(StringBuilder &str) const {
        str(Types::monitorModeName(_monitorMode));
    }

    // recordMode

    Types::RecordMode recordMode() const { return _recordMode; }
    void setRecordMode(Types::RecordMode recordMode) {
        _recordMode = ModelUtils::clampedEnum(recordMode);
    }

    void editRecordMode(int value, bool shift) {
        _recordMode = ModelUtils::adjustedEnum(_recordMode, value);
    }

    void printRecordMode(StringBuilder &str) const {
        str(Types::recordModeName(_recordMode));
    }

    // midiInput

    void editMidiInput(int value, bool shift) {
        if (_midiInputMode == Types::MidiInputMode::Source) {
            if (value < 0 && _midiInputSource.isFirst()) {
                _midiInputMode = ModelUtils::adjustedEnum(_midiInputMode, value);
            } else {
                _midiInputSource.edit(value, shift);
            }
        } else {
            _midiInputMode = ModelUtils::adjustedEnum(_midiInputMode, value);
        }
    }

    void printMidiInput(StringBuilder &str) const {
        switch (_midiInputMode) {
        case Types::MidiInputMode::Off:     str("Off"); break;
        case Types::MidiInputMode::All:     str("All"); break;
        case Types::MidiInputMode::Source:  _midiInputSource.print(str); break;
        case Types::MidiInputMode::Last:    break;
        }
    }

    Types::MidiInputMode midiInputMode() const { return _midiInputMode; }
    void setMidiInputMode(Types::MidiInputMode midiInputMode) {
        _midiInputMode = ModelUtils::clampedEnum(midiInputMode);
    }

    const MidiSourceConfig &midiInputSource() const { return _midiInputSource; }
          MidiSourceConfig &midiInputSource()       { return _midiInputSource; }

    // midiIntegrationMode

    void editMidiIntegrationMode(int value, bool shift) {
        _midiIntegrationMode = ModelUtils::adjustedEnum(_midiIntegrationMode, value);
    }

    void printMidiIntegrationMode(StringBuilder &str) const {
        str(Types::midiIntegrationModeName(_midiIntegrationMode));
    }

    void setMidiIntegrationMode(Types::MidiIntegrationMode midiIntegrationMode) {
        _midiIntegrationMode = midiIntegrationMode;
    }

    bool midiIntegrationProgramChangesEnabled() const {
        return _midiIntegrationMode == Types::MidiIntegrationMode::ProgramChanges
               || _midiIntegrationMode == Types::MidiIntegrationMode::Malekko;
    }

    bool midiIntegrationMalekkoEnabled() const {
        return _midiIntegrationMode == Types::MidiIntegrationMode::Malekko;
    }

    // midiProgramOffset

    void editMidiProgramOffset(int value, bool shift) {
        _midiProgramOffset += value;
    }

    void printMidiProgramOffset(StringBuilder &str) const {
        str("%d", _midiProgramOffset);
    }

    void setMidiProgramOffset(int midiProgramOffset) {
        _midiProgramOffset = midiProgramOffset;
    }

    int midiProgramOffset() {
        return _midiProgramOffset;
    }
    // cvGateInput

    Types::CvGateInput cvGateInput() const { return _cvGateInput; }
    void setCvGateInput(Types::CvGateInput cvGateInput) {
        _cvGateInput = ModelUtils::clampedEnum(cvGateInput);
    }

    void editCvGateInput(int value, bool shift) {
        _cvGateInput = ModelUtils::adjustedEnum(_cvGateInput, value);
    }

    void printCvGateInput(StringBuilder &str) const {
        str(Types::cvGateInputName(_cvGateInput));
    }

    // curveCvInput

    Types::CurveCvInput curveCvInput() const { return _curveCvInput; }
    void setCurveCvInput(Types::CurveCvInput curveCvInput) {
        _curveCvInput = ModelUtils::clampedEnum(curveCvInput);
    }

    void editCurveCvInput(int value, bool shift) {
        _curveCvInput = ModelUtils::adjustedEnum(_curveCvInput, value);
    }

    void printCurveCvInput(StringBuilder &str) const {
        str(Types::curveCvInput(_curveCvInput));
    }

    // curveMidiInput

    // clockSetup

    const ClockSetup &clockSetup() const { return _clockSetup; }
          ClockSetup &clockSetup()       { return _clockSetup; }

    // tracks

    const TrackArray &tracks() const { return _tracks; }
          TrackArray &tracks()       { return _tracks; }

    const Track &track(int index) const { return _tracks[index]; }
          Track &track(int index)       { return _tracks[index]; }

    // cvOutputTrack

    const CvOutputTrackArray &cvOutputTracks() const { return _cvOutputTracks; }
          CvOutputTrackArray &cvOutputTracks()       { return _cvOutputTracks; }

    int cvOutputTrack(int index) const { return _cvOutputTracks[index]; }
    void setCvOutputTrack(int index, int trackIndex) { _cvOutputTracks[index] = clamp(trackIndex, 0, CONFIG_TRACK_COUNT - 1); }

    void editCvOutputTrack(int index, int value, bool shift) {
        setCvOutputTrack(index, cvOutputTrack(index) + value);
    }

    // gateOutputTrack

    const GateOutputArray &gateOutputTracks() const { return _gateOutputTracks; }
          GateOutputArray &gateOutputTracks()       { return _gateOutputTracks; }

    int gateOutputTrack(int index) const { return _gateOutputTracks[index]; }
    void setGateOutputTrack(int index, int trackIndex) { _gateOutputTracks[index] = clamp(trackIndex, 0, CONFIG_TRACK_COUNT - 1); }

    void editGateOutputTrack(int index, int value, bool shift) {
        setGateOutputTrack(index, gateOutputTrack(index) + value);
    }

    // song

    const Song &song() const { return _song; }
          Song &song()       { return _song; }

    // playState

    const PlayState &playState() const { return _playState; }
          PlayState &playState()       { return _playState; }

    // userScales

    const UserScale::Array &userScales() const { return UserScale::userScales; }
          UserScale::Array &userScales()       { return UserScale::userScales; }

    const UserScale &userScale(int index) const { return UserScale::userScales[index]; }
          UserScale &userScale(int index)       { return UserScale::userScales[index]; }

    // routing

    const Routing &routing() const { return _routing; }
          Routing &routing()       { return _routing; }

    // midiOutput

    const MidiOutput &midiOutput() const { return _midiOutput; }
          MidiOutput &midiOutput()       { return _midiOutput; }

    const int stepsToStop() const { return _stepsToStop;}
          int stepsToStop()       { return _stepsToStop; }

    void setStepsToStop(int steps) {
        _stepsToStop = clamp(steps, 0 , CONFIG_STEP_COUNT);
    }

    void editStepsToStop(int steps) {
        int val = (steps + stepsToStop());
        if (val > 64) val  = 0;
        if (val < 0) val = 64;
        setStepsToStop(val);
    }

    void printStepsToStop(StringBuilder &str) const {
        if (_stepsToStop == 0) {
            str("Off");
        } else {
            str("%d", stepsToStop());
        }
    }

    const int recordDelay() const { return _recordDelay;}
          int recordDelay()       { return _recordDelay; }

    void setRecordDelay(int steps) {
        _recordDelay = clamp(steps, 0 , CONFIG_STEP_COUNT);
    }

    void editRecordDelay(int steps) {
        int val = (steps + recordDelay()) % 64;
        if (val > 64) val  = 0;
        if (val < 0) val = 64;
        setRecordDelay(val);
    }

    void printRecordDelay(StringBuilder &str) const {
        if (_recordDelay == 0) {
            str("Off");
        } else {
            str("%d", recordDelay());
        }
    }

    // reset cv on stop

    bool resetCvOnStop() {
        return _resetCvOnStop;
    }

    void editResetCvOnStop(int value) {
        _resetCvOnStop = value == 1;
    }

    void printResetCvOnStop(StringBuilder &str) const {
        if (_resetCvOnStop) str("On");
        else str("Off");
    }

    void setResetCvOnStop(bool enabled) {
        _resetCvOnStop = enabled;
    }

    // use multi cv recording

    bool useMultiCvRec() const { return _useMultiCv;}

    void editUseMultiCvRec(int value) {
        _useMultiCv = value == 1;
    }

    void printUseMultiCvRec(StringBuilder &str) const {
        if (_useMultiCv) str("On");
        else str("Off");
    }

    void setUseMultiCvRec(bool enabled) {
        _useMultiCv = enabled;
    }

    // selectedTrackIndex

    int selectedTrackIndex() const { return _selectedTrackIndex; }
    void setSelectedTrackIndex(int index) {
        index = clamp(index, 0, CONFIG_TRACK_COUNT - 1);
        if (index != _selectedTrackIndex) {
            _selectedTrackIndex = index;
            _observable.notify(SelectedTrackIndexChanged);

            switch (selectedTrack()._trackMode) {
                case Track::TrackMode::Note:
                    StringUtils::copy(_selectedTrackName, selectedTrack().noteTrack().name(), sizeof(_selectedTrackName));
                    break;
                case Track::TrackMode::Curve: 
                    StringUtils::copy(_selectedTrackName, selectedTrack().curveTrack().name(), sizeof(_selectedTrackName));
                    break;
                case Track::TrackMode::MidiCv:
                    StringUtils::copy(_selectedTrackName, selectedTrack().midiCvTrack().name(), sizeof(_selectedTrackName));
                    break;
                case Track::TrackMode::Stochastic:
                    StringUtils::copy(_selectedTrackName, selectedTrack().stochasticTrack().name(), sizeof(_selectedTrackName));
                    break;
                case Track::TrackMode::Logic:
                    StringUtils::copy(_selectedTrackName, selectedTrack().logicTrack().name(), sizeof(_selectedTrackName));
                    break;
                case Track::TrackMode::Arp:
                    StringUtils::copy(_selectedTrackName, selectedTrack().arpTrack().name(), sizeof(_selectedTrackName));
                    break;
                case Track::TrackMode::Last:
                    break;
            }
            // switch selected pattern
            setSelectedPatternIndex(_playState.trackState(index).pattern());
        }
    }

    const char *selectedTrackName() const { return _selectedTrackName;}

    bool isSelectedTrack(int index) const { return _selectedTrackIndex == index; }

    // selectedPatternIndex

    int selectedPatternIndex() const {
        return _playState.snapshotActive() ? PlayState::SnapshotPatternIndex : _selectedPatternIndex;
    }

    void setSelectedPatternIndex(int index) {
        _selectedPatternIndex = clamp(index, 0, CONFIG_PATTERN_COUNT - 1);
        _observable.notify(SelectedPatternIndexChanged);
    }

    bool isSelectedPattern(int index) const { return _selectedPatternIndex == index; }

    void editSelectedPatternIndex(int value, bool shift) {
        setSelectedPatternIndex(selectedPatternIndex() + value);
    }

    // selectedNoteSequenceLayer

    NoteSequence::Layer selectedNoteSequenceLayer() const { return _selectedNoteSequenceLayer; }
    void setSelectedNoteSequenceLayer(NoteSequence::Layer layer) { _selectedNoteSequenceLayer = layer; }

    // selectedStochasticSequenceLayer

    StochasticSequence::Layer selectedStochasticSequenceLayer() const { return _selectedStochasticSequenceLayer; }
    void setSelectedStochasticSequenceLayer(StochasticSequence::Layer layer) { _selectedStochasticSequenceLayer = layer; }

    // selectedLogicSequenceLayer

    LogicSequence::Layer selectedLogicSequenceLayer() const { return _selectedLogicSequenceLayer; }
    void setSelectedLogicSequenceLayer(LogicSequence::Layer layer) { _selectedLogicSequenceLayer = layer; }

    // selectedArpSequenceLayer

    ArpSequence::Layer selectedArpSequenceLayer() const { return _selectedArpSequenceLayer; }
    void setSelectedArpSequenceLayer(ArpSequence::Layer layer) { _selectedArpSequenceLayer = layer; }
    
    // selectedCurveSequenceLayer

    CurveSequence::Layer selectedCurveSequenceLayer() const { return _selectedCurveSequenceLayer; }
    void setSelectedCurveSequenceLayer(CurveSequence::Layer layer) { _selectedCurveSequenceLayer = layer; }

    void setSelectedCurveSequence(CurveSequence seq) {
        _tracks[_selectedTrackIndex].curveTrack().setSequence(selectedPatternIndex(), seq);
    }

    // selectedTrack

    const Track &selectedTrack() const { return _tracks[_selectedTrackIndex]; }
          Track &selectedTrack()       { return _tracks[_selectedTrackIndex]; }

    // noteSequence

    const NoteSequence &noteSequence(int trackIndex, int patternIndex) const { return _tracks[trackIndex].noteTrack().sequence(patternIndex); }
          NoteSequence &noteSequence(int trackIndex, int patternIndex)       { return _tracks[trackIndex].noteTrack().sequence(patternIndex); }

    // selectedNoteSequence

    const NoteSequence &selectedNoteSequence() const { return noteSequence(_selectedTrackIndex, selectedPatternIndex()); }
          NoteSequence &selectedNoteSequence()       { return noteSequence(_selectedTrackIndex, selectedPatternIndex()); }

    void setSelectedNoteSequence(NoteSequence seq) {
        _tracks[_selectedTrackIndex].noteTrack().setSequence(selectedPatternIndex(), seq);
    }

    // curveSequence

    const CurveSequence &curveSequence(int trackIndex, int patternIndex) const { return _tracks[trackIndex].curveTrack().sequence(patternIndex); }
          CurveSequence &curveSequence(int trackIndex, int patternIndex)       { return _tracks[trackIndex].curveTrack().sequence(patternIndex); }

    // selectedCurveSequence

    const CurveSequence &selectedCurveSequence() const { return curveSequence(_selectedTrackIndex, selectedPatternIndex()); }
          CurveSequence &selectedCurveSequence()       { return curveSequence(_selectedTrackIndex, selectedPatternIndex()); }

    // stochasticSequence

    const StochasticSequence &stochasticSequence(int trackIndex, int patternIndex) const { return _tracks[trackIndex].stochasticTrack().sequence(patternIndex); }
          StochasticSequence &stochasticSequence(int trackIndex, int patternIndex)       { return _tracks[trackIndex].stochasticTrack().sequence(patternIndex); }

    // selectedStochasticSequence

    const StochasticSequence &selectedStochasticSequence() const { return stochasticSequence(_selectedTrackIndex, selectedPatternIndex()); }
          StochasticSequence &selectedStochasticSequence()       { return stochasticSequence(_selectedTrackIndex, selectedPatternIndex()); }

    // logicSequence
    
    const LogicSequence &logicSequence(int trackIndex, int patternIndex) const { return _tracks[trackIndex].logicTrack().sequence(patternIndex); }
          LogicSequence &logicSequence(int trackIndex, int patternIndex)       { return _tracks[trackIndex].logicTrack().sequence(patternIndex); }

    // selectedLogicSequence

    const LogicSequence &selectedLogicSequence() const { return logicSequence(_selectedTrackIndex, selectedPatternIndex()); }
          LogicSequence &selectedLogicSequence()       { return logicSequence(_selectedTrackIndex, selectedPatternIndex()); }

    void setselectedLogicSequence(LogicSequence seq) {
        _tracks[_selectedTrackIndex].logicTrack().setSequence(selectedPatternIndex(), seq);
    }

    // arpSequence
    
    const ArpSequence &arpSequence(int trackIndex, int patternIndex) const { return _tracks[trackIndex].arpTrack().sequence(patternIndex); }
          ArpSequence &arpSequence(int trackIndex, int patternIndex)       { return _tracks[trackIndex].arpTrack().sequence(patternIndex); }

    // selectedArpSequence

    const ArpSequence &selectedArpSequence() const { return arpSequence(_selectedTrackIndex, selectedPatternIndex()); }
          ArpSequence &selectedArpSequence()       { return arpSequence(_selectedTrackIndex, selectedPatternIndex()); }          

    void setSelectedArpSequence(ArpSequence seq) {
        _tracks[_selectedTrackIndex].arpTrack().setSequence(selectedPatternIndex(), seq);
    }

    //----------------------------------------
    // Routing
    //----------------------------------------

    inline bool isRouted(Routing::Target target) const { return Routing::isRouted(target); }
    inline void printRouted(StringBuilder &str, Routing::Target target) const { Routing::printRouted(str, target); }
    void writeRouted(Routing::Target target, int intValue, float floatValue);

    //----------------------------------------
    // Observable
    //----------------------------------------

    enum Event {
        ProjectCleared,
        ProjectRead,
        TrackModeChanged,
        SelectedTrackIndexChanged,
        SelectedPatternIndexChanged,
    };

    void watch(std::function<void(Event)> handler) {
        _observable.watch(handler);
    }

    //----------------------------------------
    // Methods
    //----------------------------------------

    void clear();
    void clearPattern(int patternIndex);

    void setTrackMode(int trackIndex, Track::TrackMode trackMode);

    void write(VersionedSerializedWriter &writer) const;
    bool read(VersionedSerializedReader &reader);

private:
    uint8_t _slot = uint8_t(-1);
    char _name[NameLength + 1];
    mutable uint8_t _autoLoaded = 0;
    Routable<float> _tempo;
    float _orinalTempo;
    Routable<uint8_t> _swing;
    TimeSignature _timeSignature;
    uint8_t _syncMeasure = 1;
    uint8_t _scale = 0;
    uint8_t _rootNote = 0;
    Types::RecordMode _recordMode = Types::RecordMode::Overdub;
    Types::MonitorMode _monitorMode = Types::MonitorMode::Always;
    Types::MidiInputMode _midiInputMode = Types::MidiInputMode::All;
    MidiSourceConfig _midiInputSource;
    Types::MidiIntegrationMode _midiIntegrationMode = Types::MidiIntegrationMode::None;
    uint8_t _midiProgramOffset = 0;
    Types::CvGateInput _cvGateInput = Types::CvGateInput::Off;
    Types::CurveCvInput _curveCvInput = Types::CurveCvInput::Off;

    ClockSetup _clockSetup;
    TrackArray _tracks;
    CvOutputTrackArray _cvOutputTracks;
    GateOutputArray _gateOutputTracks;
    Song _song;
    PlayState _playState;
    Routing _routing;
    MidiOutput _midiOutput;

    uint8_t _stepsToStop = 0;
    uint8_t _recordDelay = 0;

    bool _resetCvOnStop = false;
    bool _useMultiCv = true;

    int _selectedTrackIndex = 0;
    int _selectedPatternIndex = 0;

    char _selectedTrackName[FileHeader::NameLength+1] = "";
    NoteSequence::Layer _selectedNoteSequenceLayer = NoteSequence::Layer(0);
    CurveSequence::Layer _selectedCurveSequenceLayer = CurveSequence::Layer(0);
    StochasticSequence::Layer _selectedStochasticSequenceLayer = StochasticSequence::Layer(10);
    LogicSequence::Layer _selectedLogicSequenceLayer = LogicSequence::Layer(0);
    ArpSequence::Layer _selectedArpSequenceLayer = ArpSequence::Layer(0);

    Observable<Event, 2> _observable;
};
