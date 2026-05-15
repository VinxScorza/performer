#pragma once

#include "BaseTrackPatternFollow.h"
#include "Config.h"
#include "Types.h"
#include "LogicSequence.h"
#include "Serialize.h"
#include "Routing.h"
#include "FileDefs.h"
#include "core/utils/StringUtils.h"
#include "BaseTrack.h"
#include <cstdint>


class LogicTrack : public BaseTrack, public BaseTrackPatternFollow {
public:
    //----------------------------------------
    // Types
    //----------------------------------------

    typedef std::array<LogicSequence, CONFIG_PATTERN_COUNT + CONFIG_SNAPSHOT_COUNT> LogicSequenceArray;

    // FillMode

    enum class FillMode : uint8_t {
        None,
        Gates,
        NextPattern,
        Condition,
        Last
    };

    static const char *fillModeName(FillMode fillMode) {
        switch (fillMode) {
        case FillMode::None:        return "None";
        case FillMode::Gates:       return "Gates";
        case FillMode::NextPattern: return "Next Pattern";
        case FillMode::Condition:   return "Condition";
        case FillMode::Last:        break;
        }
        return nullptr;
    }

    // CvUpdateMode

    enum class CvUpdateMode : uint8_t {
        Gate,
        Always,
        Last
    };

    static const char *cvUpdateModeName(CvUpdateMode mode) {
        switch (mode) {
        case CvUpdateMode::Gate:    return "Gate";
        case CvUpdateMode::Always:  return "Always";
        case CvUpdateMode::Last:    break;
        }
        return nullptr;
    }

    //----------------------------------------
    // Properties
    //----------------------------------------

    // playMode

    Types::PlayMode playMode() const { return _playMode; }
    void setPlayMode(Types::PlayMode playMode) {
        _playMode = ModelUtils::clampedEnum(playMode);
    }

    void editPlayMode(int value, bool shift) {
        setPlayMode(ModelUtils::adjustedEnum(playMode(), value));
    }

    void printPlayMode(StringBuilder &str) const {
        str(Types::playModeName(playMode()));
    }

    // fillMode

    FillMode fillMode() const { return _fillMode; }
    void setFillMode(FillMode fillMode) {
        _fillMode = ModelUtils::clampedEnum(fillMode);
    }

    void editFillMode(int value, bool shift) {
        setFillMode(ModelUtils::adjustedEnum(fillMode(), value));
    }

    void printFillMode(StringBuilder &str) const {
        str(fillModeName(fillMode()));
    }

    // fillMuted

    bool fillMuted() const { return _fillMuted; }
    void setFillMuted(bool fillMuted) {
        _fillMuted = fillMuted;
    }

    void editFillMuted(int value, bool shift) {
        setFillMuted(value > 0);
    }

    void printFillMuted(StringBuilder &str) const {
        ModelUtils::printYesNo(str, fillMuted());
    }

    // cvUpdateMode

    CvUpdateMode cvUpdateMode() const { return _cvUpdateMode; }
    void setCvUpdateMode(CvUpdateMode cvUpdateMode) {
        _cvUpdateMode = ModelUtils::clampedEnum(cvUpdateMode);
    }

    void editCvUpdateMode(int value, bool shift) {
        setCvUpdateMode(ModelUtils::adjustedEnum(cvUpdateMode(), value));
    }

    void printCvUpdateMode(StringBuilder &str) const {
        str(cvUpdateModeName(cvUpdateMode()));
    }

    // gateOutputMode

    Types::GateOutputMode gateOutputMode() const { return _gateOutputMode; }
    void setGateOutputMode(Types::GateOutputMode gateOutputMode) {
        _gateOutputMode = ModelUtils::clampedEnum(gateOutputMode);
    }

    void editGateOutputMode(int value, bool shift) {
        setGateOutputMode(ModelUtils::adjustedEnum(gateOutputMode(), value));
    }

    void printGateOutputMode(StringBuilder &str) const {
        str(Types::gateOutputModeName(gateOutputMode()));
    }

    // slideTime

    int slideTime() const { return _slideTime.get(isRouted(Routing::Target::SlideTime)); }
    void setSlideTime(int slideTime, bool routed = false) {
        _slideTime.set(clamp(slideTime, 0, 100), routed);
    }

    void editSlideTime(int value, bool shift) {
        if (!isRouted(Routing::Target::SlideTime)) {
            setSlideTime(ModelUtils::adjustedByStep(slideTime(), value, 5, !shift));
        }
    }

    void printSlideTime(StringBuilder &str) const {
        printRouted(str, Routing::Target::SlideTime);
        str("%d%%", slideTime());
    }

    // octave

    int octave() const { return _octave.get(isRouted(Routing::Target::Octave)); }
    void setOctave(int octave, bool routed = false) {
        _octave.set(clamp(octave, -10, 10), routed);
    }

    void editOctave(int value, bool shift) {
        if (!isRouted(Routing::Target::Octave)) {
            setOctave(octave() + value);
        }
    }

    void printOctave(StringBuilder &str) const {
        printRouted(str, Routing::Target::Octave);
        str("%+d", octave());
    }

    // transpose

    int transpose() const { return _transpose.get(isRouted(Routing::Target::Transpose)); }
    void setTranspose(int transpose, bool routed = false) {
        _transpose.set(clamp(transpose, -100, 100), routed);
    }

    void editTranspose(int value, bool shift) {
        if (!isRouted(Routing::Target::Transpose)) {
            setTranspose(transpose() + value);
        }
    }

    void printTranspose(StringBuilder &str) const {
        printRouted(str, Routing::Target::Transpose);
        str("%+d", transpose());
    }

    // rotate

    int rotate() const { return _rotate.get(isRouted(Routing::Target::Rotate)); }
    void setRotate(int rotate, bool routed = false) {
        _rotate.set(clamp(rotate, -64, 64), routed);
    }

    void editRotate(int value, bool shift) {
        if (!isRouted(Routing::Target::Rotate)) {
            setRotate(rotate() + value);
        }
    }

    void printRotate(StringBuilder &str) const {
        printRouted(str, Routing::Target::Rotate);
        str("%+d", rotate());
    }

    // gateProbabilityBias

    int gateProbabilityBias() const { return _gateProbabilityBias.get(isRouted(Routing::Target::GateProbabilityBias)); }
    void setGateProbabilityBias(int gateProbabilityBias, bool routed = false) {
        _gateProbabilityBias.set(clamp(gateProbabilityBias, -LogicSequence::GateProbability::Range, LogicSequence::GateProbability::Range), routed);
    }

    void editGateProbabilityBias(int value, bool shift) {
        if (!isRouted(Routing::Target::GateProbabilityBias)) {
            setGateProbabilityBias(gateProbabilityBias() + value);
        }
    }

    void printGateProbabilityBias(StringBuilder &str) const {
        printRouted(str, Routing::Target::GateProbabilityBias);
        str("%+.1f%%", gateProbabilityBias() * 12.5f);
    }

    // retriggerProbabilityBias

    int retriggerProbabilityBias() const { return _retriggerProbabilityBias.get(isRouted(Routing::Target::RetriggerProbabilityBias)); }
    void setRetriggerProbabilityBias(int retriggerProbabilityBias, bool routed = false) {
        _retriggerProbabilityBias.set(clamp(retriggerProbabilityBias, -LogicSequence::RetriggerProbability::Range, LogicSequence::RetriggerProbability::Range), routed);
    }

    void editRetriggerProbabilityBias(int value, bool shift) {
        if (!isRouted(Routing::Target::RetriggerProbabilityBias)) {
            setRetriggerProbabilityBias(retriggerProbabilityBias() + value);
        }
    }

    void printRetriggerProbabilityBias(StringBuilder &str) const {
        printRouted(str, Routing::Target::RetriggerProbabilityBias);
        str("%+.1f%%", retriggerProbabilityBias() * 12.5f);
    }

    // lengthBias

    int lengthBias() const { return _lengthBias.get(isRouted(Routing::Target::LengthBias)); }
    void setLengthBias(int lengthBias, bool routed = false) {
        _lengthBias.set(clamp(lengthBias, -LogicSequence::Length::Range, LogicSequence::Length::Range), routed);
    }

    void editLengthBias(int value, bool shift) {
        if (!isRouted(Routing::Target::LengthBias)) {
            setLengthBias(lengthBias() + value);
        }
    }

    void printLengthBias(StringBuilder &str) const {
        printRouted(str, Routing::Target::LengthBias);
        str("%+.1f%%", lengthBias() * 12.5f);
    }

    // noteProbabilityBias

    int noteProbabilityBias() const { return _noteProbabilityBias.get(isRouted(Routing::Target::NoteProbabilityBias)); }
    void setNoteProbabilityBias(int noteProbabilityBias, bool routed = false) {
        _noteProbabilityBias.set(clamp(noteProbabilityBias, -LogicSequence::NoteVariationProbability::Range, LogicSequence::NoteVariationProbability::Range), routed);
    }

    void editNoteProbabilityBias(int value, bool shift) {
        if (!isRouted(Routing::Target::NoteProbabilityBias)) {
            setNoteProbabilityBias(noteProbabilityBias() + value);
        }
    }

    void printNoteProbabilityBias(StringBuilder &str) const {
        printRouted(str, Routing::Target::NoteProbabilityBias);
        str("%+.1f%%", noteProbabilityBias() * 12.5f);
    }

    // sequences

    const LogicSequenceArray &sequences() const { return _sequences; }
          LogicSequenceArray &sequences()       { return _sequences; }

    const LogicSequence &sequence(int index) const { return _sequences[index]; }
          LogicSequence &sequence(int index)       { return _sequences[index]; }

    void setSequence(int index, LogicSequence seq) {
        _sequences[index] = seq;
    }

    //----------------------------------------
    // Routing
    //----------------------------------------

    inline bool isRouted(Routing::Target target) const { return Routing::isRouted(target, _trackIndex); }
    inline void printRouted(StringBuilder &str, Routing::Target target) const { Routing::printRouted(str, target, _trackIndex); }
    void writeRouted(Routing::Target target, int intValue, float floatValue);

    const int inputTrack1() const {
        return _inputTrack1;
    }

    void setInputTrack1(int trackIndex) {
        _inputTrack1 = trackIndex;
    }

    void printInputTrack1(StringBuilder &str) const {
        if (inputTrack1()==-1) {
            str("-");
        } else {
            str("%d", inputTrack1()+1);
        }
    }


    const int inputTrack2() const {
        return _inputTrack2;
    }

    void setInputTrack2(int trackIndex) {
        _inputTrack2 = trackIndex;
    }

     void printInputTrack2(StringBuilder &str) const {
        if (inputTrack2()==-1) {
            str("-");
        } else {
            str("%d", inputTrack2()+1);
        }
    }

    // detailed view
    bool detailedView() const { return _detailedView; }
    void setDetailedView(bool value) {
        if (value != _detailedView) {
            _detailedView = value;
        }
    }

    void editDetailedView(int value, bool shift) {
        setDetailedView(value > 0);
    }

    void toggleDetailedView() {
        setDetailedView(!detailedView());
    }

    void printDetailedView(StringBuilder &str) const {
        ModelUtils::printYesNo(str, detailedView());
    }

    //----------------------------------------
    // Methods
    //----------------------------------------

    LogicTrack() { clear(); }

    void clear();

    void write(VersionedSerializedWriter &writer) const;
    void read(VersionedSerializedReader &reader);

private:
    void setTrackIndex(int trackIndex) {
        _trackIndex = trackIndex;
        for (auto &sequence : _sequences) {
            sequence.setTrackIndex(trackIndex);
        }
    }

    int8_t _trackIndex = -1;
    Types::PlayMode _playMode;
    FillMode _fillMode;
    bool _fillMuted;
    CvUpdateMode _cvUpdateMode;
    Types::GateOutputMode _gateOutputMode;
    Routable<uint8_t> _slideTime;
    Routable<int8_t> _octave;
    Routable<int8_t> _transpose;
    Routable<int8_t> _rotate;
    Routable<int8_t> _gateProbabilityBias;
    Routable<int8_t> _retriggerProbabilityBias;
    Routable<int8_t> _lengthBias;
    Routable<int8_t> _noteProbabilityBias;

    int8_t _inputTrack1 = -1;
    int8_t _inputTrack2 = -1;

    bool _detailedView = false;

    LogicSequenceArray _sequences;

    friend class Track;
};
