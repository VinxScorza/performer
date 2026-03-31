#pragma once

#include "Config.h"

#include "RoutableListModel.h"

#include "model/ArpSequence.h"
#include "model/Scale.h"
#include <iostream>

class ArpSequenceListModel : public RoutableListModel {
public:
    static constexpr int ScaleSelectionCount = 1 + 20 + CONFIG_USER_SCALE_COUNT;

    enum Item {
        Name,
        Divisor,
        ResetMeasure,
        Scale,
        RootNote,
        RestProbability2,
        RestProbability4,
        RestProbability8,
        LowOctaveRange,
        HighOctaveRange,
        LengthModifier,
        Last
    };

    ArpSequenceListModel()
    {
        _scales[0] = -1;
        for (int i = 1; i < ScaleSelectionCount; ++i) {
            _scales[i] = i-1;
        }

        for (int i = 0; i < 8; ++i) {
            _selectedScale[i] = 0;
        }
    }

    void setSequence(ArpSequence *sequence) {
        _sequence = sequence;
        if (sequence != nullptr) {
            int trackIndex = _sequence->trackIndex();
            _selectedScale[trackIndex] = sequence->scale()+1;
        }
    }

    virtual int rows() const override {
        return _sequence ? Last : 0;
    }

    virtual int columns() const override {
        return 2;
    }

    virtual void cell(int row, int column, StringBuilder &str) const override {
        if (column == 0) {
            formatName(Item(row), str);
        } else if (column == 1) {
            formatValue(Item(row), str);
        }
    }

    virtual void edit(int row, int column, int value, bool shift) override {
        if (column == 1) {
            editValue(Item(row), value, shift);
        }
    }

    virtual int indexedCount(int row) const override {
        return indexedCountValue(Item(row));
    }

    virtual int indexed(int row) const override {
        return indexedValue(Item(row));
    }

    virtual void setIndexed(int row, int index) override {
        if (index >= 0 && index < indexedCount(row)) {
            setIndexedValue(Item(row), index);
        }
    }

    virtual Routing::Target routingTarget(int row) const override {
        switch (Item(row)) {
        case Divisor:
            return Routing::Target::Divisor;
        case Scale:
            return Routing::Target::Scale;
        case RootNote:
            return Routing::Target::RootNote;
        case RestProbability2:
            return Routing::Target::RestProbability2;
        case RestProbability4:
            return Routing::Target::RestProbability4;
        case RestProbability8:
            return Routing::Target::RestProbability8;
        case LowOctaveRange:
            return Routing::Target::LowOctaveRange;
        case HighOctaveRange:
            return Routing::Target::HighOctaveRange;
        case LengthModifier:
            return Routing::Target::LengthModifier;
        default:
            return Routing::Target::None;
        }
    }

    void setSelectedScale(int defaultScale, bool force = false) override {
        if (_editScale || force) {
            _sequence->editScale(_scales[_selectedScale[_sequence->trackIndex()]], false, defaultScale);
        }
        _editScale = !_editScale;
    }

    int selectedScaleValue() const {
        if (_sequence == nullptr) {
            return 0;
        }

        int trackIndex = _sequence->trackIndex();
        bool isRouted = Routing::isRouted(Routing::Target::Scale, trackIndex);
        return isRouted ? _sequence->scale() : _scales[_selectedScale[trackIndex]];
    }

    void applySelectedScale(int defaultScale) {
        if (_sequence == nullptr) {
            return;
        }

        _sequence->editScale(selectedScaleValue(), false, defaultScale);
    }

    void syncSelectedScale() {
        if (_sequence == nullptr) {
            return;
        }

        int trackIndex = _sequence->trackIndex();
        _selectedScale[trackIndex] = _sequence->scale() + 1;
        _editScale = false;
    }

private:
    static const char *itemName(Item item) {
        switch (item) {
        case Name:              return "Name";
        case Divisor:           return "Divisor";
        case ResetMeasure:      return "Reset Measure";
        case Scale:             return "Scale";
        case RootNote:          return "Root Note";
        case RestProbability2:  return "Rest Prob. 2";
        case RestProbability4:  return "Rest Prob. 4";
        case RestProbability8:  return "Rest Prob. 8";
        case LowOctaveRange:    return "L Oct Range";
        case HighOctaveRange:   return "H Oct Range";
        case LengthModifier:    return "Length Mod";
        case Last:              break;
        }
        return nullptr;
    }

    void formatName(Item item, StringBuilder &str) const {
        str(itemName(item));
    }

    void formatValue(Item item, StringBuilder &str) const {
        switch (item) {
        case Name:
             str(_sequence->name());
             break;
        case Divisor:
            _sequence->printDivisor(str);
            break;
        case ResetMeasure:
            _sequence->printResetMeasure(str);
            break;
        case Scale: {
                int trackIndex = _sequence->trackIndex();
                bool isRouted = Routing::isRouted(Routing::Target::Scale, trackIndex);
                if (isRouted) {
                    _sequence->printScale(str);
                } else {
                    auto name = _scales[_selectedScale[trackIndex]] < 0 ? "Default" : Scale::name(_scales[_selectedScale[trackIndex]]);
                    str(name);
                }
            }
            break;
        case RootNote:
            _sequence->printRootNote(str);
            break;
        case RestProbability2:
            _sequence->printRestProbability2(str);
            break;
        case RestProbability4:
            _sequence->printRestProbability4(str);
            break;
         case RestProbability8:
            _sequence->printRestProbability8(str);
            break;
        case LowOctaveRange:
            _sequence->printLowOctaveRange(str);
            break;
        case HighOctaveRange:
            _sequence->printHighOctaveRange(str);
            break;
        case LengthModifier:
            _sequence->printLengthModifier(str);
            break;
        case Last:
            break;
        }
    }

    void editValue(Item item, int value, bool shift) {
        switch (item) {
        case Name:
            break;
        case Divisor:
            _sequence->editDivisor(value, shift);
            break;
        case ResetMeasure:
            _sequence->editResetMeasure(value, shift);
            break;
        case Scale: {
                int trackIndex = _sequence->trackIndex();
                bool isRouted = Routing::isRouted(Routing::Target::Scale, trackIndex);
                if (!isRouted) {
                    int trackIndex = _sequence->trackIndex();
                    _selectedScale[trackIndex] = clamp(_selectedScale[trackIndex] + value, 0, ScaleSelectionCount - 1);
                }
            }
            break;
        case RootNote:
            _sequence->editRootNote(value, shift);
            break;
        case RestProbability2:
            _sequence->editRestProbability2(value, shift);
            break;
        case RestProbability4:
            _sequence->editRestProbability4(value, shift);
            break;
        case RestProbability8:
            _sequence->editRestProbability8(value, shift);
            break;
        case LowOctaveRange:
            _sequence->editLowOctaveRange(value, shift);
            break;
        case HighOctaveRange:
            _sequence->editHighOctaveRange(value, shift);
            break;
        case LengthModifier:
            _sequence->editLengthModifier(value, shift);
            break;
        case Last:
            break;
        }
    }

    int indexedCountValue(Item item) const {
        switch (item) {
        case Name:
            break;
        case Divisor:
        case ResetMeasure:
            return 16;
        case Scale:
            return Scale::Count + 1;
        case RootNote:
            return 12 + 1;
        case RestProbability2:
        case RestProbability4:
        case RestProbability8:
        case LowOctaveRange:
        case HighOctaveRange:
        case LengthModifier:
            return 0;
        case Last:
            break;
        }
        return -1;
    }

    int indexedValue(Item item) const {
        switch (item) {
        case Name:
            break;
        case Divisor:
            return _sequence->indexedDivisor();
        case ResetMeasure:
            return _sequence->resetMeasure();
        case Scale:
            return _sequence->indexedScale();
        case RootNote:
            return _sequence->indexedRootNote();
        case RestProbability2:
            return _sequence->restProbability2();
        case RestProbability4:
            return _sequence->restProbability4();
        case RestProbability8:
            return _sequence->restProbability8();
        case LowOctaveRange:
            return _sequence->lowOctaveRange();
        case HighOctaveRange:
            return _sequence->highOctaveRange();
        case LengthModifier:
            return _sequence->lengthModifier();
        case Last:
            break;
        }
        return -1;
    }

    void setIndexedValue(Item item, int index) {
        switch (item) {
        case Name:
            break;
        case Divisor:
            return _sequence->setIndexedDivisor(index);
        case ResetMeasure:
            return _sequence->setResetMeasure(index);
        case Scale:
            return _sequence->setIndexedScale(index);
        case RootNote:
            return _sequence->setIndexedRootNote(index);
        case RestProbability2:
            return _sequence->setRestProbability2(index);
        case RestProbability4:
            return _sequence->setRestProbability4(index);
        case RestProbability8:
            return _sequence->setRestProbability8(index);
        case LowOctaveRange:
            return _sequence->setLowOctaveRange(index);
        case HighOctaveRange:
            return _sequence->setHighOctaveRange(index);
        case LengthModifier:
            return _sequence->setLengthModifier(index);
        case Last:
            break;
        }
    }

    ArpSequence *_sequence;
    private:
        std::array<int, ScaleSelectionCount> _scales;
        std::array<int, 8> _selectedScale;
        bool _editScale = false;
};
