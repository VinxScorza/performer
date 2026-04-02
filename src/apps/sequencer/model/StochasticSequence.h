#pragma once

#include "Config.h"
#include "Bitfield.h"
#include "Serialize.h"
#include "ModelUtils.h"
#include "Types.h"
#include "Scale.h"
#include "Routing.h"

#include "core/math/Math.h"
#include "core/utils/StringBuilder.h"

#include <array>
#include <bitset>
#include <cstdint>
#include <initializer_list>

class StochasticSequence {
public:
    //----------------------------------------
    // Types
    //----------------------------------------

    typedef UnsignedValue<4> GateProbability;
    typedef SignedValue<4> GateOffset;
    typedef UnsignedValue<3> Retrigger;
    typedef UnsignedValue<4> RetriggerProbability;
    typedef UnsignedValue<4> Length;
    typedef SignedValue<4> LengthVariationRange;
    typedef UnsignedValue<4> LengthVariationProbability;
    typedef SignedValue<7> Note;
    typedef UnsignedValue<4> NoteVariationProbability;
    typedef SignedValue<3> NoteOctave;
    typedef UnsignedValue<4> NoteOctaveProbability;
    typedef UnsignedValue<7> Condition;
    typedef UnsignedValue<3> StageRepeats;
    typedef UnsignedValue<3> StageRepeatsMode;

    static_assert(int(Types::Condition::Last) <= Condition::Max + 1, "Condition enum does not fit");

    enum class Layer {
        Gate,
        GateProbability,
        GateOffset,
        Retrigger,
        RetriggerProbability,
        StageRepeats,
        StageRepeatsMode,
        Length,
        LengthVariationRange,
        LengthVariationProbability,
        NoteVariationProbability,
        NoteOctave,
        NoteOctaveProbability,
        Slide,
        Condition,
        Last
    };

    static const char *layerName(Layer layer) {
        switch (layer) {
        case Layer::Gate:                       return "GATE";
        case Layer::GateProbability:            return "GATE PROB";
        case Layer::GateOffset:                 return "GATE OFFSET";
        case Layer::Slide:                      return "SLIDE";
        case Layer::Retrigger:                  return "RETRIG";
        case Layer::RetriggerProbability:       return "RETRIG PROB";
        case Layer::Length:                     return "LENGTH";
        case Layer::LengthVariationRange:       return "LENGTH RANGE";
        case Layer::LengthVariationProbability: return "LENGTH PROB";
        case Layer::NoteOctave:                 return "OCTAVE";
        case Layer::NoteOctaveProbability:      return "OCTAVE PROB";
        case Layer::NoteVariationProbability:   return "NOTE PROB";
        case Layer::Condition:                  return "CONDITION";
        case Layer::StageRepeats:               return "REPEAT";
        case Layer::StageRepeatsMode:           return "REPEAT MODE";
        case Layer::Last:                       break;
        }
        return nullptr;
    }

    static Types::LayerRange layerRange(Layer layer);
    static int layerDefaultValue(Layer layer);

    enum StageRepeatMode {
        Each,
        First,
        Middle,
        Last,
        Odd,
        Even,
        Triplets,
        Random,

    };

    enum Message {
        None,
        LoopOn,
        LoopOff,
        Cleared,
        ReSeed,
    };

    class Step {

    public:
        //----------------------------------------
        // Properties
        //----------------------------------------
        
        // stage
        void setStageRepeats(int repeats) {
            _data1.stageRepeats = StageRepeats::clamp(repeats);
        }
        unsigned int stageRepeats() const { return _data1.stageRepeats; }

        void setStageRepeatsMode(StageRepeatMode mode) {
            _data1.stageRepeatMode = mode;
        }

        StageRepeatMode stageRepeatMode() const { 
            int value = _data1.stageRepeatMode;
            return static_cast<StageRepeatMode>(value); 
        }

        // gate

        bool gate() const { return _data0.gate ? true : false; }
        void setGate(bool gate) { _data0.gate = gate; }
        void toggleGate() { setGate(!gate()); }

        // gateProbability

        int gateProbability() const { return _data1.gateProbability; }
        void setGateProbability(int gateProbability) {
            _data1.gateProbability = GateProbability::clamp(gateProbability);
        }

        // gateOffset

        int gateOffset() const { return GateOffset::Min + (int) _data1.gateOffset; }
        void setGateOffset(int gateOffset) {
            _data1.gateOffset = GateOffset::clamp(gateOffset) - GateOffset::Min;
        }

        // slide

        bool slide() const { return _data0.slide ? true : false; }
        void setSlide(bool slide) {
            _data0.slide = slide;
        }
        void toggleSlide() {
            setSlide(!slide());
        }

        // retrigger

        int retrigger() const { return _data1.retrigger; }
        void setRetrigger(int retrigger) {
            _data1.retrigger = Retrigger::clamp(retrigger);
        }

        // retriggerProbability

        int retriggerProbability() const { return _data1.retriggerProbability; }
        void setRetriggerProbability(int retriggerProbability) {
            _data1.retriggerProbability = RetriggerProbability::clamp(retriggerProbability);
        }

        // length

        int length() const { return _data0.length; }
        void setLength(int length) {
            _data0.length = Length::clamp(length);
        }

        // lengthVariationRange

        int lengthVariationRange() const { return LengthVariationRange::Min + _data0.lengthVariationRange; }
        void setLengthVariationRange(int lengthVariationRange) {
            _data0.lengthVariationRange = LengthVariationRange::clamp(lengthVariationRange) - LengthVariationRange::Min;
        }

        // lengthVariationProbability

        int lengthVariationProbability() const { return _data0.lengthVariationProbability; }
        void setLengthVariationProbability(int lengthVariationProbability) {
            _data0.lengthVariationProbability = LengthVariationProbability::clamp(lengthVariationProbability);
        }

        // note

        int note() const { return Note::Min + _data0.note; }
        void setNote(int note) {
            _data0.note = Note::clamp(note) - Note::Min;
        }

        // noteOctave

        int noteOctave() const { return NoteOctave::Min + _data0.noteOctave; }
        void setNoteOctave(int noteOctave) {
            _data0.noteOctave = NoteOctave::clamp(noteOctave) - NoteOctave::Min;
        }

        // noteOctaveProbability

        int noteOctaveProbability() const { return _data0.noteOctaveProbability; }
        void setNoteOctaveProbability(int noteOctaveProbability) {
            _data0.noteOctaveProbability = NoteOctaveProbability::clamp(noteOctaveProbability);
        }

        // noteVariationProbability

        int noteVariationProbability() const { return _data0.noteVariationProbability; }
        void setNoteVariationProbability(int noteVariationProbability) {
            _data0.noteVariationProbability = NoteVariationProbability::clamp(noteVariationProbability);
        }

        // condition

        Types::Condition condition() const { return Types::Condition(int(_data1.condition)); }
        void setCondition(Types::Condition condition) {
            _data1.condition = int(ModelUtils::clampedEnum(condition));
        }

        int layerValue(Layer layer) const;
        void setLayerValue(Layer layer, int value);


        bool bypassScale() const { return _data1.bypassScale ? true : false; }
        void setBypassScale(bool bypass) {
            _data1.bypassScale = bypass;
        }
        void toggleBypassScale() {
            setBypassScale(!bypassScale());
        }

        //----------------------------------------
        // Methods
        //----------------------------------------

        Step() { clear(); }

        void clear();

        void write(VersionedSerializedWriter &writer) const;
        void read(VersionedSerializedReader &reader);

        bool operator==(const Step &other) const {
            return _data0.raw == other._data0.raw && _data1.raw == other._data1.raw;
        }

        bool operator!=(const Step &other) const {
            return !(*this == other);
        }

    private:
        union {
            uint32_t raw;
            BitField<uint32_t, 0, 1> gate;
            BitField<uint32_t, 1, 1> slide;
            BitField<uint32_t, 2, Length::Bits> length;
            BitField<uint32_t, 6, LengthVariationRange::Bits> lengthVariationRange;
            BitField<uint32_t, 10, LengthVariationProbability::Bits> lengthVariationProbability;
            BitField<uint32_t, 14, Note::Bits> note;
            BitField<uint32_t, 21, NoteOctave::Bits> noteOctave;
            BitField<uint32_t, 24, NoteVariationProbability::Bits> noteVariationProbability;
            BitField<uint32_t, 28, NoteOctaveProbability::Bits> noteOctaveProbability;
            
        } _data0;
        union {
            uint32_t raw;
            BitField<uint32_t, 0, 1> bypassScale;
            BitField<uint32_t, 1, Retrigger::Bits> retrigger;
            BitField<uint32_t, 4, GateProbability::Bits> gateProbability;
            BitField<uint32_t, 8, RetriggerProbability::Bits> retriggerProbability;
            BitField<uint32_t, 12, GateOffset::Bits> gateOffset;
            BitField<uint32_t, 16, Condition::Bits> condition;
            BitField<uint32_t, 23, StageRepeats::Bits> stageRepeats;
            BitField<uint32_t, 26, StageRepeatsMode::Bits> stageRepeatMode;
            // 5 bits left
        } _data1;
    };

    typedef std::array<Step, CONFIG_STEP_COUNT> StepArray;

    //----------------------------------------
    // Properties
    //----------------------------------------

    // trackIndex

    int trackIndex() const { return _trackIndex; }

    // scale

    int scale() const { return _scale.get(isRouted(Routing::Target::Scale)); }
    void setScale(int scale, bool routed = false) {
        _scale.set(clamp(scale, -1, Scale::Count - 1), routed);
    }

    int indexedScale() const { return scale() + 1; }
    void setIndexedScale(int index) {
        setScale(index - 1);
    }

    void editScale(int value, bool shift, int defaultScale = 0) {
        if (!isRouted(Routing::Target::Scale)) {
            setScale(value, false);
        }
    }

    void printScale(StringBuilder &str) const {
        printRouted(str, Routing::Target::Scale);
        str(scale() < 0 ? "Default" : Scale::name(scale()));
    }

    const Scale &selectedScale(int defaultScale) const {
        return Scale::get(scale() < 0 ? defaultScale : scale());
    }

    // rootNote

    int rootNote() const { return _rootNote.get(isRouted(Routing::Target::RootNote)); }
    void setRootNote(int rootNote, bool routed = false) {
        _rootNote.set(clamp(rootNote, -1, 11), routed);
    }

    int indexedRootNote() const { return rootNote() + 1; }
    void setIndexedRootNote(int index) {
        setRootNote(index - 1);
    }

    void editRootNote(int value, bool shift) {
        if (!isRouted(Routing::Target::RootNote)) {
            setRootNote(rootNote() + value);
        }
    }
 
    void printRootNote(StringBuilder &str) const {
        printRouted(str, Routing::Target::RootNote);
        if (rootNote() < 0) {
            str("Default");
        } else {
            Types::printNote(str, rootNote());
        }
    }

    int selectedRootNote(int defaultRootNote) const {
        return rootNote() < 0 ? defaultRootNote : rootNote();
    }

    // divisor

    int divisor() const { return _divisor.get(isRouted(Routing::Target::Divisor)); }
    void setDivisor(int divisor, bool routed = false) {
        _divisor.set(ModelUtils::clampDivisor(divisor), routed);
    }

    int indexedDivisor() const { return ModelUtils::divisorToIndex(divisor()); }
    void setIndexedDivisor(int index) {
        int divisor = ModelUtils::indexToDivisor(index);
        if (divisor > 0) {
            setDivisor(divisor);
        }
    }

    void editDivisor(int value, bool shift) {
        if (!isRouted(Routing::Target::Divisor)) {
            setDivisor(ModelUtils::adjustedByDivisor(divisor(), value, shift));
        }
    }

    void printDivisor(StringBuilder &str) const {
        printRouted(str, Routing::Target::Divisor);
        ModelUtils::printDivisor(str, divisor());
    }

    // resetMeasure

    int resetMeasure() const { return _resetMeasure; }
    void setResetMeasure(int resetMeasure) {
        _resetMeasure = clamp(resetMeasure, 0, 128);
    }

    void editResetMeasure(int value, bool shift) {
        setResetMeasure(ModelUtils::adjustedByPowerOfTwo(resetMeasure(), value, shift));
    }

    void printResetMeasure(StringBuilder &str) const {
        if (resetMeasure() == 0) {
            str("off");
        } else {
            str("%d %s", resetMeasure(), resetMeasure() > 1 ? "bars" : "bar");
        }
    }

    // runMode

    Types::RunMode runMode() const { return _runMode.get(isRouted(Routing::Target::RunMode)); }
    void setRunMode(Types::RunMode runMode, bool routed = false) {
        _runMode.set(ModelUtils::clampedEnum(runMode), routed);
    }

    void editRunMode(int value, bool shift) {
        if (!isRouted(Routing::Target::RunMode)) {
            setRunMode(ModelUtils::adjustedEnum(runMode(), value));
        }
    }

    void printRunMode(StringBuilder &str) const {
        printRouted(str, Routing::Target::RunMode);
        str(Types::runModeName(runMode()));
    }

    // firstStep

    int firstStep() const {
        return _firstStep;
    }

    void setFirstStep(int firstStep, bool routed = false) {
        _firstStep= clamp(firstStep, 0, lastStep());
    }

    void editFirstStep(int value, bool shift) {
        if (shift) {
            offsetFirstAndLastStep(value);
        } else  {
            setFirstStep(firstStep() + value);
        }
    }

    void printFirstStep(StringBuilder &str) const {
        str("%d", firstStep() + 1);
    }

    // lastStep

    int lastStep() const {
        // make sure last step is always >= first step even if stored value is invalid (due to routing changes)
        return std::max(firstStep(), int(_lastStep));
    }

    void setLastStep(int lastStep, bool routed = false) {
        _lastStep = clamp(lastStep, firstStep(), CONFIG_STEP_COUNT - 1);
    }

    void editLastStep(int value, bool shift) {
        if (shift) {
            offsetFirstAndLastStep(value);
        } else {
            setLastStep(lastStep() + value);
        }
    }

    void printLastStep(StringBuilder &str) const {
        str("%d", lastStep() + 1);
    }

    void setStepBounds(int index) {
        _firstStep = index;
        _lastStep = index;    }

    // rest probability 1 step

    int restProbability() const { 
        int prob = 15 - restProbability2() - restProbability4() - restProbability8();
        if (prob < 0) {
            prob = 0;
        }
        return prob;
    }

    void printRestProbability(StringBuilder &str) const {
        str("%+.1f%%", restProbability() * 100.f/15.f);
    }

    // rest probability 2 steps

    int restProbability2() const { return _restProbability2.get(isRouted(Routing::Target::RestProbability2)); }
    void setRestProbability2(int restProbability, bool routed = false) {
        _restProbability2.set(clamp(restProbability, 0, 15), routed);
    }

    void editRestProbability2(int value, bool shift) {
        if (!isRouted(Routing::Target::RestProbability2)) {
            setRestProbability2(restProbability2() + value);
        }
    }

    void printRestProbability2(StringBuilder &str) const {
        printRouted(str, Routing::Target::RestProbability2);
        str("%+.1f%%", (restProbability2()) * 100.f/15.f);
    }

    // rest probability 4 steps

    int restProbability4() const { return _restProbability4.get(isRouted(Routing::Target::RestProbability4)); }
    void setRestProbability4(int restProbability, bool routed = false) {
        _restProbability4.set(clamp(restProbability, 0, 15), routed);
    }

    void editRestProbability4(int value, bool shift) {
        if (!isRouted(Routing::Target::RestProbability4)) {
            setRestProbability4(restProbability4() + value);
        }
    }

    void printRestProbability4(StringBuilder &str) const {
        printRouted(str, Routing::Target::RestProbability4);
        str("%+.1f%%", restProbability4() * 100.f/15.f);
    }

    // rest probability 8 steps

    int restProbability8() const { return _restProbability8.get(isRouted(Routing::Target::RestProbability8)); }
    void setRestProbability8(int restProbability, bool routed = false) {
        _restProbability8.set(clamp(restProbability, 0, 15), routed);
    }

    void editRestProbability8(int value, bool shift) {
        if (!isRouted(Routing::Target::RestProbability8)) {
            setRestProbability8(restProbability8() + value);
        }
    }

    void printRestProbability8(StringBuilder &str) const {
        printRouted(str, Routing::Target::RestProbability8);
        str("%+.1f%%", restProbability8() * 100.f/15.f);
    }

    // reseed

    bool reseed() const {
        return _reseed.get(isRouted(Routing::Target::Reseed));
    }

    void setReseed(int r, bool routed = false) {
        if (!isRouted(Routing::Target::Reseed)) {
            _reseed.set(r, routed);
        }
        if (reseed()) {
            setMessage(Message::ReSeed);
        }
    }

    // sequence loop last step

    int sequenceLastStep() const {
        return std::max(sequenceFirstStep(), int(_sequenceLastStep.get(isRouted(Routing::Target::SequenceLastStep))));
    }

    void setSequenceLastStep(int lastStep, bool routed = false) {
         _sequenceLastStep.set(clamp(lastStep, sequenceFirstStep(), CONFIG_STEP_COUNT - 1), routed);
    }

    void editSequenceLastStep(int value, bool shift) {
        if (shift) {
            offsetSequenceFirstAndLastStep(value);
        } else if (!isRouted(Routing::Target::SequenceLastStep)) {
            setSequenceLastStep(sequenceLastStep() + value);
        }
    }

    void printSequenceLastStep(StringBuilder &str) const {
        printRouted(str, Routing::Target::SequenceLastStep);
        str("%d", sequenceLastStep()+1);
    }

    // sequence loop first step

     int sequenceFirstStep() const {
        return _sequenceFirstStep.get(isRouted(Routing::Target::SequenceFirstStep));
    }

    void setSequenceFirstStep(int firstStep, bool routed = false) {
        _sequenceFirstStep.set(clamp(firstStep, 0, sequenceLastStep()), routed);
    }

    void editSequenceFirstStep(int value, bool shift) {
        if (shift) {
            offsetSequenceFirstAndLastStep(value);
        } else if (!isRouted(Routing::Target::FirstStep)) {
            setSequenceFirstStep(sequenceFirstStep() + value);
        }
    }

    void printSequenceFirstStep(StringBuilder &str) const {
        printRouted(str, Routing::Target::SequenceFirstStep);
        str("%d", sequenceFirstStep()+1);
    }

    // sequence length

    int sequenceLength() {
        return _sequenceLastStep.base - _sequenceFirstStep.base + 1;
    }

    // buffer loop length

    int bufferLoopLength() {
        int bufferLoopLength = 16;
        if (_sequenceLastStep.base > 15) {
            bufferLoopLength = _sequenceLastStep.base+1;
        } 
        return bufferLoopLength;
    }

    // use loop

    void setUseLoop() {
        _useLoop = !_useLoop;
        if (_useLoop) {
            setMessage(Message::LoopOn);
        } else {
            setMessage(Message::LoopOff);
        }
    }

    void setUseLoop(bool value) {
        _useLoop = value;
    }

    bool useLoop() {
        return _useLoop;
    }

    const bool useLoop() const { return _useLoop;}

    // clear loop

    void setClearLoop(bool clearLoop) {
        _clearLoop = clearLoop;
        if (_clearLoop) {
            setMessage(Message::Cleared);
        }
    }

    bool clearLoop() {
        return _clearLoop;
    }

    const bool clearLoop() const { return _clearLoop; }

    // low octave range

    int lowOctaveRange() const { return _lowOctaveRange.get(isRouted(Routing::Target::LowOctaveRange)); }
    void setLowOctaveRange(int octave, bool routed = false) {
        _lowOctaveRange.set(clamp(octave, -10, highOctaveRange()), routed);
    }

    void editLowOctaveRange(int value, bool shift) {
        if (!isRouted(Routing::Target::LowOctaveRange)) {
            setLowOctaveRange(lowOctaveRange() + value);
        }
    }

    void printLowOctaveRange(StringBuilder &str) const {
        printRouted(str, Routing::Target::LowOctaveRange);
        str("%+d", lowOctaveRange());
    }

    // high octave range

    int highOctaveRange() const { return _highOctaveRange.get(isRouted(Routing::Target::HighOctaveRange)); }
    void setHighOctaveRange(int octave, bool routed = false) {
        _highOctaveRange.set(clamp(octave, lowOctaveRange(), 10), routed);
    }

    void editHighOctaveRange(int value, bool shift) {
        if (!isRouted(Routing::Target::HighOctaveRange)) {
            setHighOctaveRange(highOctaveRange() + value);
        }
    }

    void printHighOctaveRange(StringBuilder &str) const {
        printRouted(str, Routing::Target::HighOctaveRange);
        str("%+d", highOctaveRange());
    }

    // length modifier

    int lengthModifier() const { return _lengthModifier.get(isRouted(Routing::Target::LengthModifier)); }
    void setLengthModifier(int length, bool routed = false) {
        _lengthModifier.set(clamp(length, -StochasticSequence::NoteVariationProbability::Range, StochasticSequence::NoteVariationProbability::Range), routed);
    }

    void editLengthModifier(int value, bool shift) {
        if (!isRouted(Routing::Target::LengthModifier)) {
            setLengthModifier(lengthModifier() + value);
        }
    }

    void printLengthModifier(StringBuilder &str) const {
        printRouted(str, Routing::Target::LengthModifier);
        str("%+.1f%%", lengthModifier() * 12.5f);
    }


    Message message() {
        return _message;
    }

    void setMessage(Message message) {
        _message = message;
    }

    const bool isEmpty() const {
        for (int i = 0; i < 12; ++i) {
            auto s = step(i);
            if (s.gate()) {
                return false;
            }
        }
        return true;
    }


    // steps

    const StepArray &steps() const { return _steps; }
          StepArray &steps()       { return _steps; }

    const Step &step(int index) const { return _steps[index]; }
          Step &step(int index)       { return _steps[index]; }

    //----------------------------------------
    // Routing
    //----------------------------------------

    inline bool isRouted(Routing::Target target) const { return Routing::isRouted(target, _trackIndex); }
    inline void printRouted(StringBuilder &str, Routing::Target target) const { Routing::printRouted(str, target, _trackIndex); }
    void writeRouted(Routing::Target target, int intValue, float floatValue);

    //----------------------------------------
    // Methods
    //----------------------------------------

    StochasticSequence() { clear(); }

    void clear();
    void clearSteps();

    bool isEdited() const;

    void setGates(std::initializer_list<int> gates);
    void setNotes(std::initializer_list<int> notes);

    void shiftSteps(const std::bitset<CONFIG_STEP_COUNT> &selected, int direction);

    void duplicateSteps();

    void write(VersionedSerializedWriter &writer) const;
    void read(VersionedSerializedReader &reader);

private:
    void setTrackIndex(int trackIndex) { _trackIndex = trackIndex; }

    void offsetFirstAndLastStep(int value) {
        value = clamp(value, -firstStep(), CONFIG_STEP_COUNT - 1 - lastStep());
        if (value > 0) {
            editLastStep(value, false);
            editFirstStep(value, false);
        } else {
            editFirstStep(value, false);
            editLastStep(value, false);
        }
    }

    void offsetSequenceFirstAndLastStep(int value) {
        value = clamp(value, -sequenceFirstStep(), CONFIG_STEP_COUNT - 1 - sequenceLastStep());
        if (value > 0) {
            editSequenceLastStep(value, false);
            editSequenceFirstStep(value, false);
        } else {
            editSequenceFirstStep(value, false);
            editSequenceLastStep(value, false);
        }
    }

    int8_t _trackIndex = -1;
    Routable<int8_t> _scale;
    Routable<int8_t> _rootNote;
    Routable<uint16_t> _divisor;
    uint8_t _resetMeasure;
    Routable<Types::RunMode> _runMode;
    uint8_t _firstStep;
    uint8_t _lastStep;
    Routable<bool> _reseed;

    Routable<int8_t> _restProbability2;
    Routable<int8_t> _restProbability4;
    Routable<int8_t> _restProbability8;

    Routable<uint8_t> _sequenceLastStep;
    Routable<uint8_t> _sequenceFirstStep;

    Routable<int8_t> _lowOctaveRange;
    Routable<int8_t> _highOctaveRange;

    Routable<int8_t> _lengthModifier;
    
    StepArray _steps;

    bool _useLoop = false;
    bool _clearLoop = false;

    Message _message = Message::None;

    friend class StochasticTrack;
};
